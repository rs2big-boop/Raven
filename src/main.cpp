#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "raven/interpreter.hpp"
#include "raven/lexer.hpp"
#include "raven/parser.hpp"

namespace raven {

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
    std::cout << "Raven 0.1.0 Interactive Shell\n";
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
        std::string arg = argv[1];
        if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: raven [path/to/script.rvn]\n";
            std::cout << "       raven                (starts REPL)\n";
            return 0;
        }
        return raven::run_file(arg);
    }

    raven::run_prompt();
    return 0;
}
