#include "minilang/lexer.hpp"

#include <cctype>
#include <utility>

namespace minilang {

Lexer::Lexer(std::string source)
    : source_(std::move(source)) {}

char Lexer::peek() const {
    if (is_at_end()) {
        return '\0';
    }
    return source_[position_];
}

char Lexer::peek_next() const {
    if (position_ + 1 >= source_.size()) {
        return '\0';
    }
    return source_[position_ + 1];
}

char Lexer::advance() {
    char current = source_[position_++];
    column_++;
    return current;
}

bool Lexer::is_at_end() const {
    return position_ >= source_.size();
}

void Lexer::skip_whitespace() {
    while (!is_at_end()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '\n') {
            position_++;
            line_++;
            column_ = 1;
        } else {
            break;
        }
    }
}

Token Lexer::scan_number() {
    int start_line = line_;
    int start_col = column_;
    std::size_t start_pos = position_;

    while (!is_at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
    }

    std::string lexeme = source_.substr(start_pos, position_ - start_pos);
    return Token{TokenType::Number, std::move(lexeme), start_line, start_col};
}

Token Lexer::scan_identifier_or_keyword() {
    int start_line = line_;
    int start_col = column_;
    std::size_t start_pos = position_;

    while (!is_at_end()) {
        char c = peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
            advance();
        } else {
            break;
        }
    }

    std::string lexeme = source_.substr(start_pos, position_ - start_pos);
    TokenType type = TokenType::Identifier;
    if (lexeme == "let") {
        type = TokenType::Let;
    }

    return Token{type, std::move(lexeme), start_line, start_col};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        skip_whitespace();

        if (is_at_end()) {
            tokens.push_back(Token{TokenType::EndOfFile, "", line_, column_});
            break;
        }

        int start_line = line_;
        int start_col = column_;
        char c = peek();

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(scan_identifier_or_keyword());
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(scan_number());
        } else {
            advance();
            switch (c) {
                case '+':
                    tokens.push_back(Token{TokenType::Plus, "+", start_line, start_col});
                    break;
                case '-':
                    tokens.push_back(Token{TokenType::Minus, "-", start_line, start_col});
                    break;
                case '*':
                    tokens.push_back(Token{TokenType::Star, "*", start_line, start_col});
                    break;
                case '/':
                    tokens.push_back(Token{TokenType::Slash, "/", start_line, start_col});
                    break;
                case '=':
                    tokens.push_back(Token{TokenType::Equal, "=", start_line, start_col});
                    break;
                case ';':
                    tokens.push_back(Token{TokenType::Semicolon, ";", start_line, start_col});
                    break;
                default:
                    tokens.push_back(Token{TokenType::Invalid, std::string(1, c), start_line, start_col});
                    break;
            }
        }
    }

    return tokens;
}

} // namespace minilang
