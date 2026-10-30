#include <gtest/gtest.h>
#include <sstream>
#include <string>

#include "raven/compiler.hpp"
#include "raven/interpreter.hpp"
#include "raven/lexer.hpp"
#include "raven/parser.hpp"
#include "raven/vm.hpp"

using namespace raven;

namespace {

std::string run_with_interpreter(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto stmts = parser.parse();

    std::ostringstream out;
    Interpreter interp(out);
    interp.interpret(stmts);
    return out.str();
}

std::string run_with_vm(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto stmts = parser.parse();

    Chunk chunk;
    Compiler compiler(chunk);
    compiler.compile(stmts);

    std::ostringstream out;
    VM vm(out);
    vm.run(chunk);
    return out.str();
}

void assert_differential_match(const std::string& source) {
    std::string interp_res = run_with_interpreter(source);
    std::string vm_res = run_with_vm(source);
    EXPECT_EQ(interp_res, vm_res) << "Differential mismatch for source:\n" << source;
}

} // namespace

TEST(DifferentialTest, ArithmeticAndLogicPrecedence) {
    std::string source =
        "let a = 10 + 20 * 3;\n"
        "let b = (a - 5) / 5;\n"
        "let c = a > b && b < 100;\n"
        "print(a);\n"
        "print(b);\n"
        "print(c);\n";
    assert_differential_match(source);
}

TEST(DifferentialTest, VariablesAndBlockScoping) {
    std::string source =
        "let x = \"outer\";\n"
        "{\n"
        "    let x = \"inner\";\n"
        "    let y = 42;\n"
        "    print(x);\n"
        "    print(y);\n"
        "}\n"
        "print(x);\n";
    assert_differential_match(source);
}

TEST(DifferentialTest, IfElseBranches) {
    std::string source =
        "let x = 15;\n"
        "if (x > 10) {\n"
        "    print(\"Greater\");\n"
        "} else {\n"
        "    print(\"Smaller\");\n"
        "}\n"
        "if (x == 20) {\n"
        "    print(\"Twenty\");\n"
        "} else {\n"
        "    print(\"Not Twenty\");\n"
        "}\n";
    assert_differential_match(source);
}

TEST(DifferentialTest, WhileLoopAccumulation) {
    std::string source =
        "let i = 0;\n"
        "let sum = 0;\n"
        "while (i < 5) {\n"
        "    sum = sum + i;\n"
        "    i = i + 1;\n"
        "}\n"
        "print(sum);\n"
        "print(i);\n";
    assert_differential_match(source);
}

TEST(DifferentialTest, UserDefinedFunctionsAndRecursion) {
    std::string source =
        "fn fib(n) {\n"
        "    if (n <= 1) { return n; }\n"
        "    return fib(n - 1) + fib(n - 2);\n"
        "}\n"
        "print(fib(0));\n"
        "print(fib(1));\n"
        "print(fib(5));\n"
        "print(fib(7));\n";
    assert_differential_match(source);
}

TEST(DifferentialTest, ArrayAccessAndMutation) {
    std::string source =
        "let arr = [10, 20, 30];\n"
        "print(arr[0]);\n"
        "print(arr[1]);\n"
        "print(arr[2]);\n"
        "arr[1] = 99;\n"
        "print(arr[1]);\n"
        "print(len(arr));\n";
    assert_differential_match(source);
}

TEST(DifferentialTest, ArrayBuiltinsPushPop) {
    std::string source =
        "let arr = [10, 20, 30];\n"
        "print(len(arr));\n"
        "push(arr, 40);\n"
        "print(len(arr));\n"
        "let last = pop(arr);\n"
        "print(last);\n"
        "print(len(arr));\n";
    assert_differential_match(source);
}

TEST(DifferentialTest, MapBuiltins) {
    std::string source =
        "let m = {\"a\": 1, \"b\": 2};\n"
        "print(len(m));\n"
        "let ks = keys(m);\n"
        "print(len(ks));\n"
        "print(map_has_key(m, \"a\"));\n"
        "print(map_has_key(m, \"z\"));\n";
    assert_differential_match(source);
}

TEST(DifferentialTest, TypeConversions) {
    std::string source =
        "print(type(42));\n"
        "print(type(\"hello\"));\n"
        "print(type(true));\n"
        "print(type([1,2]));\n"
        "print(str(100));\n"
        "print(num(\"3.14\"));\n";
    assert_differential_match(source);
}

TEST(DifferentialTest, ArraySlicing) {
    std::string source =
        "let arr = [10, 20, 30, 40, 50];\n"
        "let s = arr[1:3];\n"
        "print(s[0]);\n"
        "print(s[1]);\n"
        "print(len(s));\n";
    assert_differential_match(source);
}

TEST(DifferentialTest, StringSlicing) {
    std::string source =
        "let s = \"Hello, World!\";\n"
        "let sub = s[0:5];\n"
        "print(sub);\n"
        "print(len(sub));\n";
    assert_differential_match(source);
}

TEST(DifferentialTest, NestedLoops) {
    std::string source =
        "let sum = 0;\n"
        "let i = 0;\n"
        "while (i < 5) {\n"
        "    let j = 0;\n"
        "    while (j < 3) {\n"
        "        sum = sum + 1;\n"
        "        j = j + 1;\n"
        "    }\n"
        "    i = i + 1;\n"
        "}\n"
        "print(sum);\n";
    assert_differential_match(source);
}
