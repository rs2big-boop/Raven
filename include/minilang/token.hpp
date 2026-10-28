#pragma once

#include <string>
#include <string_view>
#include <ostream>

namespace minilang {

enum class TokenType {
    Let,
    Identifier,
    Number,

    Plus,
    Minus,
    Star,
    Slash,

    Equal,
    Semicolon,

    EndOfFile,
    Invalid
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;

    bool operator==(const Token& other) const {
        return type == other.type &&
               lexeme == other.lexeme &&
               line == other.line &&
               column == other.column;
    }
};

inline std::string_view token_type_to_string(TokenType type) {
    switch (type) {
        case TokenType::Let: return "Let";
        case TokenType::Identifier: return "Identifier";
        case TokenType::Number: return "Number";
        case TokenType::Plus: return "Plus";
        case TokenType::Minus: return "Minus";
        case TokenType::Star: return "Star";
        case TokenType::Slash: return "Slash";
        case TokenType::Equal: return "Equal";
        case TokenType::Semicolon: return "Semicolon";
        case TokenType::EndOfFile: return "EndOfFile";
        case TokenType::Invalid: return "Invalid";
    }
    return "Unknown";
}

inline std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << "Token(" << token_type_to_string(token.type)
       << ", \"" << token.lexeme << "\", line: "
       << token.line << ", col: " << token.column << ")";
    return os;
}

} // namespace minilang
