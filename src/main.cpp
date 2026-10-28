#include <iostream>
#include <fstream>
#include <sstream>
#include "minilang/lexer.hpp"

int main(int argc, char* argv[]) {
    std::string source;

    if (argc > 1) {
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file '" << argv[1] << "'\n";
            return 1;
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        source = ss.str();
    } else {
        source = "let age = 20;\n";
        std::cout << "No input file provided. Running default demo:\n";
        std::cout << source << "\n";
    }

    minilang::Lexer lexer(source);
    auto tokens = lexer.tokenize();

    std::cout << "Tokens (" << tokens.size() << "):\n";
    std::cout << "----------------------------------------\n";
    for (const auto& token : tokens) {
        std::cout << token << "\n";
    }
    std::cout << "----------------------------------------\n";

    return 0;
}
