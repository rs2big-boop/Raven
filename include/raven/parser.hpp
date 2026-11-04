#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "raven/ast.hpp"
#include "raven/token.hpp"

namespace raven {

class ParseError : public std::runtime_error {
public:
    ParseError(Token token, const std::string& message)
        : std::runtime_error(message), token_(std::move(token)) {}

    const Token& token() const { return token_; }

private:
    Token token_;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    std::vector<std::shared_ptr<Stmt>> parse();
    std::unique_ptr<Expr> parse_expression();

    bool has_error() const { return !errors_.empty(); }
    const std::vector<std::string>& errors() const { return errors_; }

private:
    // Grammar rules
    std::shared_ptr<Stmt> declaration();
    std::shared_ptr<Stmt> function_declaration();
    std::shared_ptr<Stmt> var_declaration();
    std::shared_ptr<Stmt> statement();
    std::shared_ptr<Stmt> print_statement();
    std::shared_ptr<Stmt> if_statement();
    std::shared_ptr<Stmt> while_statement();
    std::shared_ptr<Stmt> loop_statement();
    std::shared_ptr<Stmt> for_in_statement();
    std::shared_ptr<Stmt> try_catch_statement();
    std::shared_ptr<Stmt> throw_statement();
    std::shared_ptr<Stmt> import_statement();
    std::shared_ptr<Stmt> struct_declaration();
    std::shared_ptr<Stmt> class_declaration();
    std::shared_ptr<Stmt> break_statement();
    std::shared_ptr<Stmt> continue_statement();
    std::shared_ptr<Stmt> return_statement();
    std::shared_ptr<Stmt> block_statement();
    std::shared_ptr<Stmt> expression_statement();

    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> assignment();
    std::unique_ptr<Expr> pipeline();
    std::unique_ptr<Expr> coalesce();
    std::unique_ptr<Expr> logic_or();
    std::unique_ptr<Expr> logic_and();
    std::unique_ptr<Expr> equality();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> term();
    std::unique_ptr<Expr> factor();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> call_or_index();
    std::unique_ptr<Expr> finish_call(std::unique_ptr<Expr> callee);
    std::unique_ptr<Expr> primary();
    std::unique_ptr<Expr> parse_arrow_function();
    std::unique_ptr<Expr> parse_lambda_from_paren();
    bool starts_arrow_function() const;

    // Helper methods
    bool match(const std::vector<TokenType>& types);
    bool check(TokenType type) const;
    Token advance();
    bool is_at_end() const;
    Token peek() const;
    Token peek_next() const;
    Token previous() const;
    Token consume(TokenType type, const std::string& message);
    void consume_semicolon(const std::string& message);
    ParseError error(const Token& token, const std::string& message);
    void synchronize();

    std::vector<Token> tokens_;
    std::size_t current_ = 0;
    std::vector<std::string> errors_;
    bool allow_arrow_ = true;
};

} // namespace raven
