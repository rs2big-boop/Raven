#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "raven/bytecode_serializer.hpp"
#include "raven/bytecode_verifier.hpp"
#include "raven/compiler.hpp"
#include "raven/lexer.hpp"
#include "raven/parser.hpp"
#include "raven/vm.hpp"

using namespace raven;

static const char RAVEN_NATIVE_MAGIC[16] = "RAVEN_NATIVE_V1";

static bool check_and_run_embedded_native(int argc, char* argv[]) {
    std::string exe_path = "/proc/self/exe";
    std::ifstream file(exe_path, std::ios::binary | std::ios::ate);
    if (!file.is_open() && argc > 0 && argv[0] != nullptr) {
        exe_path = argv[0];
        file.open(exe_path, std::ios::binary | std::ios::ate);
    }
    if (!file.is_open()) return false;

    std::streamsize file_size = file.tellg();
    if (file_size < 32) return false;

    file.seekg(file_size - 24);
    char magic[16];
    file.read(magic, 16);
    if (std::memcmp(magic, RAVEN_NATIVE_MAGIC, 16) != 0) {
        return false;
    }

    uint64_t payload_size = 0;
    file.read(reinterpret_cast<char*>(&payload_size), sizeof(payload_size));
    if (payload_size == 0 || static_cast<std::streamsize>(payload_size) + 24 > file_size) {
        return false;
    }

    file.seekg(file_size - 24 - payload_size);
    std::string payload(payload_size, '\0');
    file.read(&payload[0], payload_size);

    std::istringstream stream(payload, std::ios::binary);
    Chunk chunk;
    if (!BytecodeSerializer::deserialize(stream, chunk)) {
        std::cerr << "Fatal error: Embedded bytecode payload is corrupted.\n";
        std::exit(1);
    }

    VM vm(std::cout);
    auto result = vm.run(chunk);
    std::exit(result == InterpretResult::Ok ? 0 : 70);
}

void print_usage() {
    std::cout << "Raven Virtual Machine (RVM) - Corvus Engine\n\n"
              << "Usage: rvm [options] <bytecode.rvc | script.rvn>\n\n"
              << "Options:\n"
              << "  -b, --benchmark Benchmark VM execution time\n"
              << "  -d, --disasm    Print bytecode disassembly before execution\n"
              << "  -h, --help      Display this help message\n";
}

int main(int argc, char* argv[]) {
    check_and_run_embedded_native(argc, argv);

    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string file_path;
    bool benchmark = false;
    bool disasm = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage();
            return 0;
        } else if (arg == "-b" || arg == "--benchmark") {
            benchmark = true;
        } else if (arg == "-d" || arg == "--disasm") {
            disasm = true;
        } else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        } else {
            file_path = arg;
        }
    }

    if (file_path.empty()) {
        std::cerr << "Error: No file specified.\n";
        return 1;
    }

    Chunk chunk;
    bool is_bytecode = (file_path.size() >= 4 &&
                        file_path.substr(file_path.size() - 4) == ".rvc");

    if (is_bytecode) {
        if (!BytecodeSerializer::load_from_file(file_path, chunk)) {
            std::cerr << "Error: Failed to load or invalid bytecode file '" << file_path << "'\n";
            return 1;
        }
    } else {
        // Compile on-the-fly
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file '" << file_path << "'\n";
            return 1;
        }
        std::ostringstream ss;
        ss << file.rdbuf();

        Lexer lexer(ss.str());
        Parser parser(lexer.tokenize());
        auto statements = parser.parse();

        if (parser.has_error()) {
            for (const auto& err : parser.errors()) {
                std::cerr << err << "\n";
            }
            return 65;
        }

        Compiler compiler(chunk);
        if (!compiler.compile(statements)) {
            std::cerr << "Compilation failed.\n";
            return 1;
        }
    }

    if (disasm) {
        chunk.disassemble(std::cout, file_path);
    }

    auto verify_res = BytecodeVerifier::verify(chunk);
    if (!verify_res.is_valid) {
        std::cerr << "Bytecode Verification Error: " << verify_res.error_message << "\n";
        return 70;
    }

    VM vm(std::cout);

    auto start_time = std::chrono::high_resolution_clock::now();
    InterpretResult result = vm.run(chunk);
    auto end_time = std::chrono::high_resolution_clock::now();

    if (benchmark) {
        auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        std::cout << "\n[RVM Benchmark: execution finished in " << duration_us << " us ("
                  << (duration_us / 1000.0) << " ms)]\n";
    }

    return result == InterpretResult::Ok ? 0 : 70;
}
