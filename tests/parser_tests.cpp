#include <gtest/gtest.h>
#include "minilang/lexer.hpp"
#include "minilang/parser.hpp"

using namespace minilang;

class AstStringifier : public ExprVisitor<std::string> {
public:
    std::string print(const Expr& expr) {
        return expr.accept(*this);
    }

    std::string visit(const BinaryExpr& expr) override {
        return "(" + expr.op.lexeme + " " + expr.left->accept(*this) + " " + expr.right->accept(*this) + ")";
    }

    std::string visit(const GroupingExpr& expr) override {
        return "(group " + expr.expression->accept(*this) + ")";
    }

    std::string visit(const LiteralExpr& expr) override {
        return expr.value.to_string();
    }

    std::string visit(const UnaryExpr& expr) override {
        return "(" + expr.op.lexeme + " " + expr.right->accept(*this) + ")";
    }

    std::string visit(const VariableExpr& expr) override {
        return expr.name.lexeme;
    }

    std::string visit(const AssignExpr& expr) override {
        return "(= " + expr.name.lexeme + " " + expr.value->accept(*this) + ")";
    }

    std::string visit(const LogicalExpr& expr) override {
        return "(" + expr.op.lexeme + " " + expr.left->accept(*this) + " " + expr.right->accept(*this) + ")";
    }

    std::string visit(const CallExpr& expr) override {
        std::string res = "(call " + expr.callee->accept(*this);
        for (const auto& arg : expr.arguments) {
            res += " " + arg->accept(*this);
        }
        res += ")";
        return res;
    }
};

TEST(ParserTest, OperatorPrecedence) {
    Lexer lexer("2 + 3 * 4");
    Parser parser(lexer.tokenize());
    auto expr = parser.parse_expression();

    ASSERT_NE(expr, nullptr);
    AstStringifier printer;
    EXPECT_EQ(printer.print(*expr), "(+ 2 (* 3 4))");
}

TEST(ParserTest, GroupingPrecedence) {
    Lexer lexer("(2 + 3) * 4");
    Parser parser(lexer.tokenize());
    auto expr = parser.parse_expression();

    ASSERT_NE(expr, nullptr);
    AstStringifier printer;
    EXPECT_EQ(printer.print(*expr), "(* (group (+ 2 3)) 4)");
}

TEST(ParserTest, ComparisonAndEquality) {
    Lexer lexer("a >= 18 == true");
    Parser parser(lexer.tokenize());
    auto expr = parser.parse_expression();

    ASSERT_NE(expr, nullptr);
    AstStringifier printer;
    EXPECT_EQ(printer.print(*expr), "(== (>= a 18) true)");
}

TEST(ParserTest, LetStatement) {
    Lexer lexer("let age = 20;");
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    ASSERT_EQ(stmts.size(), 1);
    auto var_decl = dynamic_cast<VarDeclStmt*>(stmts[0].get());
    ASSERT_NE(var_decl, nullptr);
    EXPECT_EQ(var_decl->name.lexeme, "age");
    ASSERT_NE(var_decl->initializer, nullptr);

    AstStringifier printer;
    EXPECT_EQ(printer.print(*var_decl->initializer), "20");
}

TEST(ParserTest, IfElseStatement) {
    Lexer lexer("if (x > 0) { print(x); } else { print(0); }");
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    ASSERT_EQ(stmts.size(), 1);
    auto if_stmt = dynamic_cast<IfStmt*>(stmts[0].get());
    ASSERT_NE(if_stmt, nullptr);

    AstStringifier printer;
    EXPECT_EQ(printer.print(*if_stmt->condition), "(> x 0)");
    EXPECT_NE(if_stmt->then_branch, nullptr);
    EXPECT_NE(if_stmt->else_branch, nullptr);
}

TEST(ParserTest, WhileStatement) {
    Lexer lexer("while (i < 10) { i = i + 1; }");
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    ASSERT_EQ(stmts.size(), 1);
    auto while_stmt = dynamic_cast<WhileStmt*>(stmts[0].get());
    ASSERT_NE(while_stmt, nullptr);

    AstStringifier printer;
    EXPECT_EQ(printer.print(*while_stmt->condition), "(< i 10)");
}

TEST(ParserTest, FunctionDeclaration) {
    Lexer lexer("fn add(a, b) { return a + b; }");
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    ASSERT_EQ(stmts.size(), 1);
    auto fn_stmt = dynamic_cast<FunctionStmt*>(stmts[0].get());
    ASSERT_NE(fn_stmt, nullptr);
    EXPECT_EQ(fn_stmt->name.lexeme, "add");
    ASSERT_EQ(fn_stmt->params.size(), 2);
    EXPECT_EQ(fn_stmt->params[0].lexeme, "a");
    EXPECT_EQ(fn_stmt->params[1].lexeme, "b");
    EXPECT_EQ(fn_stmt->body.size(), 1);
}
