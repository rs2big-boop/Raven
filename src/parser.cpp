#include "raven/parser.hpp"

namespace raven {

Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens)) {}

std::vector<std::shared_ptr<Stmt>> Parser::parse() {
    std::vector<std::shared_ptr<Stmt>> statements;
    while (!is_at_end()) {
        try {
            auto stmt = declaration();
            if (stmt) {
                statements.push_back(std::move(stmt));
            }
        } catch (const ParseError& e) {
            synchronize();
        }
    }
    return statements;
}

std::unique_ptr<Expr> Parser::parse_expression() {
    try {
        return expression();
    } catch (const ParseError& e) {
        return nullptr;
    }
}

std::shared_ptr<Stmt> Parser::declaration() {
    if (match({TokenType::Fn}) && check(TokenType::Identifier)) {
        return function_declaration();
    }
    if (match({TokenType::Let})) {
        return var_declaration();
    }
    if (match({TokenType::Import})) {
        return import_statement();
    }
    if (match({TokenType::Struct})) {
        return struct_declaration();
    }
    return statement();
}

std::shared_ptr<Stmt> Parser::function_declaration() {
    Token name = consume(TokenType::Identifier, "Expected function name.");
    consume(TokenType::LeftParen, "Expected '(' after function name.");

    std::vector<Token> parameters;
    if (!check(TokenType::RightParen)) {
        do {
            parameters.push_back(consume(TokenType::Identifier, "Expected parameter name."));
        } while (match({TokenType::Comma}));
    }
    consume(TokenType::RightParen, "Expected ')' after parameters.");

    consume(TokenType::LeftBrace, "Expected '{' before function body.");
    std::vector<std::shared_ptr<Stmt>> body;
    while (!check(TokenType::RightBrace) && !is_at_end()) {
        body.push_back(declaration());
    }
    consume(TokenType::RightBrace, "Expected '}' after function body.");

    return std::make_shared<FunctionStmt>(std::move(name), std::move(parameters), std::move(body));
}

std::shared_ptr<Stmt> Parser::var_declaration() {
    Token name = consume(TokenType::Identifier, "Expected variable name.");

    std::unique_ptr<Expr> initializer = nullptr;
    if (match({TokenType::Equal})) {
        initializer = expression();
    }

    consume(TokenType::Semicolon, "Expected ';' after variable declaration.");
    return std::make_shared<VarDeclStmt>(std::move(name), std::move(initializer));
}

std::shared_ptr<Stmt> Parser::statement() {
    if (match({TokenType::Print})) {
        return print_statement();
    }
    if (match({TokenType::If})) {
        return if_statement();
    }
    if (match({TokenType::While})) {
        return while_statement();
    }
    if (match({TokenType::For})) {
        return for_in_statement();
    }
    if (match({TokenType::Try})) {
        return try_catch_statement();
    }
    if (match({TokenType::Throw})) {
        return throw_statement();
    }
    if (match({TokenType::Import})) {
        return import_statement();
    }
    if (match({TokenType::Struct})) {
        return struct_declaration();
    }
    if (match({TokenType::Return})) {
        return return_statement();
    }
    if (match({TokenType::LeftBrace})) {
        return block_statement();
    }
    return expression_statement();
}

std::shared_ptr<Stmt> Parser::print_statement() {
    std::unique_ptr<Expr> value;
    if (match({TokenType::LeftParen})) {
        value = expression();
        consume(TokenType::RightParen, "Expected ')' after print argument.");
    } else {
        value = expression();
    }
    consume(TokenType::Semicolon, "Expected ';' after print statement.");
    return std::make_shared<PrintStmt>(std::move(value));
}

std::shared_ptr<Stmt> Parser::if_statement() {
    consume(TokenType::LeftParen, "Expected '(' after 'if'.");
    auto condition = expression();
    consume(TokenType::RightParen, "Expected ')' after if condition.");

    auto then_branch = statement();
    std::shared_ptr<Stmt> else_branch = nullptr;
    if (match({TokenType::Else})) {
        else_branch = statement();
    }

    return std::make_shared<IfStmt>(std::move(condition), std::move(then_branch), std::move(else_branch));
}

