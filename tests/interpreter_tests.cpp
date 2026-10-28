#include <gtest/gtest.h>
#include <sstream>
#include "minilang/lexer.hpp"
#include "minilang/parser.hpp"
#include "minilang/interpreter.hpp"

using namespace minilang;

static std::string run_code(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto statements = parser.parse();

    std::ostringstream out;
    Interpreter interpreter(out);
    interpreter.interpret(statements);

    return out.str();
}

TEST(InterpreterTest, UserTargetScript) {
    std::string source =
        "let name = \"Ran\";\n"
        "let age = 20;\n"
        "\n"
        "print(\"Hello, \" + name);\n"
        "\n"
        "if (age >= 18) {\n"
        "    print(\"Adult\");\n"
        "}\n";

    EXPECT_EQ(run_code(source), "Hello, Ran\nAdult\n");
}

TEST(InterpreterTest, ArithmeticAndPrecedence) {
    std::string source =
        "print(2 + 3 * 4);\n"
        "print((2 + 3) * 4);\n"
        "print(10 - 4 / 2);\n";

    EXPECT_EQ(run_code(source), "14\n20\n8\n");
}

TEST(InterpreterTest, VariablesAndScopes) {
    std::string source =
        "let x = 10;\n"
        "{\n"
        "    let x = 20;\n"
        "    print(x);\n"
        "}\n"
        "print(x);\n";

    EXPECT_EQ(run_code(source), "20\n10\n");
}

TEST(InterpreterTest, WhileLoop) {
    std::string source =
        "let sum = 0;\n"
        "let i = 1;\n"
        "while (i <= 5) {\n"
        "    sum = sum + i;\n"
        "    i = i + 1;\n"
        "}\n"
        "print(sum);\n";

    EXPECT_EQ(run_code(source), "15\n");
}

TEST(InterpreterTest, FunctionsAndRecursion) {
    std::string source =
        "fn fib(n) {\n"
        "    if (n <= 1) {\n"
        "        return n;\n"
        "    }\n"
        "    return fib(n - 1) + fib(n - 2);\n"
        "}\n"
        "print(fib(7));\n";

    EXPECT_EQ(run_code(source), "13\n");
}

TEST(InterpreterTest, RuntimeErrorDivisionByZero) {
    std::string source = "let x = 10 / 0;\n";
    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    std::ostringstream out;
    Interpreter interpreter(out);
    EXPECT_THROW(interpreter.interpret(stmts), RuntimeError);
}

TEST(InterpreterTest, RuntimeErrorUndefinedVariable) {
    std::string source = "print(notDefined);\n";
    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    std::ostringstream out;
    Interpreter interpreter(out);
    EXPECT_THROW(interpreter.interpret(stmts), RuntimeError);
}
