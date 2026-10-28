#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "raven/token.hpp"

namespace raven {

class Lexer {
public:
    explicit Lexer(std::string source);

    std::vector<Token> tokenize();

private:
    char peek() const;
    char peek_next() const;
    char advance();
    bool match(char expected);
    bool is_at_end() const;

    void skip_whitespace_and_comments();
    Token scan_number();
    Token scan_identifier_or_keyword();
    Token scan_string();
    std::vector<Token> scan_format_string();

    std::string source_;
    std::size_t position_ = 0;
    int line_ = 1;
    int column_ = 1;
};

} // namespace raven
