#include <gtest/gtest.h>
#include <sstream>

#include "raven/bytecode_serializer.hpp"
#include "raven/compiler.hpp"
#include "raven/interpreter.hpp"
#include "raven/lexer.hpp"
#include "raven/optimizer.hpp"
#include "raven/parser.hpp"
#include "raven/vm.hpp"

using namespace raven;

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

static std::string run_vm_code(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto statements = parser.parse();

    Chunk chunk;
    Compiler compiler(chunk);
    compiler.compile(statements);

    std::ostringstream out;
    VM vm(out);
    vm.run(chunk);

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
    std::string valid_source =
        "fn fib(n) {\n"
        "    if (n <= 1) {\n"
        "        return n;\n"
        "    }\n"
        "    return fib(n - 1) + fib(n - 2);\n"
        "}\n"
        "print(fib(7));\n";

    EXPECT_EQ(run_code(valid_source), "13\n");
}

TEST(InterpreterTest, DynamicArrays) {
    std::string source =
        "let arr = [10, 20, 30];\n"
        "push(arr, 40);\n"
        "print(len(arr));\n"
        "print(arr[0]);\n"
        "print(arr[-1]);\n"
        "arr[1] = 99;\n"
        "print(arr);\n";

    EXPECT_EQ(run_code(source), "4\n10\n40\n[10, 99, 30, 40]\n");
}

TEST(InterpreterTest, HashMapsAndObjects) {
    std::string source =
        "let user = {\"name\": \"Ran\", \"score\": 100};\n"
        "print(user.name);\n"
        "user.score = 150;\n"
        "print(user[\"score\"]);\n";

    EXPECT_EQ(run_code(source), "Ran\n150\n");
}

TEST(InterpreterTest, FunctionalPipelineAndLambdas) {
    std::string source =
        "let nums = [1, 2, 3, 4, 5];\n"
        "let double_val = fn(x) { return x * 2; };\n"
        "let is_even = fn(x) { return x % 2 == 0; };\n"
        "let evens = filter(nums, is_even);\n"
        "let doubled = map(evens, double_val);\n"
        "print(doubled);\n";

    EXPECT_EQ(run_code(source), "[4, 8]\n");
}

TEST(InterpreterTest, PipelineOperator) {
    std::string source =
        "let inc = fn(x) { return x + 10; };\n"
        "let res = 5 |> inc;\n"
        "print(res);\n";

    EXPECT_EQ(run_code(source), "15\n");
}

TEST(InterpreterTest, ForInLoop) {
    std::string source =
        "let sum = 0;\n"
        "for (let x in [10, 20, 30]) {\n"
        "    sum = sum + x;\n"
        "}\n"
        "print(sum);\n";

    EXPECT_EQ(run_code(source), "60\n");
}

TEST(InterpreterTest, TryCatchAndThrow) {
    std::string source =
        "try {\n"
        "    throw \"Custom failure\";\n"
        "} catch (err) {\n"
        "    print(\"Caught: \" + err);\n"
        "}\n";

    EXPECT_EQ(run_code(source), "Caught: Custom failure\n");
}

TEST(InterpreterTest, BytecodeVMExecution) {
    std::string source =
        "let a = 15;\n"
        "let b = 25;\n"
        "print(a + b);\n"
        "let arr = [1, 2, 3];\n"
        "print(arr[0] + arr[1]);\n";

    EXPECT_EQ(run_vm_code(source), "40\n3\n");
}

TEST(InterpreterTest, BytecodeSerializationRoundtrip) {
    std::string source =
        "let msg = \"Bytecode Binary Serializer\";\n"
        "print(msg);\n";

    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    Chunk original_chunk;
    Compiler compiler(original_chunk);
    compiler.compile(stmts);

    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    ASSERT_TRUE(BytecodeSerializer::serialize(original_chunk, stream));

    Chunk loaded_chunk;
    ASSERT_TRUE(BytecodeSerializer::deserialize(stream, loaded_chunk));

    std::ostringstream out;
    VM vm(out);
    vm.run(loaded_chunk);
    EXPECT_EQ(out.str(), "Bytecode Binary Serializer\n");
}

TEST(InterpreterTest, OptimizerConstantFolding) {
    std::string source = "let x = 10 + 20 * 3;\n";
    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    auto optimized = Optimizer::optimize(stmts);
    ASSERT_EQ(optimized.size(), 1);

    auto* var = dynamic_cast<VarDeclStmt*>(optimized[0].get());
    ASSERT_NE(var, nullptr);

    auto* lit = dynamic_cast<LiteralExpr*>(var->initializer.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->value.as_number(), 70); // 10 + (20 * 3) = 70 pre-computed!
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

TEST(InterpreterTest, PatternMatching) {
    std::string source =
        "let code = 404;\n"
        "let msg = match (code) {\n"
        "    200 => \"Success\",\n"
        "    404 => \"Not Found\",\n"
        "    _ => \"Unknown\"\n"
        "};\n"
        "print(msg);\n";

    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    std::ostringstream out;
    Interpreter interpreter(out);
    interpreter.interpret(stmts);
    EXPECT_EQ(out.str(), "Not Found\n");
}

TEST(InterpreterTest, ImportStandardLibraryMath) {
    std::string source =
        "import \"stdlib/math\";\n"
        "let m = max(10, 42);\n"
        "let p = pow(2, 5);\n"
        "let f = factorial(5);\n"
        "print(m);\n"
        "print(p);\n"
        "print(f);\n";

    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    std::ostringstream out;
    Interpreter interpreter(out);
    interpreter.interpret(stmts);
    EXPECT_EQ(out.str(), "42\n32\n120\n");
}

TEST(InterpreterTest, ImportStandardLibraryString) {
    std::string source =
        "import \"stdlib/string\";\n"
        "let rep = repeat(\"ha\", 3);\n"
        "let pad = pad_left(\"7\", 4, \"0\");\n"
        "print(rep);\n"
        "print(pad);\n";

    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    std::ostringstream out;
    Interpreter interpreter(out);
    interpreter.interpret(stmts);
    EXPECT_EQ(out.str(), "hahaha\n0007\n");
}
