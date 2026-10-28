#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "minilang/interpreter.hpp"
#include "minilang/lexer.hpp"
#include "minilang/parser.hpp"

namespace minilang {

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

static int run_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << path << "'\n";
        return 1;
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    Interpreter interpreter;
    return run(ss.str(), interpreter);
}

static void run_prompt() {
    std::cout << "MiniLang 0.1.0 Interactive Shell\n";
    std::cout << "Type 'exit' or press Ctrl+D to quit.\n";

    Interpreter interpreter;
    std::string line;

    while (true) {
        std::cout << "minilang> ";
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

        // Auto append semicolon if user entered an expression or single statement without it
        if (line.back() != ';' && line.back() != '}') {
            line += ";";
        }

        run(line, interpreter);
    }
}

} // namespace minilang

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: minilang [path/to/script.mini]\n";
            std::cout << "       minilang                (starts REPL)\n";
            return 0;
        }
        return minilang::run_file(arg);
    }

    minilang::run_prompt();
    return 0;
}
