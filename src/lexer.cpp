#include "minilang/lexer.hpp"

#include <cctype>
#include <unordered_map>
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

bool Lexer::match(char expected) {
    if (is_at_end() || source_[position_] != expected) {
        return false;
    }
    position_++;
    column_++;
    return true;
}

bool Lexer::is_at_end() const {
    return position_ >= source_.size();
}

void Lexer::skip_whitespace_and_comments() {
    while (!is_at_end()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '\n') {
            position_++;
            line_++;
            column_ = 1;
        } else if (c == '/' && peek_next() == '/') {
            // Single-line comment
            advance();
            advance();
            while (!is_at_end() && peek() != '\n') {
                advance();
            }
        } else if (c == '/' && peek_next() == '*') {
            // Multi-line comment
            advance();
            advance();
            while (!is_at_end()) {
                if (peek() == '*' && peek_next() == '/') {
                    advance();
                    advance();
                    break;
                }
                if (peek() == '\n') {
                    position_++;
                    line_++;
                    column_ = 1;
                } else {
                    advance();
                }
            }
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

    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek_next()))) {
        advance(); // consume '.'
        while (!is_at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
    }

    std::string lexeme = source_.substr(start_pos, position_ - start_pos);
    return Token{TokenType::Number, std::move(lexeme), start_line, start_col};
}

Token Lexer::scan_identifier_or_keyword() {
    static const std::unordered_map<std::string_view, TokenType> keywords = {
        {"let", TokenType::Let},
        {"if", TokenType::If},
        {"else", TokenType::Else},
        {"while", TokenType::While},
        {"print", TokenType::Print},
        {"fn", TokenType::Fn},
        {"return", TokenType::Return},
        {"true", TokenType::True},
        {"false", TokenType::False},
    };

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
    auto it = keywords.find(lexeme);
    if (it != keywords.end()) {
        type = it->second;
    }

    return Token{type, std::move(lexeme), start_line, start_col};
}

Token Lexer::scan_string() {
    int start_line = line_;
    int start_col = column_;
    advance(); // Consume opening quote '"'

    std::string value;
    while (!is_at_end() && peek() != '"') {
        char c = peek();
        if (c == '\n') {
            line_++;
            column_ = 0;
            value += advance();
        } else if (c == '\\') {
            advance(); // consume backslash
            if (is_at_end()) {
                return Token{TokenType::Invalid, "Unterminated escape sequence", start_line, start_col};
            }
            char escaped = advance();
            switch (escaped) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '"': value += '"'; break;
                case '\\': value += '\\'; break;
                default:
                    value += escaped;
                    break;
            }
        } else {
            value += advance();
        }
    }

    if (is_at_end()) {
        return Token{TokenType::Invalid, "Unterminated string", start_line, start_col};
    }

    advance(); // Consume closing quote '"'
    return Token{TokenType::String, std::move(value), start_line, start_col};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        skip_whitespace_and_comments();

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
        } else if (c == '"') {
            tokens.push_back(scan_string());
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
                case '%':
                    tokens.push_back(Token{TokenType::Percent, "%", start_line, start_col});
                    break;
                case ';':
                    tokens.push_back(Token{TokenType::Semicolon, ";", start_line, start_col});
                    break;
                case ',':
                    tokens.push_back(Token{TokenType::Comma, ",", start_line, start_col});
                    break;
                case '(':
                    tokens.push_back(Token{TokenType::LeftParen, "(", start_line, start_col});
                    break;
                case ')':
                    tokens.push_back(Token{TokenType::RightParen, ")", start_line, start_col});
                    break;
                case '{':
                    tokens.push_back(Token{TokenType::LeftBrace, "{", start_line, start_col});
                    break;
                case '}':
                    tokens.push_back(Token{TokenType::RightBrace, "}", start_line, start_col});
                    break;
                case '=':
                    if (match('=')) {
                        tokens.push_back(Token{TokenType::EqualEqual, "==", start_line, start_col});
                    } else {
                        tokens.push_back(Token{TokenType::Equal, "=", start_line, start_col});
                    }
                    break;
                case '!':
                    if (match('=')) {
                        tokens.push_back(Token{TokenType::BangEqual, "!=", start_line, start_col});
                    } else {
                        tokens.push_back(Token{TokenType::Bang, "!", start_line, start_col});
                    }
                    break;
                case '<':
                    if (match('=')) {
                        tokens.push_back(Token{TokenType::LessEqual, "<=", start_line, start_col});
                    } else {
                        tokens.push_back(Token{TokenType::Less, "<", start_line, start_col});
                    }
                    break;
                case '>':
                    if (match('=')) {
                        tokens.push_back(Token{TokenType::GreaterEqual, ">=", start_line, start_col});
                    } else {
                        tokens.push_back(Token{TokenType::Greater, ">", start_line, start_col});
                    }
                    break;
                case '&':
                    if (match('&')) {
                        tokens.push_back(Token{TokenType::AmpAmp, "&&", start_line, start_col});
                    } else {
                        tokens.push_back(Token{TokenType::Invalid, "&", start_line, start_col});
                    }
                    break;
                case '|':
                    if (match('|')) {
                        tokens.push_back(Token{TokenType::PipePipe, "||", start_line, start_col});
                    } else {
                        tokens.push_back(Token{TokenType::Invalid, "|", start_line, start_col});
                    }
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
