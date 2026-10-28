#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "raven/compiler.hpp"
#include "raven/interpreter.hpp"
#include "raven/lexer.hpp"
#include "raven/parser.hpp"
#include "raven/vm.hpp"

namespace raven {

static int run_disasm(const std::string& source, const std::string& name) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto statements = parser.parse();

    if (parser.has_error()) {
        for (const auto& err : parser.errors()) {
            std::cerr << err << "\n";
        }
        return 65;
    }

    Chunk chunk;
    Compiler compiler(chunk);
    compiler.compile(statements);

    chunk.disassemble(std::cout, name);
    return 0;
}

static int run_vm(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto statements = parser.parse();

    if (parser.has_error()) {
        for (const auto& err : parser.errors()) {
            std::cerr << err << "\n";
        }
        return 65;
    }

    Chunk chunk;
    Compiler compiler(chunk);
    compiler.compile(statements);

    VM vm(std::cout);
    auto result = vm.run(chunk);
    return result == InterpretResult::Ok ? 0 : 70;
}

static int run(const std::string& source, Interpreter& interpreter) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto statements = parser.parse();

    if (parser.has_error()) {
        for (const auto& err : parser.errors()) {
            std::cerr << err << "\n";
        }
        return 65;
    }

    try {
        interpreter.interpret(statements);
    } catch (const RuntimeError& error) {
        std::cerr << "[line " << error.token().line << ", col "
                  << error.token().column << "] Runtime Error: "
                  << error.what() << "\n";
        return 70;
    }

    return 0;
}

static int run_file(const std::string& path, bool use_vm = false, bool disasm = false) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << path << "'\n";
        return 1;
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    if (disasm) {
        return run_disasm(ss.str(), path);
    }
    if (use_vm) {
        return run_vm(ss.str());
    }

    Interpreter interpreter;
    return run(ss.str(), interpreter);
}

static void run_prompt() {
    std::cout << "Raven 0.2.0 Interactive Shell (Tree-Walk & Bytecode VM)\n";
    std::cout << "Type 'exit' or press Ctrl+D to quit.\n";

    Interpreter interpreter;
    std::string line;

    while (true) {
        std::cout << "raven> ";
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break;
        }

        if (line == "exit" || line == "quit") {
            break;
        }

        if (line.empty()) {
            continue;
        }

        if (line.back() != ';' && line.back() != '}') {
            line += ";";
        }

        run(line, interpreter);
    }
}

} // namespace raven

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string first_arg = argv[1];

        if (first_arg == "--help" || first_arg == "-h") {
            std::cout << "Raven Programming Language (C++20)\n\n"
                      << "Usage: raven [options] [script.rvn]\n\n"
                      << "Options:\n"
                      << "  --vm       Execute using the Raven Bytecode Virtual Machine\n"
                      << "  --disasm   Compile and display the Bytecode disassembly\n"
                      << "  --help, -h Show this help message\n";
            return 0;
        }

        if (first_arg == "--disasm" && argc > 2) {
            return raven::run_file(argv[2], false, true);
        }

        if (first_arg == "--vm" && argc > 2) {
            return raven::run_file(argv[2], true, false);
        }

        return raven::run_file(first_arg);
    }

    raven::run_prompt();
    return 0;
}
