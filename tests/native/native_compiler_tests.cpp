#include <gtest/gtest.h>
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "raven/native/native_compiler.hpp"
#include "raven/interpreter.hpp"
#include "raven/lexer.hpp"
#include "raven/parser.hpp"

namespace fs = std::filesystem;
using namespace raven;
using namespace raven::native;

namespace {

static int s_test_counter = 0;

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

struct PipeCloser {
    void operator()(FILE* fp) const {
        if (fp) pclose(fp);
    }
};

std::string execute_binary(const std::string& bin_path) {
    std::array<char, 256> buffer;
    std::string result;
    std::unique_ptr<FILE, PipeCloser> pipe(popen(bin_path.c_str(), "r"));
    if (!pipe) {
        return "ERROR: popen failed";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string run_with_native_compiler(const std::string& source) {
    int id = s_test_counter++;
    fs::path src_path = fs::temp_directory_path() / ("test_rvn_" + std::to_string(id) + ".rvn");
    fs::path bin_path = fs::temp_directory_path() / ("test_bin_" + std::to_string(id));

    {
        std::ofstream out(src_path);
        out << source;
    }

    NativeCompilerOptions opts;
    opts.optimize = true;
    bool ok = NativeCompiler::compile_to_file(src_path.string(), bin_path.string(), opts);

    std::error_code ec;
    fs::remove(src_path, ec);

    if (!ok) {
        return "ERROR: Native compilation failed";
    }

    std::string out_str = execute_binary(bin_path.string());
    fs::remove(bin_path, ec);
    return out_str;
}

void assert_native_match(const std::string& source) {
    std::string interp_res = run_with_interpreter(source);
    std::string native_res = run_with_native_compiler(source);
    EXPECT_EQ(interp_res, native_res) << "Differential mismatch for source:\n" << source;
}

} // namespace

TEST(NativeCompilerTest, EmitCBasic) {
    std::string source = "let a = 10; print(a);\n";
    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    std::string c_code = NativeCompiler::compile_to_c(stmts, "test");
    EXPECT_NE(c_code.find("#include \"raven/native/native_runtime.hpp\""), std::string::npos);
    EXPECT_NE(c_code.find("int main(int argc, char* argv[])"), std::string::npos);
    EXPECT_NE(c_code.find("rvn_rt::print"), std::string::npos);
}

TEST(NativeCompilerTest, ArithmeticAndPrecedence) {
    std::string source =
        "let a = 10 + 20 * 3;\n"
        "let b = (a - 5) / 5;\n"
        "let c = a > b && b < 100;\n"
        "print(a);\n"
        "print(b);\n"
        "print(c);\n";
    assert_native_match(source);
}

TEST(NativeCompilerTest, VariablesAndBlockScoping) {
    std::string source =
        "let x = \"outer\";\n"
        "{\n"
        "    let x = \"inner\";\n"
        "    let y = 42;\n"
        "    print(x);\n"
        "    print(y);\n"
        "}\n"
        "print(x);\n";
    assert_native_match(source);
}

TEST(NativeCompilerTest, IfElseAndWhileLoop) {
    std::string source =
        "let i = 0;\n"
        "let sum = 0;\n"
        "while (i < 5) {\n"
        "    if (i % 2 == 0) {\n"
        "        sum = sum + i;\n"
        "    }\n"
        "    i = i + 1;\n"
        "}\n"
        "print(sum);\n"
        "print(i);\n";
    assert_native_match(source);
}

TEST(NativeCompilerTest, FunctionsAndRecursion) {
    std::string source =
        "fn fib(n) {\n"
        "    if (n <= 1) { return n; }\n"
        "    return fib(n - 1) + fib(n - 2);\n"
        "}\n"
        "print(fib(0));\n"
        "print(fib(1));\n"
        "print(fib(5));\n"
        "print(fib(7));\n";
    assert_native_match(source);
}

TEST(NativeCompilerTest, ArraysMapsAndSlicing) {
    std::string source =
        "let arr = [10, 20, 30, 40, 50];\n"
        "print(arr[0]);\n"
        "arr[1] = 99;\n"
        "print(arr[1]);\n"
        "print(len(arr));\n"
        "push(arr, 60);\n"
        "print(len(arr));\n"
        "let last = pop(arr);\n"
        "print(last);\n"
        "let s = arr[1:3];\n"
        "print(s[0]);\n"
        "print(s[1]);\n"
        "let m = {\"key1\": 100, \"key2\": 200};\n"
        "print(m[\"key1\"]);\n"
        "print(map_has_key(m, \"key1\"));\n"
        "print(map_has_key(m, \"missing\"));\n";
    assert_native_match(source);
}

TEST(NativeCompilerTest, HigherOrderBuiltinsFilterMapReduce) {
    std::string source =
        "let nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];\n"
        "let evens = filter(nums, fn(x) { return x % 2 == 0; });\n"
        "let doubled = map(evens, fn(x) { return x * 2; });\n"
        "let total = reduce(doubled, fn(acc, x) { return acc + x; }, 0);\n"
        "print(total);\n";
    assert_native_match(source);
}

TEST(NativeCompilerTest, StructAndFieldAccess) {
    std::string source =
        "struct Point {\n"
        "    x, y\n"
        "}\n"
        "let p = Point(10, 20);\n"
        "print(p.x);\n"
        "print(p.y);\n"
        "p.x = 99;\n"
        "print(p.x);\n"
        "print(p is Point);\n";
    assert_native_match(source);
}