std::shared_ptr<Stmt> Parser::while_statement() {
    consume(TokenType::LeftParen, "Expected '(' after 'while'.");
    auto condition = expression();
    consume(TokenType::RightParen, "Expected ')' after while condition.");

    auto body = statement();
    return std::make_shared<WhileStmt>(std::move(condition), std::move(body));
}

std::shared_ptr<Stmt> Parser::for_in_statement() {
    consume(TokenType::LeftParen, "Expected '(' after 'for'.");
    match({TokenType::Let}); // Optional 'let' in 'for (let x in coll)'
    Token variable = consume(TokenType::Identifier, "Expected loop variable name.");
    consume(TokenType::In, "Expected 'in' after loop variable.");
    auto iterable = expression();
    consume(TokenType::RightParen, "Expected ')' after for clauses.");

    auto body = statement();
    return std::make_shared<ForInStmt>(std::move(variable), std::move(iterable), std::move(body));
}

std::shared_ptr<Stmt> Parser::try_catch_statement() {
    auto try_block = statement();
    consume(TokenType::Catch, "Expected 'catch' after try block.");
    consume(TokenType::LeftParen, "Expected '(' after 'catch'.");
    Token error_var = consume(TokenType::Identifier, "Expected error variable name.");
    consume(TokenType::RightParen, "Expected ')' after error variable.");
    auto catch_block = statement();

    return std::make_shared<TryCatchStmt>(std::move(try_block), std::move(error_var), std::move(catch_block));
}

std::shared_ptr<Stmt> Parser::throw_statement() {
    Token keyword = previous();
    auto value = expression();
    consume(TokenType::Semicolon, "Expected ';' after throw expression.");
    return std::make_shared<ThrowStmt>(std::move(keyword), std::move(value));
}

std::shared_ptr<Stmt> Parser::import_statement() {
    Token keyword = previous();
    std::string path;
    if (match({TokenType::String})) {
        path = previous().lexeme;
    } else if (match({TokenType::Identifier})) {
        path = previous().lexeme;
    } else {
        throw error(peek(), "Expected module path (string or identifier) after 'import'.");
    }
    consume(TokenType::Semicolon, "Expected ';' after import statement.");
    return std::make_shared<ImportStmt>(std::move(keyword), std::move(path));
}

std::shared_ptr<Stmt> Parser::struct_declaration() {
    Token name = consume(TokenType::Identifier, "Expected struct name.");
    consume(TokenType::LeftBrace, "Expected '{' after struct name.");
    std::vector<Token> fields;
    if (!check(TokenType::RightBrace)) {
        do {
            fields.push_back(consume(TokenType::Identifier, "Expected field name in struct."));
        } while (match({TokenType::Comma}));
    }
    consume(TokenType::RightBrace, "Expected '}' after struct fields.");
    match({TokenType::Semicolon});
    return std::make_shared<StructStmt>(std::move(name), std::move(fields));
}

std::shared_ptr<Stmt> Parser::return_statement() {
    Token keyword = previous();
    std::unique_ptr<Expr> value = nullptr;
    if (!check(TokenType::Semicolon)) {
        value = expression();
    }
    consume(TokenType::Semicolon, "Expected ';' after return value.");
    return std::make_shared<ReturnStmt>(std::move(keyword), std::move(value));
}

std::shared_ptr<Stmt> Parser::block_statement() {
    std::vector<std::shared_ptr<Stmt>> statements;
    while (!check(TokenType::RightBrace) && !is_at_end()) {
        statements.push_back(declaration());
    }
    consume(TokenType::RightBrace, "Expected '}' after block.");
    return std::make_shared<BlockStmt>(std::move(statements));
}

std::shared_ptr<Stmt> Parser::expression_statement() {
    auto expr = expression();
    consume(TokenType::Semicolon, "Expected ';' after expression.");
    return std::make_shared<ExpressionStmt>(std::move(expr));
}

std::unique_ptr<Expr> Parser::expression() {
    return assignment();
}

