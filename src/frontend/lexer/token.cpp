// token.cpp — Token type utilities for the Raven lexer
#include "raven/token.hpp"
#include <string>

namespace raven {

bool token_is_literal(TokenType type) {
    switch (type) {
        case TokenType::Number:
        case TokenType::String:
        case TokenType::True:
        case TokenType::False:
        case TokenType::Nil:
            return true;
        default:
            return false;
    }
}

bool token_is_binary_op(TokenType type) {
    switch (type) {
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent:
        case TokenType::EqualEqual:
        case TokenType::BangEqual:
        case TokenType::Less:
        case TokenType::LessEqual:
        case TokenType::Greater:
        case TokenType::GreaterEqual:
        case TokenType::AmpAmp:
        case TokenType::PipePipe:
        case TokenType::QuestionQuestion:
            return true;
        default:
            return false;
    }
}

std::string token_type_name(TokenType type) {
    switch (type) {
        case TokenType::Let: return "Let";
        case TokenType::If: return "If";
        case TokenType::Else: return "Else";
        case TokenType::While: return "While";
        case TokenType::For: return "For";
        case TokenType::In: return "In";
        case TokenType::Try: return "Try";
        case TokenType::Catch: return "Catch";
        case TokenType::Throw: return "Throw";
        case TokenType::Match: return "Match";
        case TokenType::Import: return "Import";
        case TokenType::Struct: return "Struct";
        case TokenType::Break: return "Break";
        case TokenType::Continue: return "Continue";
        case TokenType::Print: return "Print";
        case TokenType::Fn: return "Fn";
        case TokenType::Return: return "Return";
        case TokenType::True: return "True";
        case TokenType::False: return "False";
        case TokenType::Nil: return "Nil";
        case TokenType::Loop: return "Loop";
        case TokenType::Is: return "Is";
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
        case TokenType::Colon: return "Colon";
        case TokenType::Dot: return "Dot";
        case TokenType::Comma: return "Comma";
        case TokenType::LeftParen: return "LeftParen";
        case TokenType::RightParen: return "RightParen";
        case TokenType::LeftBrace: return "LeftBrace";
        case TokenType::RightBrace: return "RightBrace";
        case TokenType::LeftBracket: return "LeftBracket";
        case TokenType::RightBracket: return "RightBracket";
        case TokenType::EqualEqual: return "EqualEqual";
        case TokenType::BangEqual: return "BangEqual";
        case TokenType::LessEqual: return "LessEqual";
        case TokenType::GreaterEqual: return "GreaterEqual";
        case TokenType::PlusEqual: return "PlusEqual";
        case TokenType::MinusEqual: return "MinusEqual";
        case TokenType::StarEqual: return "StarEqual";
        case TokenType::SlashEqual: return "SlashEqual";
        case TokenType::PercentEqual: return "PercentEqual";
        case TokenType::AmpAmp: return "AmpAmp";
        case TokenType::PipePipe: return "PipePipe";
        case TokenType::PipeGreater: return "PipeGreater";
        case TokenType::Arrow: return "Arrow";
        case TokenType::QuestionQuestion: return "QuestionQuestion";
        case TokenType::QuestionDot: return "QuestionDot";
        case TokenType::DotDotDot: return "DotDotDot";
        case TokenType::EndOfFile: return "EndOfFile";
        case TokenType::Invalid: return "Invalid";
        default: return "Unknown";
    }
}

} // namespace raven
