#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/stat.h>
#endif

#include "raven/bytecode_serializer.hpp"
#include "raven/compiler.hpp"
#include "raven/lexer.hpp"
#include "raven/optimizer.hpp"
#include "raven/parser.hpp"

namespace fs = std::filesystem;
using namespace raven;

static const char RAVEN_NATIVE_MAGIC[16] = "RAVEN_NATIVE_V1";

void print_usage() {
    std::cout << "Raven Bytecode & Native Compiler (ravenc)\n\n"
              << "Usage: ravenc [options] <input.rvn>\n\n"
              << "Options:\n"
              << "  -o <file>       Output compiled file (default: <input>.rvc or executable)\n"
              << "  --native        Compile directly to a standalone native executable\n"
              << "  -O, --optimize  Enable compile-time constant folding & dead code elimination\n"
              << "  -d, --disasm    Print bytecode disassembly to stdout\n"
              << "  -h, --help      Display this help message\n";
}

static std::string find_rvm_runtime() {
    std::vector<std::string> candidates = {
        "/proc/self/exe",
        "/home/ran/MiniLang/build/rvm",
        "/home/ran/Raven/build/rvm",
        "./rvm",
        "./build/rvm",
        "/usr/local/bin/rvm",
        "/usr/bin/rvm"
    };

    try {
        auto self_dir = fs::canonical("/proc/self/exe").parent_path();
        candidates.insert(candidates.begin(), (self_dir / "rvm").string());
    } catch (...) {}

    for (const auto& path : candidates) {
        if (path == "/proc/self/exe") continue;
        if (fs::exists(path) && !fs::is_directory(path)) {
            return path;
        }
    }
    return "";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string input_path;
    std::string output_path;
    bool optimize = false;
    bool disasm = false;
    bool native = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage();
            return 0;
        } else if (arg == "-O" || arg == "--optimize") {
            optimize = true;
        } else if (arg == "-d" || arg == "--disasm") {
            disasm = true;
        } else if (arg == "--native" || arg == "--standalone") {
            native = true;
        } else if (arg == "-o" && i + 1 < argc) {
            output_path = argv[++i];
        } else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        } else {
            input_path = arg;
        }
    }

    if (input_path.empty()) {
        std::cerr << "Error: No input file specified.\n";
        return 1;
    }

    if (output_path.empty()) {
        auto dot = input_path.find_last_of('.');
        if (native) {
            output_path = (dot != std::string::npos) ? input_path.substr(0, dot) : (input_path + ".bin");
        } else {
            output_path = (dot != std::string::npos) ? input_path.substr(0, dot) + ".rvc" : (input_path + ".rvc");
        }
    }

    std::ifstream file(input_path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open source file '" << input_path << "'\n";
        return 1;
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    Lexer lexer(ss.str());
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto statements = parser.parse();

    if (parser.has_error()) {
        for (const auto& err : parser.errors()) {
            std::cerr << err << "\n";
        }
        return 65;
    }

    if (optimize) {
        statements = Optimizer::optimize(statements);
    }

    Chunk chunk;
    Compiler compiler(chunk);
    if (!compiler.compile(statements)) {
        std::cerr << "Compilation failed.\n";
        return 1;
    }

    if (disasm) {
        chunk.disassemble(std::cout, input_path);
    }

    if (native) {
        std::string rvm_path = find_rvm_runtime();
        if (rvm_path.empty()) {
            std::cerr << "Error: Could not locate native runtime engine template ('rvm').\n";
            return 1;
        }

        std::ifstream rvm_in(rvm_path, std::ios::binary);
        if (!rvm_in.is_open()) {
            std::cerr << "Error: Failed to read runtime template '" << rvm_path << "'\n";
            return 1;
        }

        std::ostringstream payload_stream(std::ios::binary);
        if (!BytecodeSerializer::serialize(chunk, payload_stream)) {
            std::cerr << "Error: Failed to serialize bytecode payload.\n";
            return 1;
        }
        std::string payload = payload_stream.str();
        uint64_t payload_size = static_cast<uint64_t>(payload.size());

        std::ofstream out(output_path, std::ios::binary);
        if (!out.is_open()) {
            std::cerr << "Error: Cannot open output executable '" << output_path << "' for writing.\n";
            return 1;
        }

        // 1. Copy entire RVM engine
        out << rvm_in.rdbuf();
        // 2. Append serialized bytecode
        out.write(payload.data(), payload.size());
        // 3. Append magic trailer
        out.write(RAVEN_NATIVE_MAGIC, 16);
        // 4. Append uint64 payload size
        out.write(reinterpret_cast<const char*>(&payload_size), sizeof(payload_size));
        out.close();

        // 5. Set executable permissions (chmod +x, POSIX only)
#if !defined(_WIN32) && !defined(_WIN64)
        chmod(output_path.c_str(), 0755);
#endif

        std::cout << "⚡ Successfully compiled standalone native binary: '" << output_path << "'\n"
                  << "   Binary size: " << fs::file_size(output_path) << " bytes\n"
                  << "   Direct execution: ./" << output_path << "\n";
        return 0;
    }

    if (!BytecodeSerializer::save_to_file(output_path, chunk)) {
        std::cerr << "Error: Failed to write bytecode file '" << output_path << "'\n";
        return 1;
    }

    std::cout << "Successfully compiled '" << input_path << "' -> '" << output_path
              << "' (" << chunk.code.size() << " bytes bytecode, "
              << chunk.constants.size() << " constants)\n";
    return 0;
}
