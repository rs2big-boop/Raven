#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "raven/bytecode_serializer.hpp"
#include "raven/compiler.hpp"
#include "raven/lexer.hpp"
#include "raven/optimizer.hpp"
#include "raven/parser.hpp"

using namespace raven;

void print_usage() {
    std::cout << "Raven Bytecode Compiler (ravenc)\n\n"
              << "Usage: ravenc [options] <input.rvn>\n\n"
              << "Options:\n"
              << "  -o <file.rvc>   Output compiled bytecode file (default: <input>.rvc)\n"
              << "  -O, --optimize  Enable compile-time constant folding & dead code elimination\n"
              << "  -d, --disasm    Print bytecode disassembly to stdout\n"
              << "  -h, --help      Display this help message\n";
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

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage();
            return 0;
        } else if (arg == "-O" || arg == "--optimize") {
            optimize = true;
        } else if (arg == "-d" || arg == "--disasm") {
            disasm = true;
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
        if (dot != std::string::npos) {
            output_path = input_path.substr(0, dot) + ".rvc";
        } else {
            output_path = input_path + ".rvc";
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

    if (!BytecodeSerializer::save_to_file(output_path, chunk)) {
        std::cerr << "Error: Failed to write bytecode file '" << output_path << "'\n";
        return 1;
    }

    std::cout << "Successfully compiled '" << input_path << "' -> '" << output_path
              << "' (" << chunk.code.size() << " bytes bytecode, "
              << chunk.constants.size() << " constants)\n";
    return 0;
}
