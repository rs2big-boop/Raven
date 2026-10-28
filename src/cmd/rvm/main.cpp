#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "raven/bytecode_serializer.hpp"
#include "raven/compiler.hpp"
#include "raven/lexer.hpp"
#include "raven/parser.hpp"
#include "raven/vm.hpp"

using namespace raven;

void print_usage() {
    std::cout << "Raven Virtual Machine (RVM) - Corvus Engine\n\n"
              << "Usage: rvm [options] <bytecode.rvc | script.rvn>\n\n"
              << "Options:\n"
              << "  -b, --benchmark Benchmark VM execution time\n"
              << "  -d, --disasm    Print bytecode disassembly before execution\n"
              << "  -h, --help      Display this help message\n";
}

int main(int argc, char* argv[]) {
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
