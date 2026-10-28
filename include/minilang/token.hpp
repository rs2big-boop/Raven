#pragma once

#include <string>
#include <string_view>
#include <ostream>

namespace minilang {

enum class TokenType {
    // Keywords
    Let,
    If,
    Else,
    While,
    Print,
    Fn,
    Return,
    True,
    False,

    // Literals & Identifiers
    Identifier,
    Number,
    String,

    // Single-character Operators & Punctuation
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Equal,
    Bang,
    Less,
    Greater,
    Semicolon,
    Comma,
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,

    // Two-character Operators
    EqualEqual,
    BangEqual,
    LessEqual,
    GreaterEqual,
    AmpAmp,
    PipePipe,

    // Control
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
        case TokenType::If: return "If";
        case TokenType::Else: return "Else";
        case TokenType::While: return "While";
        case TokenType::Print: return "Print";
        case TokenType::Fn: return "Fn";
        case TokenType::Return: return "Return";
        case TokenType::True: return "True";
        case TokenType::False: return "False";

        case TokenType::Identifier: return "Identifier";
        case TokenType::Number: return "Number";
        case TokenType::String: return "String";

        case TokenType::Plus: return "Plus";
        case TokenType::Minus: return "Minus";
        case TokenType::Star: return "Star";
        case TokenType::Slash: return "Slash";
        case TokenType::Percent: return "Percent";
        case TokenType::Equal: return "Equal";
        case TokenType::Bang: return "Bang";
        case TokenType::Less: return "Less";
        case TokenType::Greater: return "Greater";
        case TokenType::Semicolon: return "Semicolon";
        case TokenType::Comma: return "Comma";
        case TokenType::LeftParen: return "LeftParen";
        case TokenType::RightParen: return "RightParen";
        case TokenType::LeftBrace: return "LeftBrace";
        case TokenType::RightBrace: return "RightBrace";

        case TokenType::EqualEqual: return "EqualEqual";
        case TokenType::BangEqual: return "BangEqual";
        case TokenType::LessEqual: return "LessEqual";
        case TokenType::GreaterEqual: return "GreaterEqual";
        case TokenType::AmpAmp: return "AmpAmp";
        case TokenType::PipePipe: return "PipePipe";

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