std::unique_ptr<Expr> Parser::assignment() {
    auto expr = pipeline();

    if (match({TokenType::Equal})) {
        Token equals = previous();
        auto value = assignment();

        if (auto* var_expr = dynamic_cast<VariableExpr*>(expr.get())) {
            Token name = var_expr->name;
            return std::make_unique<AssignExpr>(std::move(name), std::move(value));
        }

        if (auto* idx_expr = dynamic_cast<IndexExpr*>(expr.get())) {
            return std::make_unique<IndexAssignExpr>(
                std::move(idx_expr->object),
                idx_expr->bracket,
                std::move(idx_expr->index),
                std::move(value)
            );
        }

        if (auto* prop_expr = dynamic_cast<PropertyExpr*>(expr.get())) {
            return std::make_unique<PropertyAssignExpr>(
                std::move(prop_expr->object),
                prop_expr->name,
                std::move(value)
            );
        }

        throw error(equals, "Invalid assignment target.");
    }

    return expr;
}

std::unique_ptr<Expr> Parser::pipeline() {
    auto expr = logic_or();

    while (match({TokenType::PipeGreater})) {
        auto right = logic_or();
        expr = std::make_unique<PipelineExpr>(std::move(expr), std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::logic_or() {
    auto expr = logic_and();

    while (match({TokenType::PipePipe})) {
        Token op = previous();
        auto right = logic_and();
        expr = std::make_unique<LogicalExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::logic_and() {
    auto expr = equality();

    while (match({TokenType::AmpAmp})) {
        Token op = previous();
        auto right = equality();
        expr = std::make_unique<LogicalExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::equality() {
    auto expr = comparison();

    while (match({TokenType::EqualEqual, TokenType::BangEqual})) {
        Token op = previous();
        auto right = comparison();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::comparison() {
    auto expr = term();

    while (match({TokenType::Greater, TokenType::GreaterEqual, TokenType::Less, TokenType::LessEqual})) {
        Token op = previous();
        auto right = term();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::term() {
    auto expr = factor();

    while (match({TokenType::Plus, TokenType::Minus})) {
        Token op = previous();
        auto right = factor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::factor() {
    auto expr = unary();

    while (match({TokenType::Star, TokenType::Slash, TokenType::Percent})) {
        Token op = previous();
        auto right = unary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::unary() {
    if (match({TokenType::Bang, TokenType::Minus})) {
        Token op = previous();
        auto right = unary();
        return std::make_unique<UnaryExpr>(std::move(op), std::move(right));
    }

    return call_or_index();
}

std::unique_ptr<Expr> Parser::call_or_index() {
    auto expr = primary();

    while (true) {
        if (match({TokenType::LeftParen})) {
            expr = finish_call(std::move(expr));
        } else if (match({TokenType::LeftBracket})) {
            Token bracket = previous();
            auto index = expression();
            consume(TokenType::RightBracket, "Expected ']' after index.");
            expr = std::make_unique<IndexExpr>(std::move(expr), bracket, std::move(index));
        } else if (match({TokenType::Dot})) {
            Token name = consume(TokenType::Identifier, "Expected property name after '.'.");
            expr = std::make_unique<PropertyExpr>(std::move(expr), std::move(name));
        } else {
            break;
        }
    }

    return expr;
}

std::unique_ptr<Expr> Parser::finish_call(std::unique_ptr<Expr> callee) {
    std::vector<std::unique_ptr<Expr>> arguments;

    if (!check(TokenType::RightParen)) {
        do {
            arguments.push_back(expression());
        } while (match({TokenType::Comma}));
    }

    Token paren = consume(TokenType::RightParen, "Expected ')' after arguments.");
    return std::make_unique<CallExpr>(std::move(callee), std::move(paren), std::move(arguments));
}

std::unique_ptr<Expr> Parser::primary() {
    if (match({TokenType::False})) {
        return std::make_unique<LiteralExpr>(Value(false));
    }
    if (match({TokenType::True})) {
        return std::make_unique<LiteralExpr>(Value(true));
    }
    if (match({TokenType::Number})) {
        return std::make_unique<LiteralExpr>(Value(std::stod(previous().lexeme)));
    }
    if (match({TokenType::String})) {
        return std::make_unique<LiteralExpr>(Value(previous().lexeme));
    }
    if (match({TokenType::Identifier})) {
        return std::make_unique<VariableExpr>(previous());
    }

    // Array literal: [elem1, elem2, ...]
    if (match({TokenType::LeftBracket})) {
        std::vector<std::unique_ptr<Expr>> elements;
        if (!check(TokenType::RightBracket)) {
            do {
                elements.push_back(expression());
            } while (match({TokenType::Comma}));
        }
        consume(TokenType::RightBracket, "Expected ']' after array elements.");
        return std::make_unique<ArrayExpr>(std::move(elements));
    }

    // Map literal: { key: value, ... }
    if (match({TokenType::LeftBrace})) {
        std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> entries;
        if (!check(TokenType::RightBrace)) {
            do {
                auto key = expression();
                consume(TokenType::Colon, "Expected ':' after map key.");
                auto val = expression();
                entries.emplace_back(std::move(key), std::move(val));
            } while (match({TokenType::Comma}));
        }
        consume(TokenType::RightBrace, "Expected '}' after map entries.");
        return std::make_unique<MapExpr>(std::move(entries));
    }

    // Anonymous lambda: fn(a, b) { ... }
    if (match({TokenType::Fn})) {
        consume(TokenType::LeftParen, "Expected '(' after 'fn'.");
        std::vector<Token> parameters;
        if (!check(TokenType::RightParen)) {
            do {
                parameters.push_back(consume(TokenType::Identifier, "Expected parameter name."));
            } while (match({TokenType::Comma}));
        }
        consume(TokenType::RightParen, "Expected ')' after parameters.");
        consume(TokenType::LeftBrace, "Expected '{' before function body.");
        std::vector<std::shared_ptr<Stmt>> body;
        while (!check(TokenType::RightBrace) && !is_at_end()) {
            body.push_back(declaration());
        }
        consume(TokenType::RightBrace, "Expected '}' after function body.");
        return std::make_unique<LambdaExpr>(std::move(parameters), std::move(body));
    }

    // Match expression: match (expr) { pattern => body, _ => default }
    if (match({TokenType::Match})) {
        std::unique_ptr<Expr> target;
        if (match({TokenType::LeftParen})) {
            target = expression();
            consume(TokenType::RightParen, "Expected ')' after match target.");
        } else {
            target = expression();
        }

        consume(TokenType::LeftBrace, "Expected '{' before match arms.");
        std::vector<MatchArm> arms;
        while (!check(TokenType::RightBrace) && !is_at_end()) {
            MatchArm arm;
            if (check(TokenType::Identifier) && peek().lexeme == "_") {
                advance();
                arm.is_wildcard = true;
            } else {
                arm.pattern = expression();
            }
            consume(TokenType::Arrow, "Expected '=>' after match pattern.");
            arm.body = expression();
            arms.push_back(std::move(arm));

            match({TokenType::Comma, TokenType::Semicolon});
        }
        consume(TokenType::RightBrace, "Expected '}' after match arms.");
        return std::make_unique<MatchExpr>(std::move(target), std::move(arms));
    }

    if (match({TokenType::LeftParen})) {
        auto expr = expression();
        consume(TokenType::RightParen, "Expected ')' after expression.");
        return std::make_unique<GroupingExpr>(std::move(expr));
    }

    throw error(peek(), "Expected expression.");
}

bool Parser::match(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (is_at_end()) return false;
    return peek().type == type;
}

Token Parser::advance() {
    if (!is_at_end()) current_++;
    return previous();
}

bool Parser::is_at_end() const {
    return peek().type == TokenType::EndOfFile;
}

Token Parser::peek() const {
    return tokens_[current_];
}

Token Parser::peek_next() const {
    if (current_ + 1 >= tokens_.size()) return tokens_.back();
    return tokens_[current_ + 1];
}

Token Parser::previous() const {
    return tokens_[current_ - 1];
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw error(peek(), message);
}

ParseError Parser::error(const Token& token, const std::string& message) {
    std::string err = "[line " + std::to_string(token.line) + ", col " +
                      std::to_string(token.column) + "] Error";
    if (token.type == TokenType::EndOfFile) {
        err += " at end: " + message;
    } else {
        err += " at '" + token.lexeme + "': " + message;
    }
    errors_.push_back(err);
    return ParseError(token, err);
}

void Parser::synchronize() {
    advance();

    while (!is_at_end()) {
        if (previous().type == TokenType::Semicolon) return;

        switch (peek().type) {
            case TokenType::Fn:
            case TokenType::Let:
            case TokenType::If:
            case TokenType::While:
            case TokenType::For:
            case TokenType::Try:
            case TokenType::Throw:
            case TokenType::Print:
            case TokenType::Return:
                return;
            default:
                break;
        }

        advance();
    }
}

} // namespace raven
