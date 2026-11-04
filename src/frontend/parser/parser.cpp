#include "raven/parser.hpp"

namespace raven {

namespace {

std::unique_ptr<Expr> clone_expr(const Expr* expr) {
    if (!expr) {
        return nullptr;
    }
    if (auto* var_expr = dynamic_cast<const VariableExpr*>(expr)) {
        return std::make_unique<VariableExpr>(var_expr->name);
    }
    if (auto* literal = dynamic_cast<const LiteralExpr*>(expr)) {
        return std::make_unique<LiteralExpr>(literal->value);
    }
    if (auto* prop = dynamic_cast<const PropertyExpr*>(expr)) {
        auto object = clone_expr(prop->object.get());
        if (!object) {
            return nullptr;
        }
        return std::make_unique<PropertyExpr>(std::move(object), prop->name, prop->is_optional);
    }
    if (auto* index = dynamic_cast<const IndexExpr*>(expr)) {
        if (index->is_slice) {
            return nullptr;
        }
        auto object = clone_expr(index->object.get());
        auto idx = clone_expr(index->index.get());
        if (!object || !idx) {
            return nullptr;
        }
        return std::make_unique<IndexExpr>(std::move(object), index->bracket, std::move(idx), index->is_optional);
    }
    return nullptr;
}

} // namespace


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
    if (match({TokenType::Class})) {
        return class_declaration();
    }
    return statement();
}

std::shared_ptr<Stmt> Parser::function_declaration() {
    Token name = consume(TokenType::Identifier, "Expected function name.");
    consume(TokenType::LeftParen, "Expected '(' after function name.");

    std::vector<Token> parameters;
    std::vector<std::shared_ptr<Expr>> param_defaults;
    bool is_variadic = false;
    if (!check(TokenType::RightParen)) {
        do {
            if (match({TokenType::DotDotDot})) {
                is_variadic = true;
                parameters.push_back(consume(TokenType::Identifier, "Expected parameter name after '...'."));
                param_defaults.push_back(nullptr);
                break;
            }
            parameters.push_back(consume(TokenType::Identifier, "Expected parameter name."));
            if (match({TokenType::Colon})) {
                consume(TokenType::Identifier, "Expected type name after ':'.");
            }
            if (match({TokenType::Equal})) {
                param_defaults.push_back(expression());
            } else {
                param_defaults.push_back(nullptr);
            }
        } while (match({TokenType::Comma}));
    }
    consume(TokenType::RightParen, "Expected ')' after parameters.");

    if (match({TokenType::Colon}) || match({TokenType::Arrow})) {
        consume(TokenType::Identifier, "Expected return type.");
    }

    consume(TokenType::LeftBrace, "Expected '{' before function body.");
    std::vector<std::shared_ptr<Stmt>> body;
    while (!check(TokenType::RightBrace) && !is_at_end()) {
        body.push_back(declaration());
    }
    consume(TokenType::RightBrace, "Expected '}' after function body.");

    return std::make_shared<FunctionStmt>(std::move(name), std::move(parameters), std::move(body), std::move(param_defaults), is_variadic);
}

