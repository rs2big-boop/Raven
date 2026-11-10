#include <gtest/gtest.h>
#include "raven/semantic/resolver.hpp"
#include "raven/semantic/scopes.hpp"
#include "raven/semantic/symbols.hpp"
#include "raven/lexer.hpp"
#include "raven/parser.hpp"

using namespace raven::semantic;

TEST(SemanticTest, ScopeDefineAndLookup) {
    Scope scope;
    Symbol sym("x", SymbolKind::Variable, 0);

    EXPECT_TRUE(scope.define(sym));
    auto result = scope.lookup("x");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "x");
    EXPECT_EQ(result->kind, SymbolKind::Variable);
}

TEST(SemanticTest, ScopeDuplicateDefine) {
    Scope scope;
    Symbol sym("x", SymbolKind::Variable, 0);

    EXPECT_TRUE(scope.define(sym));
    EXPECT_FALSE(scope.define(sym));
}

TEST(SemanticTest, ScopeLookupMiss) {
    Scope scope;
    auto result = scope.lookup("undefined_var");
    EXPECT_FALSE(result.has_value());
}

TEST(SemanticTest, ScopeChainParentLookup) {
    ScopeChain chain;

    Symbol sym("x", SymbolKind::Variable, 0);
    chain.define(sym);

    chain.push_scope(false);
    auto result = chain.lookup("x");
    EXPECT_TRUE(result.has_value());
    chain.pop_scope();
}

TEST(SemanticTest, ScopeChainShadowing) {
    ScopeChain chain;

    Symbol outer("x", SymbolKind::Variable, 0);
    chain.define(outer);

    chain.push_scope(false);
    Symbol inner("x", SymbolKind::Variable, 1);
    EXPECT_TRUE(chain.define(inner));

    auto result = chain.lookup("x");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->depth, 1);

    chain.pop_scope();
    result = chain.lookup("x");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->depth, 0);
}

TEST(SemanticTest, ResolverSimpleProgram) {
    Resolver resolver;
    raven::Lexer lexer("let x = 42;\n");
    auto tokens = lexer.tokenize();
    raven::Parser parser(tokens);
    auto stmts = parser.parse();

    EXPECT_TRUE(resolver.resolve(stmts));
    EXPECT_FALSE(resolver.has_errors());
}

TEST(SemanticTest, ResolverFunctionDecl) {
    Resolver resolver;
    raven::Lexer lexer("fn add(a, b) { return a + b; }\n");
    auto tokens = lexer.tokenize();
    raven::Parser parser(tokens);
    auto stmts = parser.parse();

    EXPECT_TRUE(resolver.resolve(stmts));
    EXPECT_FALSE(resolver.has_errors());
}

TEST(SemanticTest, ResolverReturnOutsideFunction) {
    Resolver resolver;
    raven::Lexer lexer("return 42;\n");
    auto tokens = lexer.tokenize();
    raven::Parser parser(tokens);
    auto stmts = parser.parse();

    resolver.resolve(stmts);
    EXPECT_TRUE(resolver.has_errors());
}

TEST(SemanticTest, ResolverBreakOutsideLoop) {
    Resolver resolver;
    raven::Lexer lexer("break;\n");
    auto tokens = lexer.tokenize();
    raven::Parser parser(tokens);
    auto stmts = parser.parse();

    resolver.resolve(stmts);
    EXPECT_TRUE(resolver.has_errors());
}

TEST(SemanticTest, ResolverUseAfterMoveFails) {
    Resolver resolver;
    raven::Lexer lexer("let a = 42;\nlet b = move a;\nlet c = a;\n");
    auto tokens = lexer.tokenize();
    raven::Parser parser(tokens);
    auto stmts = parser.parse();

    EXPECT_FALSE(resolver.resolve(stmts));
    EXPECT_TRUE(resolver.has_errors());
    bool found_use_after_move = false;
    for (const auto& err : resolver.errors()) {
        if (err.message.find("used after move") != std::string::npos) {
            found_use_after_move = true;
        }
    }
    EXPECT_TRUE(found_use_after_move);
}

TEST(SemanticTest, ResolverDoubleMoveFails) {
    Resolver resolver;
    raven::Lexer lexer("let a = 42;\nlet b = move a;\nlet c = move a;\n");
    auto tokens = lexer.tokenize();
    raven::Parser parser(tokens);
    auto stmts = parser.parse();

    EXPECT_FALSE(resolver.resolve(stmts));
    EXPECT_TRUE(resolver.has_errors());
    bool found_already_moved = false;
    for (const auto& err : resolver.errors()) {
        if (err.message.find("already been moved") != std::string::npos) {
            found_already_moved = true;
        }
    }
    EXPECT_TRUE(found_already_moved);
}

TEST(SemanticTest, ResolverMoveAndReassignRevivesVariable) {
    Resolver resolver;
    raven::Lexer lexer("let a = 42;\nlet b = move a;\na = 100;\nlet c = a;\n");
    auto tokens = lexer.tokenize();
    raven::Parser parser(tokens);
    auto stmts = parser.parse();

    EXPECT_TRUE(resolver.resolve(stmts));
    EXPECT_FALSE(resolver.has_errors());
}

TEST(SemanticTest, ResolverMoveInNestedScopeInvalidatesOuterVariable) {
    Resolver resolver;
    raven::Lexer lexer("let a = 42;\n{\n    let b = move a;\n}\nlet c = a;\n");
    auto tokens = lexer.tokenize();
    raven::Parser parser(tokens);
    auto stmts = parser.parse();

    EXPECT_FALSE(resolver.resolve(stmts));
    EXPECT_TRUE(resolver.has_errors());
}
