#include <gtest/gtest.h>
#include "raven/lexer.hpp"
#include "raven/parser.hpp"
#include "raven/linter.hpp"
#include "raven/formatter.hpp"
#include "raven/doc_generator.hpp"

using namespace raven;

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

    std::string visit(const ArrayExpr& expr) override {
        std::string res = "[";
        for (size_t i = 0; i < expr.elements.size(); ++i) {
            res += expr.elements[i]->accept(*this);
            if (i + 1 < expr.elements.size()) res += ", ";
        }
        res += "]";
        return res;
    }

    std::string visit(const MapExpr& expr) override {
        std::string res = "{";
        for (size_t i = 0; i < expr.entries.size(); ++i) {
            res += expr.entries[i].first->accept(*this);
            if (expr.entries[i].second) {
                res += ": " + expr.entries[i].second->accept(*this);
            }
            if (i + 1 < expr.entries.size()) res += ", ";
        }
        res += "}";
        return res;
    }

    std::string visit(const IndexExpr& expr) override {
        return "(index " + expr.object->accept(*this) + " " + expr.index->accept(*this) + ")";
    }

    std::string visit(const IndexAssignExpr& expr) override {
        return "(= (index " + expr.object->accept(*this) + " " + expr.index->accept(*this) + ") " + expr.value->accept(*this) + ")";
    }

    std::string visit(const PropertyExpr& expr) override {
        return "(prop " + expr.object->accept(*this) + " " + expr.name.lexeme + ")";
    }

    std::string visit(const PropertyAssignExpr& expr) override {
        return "(= (prop " + expr.object->accept(*this) + " " + expr.name.lexeme + ") " + expr.value->accept(*this) + ")";
    }

    std::string visit(const LambdaExpr& /*expr*/) override {
        return "(lambda)";
    }

    std::string visit(const PipelineExpr& expr) override {
        return "(pipeline " + expr.left->accept(*this) + " " + expr.right->accept(*this) + ")";
    }

    std::string visit(const MatchExpr& expr) override {
        std::string res = "(match " + expr.value->accept(*this);
        for (const auto& arm : expr.arms) {
            res += " (arm ";
            if (arm.is_wildcard) res += "_";
            else res += arm.pattern->accept(*this);
            res += " => " + arm.body->accept(*this) + ")";
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

TEST(ParserTest, ArrayAndMapLiterals) {
    Lexer lexer("[1, 2, 3]");
    Parser parser(lexer.tokenize());
    auto expr = parser.parse_expression();

    ASSERT_NE(expr, nullptr);
    AstStringifier printer;
    EXPECT_EQ(printer.print(*expr), "[1, 2, 3]");
}

TEST(ParserTest, MatchExpression) {
    Lexer lexer("match (status) { 200 => \"OK\", 404 => \"Not Found\", _ => \"Unknown\" }");
    Parser parser(lexer.tokenize());
    auto expr = parser.parse_expression();

    ASSERT_NE(expr, nullptr);
    AstStringifier printer;
    EXPECT_EQ(printer.print(*expr), "(match status (arm 200 => OK) (arm 404 => Not Found) (arm _ => Unknown))");
}

TEST(ParserTest, ImportStatement) {
    Lexer lexer("import \"stdlib/math\";");
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    ASSERT_EQ(stmts.size(), 1);
    auto import_stmt = dynamic_cast<ImportStmt*>(stmts[0].get());
    ASSERT_NE(import_stmt, nullptr);
    EXPECT_EQ(import_stmt->module_path, "stdlib/math");
}

TEST(ParserTest, LinterDetectsUnusedVariables) {
    Lexer lexer("let unused_val = 100; let used_val = 50; print(used_val);");
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    Linter linter;
    auto issues = linter.analyze(stmts);
    ASSERT_EQ(issues.size(), 1);
    EXPECT_EQ(issues[0].severity, LintSeverity::Warning);
    EXPECT_NE(issues[0].message.find("unused_val"), std::string::npos);
}

TEST(ParserTest, FormatterIndentation) {
    std::string unformatted =
        "fn calculate(a, b) {\n"
        "let res = a + b;\n"
        "return res;\n"
        "}\n";

    std::string formatted = Formatter::format(unformatted);
    EXPECT_EQ(formatted,
        "fn calculate(a, b) {\n"
        "    let res = a + b;\n"
        "    return res;\n"
        "}\n");
}

TEST(ParserTest, DocGeneratorExtraction) {
    std::string source =
        "// Adds two numbers together\n"
        "fn add(x, y) {\n"
        "    return x + y;\n"
        "}\n";

    auto items = DocGenerator::extract(source);
    ASSERT_EQ(items.size(), 1);
    EXPECT_EQ(items[0].name, "add");
    EXPECT_EQ(items[0].params.size(), 2);
    EXPECT_EQ(items[0].comment, "Adds two numbers together");

    std::string md = DocGenerator::to_markdown("math", items);
    EXPECT_NE(md.find("Module `math`"), std::string::npos);
    EXPECT_NE(md.find("fn add(x, y)"), std::string::npos);
}