std::shared_ptr<Stmt> Parser::var_declaration() {
    if (match({TokenType::LeftBracket})) {
        std::vector<Token> names;
        if (!check(TokenType::RightBracket)) {
            do {
                names.push_back(consume(TokenType::Identifier, "Expected variable name in array destructuring."));
            } while (match({TokenType::Comma}));
        }
        consume(TokenType::RightBracket, "Expected ']' after destructuring pattern.");
        consume(TokenType::Equal, "Expected '=' after destructuring pattern.");
        auto initializer = expression();
        consume_semicolon("Expected ';' after variable declaration.");
        return std::make_shared<DestructureStmt>(DestructureKind::Array, std::move(names), std::move(initializer));
    }

    if (match({TokenType::LeftBrace})) {
        std::vector<Token> names;
        if (!check(TokenType::RightBrace)) {
            do {
                names.push_back(consume(TokenType::Identifier, "Expected variable name in map destructuring."));
            } while (match({TokenType::Comma}));
        }
        consume(TokenType::RightBrace, "Expected '}' after destructuring pattern.");
        consume(TokenType::Equal, "Expected '=' after destructuring pattern.");
        auto initializer = expression();
        consume_semicolon("Expected ';' after variable declaration.");
        return std::make_shared<DestructureStmt>(DestructureKind::Map, std::move(names), std::move(initializer));
    }

    Token name = consume(TokenType::Identifier, "Expected variable name.");
    std::string type_annotation;
    if (match({TokenType::Colon})) {
        Token type_token = consume(TokenType::Identifier, "Expected type name after ':'.");
        type_annotation = type_token.lexeme;
    }

    std::unique_ptr<Expr> initializer = nullptr;
    if (match({TokenType::Equal})) {
        initializer = expression();
    }

    consume_semicolon("Expected ';' after variable declaration.");
    return std::make_shared<VarDeclStmt>(std::move(name), std::move(initializer), std::move(type_annotation));
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
    if (match({TokenType::Loop})) {
        return loop_statement();
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
    if (match({TokenType::Class})) {
        return class_declaration();
    }
    if (match({TokenType::Break})) {
        return break_statement();
    }
    if (match({TokenType::Continue})) {
        return continue_statement();
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
    std::vector<std::unique_ptr<Expr>> values;
    if (match({TokenType::LeftParen})) {
        if (!check(TokenType::RightParen)) {
            do {
                values.push_back(expression());
            } while (match({TokenType::Comma}));
        }
        consume(TokenType::RightParen, "Expected ')' after print argument.");
    } else {
        do {
            values.push_back(expression());
        } while (match({TokenType::Comma}));
    }
    consume_semicolon("Expected ';' after print statement.");
    return std::make_shared<PrintStmt>(std::move(values));
}

std::shared_ptr<Stmt> Parser::if_statement() {
    std::unique_ptr<Expr> condition;
    if (match({TokenType::LeftParen})) {
        condition = expression();
        consume(TokenType::RightParen, "Expected ')' after if condition.");
    } else {
        condition = expression();
    }

    auto then_branch = statement();
    std::shared_ptr<Stmt> else_branch = nullptr;
    if (match({TokenType::Else})) {
        else_branch = statement();
    }

    return std::make_shared<IfStmt>(std::move(condition), std::move(then_branch), std::move(else_branch));
}

std::shared_ptr<Stmt> Parser::while_statement() {
    std::unique_ptr<Expr> condition;
    if (match({TokenType::LeftParen})) {
        condition = expression();
        consume(TokenType::RightParen, "Expected ')' after while condition.");
    } else {
        condition = expression();
    }

    auto body = statement();
    return std::make_shared<WhileStmt>(std::move(condition), std::move(body));
}

std::shared_ptr<Stmt> Parser::loop_statement() {
    consume(TokenType::LeftBrace, "Expected '{' after 'loop'.");
    std::vector<std::shared_ptr<Stmt>> body_stmts;
    while (!check(TokenType::RightBrace) && !is_at_end()) {
        body_stmts.push_back(declaration());
    }
    consume(TokenType::RightBrace, "Expected '}' after loop body.");
    auto body = std::make_shared<BlockStmt>(std::move(body_stmts));
    return std::make_shared<WhileStmt>(
        std::make_unique<LiteralExpr>(Value(true)),
        std::move(body)
    );
}

std::shared_ptr<Stmt> Parser::for_in_statement() {
    bool has_paren = match({TokenType::LeftParen});
    match({TokenType::Let}); // Optional 'let' in 'for (let x in coll)'
    Token variable = consume(TokenType::Identifier, "Expected loop variable name.");
    consume(TokenType::In, "Expected 'in' after loop variable.");
    auto iterable = expression();
    if (has_paren) {
        consume(TokenType::RightParen, "Expected ')' after for clauses.");
    }

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
    consume_semicolon("Expected ';' after throw expression.");
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
    consume_semicolon("Expected ';' after import statement.");
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

std::shared_ptr<Stmt> Parser::class_declaration() {
    Token name = consume(TokenType::Identifier, "Expected class name.");
    Token superclass;
    bool has_superclass = false;
    if (match({TokenType::Extends})) {
        superclass = consume(TokenType::Identifier, "Expected superclass name after 'extends'.");
        has_superclass = true;
    }
    consume(TokenType::LeftBrace, "Expected '{' after class name.");

    std::vector<Token> fields;
    std::vector<ClassMember> members;

    while (!check(TokenType::RightBrace) && !is_at_end()) {
        bool is_static = false;
        if (match({TokenType::Static})) {
            is_static = true;
        }
        if (match({TokenType::Fn})) {
            // optional fn keyword before a method
        }

        bool is_method = check(TokenType::Identifier) && peek_next().type == TokenType::LeftParen;
        if (!is_method && !is_static) {
            Token field = consume(TokenType::Identifier, "Expected field or method in class.");
            fields.push_back(field);
            while (match({TokenType::Comma})) {
                fields.push_back(consume(TokenType::Identifier, "Expected field name."));
            }
            match({TokenType::Semicolon});
            continue;
        }

        ClassMember member;
        member.name = advance();
        member.is_constructor = !is_static && (member.name.lexeme == name.lexeme);
        member.is_static = is_static;
        consume(TokenType::LeftParen, "Expected '(' after method name.");
        if (!check(TokenType::RightParen)) {
            do {
                if (match({TokenType::DotDotDot})) {
                    member.is_variadic = true;
                    member.params.push_back(consume(TokenType::Identifier, "Expected parameter name after '...'."));
                    member.defaults.push_back(nullptr);
                    break;
                }
                member.params.push_back(consume(TokenType::Identifier, "Expected parameter name."));
                if (match({TokenType::Equal})) {
                    member.defaults.push_back(expression());
                } else {
                    member.defaults.push_back(nullptr);
                }
            } while (match({TokenType::Comma}));
        }
        consume(TokenType::RightParen, "Expected ')' after parameters.");
        consume(TokenType::LeftBrace, "Expected '{' before method body.");
        while (!check(TokenType::RightBrace) && !is_at_end()) {
            member.body.push_back(declaration());
        }
        consume(TokenType::RightBrace, "Expected '}' after method body.");
        members.push_back(std::move(member));
    }

    consume(TokenType::RightBrace, "Expected '}' after class body.");
    match({TokenType::Semicolon});
    return std::make_shared<ClassStmt>(std::move(name), std::move(fields), std::move(members), std::move(superclass), has_superclass);
}

std::shared_ptr<Stmt> Parser::break_statement() {
    Token keyword = previous();
    consume_semicolon("Expected ';' after 'break'.");
    return std::make_shared<BreakStmt>(keyword);
}

std::shared_ptr<Stmt> Parser::continue_statement() {
    Token keyword = previous();
    consume_semicolon("Expected ';' after 'continue'.");
    return std::make_shared<ContinueStmt>(keyword);
}

std::shared_ptr<Stmt> Parser::return_statement() {
    Token keyword = previous();
    std::unique_ptr<Expr> value = nullptr;
    if (!check(TokenType::Semicolon) && !check(TokenType::RightBrace) && !is_at_end() &&
        (current_ == 0 || previous().line == peek().line)) {
        value = expression();
    }
    consume_semicolon("Expected ';' after return value.");
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
    consume_semicolon("Expected ';' after expression.");
    return std::make_shared<ExpressionStmt>(std::move(expr));
}

std::unique_ptr<Expr> Parser::expression() {
    return assignment();
}

std::unique_ptr<Expr> Parser::assignment() {
    auto expr = pipeline();

    if (match({TokenType::Equal, TokenType::PlusEqual, TokenType::MinusEqual, TokenType::StarEqual, TokenType::SlashEqual, TokenType::PercentEqual})) {
        Token equals = previous();
        auto value = assignment();

        TokenType bin_op = TokenType::Plus;
        std::string op_lexeme = "+";
        if (equals.type == TokenType::PlusEqual) { bin_op = TokenType::Plus; op_lexeme = "+"; }
        else if (equals.type == TokenType::MinusEqual) { bin_op = TokenType::Minus; op_lexeme = "-"; }
        else if (equals.type == TokenType::StarEqual) { bin_op = TokenType::Star; op_lexeme = "*"; }
        else if (equals.type == TokenType::SlashEqual) { bin_op = TokenType::Slash; op_lexeme = "/"; }
        else if (equals.type == TokenType::PercentEqual) { bin_op = TokenType::Percent; op_lexeme = "%"; }

        if (equals.type != TokenType::Equal) {
            auto left = clone_expr(expr.get());
            if (!left) {
                throw error(equals, "Compound assignment is not supported for this target.");
            }
            Token bin_token{bin_op, op_lexeme, equals.line, equals.column};
            value = std::make_unique<BinaryExpr>(std::move(left), bin_token, std::move(value));
        }

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
    auto expr = coalesce();

    while (match({TokenType::PipeGreater})) {
        auto right = coalesce();
        expr = std::make_unique<PipelineExpr>(std::move(expr), std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::coalesce() {
    auto expr = logic_or();

    while (match({TokenType::QuestionQuestion})) {
        Token op = previous();
        auto right = logic_or();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
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

    while (match({TokenType::Greater, TokenType::GreaterEqual, TokenType::Less, TokenType::LessEqual, TokenType::Is})) {
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
            std::unique_ptr<Expr> start = nullptr;
            std::unique_ptr<Expr> end = nullptr;
            bool is_slice = false;

            if (match({TokenType::Colon})) {
                is_slice = true;
                if (!check(TokenType::RightBracket)) {
                    end = expression();
                }
            } else {
                start = expression();
                if (match({TokenType::Colon})) {
                    is_slice = true;
                    if (!check(TokenType::RightBracket)) {
                        end = expression();
                    }
                }
            }
            consume(TokenType::RightBracket, "Expected ']' after index.");
            if (is_slice) {
                expr = std::make_unique<IndexExpr>(std::move(expr), bracket, std::move(start), std::move(end), false);
            } else {
                expr = std::make_unique<IndexExpr>(std::move(expr), bracket, std::move(start));
            }
        } else if (match({TokenType::Dot})) {
            Token name = consume(TokenType::Identifier, "Expected property name after '.'.");
            expr = std::make_unique<PropertyExpr>(std::move(expr), std::move(name));
        } else if (match({TokenType::QuestionDot})) {
            if (match({TokenType::LeftBracket})) {
                Token bracket = previous();
                std::unique_ptr<Expr> start = nullptr;
                std::unique_ptr<Expr> end = nullptr;
                bool is_slice = false;

                if (match({TokenType::Colon})) {
                    is_slice = true;
                    if (!check(TokenType::RightBracket)) {
                        end = expression();
                    }
                } else {
                    start = expression();
                    if (match({TokenType::Colon})) {
                        is_slice = true;
                        if (!check(TokenType::RightBracket)) {
                            end = expression();
                        }
                    }
                }
                consume(TokenType::RightBracket, "Expected ']' after index.");
                if (is_slice) {
                    expr = std::make_unique<IndexExpr>(std::move(expr), bracket, std::move(start), std::move(end), true);
                } else {
                    expr = std::make_unique<IndexExpr>(std::move(expr), bracket, std::move(start), true);
                }
            } else {
                Token name = consume(TokenType::Identifier, "Expected property name after '?.'.");
                expr = std::make_unique<PropertyExpr>(std::move(expr), std::move(name), true);
            }
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
    if (match({TokenType::Nil})) {
        return std::make_unique<LiteralExpr>(Value());
    }
    if (match({TokenType::Number})) {
        return std::make_unique<LiteralExpr>(Value(std::stod(previous().lexeme)));
    }
    if (match({TokenType::String})) {
        return std::make_unique<LiteralExpr>(Value(previous().lexeme));
    }
    // Arrow function must win over a bare identifier: x => x * 2
    if (allow_arrow_ && starts_arrow_function()) {
        return parse_arrow_function();
    }
    if (match({TokenType::Identifier}) || match({TokenType::This}) || match({TokenType::Super})) {
        return std::make_unique<VariableExpr>(previous());
    }

    // Array literal: [elem1, elem2, ...]
    if (match({TokenType::LeftBracket})) {
        std::vector<std::unique_ptr<Expr>> elements;
        if (!check(TokenType::RightBracket)) {
            do {
                if (match({TokenType::DotDotDot})) {
                    Token dot_tok = previous();
                    auto spread_operand = expression();
                    elements.push_back(std::make_unique<UnaryExpr>(dot_tok, std::move(spread_operand)));
                } else {
                    elements.push_back(expression());
                }
            } while (match({TokenType::Comma}));
        }
        consume(TokenType::RightBracket, "Expected ']' after array elements.");
        return std::make_unique<ArrayExpr>(std::move(elements));
    }

    // Map literal: { key: value, shorthand, "quoted": value, ... }
    if (match({TokenType::LeftBrace})) {
        std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> entries;
        if (!check(TokenType::RightBrace)) {
            do {
                if (match({TokenType::DotDotDot})) {
                    Token dot_tok = previous();
                    auto spread_map = expression();
                    entries.emplace_back(std::make_unique<UnaryExpr>(dot_tok, std::move(spread_map)), nullptr);
                } else if (check(TokenType::Identifier) && peek_next().type == TokenType::LeftParen) {
                    // Method shorthand: { get() { return this.data; } }
                    Token name = advance();
                    auto method = parse_lambda_from_paren();
                    entries.emplace_back(std::make_unique<LiteralExpr>(Value(name.lexeme)), std::move(method));
                } else if (check(TokenType::Identifier) &&
                           (peek_next().type == TokenType::Colon ||
                            peek_next().type == TokenType::Comma ||
                            peek_next().type == TokenType::RightBrace)) {
                    Token key_tok = advance();
                    auto key = std::make_unique<LiteralExpr>(Value(key_tok.lexeme));
                    if (match({TokenType::Colon})) {
                        entries.emplace_back(std::move(key), expression());
                    } else {
                        // { data }  =>  { "data": data }
                        entries.emplace_back(
                            std::move(key),
                            std::make_unique<VariableExpr>(key_tok));
                    }
                } else {
                    auto key = expression();
                    consume(TokenType::Colon, "Expected ':' after map key.");
                    auto val = expression();
                    entries.emplace_back(std::move(key), std::move(val));
                }
            } while (match({TokenType::Comma}));
        }
        consume(TokenType::RightBrace, "Expected '}' after map entries.");
        return std::make_unique<MapExpr>(std::move(entries));
    }

    // Anonymous lambda: fn(a, b = 2, ...rest) { ... }
    if (match({TokenType::Fn})) {
        return parse_lambda_from_paren();
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
                bool previous = allow_arrow_;
                allow_arrow_ = false;
                arm.pattern = expression();
                allow_arrow_ = previous;
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

std::unique_ptr<Expr> Parser::parse_lambda_from_paren() {
    consume(TokenType::LeftParen, "Expected '(' after 'fn'.");
    std::vector<Token> parameters;
    std::vector<std::shared_ptr<Expr>> param_defaults;
    bool is_variadic = false;
    if (!check(TokenType::RightParen)) {
        do {
            if (match({TokenType::DotDotDot})) {
                is_variadic = true;
                parameters.push_back(consume(TokenType::Identifier, "Expected parameter name after '...'."));
                param_defaults.push_back(nullptr);
                break;
            }
            parameters.push_back(consume(TokenType::Identifier, "Expected parameter name."));
            if (match({TokenType::Equal})) {
                param_defaults.push_back(expression());
            } else {
                param_defaults.push_back(nullptr);
            }
        } while (match({TokenType::Comma}));
    }
    consume(TokenType::RightParen, "Expected ')' after parameters.");
    consume(TokenType::LeftBrace, "Expected '{' before function body.");
    std::vector<std::shared_ptr<Stmt>> body;
    while (!check(TokenType::RightBrace) && !is_at_end()) {
        body.push_back(declaration());
    }
    consume(TokenType::RightBrace, "Expected '}' after function body.");
    return std::make_unique<LambdaExpr>(std::move(parameters), std::move(body), std::move(param_defaults), is_variadic);
}

bool Parser::starts_arrow_function() const {
    if (!allow_arrow_) {
        return false;
    }
    if (check(TokenType::Identifier) && peek_next().type == TokenType::Arrow) {
        return true;
    }
    if (!check(TokenType::LeftParen)) {
        return false;
    }

    std::size_t i = current_ + 1;
    if (i >= tokens_.size()) {
        return false;
    }
    if (tokens_[i].type == TokenType::RightParen) {
        return i + 1 < tokens_.size() && tokens_[i + 1].type == TokenType::Arrow;
    }

    bool expect_param = true;
    while (i < tokens_.size()) {
        TokenType type = tokens_[i].type;
        if (type == TokenType::RightParen) {
            return i + 1 < tokens_.size() && tokens_[i + 1].type == TokenType::Arrow;
        }
        if (expect_param) {
            if (type == TokenType::DotDotDot) {
                if (i + 1 >= tokens_.size() || tokens_[i + 1].type != TokenType::Identifier) {
                    return false;
                }
                i += 2;
                continue;
            }
            if (type != TokenType::Identifier) {
                return false;
            }
            i++;
            if (i < tokens_.size() && tokens_[i].type == TokenType::Equal) {
                i++;
                int depth = 0;
                while (i < tokens_.size()) {
                    TokenType nested = tokens_[i].type;
                    if (nested == TokenType::LeftParen || nested == TokenType::LeftBrace || nested == TokenType::LeftBracket) {
                        depth++;
                    } else if (nested == TokenType::RightParen || nested == TokenType::RightBrace || nested == TokenType::RightBracket) {
                        if (depth == 0) {
                            break;
                        }
                        depth--;
                    } else if (depth == 0 && nested == TokenType::Comma) {
                        break;
                    }
                    i++;
                }
            }
            expect_param = false;
        } else {
            if (type != TokenType::Comma) {
                return false;
            }
            i++;
            expect_param = true;
        }
    }
    return false;
}

std::unique_ptr<Expr> Parser::parse_arrow_function() {
    std::vector<Token> parameters;
    std::vector<std::shared_ptr<Expr>> param_defaults;
    bool is_variadic = false;

    if (check(TokenType::Identifier)) {
        parameters.push_back(advance());
        param_defaults.push_back(nullptr);
    } else {
        consume(TokenType::LeftParen, "Expected '(' before arrow parameters.");
        if (!check(TokenType::RightParen)) {
            do {
                if (match({TokenType::DotDotDot})) {
                    is_variadic = true;
                    parameters.push_back(consume(TokenType::Identifier, "Expected parameter name after '...'."));
                    param_defaults.push_back(nullptr);
                    break;
                }
                parameters.push_back(consume(TokenType::Identifier, "Expected parameter name."));
                if (match({TokenType::Equal})) {
                    param_defaults.push_back(expression());
                } else {
                    param_defaults.push_back(nullptr);
                }
            } while (match({TokenType::Comma}));
        }
        consume(TokenType::RightParen, "Expected ')' after arrow parameters.");
    }

    Token arrow = consume(TokenType::Arrow, "Expected '=>' in arrow function.");
    std::vector<std::shared_ptr<Stmt>> body;
    if (match({TokenType::LeftBrace})) {
        while (!check(TokenType::RightBrace) && !is_at_end()) {
            body.push_back(declaration());
        }
        consume(TokenType::RightBrace, "Expected '}' after arrow function body.");
    } else {
        auto value = assignment();
        body.push_back(std::make_shared<ReturnStmt>(arrow, std::move(value)));
    }
    return std::make_unique<LambdaExpr>(std::move(parameters), std::move(body), std::move(param_defaults), is_variadic);
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

void Parser::consume_semicolon(const std::string& message) {
    if (match({TokenType::Semicolon})) {
        return;
    }
    // Implicit semicolon permitted at end of block, end of file, or upon newline
    if (check(TokenType::RightBrace) || is_at_end() || check(TokenType::EndOfFile) ||
        (current_ > 0 && previous().line < peek().line)) {
        return;
    }
    consume(TokenType::Semicolon, message);
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
