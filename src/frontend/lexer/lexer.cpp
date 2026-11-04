#include "raven/lexer.hpp"

#include <cctype>
#include <unordered_map>
#include <utility>

namespace raven {

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
            advance();
            advance();
            while (!is_at_end() && peek() != '\n') {
                advance();
            }
        } else if (c == '/' && peek_next() == '*') {
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
        advance();
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
        {"for", TokenType::For},
        {"in", TokenType::In},
        {"try", TokenType::Try},
        {"catch", TokenType::Catch},
        {"throw", TokenType::Throw},
        {"match", TokenType::Match},
        {"import", TokenType::Import},
        {"struct", TokenType::Struct},
        {"class", TokenType::Class},
        {"break", TokenType::Break},
        {"continue", TokenType::Continue},
        {"print", TokenType::Print},
        {"fn", TokenType::Fn},
        {"return", TokenType::Return},
        {"true", TokenType::True},
        {"false", TokenType::False},
        {"nil", TokenType::Nil},
        {"null", TokenType::Nil},
        {"loop", TokenType::Loop},
        {"is", TokenType::Is},
        {"this", TokenType::This},
        {"extends", TokenType::Extends},
        {"super", TokenType::Super},
        {"static", TokenType::Static},
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
    advance(); // consume opening "

    bool is_triple = false;
    if (!is_at_end() && peek() == '"' && peek_next() == '"') {
        is_triple = true;
        advance(); // second "
        advance(); // third "
    }

    std::string value;
    if (is_triple) {
        while (!is_at_end()) {
            if (peek() == '"' && peek_next() == '"' &&
                position_ + 2 < source_.size() && source_[position_ + 2] == '"') {
                advance(); // 1
                advance(); // 2
                advance(); // 3
                return Token{TokenType::String, std::move(value), start_line, start_col};
            }
            char c = peek();
            if (c == '\n') {
                line_++;
                column_ = 0;
                value += advance();
            } else if (c == '\\') {
                advance();
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
        return Token{TokenType::Invalid, "Unterminated multi-line string", start_line, start_col};
    }

    while (!is_at_end() && peek() != '"') {
        char c = peek();
        if (c == '\n') {
            line_++;
            column_ = 0;
            value += advance();
        } else if (c == '\\') {
            advance();
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

    advance();
    return Token{TokenType::String, std::move(value), start_line, start_col};
}

std::vector<Token> Lexer::scan_format_string() {
    int start_line = line_;
    int start_col = column_;
    advance(); // consume 'f'
    advance(); // consume '"'

    std::vector<Token> result;
    result.push_back(Token{TokenType::LeftParen, "(", start_line, start_col});
    result.push_back(Token{TokenType::String, "", start_line, start_col});

    std::string text_chunk;
    while (!is_at_end() && peek() != '"') {
        char c = peek();
        if (c == '\\') {
            advance();
            if (is_at_end()) break;
            char escaped = advance();
            switch (escaped) {
                case 'n': text_chunk += '\n'; break;
                case 't': text_chunk += '\t'; break;
                case 'r': text_chunk += '\r'; break;
                case '"': text_chunk += '"'; break;
                case '\\': text_chunk += '\\'; break;
                case '{': text_chunk += '{'; break;
                case '}': text_chunk += '}'; break;
                default: text_chunk += escaped; break;
            }
        } else if (c == '{') {
            if (!text_chunk.empty()) {
                result.push_back(Token{TokenType::Plus, "+", line_, column_});
                result.push_back(Token{TokenType::String, std::move(text_chunk), line_, column_});
                text_chunk.clear();
            }
            advance(); // consume '{'
            std::string expr_str;
            int brace_depth = 1;
            while (!is_at_end() && brace_depth > 0) {
                if (peek() == '\\') {
                    advance(); // skip backslash
                    if (!is_at_end()) expr_str += advance();
                } else if (peek() == '{') {
                    brace_depth++;
                    expr_str += advance();
                } else if (peek() == '}') {
                    brace_depth--;
                    if (brace_depth == 0) {
                        advance(); // consume matching '}'
                        break;
                    }
                    expr_str += advance();
                } else {
                    expr_str += advance();
                }
            }
            Lexer sub_lexer(expr_str);
            auto expr_tokens = sub_lexer.tokenize();
            if (!expr_tokens.empty() && expr_tokens.back().type == TokenType::EndOfFile) {
                expr_tokens.pop_back();
            }
            result.push_back(Token{TokenType::Plus, "+", line_, column_});
            result.push_back(Token{TokenType::Identifier, "str", line_, column_});
            result.push_back(Token{TokenType::LeftParen, "(", line_, column_});
            result.insert(result.end(), expr_tokens.begin(), expr_tokens.end());
            result.push_back(Token{TokenType::RightParen, ")", line_, column_});
        } else {
            if (c == '\n') {
                line_++;
                column_ = 0;
            }
            text_chunk += advance();
        }
    }

    if (!text_chunk.empty()) {
        result.push_back(Token{TokenType::Plus, "+", line_, column_});
        result.push_back(Token{TokenType::String, std::move(text_chunk), line_, column_});
    }

    if (!is_at_end() && peek() == '"') {
        advance(); // consume closing '"'
    }

    result.push_back(Token{TokenType::RightParen, ")", start_line, start_col});
    return result;
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

        if (c == 'f' && peek_next() == '"') {
            auto f_tokens = scan_format_string();
            tokens.insert(tokens.end(), f_tokens.begin(), f_tokens.end());
        } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(scan_identifier_or_keyword());
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(scan_number());
        } else if (c == '"') {
            tokens.push_back(scan_string());
        } else {
            advance();
            switch (c) {
                case '+':
                    tokens.push_back(match('=')
                        ? Token{TokenType::PlusEqual, "+=", start_line, start_col}
                        : Token{TokenType::Plus, "+", start_line, start_col});
                    break;
                case '-':
                    tokens.push_back(match('=')
                        ? Token{TokenType::MinusEqual, "-=", start_line, start_col}
                        : Token{TokenType::Minus, "-", start_line, start_col});
                    break;
                case '*':
                    tokens.push_back(match('=')
                        ? Token{TokenType::StarEqual, "*=", start_line, start_col}
                        : Token{TokenType::Star, "*", start_line, start_col});
                    break;
                case '/':
                    tokens.push_back(match('=')
                        ? Token{TokenType::SlashEqual, "/=", start_line, start_col}
                        : Token{TokenType::Slash, "/", start_line, start_col});
                    break;
                case '%':
                    tokens.push_back(match('=')
                        ? Token{TokenType::PercentEqual, "%=", start_line, start_col}
                        : Token{TokenType::Percent, "%", start_line, start_col});
                    break;
                case ';':
                    tokens.push_back(Token{TokenType::Semicolon, ";", start_line, start_col});
                    break;
                case ':':
                    tokens.push_back(Token{TokenType::Colon, ":", start_line, start_col});
                    break;
                case '.':
                    if (peek() == '.' && peek_next() == '.') {
                        advance();
                        advance();
                        tokens.push_back(Token{TokenType::DotDotDot, "...", start_line, start_col});
                    } else {
                        tokens.push_back(Token{TokenType::Dot, ".", start_line, start_col});
                    }
                    break;
                case '?':
                    if (match('?')) {
                        tokens.push_back(Token{TokenType::QuestionQuestion, "??", start_line, start_col});
                    } else if (match('.')) {
                        tokens.push_back(Token{TokenType::QuestionDot, "?.", start_line, start_col});
                    } else {
                        tokens.push_back(Token{TokenType::Invalid, "?", start_line, start_col});
                    }
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
                case '[':
                    tokens.push_back(Token{TokenType::LeftBracket, "[", start_line, start_col});
                    break;
                case ']':
                    tokens.push_back(Token{TokenType::RightBracket, "]", start_line, start_col});
                    break;
                case '=':
                    if (match('=')) {
                        tokens.push_back(Token{TokenType::EqualEqual, "==", start_line, start_col});
                    } else if (match('>')) {
                        tokens.push_back(Token{TokenType::Arrow, "=>", start_line, start_col});
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
                    if (match('>')) {
                        tokens.push_back(Token{TokenType::PipeGreater, "|>", start_line, start_col});
                    } else if (match('|')) {
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

} // namespace raven
