#include <gtest/gtest.h>
#include <sstream>

#include "raven/bytecode_serializer.hpp"
#include "raven/bytecode_verifier.hpp"
#include "raven/compiler.hpp"
#include "raven/interpreter.hpp"
#include "raven/lexer.hpp"
#include "raven/optimizer.hpp"
#include "raven/parser.hpp"
#include "raven/vm.hpp"
#include "raven/debugger.hpp"
#include "raven/runtime/memory/memory_manager.hpp"
#include "raven/runtime/memory/root_registry.hpp"
#include "raven/runtime/memory/managed_heap.hpp"
#include "raven/semantic/resolver.hpp"
#include "raven/semantic/analyzer.hpp"
#include "raven/concurrency/channel.hpp"
#include "raven/concurrency/wait_group.hpp"
#include "raven/concurrency/mutex.hpp"

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

TEST(InterpreterTest, NativeMathAndStringBuiltins) {
    std::string source =
        "let s = sqrt(49);\n"
        "let r = round(4.8);\n"
        "let words = split(\"alpha,beta,gamma\", \",\");\n"
        "let upper = to_upper(words[0]);\n"
        "let joined = join(words, \":\");\n"
        "print(s);\n"
        "print(r);\n"
        "print(upper);\n"
        "print(joined);\n";

    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    std::ostringstream out;
    Interpreter interpreter(out);
    interpreter.interpret(stmts);
    EXPECT_EQ(out.str(), "7\n5\nALPHA\nalpha:beta:gamma\n");
}

TEST(InterpreterTest, NativeJsonSerializationAndParsing) {
    std::string source =
        "let data = {\"name\": \"Raven\", \"version\": 2, \"active\": true};\n"
        "let json_str = to_json(data);\n"
        "let parsed = parse_json(json_str);\n"
        "print(parsed.name);\n"
        "print(parsed.version);\n"
        "print(parsed.active);\n";

    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    std::ostringstream out;
    Interpreter interpreter(out);
    interpreter.interpret(stmts);
    EXPECT_EQ(out.str(), "Raven\n2\ntrue\n");
}

TEST(InterpreterTest, StandardLibraryCollections) {
    std::string source =
        "import \"stdlib/collections\";\n"
        "let stack = create_stack();\n"
        "stack.push(\"first\");\n"
        "stack.push(\"second\");\n"
        "print(stack.pop());\n"
        "print(stack.peek());\n"
        "let q = create_queue();\n"
        "q.enqueue(\"A\");\n"
        "q.enqueue(\"B\");\n"
        "print(q.dequeue());\n"
        "let set = create_set();\n"
        "set.add(\"x\");\n"
        "set.add(\"x\");\n"
        "print(set.size());\n";

    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    std::ostringstream out;
    Interpreter interpreter(out);
    interpreter.interpret(stmts);
    EXPECT_EQ(out.str(), "second\nfirst\nA\n1\n");
}

TEST(InterpreterTest, StandardLibraryIterators) {
    std::string source =
        "import \"stdlib/iter\";\n"
        "let paired = zip([1, 2], [\"a\", \"b\"]);\n"
        "print(paired[0][0] + \":\" + paired[0][1]);\n"
        "let chunks = chunk([1, 2, 3, 4], 2);\n"
        "print(len(chunks));\n"
        "let flat = flatten([[10, 20], [30]]);\n"
        "print(len(flat));\n";

    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    std::ostringstream out;
    Interpreter interpreter(out);
    interpreter.interpret(stmts);
    EXPECT_EQ(out.str(), "1:a\n2\n3\n");
}

TEST(InterpreterTest, NativeAbsAndSortBuiltins) {
    EXPECT_EQ(run_code("print(abs(-5));\n"), "5\n");
    EXPECT_EQ(run_code("print(abs(3.14));\n"), "3.14\n");
    EXPECT_EQ(run_code(
        "let arr = [3, 1, 4, 1, 5, 9, 2, 6];\n"
        "sort_arr(arr);\n"
        "print(arr[0]);\n"
        "print(arr[7]);\n"
    ), "1\n9\n");
}

TEST(InterpreterTest, NativeCharBuiltins) {
    EXPECT_EQ(run_code("print(char_code(\"A\"));\n"), "65\n");
    EXPECT_EQ(run_code("print(char(65));\n"), "A\n");
}

TEST(InterpreterTest, StandardLibrarySortAndStringUtils) {
    std::string source =
        "import \"stdlib/sort\";\n"
        "import \"stdlib/string_utils\";\n"
        "let sorted = bubble_sort([5, 2, 8, 1]);\n"
        "print(sorted[0]);\n"
        "print(sorted[3]);\n"
        "print(min_of_arr(sorted));\n"
        "print(max_of_arr(sorted));\n"
        "print(starts_with(\"raven-lang\", \"raven\"));\n"
        "print(ends_with(\"code.rvn\", \".rvn\"));\n"
        "print(count_occurrences(\"banana\", \"an\"));\n";

    EXPECT_EQ(run_code(source), "1\n8\n1\n8\ntrue\ntrue\n2\n");
}

TEST(InterpreterTest, StringInterpolation) {
    std::string source =
        "let name = \"Raven\";\n"
        "let ver = 2;\n"
        "let msg = f\"Welcome to {name} v{ver + 0.5}!\";\n"
        "print(msg);\n";
    EXPECT_EQ(run_code(source), "Welcome to Raven v2.5!\n");
}

TEST(InterpreterTest, StructDeclarationAndAccess) {
    std::string source =
        "struct Vector2D { x, y };\n"
        "let v = Vector2D(3, 4);\n"
        "print(v.__type__);\n"
        "print(v.x);\n"
        "print(v.y);\n"
        "v.x = 100;\n"
        "print(v.x);\n";
    EXPECT_EQ(run_code(source), "Vector2D\n3\n4\n100\n");
}

TEST(InterpreterTest, BreakAndContinue) {
    std::string source =
        "let i = 0;\n"
        "let sum = 0;\n"
        "while (i < 10) {\n"
        "    i += 1;\n"
        "    if (i == 3) { continue; }\n"
        "    if (i > 5) { break; }\n"
        "    sum += i;\n"
        "}\n"
        "print(sum);\n";
    // i=1 (sum=1), i=2 (sum=3), i=3 (skip), i=4 (sum=7), i=5 (sum=12), i=6 (break)
    EXPECT_EQ(run_code(source), "12\n");
}

TEST(InterpreterTest, CompoundAssignmentOperators) {
    std::string source =
        "let x = 10;\n"
        "x += 5;\n"
        "x -= 3;\n"
        "x *= 2;\n"
        "x /= 4;\n"
        "print(x);\n";
    // 10 + 5 = 15; 15 - 3 = 12; 12 * 2 = 24; 24 / 4 = 6
    EXPECT_EQ(run_code(source), "6\n");
}

TEST(InterpreterTest, NativeRegexAndSliceAndHash) {
    std::string source =
        "let matched = regex_match(\"user@example.com\", \".*@.*\\\\.com\");\n"
        "print(matched);\n"
        "let s = slice(\"Hello, World\", 0, 5);\n"
        "print(s);\n"
        "let arr_s = slice([10, 20, 30, 40], 1, 3);\n"
        "print(arr_s[0]);\n"
        "print(arr_s[1]);\n"
        "let b64 = base64_encode(\"Raven\");\n"
        "print(b64);\n"
        "print(base64_decode(b64));\n";
    EXPECT_EQ(run_code(source), "true\nHello\n20\n30\nUmF2ZW4=\nRaven\n");
}

TEST(InterpreterTest, NewStringAndArrayBuiltins) {
    std::string source =
        "print(str_repeat(\"ab\", 3));\n"
        "print(pad_left(\"42\", 5, \"0\"));\n"
        "print(is_digit(\"7\"));\n"
        "print(is_alpha(\"k\"));\n"
        "print(has_prefix(\"golang\", \"go\"));\n"
        "print(has_suffix(\"golang\", \"lang\"));\n"
        "let a = [3, 1, 4, 1, 5];\n"
        "print(sum(a));\n"
        "print(arr_min(a));\n"
        "print(arr_max(a));\n"
        "print(len(unique(a)));\n"
        "print(format(\"Raven v{} is {}\", 2, \"modern\"));\n";
    EXPECT_EQ(run_code(source), "ababab\n00042\ntrue\ntrue\ntrue\ntrue\n14\n1\n5\n4\nRaven v2 is modern\n");
}

TEST(InterpreterTest, GoInspiredStandardLibraryModules) {
    std::string source =
        "import \"stdlib/fmt\";\n"
        "import \"stdlib/strings\";\n"
        "import \"stdlib/slices\";\n"
        "import \"stdlib/maps\";\n"
        "import \"stdlib/strconv\";\n"
        "import \"stdlib/encoding/json\";\n"
        "import \"stdlib/cmp\";\n"
        "import \"stdlib/container/heap\";\n"
        "println(\"Testing Go-like stdlib\");\n"
        "print(str_title(\"hello world raven\"));\n"
        "print(atoi(\"42\") + 8);\n"
        "print(slices_sum([10, 20, 30]));\n"
        "let m1 = {\"a\": 1};\n"
        "let m2 = {\"b\": 2};\n"
        "let m = map_merge(m1, m2);\n"
        "print(m[\"a\"] + m[\"b\"]);\n"
        "print(compare(5, 10));\n"
        "let h = create_min_heap();\n"
        "h.push(40);\n"
        "h.push(10);\n"
        "h.push(20);\n"
        "print(h.pop());\n"
        "print(h.pop());\n";
    EXPECT_EQ(run_code(source),
        "Testing Go-like stdlib\n"
        "Hello World Raven\n"
        "50\n"
        "60\n"
        "3\n"
        "-1\n"
        "10\n"
        "20\n");
}

TEST(InterpreterTest, GoEcosystemModules) {
    std::string source =
        "import \"stdlib/uuid\";\n"
        "import \"stdlib/xid\";\n"
        "import \"stdlib/encoding/yaml\";\n"
        "import \"stdlib/encoding/ini\";\n"
        "import \"stdlib/mime\";\n"
        "import \"stdlib/text/template\";\n"
        "import \"stdlib/sync/atomic\";\n"
        "import \"stdlib/arena\";\n"
        "import \"stdlib/unique\";\n"
        "import \"stdlib/embed\";\n"
        "\n"
        "let u = uuid_v4();\n"
        "print(len(u));\n"
        "let x = xid_new();\n"
        "print(len(x));\n"
        "let y = yaml_parse(\"name: Raven\\nversion: 3\\nready: true\\n\");\n"
        "print(y[\"name\"]);\n"
        "print(y[\"ready\"]);\n"
        "let cfg = ini_parse(\"[server]\\nhost = 127.0.0.1\\nport = 8080\\n\");\n"
        "print(cfg[\"server\"][\"host\"]);\n"
        "print(type_by_extension(\".json\"));\n"
        "let rendered = template_render(\"Hello {{name}}!\", {\"name\": \"Gopher\"});\n"
        "print(rendered);\n"
        "let atom = create_atomic_int(10);\n"
        "atom.add(5);\n"
        "print(atom.load());\n"
        "let ar = create_arena();\n"
        "let p1 = ar.allocate(\"item\");\n"
        "print(ar.count());\n"
        "print(unique_make(\"test_val\"));\n"
        "embed_file(\"/virtual/config.txt\", \"data\");\n"
        "print(embed_read(\"/virtual/config.txt\"));\n";

    EXPECT_EQ(run_code(source),
        "36\n"
        "20\n"
        "Raven\n"
        "true\n"
        "127.0.0.1\n"
        "application/json\n"
        "Hello Gopher!\n"
        "15\n"
        "1\n"
        "test_val\n"
        "data\n");
}

TEST(InterpreterTest, NativeConcurrencyAndChannels) {
    std::string source =
        "let ch = chan_create(2);\n"
        "chan_send(ch, 100);\n"
        "chan_send(ch, 200);\n"
        "print(chan_len(ch));\n"
        "print(chan_recv(ch));\n"
        "print(chan_recv(ch));\n"
        "chan_close(ch);\n"
        "\n"
        "import \"stdlib/sync\";\n"
        "let sc = create_channel(1);\n"
        "go(fn() { sc.send(\"from_go\"); });\n"
        "print(sc.recv());\n"
        "\n"
        "let wg = create_wait_group();\n"
        "wg.add(2);\n"
        "go(fn() { wg.done(); });\n"
        "go(fn() { wg.done(); });\n"
        "wg.wait();\n"
        "print(\"wg_ok\");\n";

    EXPECT_EQ(run_code(source),
        "2\n"
        "100\n"
        "200\n"
        "from_go\n"
        "wg_ok\n");
}

TEST(InterpreterTest, CallStackTraceVerification) {
    std::string source =
        "fn func_c() { return 10 / 0; }\n"
        "fn func_b() { func_c(); }\n"
        "fn func_a() { func_b(); }\n"
        "func_a();\n";

    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto stmts = parser.parse();
    Interpreter interp;

    try {
        interp.interpret(stmts);
        FAIL() << "Expected RuntimeError";
    } catch (const RuntimeError& err) {
        EXPECT_EQ(err.what(), std::string("Division by zero."));
        EXPECT_GE(err.stack_trace().size(), 3UL);
        EXPECT_EQ(err.stack_trace().back().function_name, "func_c");
    }
}

TEST(InterpreterTest, DefaultParameters) {
    EXPECT_EQ(run_code("fn greet(name = \"World\") { return \"Hello, \" + name + \"!\"; }\n"
                       "print(greet());\n"
                       "print(greet(\"Raven\"));\n"), "Hello, World!\nHello, Raven!\n");
}

TEST(InterpreterTest, NullCoalescing) {
    EXPECT_EQ(run_code("let x = nil;\n"
                       "let y = x ?? \"default\";\n"
                       "print(y);\n"
                       "let z = 42 ?? \"default\";\n"
                       "print(z);\n"), "default\n42\n");
}

TEST(InterpreterTest, IsTypeOperator) {
    EXPECT_EQ(run_code("print(42 is Number);\n"
                       "print(\"hi\" is String);\n"
                       "print(true is Bool);\n"
                       "print([1,2] is Array);\n"
                       "print(nil is Nil);\n"), "true\ntrue\ntrue\ntrue\ntrue\n");
}

TEST(InterpreterTest, OptionalChaining) {
    EXPECT_EQ(run_code("let obj = nil;\n"
                       "print(obj?.name);\n"
                       "print(obj?.[0]);\n"
                       "let person = {\"name\": \"Alice\"};\n"
                       "print(person?.name);\n"), "nil\nnil\nAlice\n");
}

TEST(InterpreterTest, SpreadOperator) {
    EXPECT_EQ(run_code("let a = [1, 2, 3];\n"
                       "let b = [...a, 4, 5];\n"
                       "print(len(b));\n"
                       "print(b[3]);\n"
                       "let m1 = {\"x\": 10};\n"
                       "let m2 = {...m1, \"y\": 20};\n"
                       "print(m2[\"x\"] + m2[\"y\"]);\n"), "5\n4\n30\n");
}

TEST(InterpreterTest, LoopKeyword) {
    EXPECT_EQ(run_code("let i = 0;\n"
                       "loop {\n"
                       "    i += 1;\n"
                       "    if (i >= 3) { break; }\n"
                       "}\n"
                       "print(i);\n"), "3\n");
}

TEST(InterpreterTest, MultiLineStrings) {
    std::string source =
        "let msg = \"\"\"line 1\n"
        "line 2\"\"\";\n"
        "print(msg);\n";
    EXPECT_EQ(run_code(source), "line 1\nline 2\n");
}

TEST(InterpreterTest, DidYouMeanSuggestion) {
    std::string source =
        "let message = \"hello\";\n"
        "mesage;\n";
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto stmts = parser.parse();
    Interpreter interp;
    try {
        interp.interpret(stmts);
        FAIL() << "Expected RuntimeError";
    } catch (const RuntimeError& err) {
        std::string msg = err.what();
        EXPECT_NE(msg.find("Did you mean 'message'?"), std::string::npos);
    }
}

TEST(InterpreterTest, NewStdlibModules) {
    std::string source =
        "import \"stdlib/event\";\n"
        "import \"stdlib/cache\";\n"
        "import \"stdlib/validate\";\n"
        "\n"
        "let em = create_emitter();\n"
        "let received = nil;\n"
        "em.on(\"data\", fn(d) { received = d; });\n"
        "em.emit(\"data\", 42);\n"
        "print(received);\n"
        "\n"
        "let cache = create_lru_cache(2);\n"
        "cache.set(\"a\", 1);\n"
        "cache.set(\"b\", 2);\n"
        "print(cache.get(\"a\"));\n"
        "print(cache.size());\n"
        "\n"
        "let errs = validate_schema({\"name\": \"Ran\", \"age\": 25}, {\n"
        "    \"name\": {\"required\": true, \"type\": \"string\"},\n"
        "    \"age\": {\"required\": true, \"type\": \"number\"}\n"
        "});\n"
        "print(len(errs));\n";

    EXPECT_EQ(run_code(source), "42\n1\n2\n0\n");
}

TEST(InterpreterTest, BytecodeVerifierValidChunk) {
    std::string source =
        "let x = 100;\n"
        "let y = 200;\n"
        "print(x + y);\n";

    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    Chunk chunk;
    Compiler compiler(chunk);
    ASSERT_TRUE(compiler.compile(stmts));

    auto res = BytecodeVerifier::verify(chunk);
    EXPECT_TRUE(res.is_valid);
    EXPECT_TRUE(res.error_message.empty());
}

TEST(InterpreterTest, BytecodeVerifierDetectsInvalidOpcode) {
    Chunk chunk;
    chunk.write(254, 1); // Invalid opcode
    auto res = BytecodeVerifier::verify(chunk);
    EXPECT_FALSE(res.is_valid);
    EXPECT_NE(res.error_message.find("Invalid opcode byte"), std::string::npos);
}

TEST(InterpreterTest, BytecodeVerifierDetectsOutOfBoundsConstant) {
    Chunk chunk;
    chunk.write_op(OpCode::Constant, 1);
    chunk.write(99, 1); // Out of bounds constant pool index (pool is empty)
    auto res = BytecodeVerifier::verify(chunk);
    EXPECT_FALSE(res.is_valid);
    EXPECT_NE(res.error_message.find("out of bounds"), std::string::npos);
}

TEST(InterpreterTest, BytecodeVerifierDetectsOutOfBoundsJump) {
    Chunk chunk;
    chunk.write_op(OpCode::Jump, 1);
    chunk.write(0xFF, 1);
    chunk.write(0xFF, 1); // Jump +65535 exceeds chunk bounds
    auto res = BytecodeVerifier::verify(chunk);
    EXPECT_FALSE(res.is_valid);
    EXPECT_NE(res.error_message.find("exceeds code bounds"), std::string::npos);
}

TEST(InterpreterTest, DualEngineDifferentialParityArithmetic) {
    std::string source =
        "let a = 12 * 8 + 4;\n"
        "let b = (a - 10) / 2;\n"
        "print(a);\n"
        "print(b);\n"
        "print(a > b);\n"
        "print(a == b);\n";

    std::string interp_out = run_code(source);
    std::string vm_out = run_vm_code(source);
    EXPECT_EQ(interp_out, vm_out);
    EXPECT_EQ(interp_out, "100\n45\ntrue\nfalse\n");
}

TEST(InterpreterTest, DualEngineDifferentialParityArraysAndLoops) {
    std::string source =
        "let arr = [10, 20, 30];\n"
        "let sum = 0;\n"
        "let i = 0;\n"
        "while (i < 3) {\n"
        "    sum = sum + arr[i];\n"
        "    i = i + 1;\n"
        "}\n"
        "print(sum);\n";

    std::string interp_out = run_code(source);
    std::string vm_out = run_vm_code(source);
    EXPECT_EQ(interp_out, vm_out);
    EXPECT_EQ(interp_out, "60\n");
}

TEST(InterpreterTest, DualEngineDifferentialParityNullCoalescingAndTypeCheck) {
    std::string source =
        "let a = nil;\n"
        "let b = a ?? 100;\n"
        "let c = 42 ?? 200;\n"
        "print(b);\n"
        "print(c);\n"
        "print(b is Number);\n"
        "print(\"hello\" is String);\n"
        "print(nil is Nil);\n";

    std::string interp_out = run_code(source);
    std::string vm_out = run_vm_code(source);
    EXPECT_EQ(interp_out, vm_out);
    EXPECT_EQ(interp_out, "100\n42\ntrue\ntrue\ntrue\n");
}

TEST(InterpreterTest, ChannelRecvTimeout) {
    std::string source =
        "import \"stdlib/sync\";\n"
        "let ch = create_channel(1);\n"
        "// 1. Channel is empty, expect timeout -> nil\n"
        "let res1 = ch.recv_timeout(20);\n"
        "print(res1 == nil);\n"
        "// 2. Send value, expect immediate recv with timeout\n"
        "ch.send(\"hello\");\n"
        "let res2 = ch.recv_timeout(100);\n"
        "print(res2);\n";

    EXPECT_EQ(run_code(source), "true\nhello\n");
}

TEST(InterpreterTest, InteractiveDebuggerBasics) {
    std::string source =
        "let x = 10;\n"
        "let y = 20;\n"
        "let z = x + y;\n";

    Debugger dbg("test.rvn", source);
    dbg.add_breakpoint(2);
    dbg.add_breakpoint(3);
    EXPECT_EQ(dbg.breakpoints().size(), 2);
    EXPECT_TRUE(dbg.breakpoints().count(2));
    EXPECT_TRUE(dbg.breakpoints().count(3));

    dbg.remove_breakpoint(2);
    EXPECT_EQ(dbg.breakpoints().size(), 1);
    EXPECT_FALSE(dbg.breakpoints().count(2));
    EXPECT_TRUE(dbg.breakpoints().count(3));
}

TEST(InterpreterTest, GarbageCollectorBasics) {
    std::string source =
        "let initial_stats = gc_stats();\n"
        "let broken = gc_collect();\n"
        "print(broken);\n"
        "let after_stats = gc_stats();\n"
        "print(after_stats[\"total_collections\"] > initial_stats[\"total_collections\"]);\n";

    EXPECT_EQ(run_code(source), "0\ntrue\n");
}

TEST(InterpreterTest, DualEngineDifferentialParityMapsAndMatch) {
    std::string source =
        "let user = {\"name\": \"Alice\", \"age\": 30};\n"
        "print(user[\"name\"]);\n"
        "print(user.age);\n"
        "user.age = 31;\n"
        "print(user.age);\n"
        "let status = match (user.age) {\n"
        "    20 => \"junior\",\n"
        "    31 => \"senior\",\n"
        "    _ => \"unknown\"\n"
        "};\n"
        "print(status);\n";

    std::string interp_out = run_code(source);
    std::string vm_out = run_vm_code(source);
    EXPECT_EQ(interp_out, vm_out);
    EXPECT_EQ(interp_out, "Alice\n30\n31\nsenior\n");
}

TEST(InterpreterTest, ForeignFunctionInterfaceBasics) {
    std::string source =
        "import \"stdlib/ffi\";\n"
        "let libc = open_lib(nil);\n"
        "let len = libc.call(\"strlen\", \"int\", [\"string\"], [\"Raven Language\"]);\n"
        "print(len);\n"
        "let pid = libc.call(\"getpid\", \"int\", [], []);\n"
        "print(pid > 0);\n"
        "libc.close();\n";

    EXPECT_EQ(run_code(source), "14\ntrue\n");
}

TEST(InterpreterTest, DualEngineDifferentialParityForInLoops) {
    std::string source =
        "let items = [10, 20, 30];\n"
        "let total = 0;\n"
        "for (let x in items) {\n"
        "    total = total + x;\n"
        "}\n"
        "print(total);\n";

    std::string interp_out = run_code(source);
    std::string vm_out = run_vm_code(source);
    EXPECT_EQ(interp_out, vm_out);
    EXPECT_EQ(interp_out, "60\n");
}

TEST(InterpreterTest, DualEngineDifferentialParityBreakAndContinue) {
    std::string source =
        "let i = 0;\n"
        "let sum = 0;\n"
        "while (i < 10) {\n"
        "    i = i + 1;\n"
        "    if (i == 2) { continue; }\n"
        "    if (i == 5) { break; }\n"
        "    sum = sum + i;\n"
        "}\n"
        "print(sum);\n";

    std::string interp_out = run_code(source);
    std::string vm_out = run_vm_code(source);
    EXPECT_EQ(interp_out, vm_out);
    EXPECT_EQ(interp_out, "8\n"); // 1 + 3 + 4 = 8
}

TEST(InterpreterTest, DualEngineDifferentialParityLambdas) {
    std::string source =
        "let square = fn(x) { return x * x; };\n"
        "print(square(5));\n"
        "let apply = fn(f, val) { return f(val); };\n"
        "print(apply(square, 6));\n";

    std::string interp_out = run_code(source);
    std::string vm_out = run_vm_code(source);
    EXPECT_EQ(interp_out, vm_out);
    EXPECT_EQ(interp_out, "25\n36\n");
}

TEST(InterpreterTest, MemoryManagerModularSubsystem) {
    auto& mm = memory::MemoryManager::instance();
    size_t broken = mm.collect_cycles();
    EXPECT_EQ(broken, 0);

    auto stats = mm.statistics();
    EXPECT_GE(stats.total_collections, 1);
}

TEST(InterpreterTest, ConcurrencyModularSubsystem) {
    auto& cm = concurrency::ChannelManager::instance();
    int64_t ch_id = cm.create_channel(2);
    auto ch = cm.get_channel(ch_id);
    EXPECT_NE(ch, nullptr);
    EXPECT_EQ(ch->capacity, 2);
    EXPECT_TRUE(cm.close_channel(ch_id));
    EXPECT_TRUE(ch->closed);

    auto& wgm = concurrency::WaitGroupManager::instance();
    int64_t wg_id = wgm.create();
    EXPECT_TRUE(wgm.add(wg_id, 1));
    EXPECT_TRUE(wgm.done(wg_id));
    EXPECT_TRUE(wgm.wait(wg_id));

    auto& mm = concurrency::MutexManager::instance();
    int64_t m_id = mm.create();
    EXPECT_TRUE(mm.lock(m_id));
    EXPECT_TRUE(mm.unlock(m_id));
}

TEST(InterpreterTest, ManagedHeapAndRootRegistry) {
    raven::runtime::memory::HeapStats stats{};
    EXPECT_EQ(stats.total_objects, 0u);
    EXPECT_EQ(stats.total_alloc_ops, 0u);

    auto& reg = raven::runtime::memory::RootRegistry::instance();
    size_t initial_roots = reg.root_count();
    EXPECT_GE(initial_roots, 0u);

    auto& heap = raven::runtime::memory::ManagedHeap::instance();
    auto hstats = heap.stats();
    EXPECT_GE(hstats.total_alloc_ops, 0u);

    EXPECT_EQ(run_code("let x = [1, 2, 3]; print(len(x));\n"), "3\n");
}

TEST(InterpreterTest, SemanticResolver) {
    raven::semantic::Resolver resolver;

    Lexer lexer("let x = 42; fn foo(a) { return a + x; }");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto stmts = parser.parse();

    bool ok = resolver.resolve(stmts);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(resolver.has_errors());

    EXPECT_EQ(run_code("let x = 10; print(x);\n"), "10\n");
}

TEST(InterpreterTest, NativeCryptographySHA256AndMD5) {
    EXPECT_EQ(run_code("print(native_sha256(\"hello\"));\n"),
              "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824\n");
    EXPECT_EQ(run_code("print(native_md5(\"hello\"));\n"),
              "5d41402abc4b2a76b9719d911017c592\n");
    EXPECT_EQ(run_code("import \"stdlib/crypto\"; print(sha256_sum(\"hello\"));\n"),
              "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824\n");
}

TEST(InterpreterTest, ModularNetHttpPackage) {
    // Verify HTTP modular structures compile and execute in Raven
    std::string code =
        "import \"stdlib/net/http/status\";\n"
        "import \"stdlib/net/http/headers\";\n"
        "import \"stdlib/net/http/response\";\n"
        "print(status_text(STATUS_OK));\n"
        "let h = new_headers();\n"
        "header_set(h, \"Authorization\", \"Bearer test-token\");\n"
        "print(header_get(h, \"authorization\", \"none\"));\n";

    EXPECT_EQ(run_code(code), "OK\nBearer test-token\n");
}

TEST(InterpreterTest, NativeSocketsUDPDatagrams) {
    std::string code =
        "import \"stdlib/net/udp\";\n"
        "let sock = udp_listen(\"127.0.0.1\", 28123);\n"
        "print(sock != nil);\n"
        "let sent = sock.send_to(\"127.0.0.1\", 28123, \"ping-udp\");\n"
        "print(sent > 0);\n"
        "let pkt = sock.recv_from(1024);\n"
        "print(pkt[\"data\"]);\n"
        "print(pkt[\"sender_ip\"]);\n"
        "sock.close();\n";
    EXPECT_EQ(run_code(code), "true\ntrue\nping-udp\n127.0.0.1\n");
}

TEST(InterpreterTest, NativeSocketsTCPCommunication) {
    std::string code =
        "import \"stdlib/net/tcp\";\n"
        "let srv = tcp_listen(\"127.0.0.1\", 28124);\n"
        "print(srv != nil);\n"
        "spawn(fn() {\n"
        "    let conn = srv.accept();\n"
        "    if (conn != nil) {\n"
        "        let msg = conn.read(1024);\n"
        "        conn.write(\"pong:\" + msg);\n"
        "        conn.close();\n"
        "    }\n"
        "});\n"
        "sleep(50);\n"
        "let cli = tcp_dial(\"127.0.0.1\", 28124, 2000);\n"
        "print(cli != nil);\n"
        "cli.write(\"hello\");\n"
        "let resp = cli.read(1024);\n"
        "print(resp);\n"
        "cli.close();\n"
        "srv.close();\n";
    EXPECT_EQ(run_code(code), "true\ntrue\npong:hello\n");
}

TEST(InterpreterTest, NetHttpCookiesAndErrors) {
    std::string code =
        "import \"stdlib/net/http/cookies\";\n"
        "import \"stdlib/net/http/errors\";\n"
        "let c = parse_cookies(\"session=xyz123; user=alice\");\n"
        "print(cookie_get(c, \"session\", \"\"));\n"
        "print(cookie_get(c, \"user\", \"\"));\n"
        "let formatted = format_cookie(\"token\", \"secret\", {\"path\": \"/api\", \"http_only\": true});\n"
        "print(formatted);\n"
        "let err = not_found_error(\"User missing\");\n"
        "print(err[\"status\"]);\n"
        "print(err[\"message\"]);\n";
    EXPECT_EQ(run_code(code),
              "xyz123\n"
              "alice\n"
              "token=secret; Path=/api; HttpOnly\n"
              "404\n"
              "User missing\n");
}

TEST(InterpreterTest, NetHttpServerRequestParsing) {
    std::string code =
        "import \"stdlib/net/http/server\";\n"
        "let raw = \"POST /items?sort=desc HTTP/1.1\\r\\n\" +\n"
        "          \"Host: localhost:8080\\r\\n\" +\n"
        "          \"Content-Type: application/json\\r\\n\" +\n"
        "          \"Cookie: sid=123\\r\\n\" +\n"
        "          \"\\r\\n\" +\n"
        "          \"{\\\"name\\\": \\\"raven\\\"}\";\n"
        "let req = parse_http_request(raw);\n"
        "print(req[\"method\"]);\n"
        "print(req[\"path\"]);\n"
        "print(req[\"query\"]);\n"
        "print(req[\"cookies\"][\"sid\"]);\n"
        "print(req[\"json\"][\"name\"]);\n";
    EXPECT_EQ(run_code(code), "POST\n/items\nsort=desc\n123\nraven\n");
}

TEST(InterpreterTest, StdlibCoreResultAndOption) {
    std::string code =
        "import \"stdlib/core/result\";\n"
        "import \"stdlib/core/option\";\n"
        "let r1 = Ok(100);\n"
        "print(r1.is_ok());\n"
        "print(r1.unwrap());\n"
        "let r2 = Err(\"failed\");\n"
        "print(r2.is_err());\n"
        "print(r2.unwrap_or(42));\n"
        "let o1 = Some(\"raven\");\n"
        "print(o1.is_some());\n"
        "print(o1.unwrap());\n"
        "let o2 = None();\n"
        "print(o2.is_none());\n"
        "print(o2.unwrap_or(\"fallback\"));\n";
    EXPECT_EQ(run_code(code),
              "true\n"
              "100\n"
              "true\n"
              "42\n"
              "true\n"
              "raven\n"
              "true\n"
              "fallback\n");
}

TEST(InterpreterTest, StdlibModularCollections) {
    std::string code =
        "import \"stdlib/collections/stack\";\n"
        "import \"stdlib/collections/queue\";\n"
        "import \"stdlib/collections/set\";\n"
        "let st = new_stack();\n"
        "st.push(10);\n"
        "st.push(20);\n"
        "print(st.pop());\n"
        "print(st.peek());\n"
        "let q = new_queue();\n"
        "q.enqueue(\"first\");\n"
        "q.enqueue(\"second\");\n"
        "print(q.dequeue());\n"
        "print(q.peek());\n"
        "let s = new_set();\n"
        "s.add(\"alpha\");\n"
        "s.add(\"beta\");\n"
        "print(s.has(\"alpha\"));\n"
        "print(s.size());\n";
    EXPECT_EQ(run_code(code),
              "20\n"
              "10\n"
              "first\n"
              "second\n"
              "true\n"
              "2\n");
}

TEST(InterpreterTest, ArrayAndMapDestructuring) {
    std::string code =
        "let [x, y, z] = [10, 20, 30];\n"
        "print(x);\n"
        "print(y);\n"
        "print(z);\n"
        "let {name, role} = {\"name\": \"Raven\", \"role\": \"Compiler\"};\n"
        "print(name);\n"
        "print(role);\n";
    EXPECT_EQ(run_code(code), "10\n20\n30\nRaven\nCompiler\n");
}

TEST(InterpreterTest, VariadicFunctionsAndLambdas) {
    std::string code =
        "fn collect_all(...items) {\n"
        "    return items;\n"
        "}\n"
        "let res1 = collect_all(1, 2, 3);\n"
        "print(len(res1));\n"
        "print(res1[0] + res1[1] + res1[2]);\n"
        "fn prefix_log(prefix, ...words) {\n"
        "    return prefix + \": \" + join(words, \" \");\n"
        "}\n"
        "print(prefix_log(\"INFO\", \"System\", \"online\", \"and\", \"ready\"));\n"
        "let lam = fn(first, ...rest) {\n"
        "    return first + len(rest);\n"
        "};\n"
        "print(lam(10, 1, 2, 3));\n";
    EXPECT_EQ(run_code(code), "3\n6\nINFO: System online and ready\n13\n");
}

TEST(InterpreterTest, StdlibConfigEnvParsing) {
    std::string code =
        "import \"stdlib/config\";\n"
        "let env_str = \"# database config\\nPORT=8080\\nDEBUG=true\\nNAME=\\\"Raven Server\\\"\\n\";\n"
        "let parsed = parse_env_content(env_str);\n"
        "print(parsed[\"PORT\"]);\n"
        "print(parsed[\"DEBUG\"]);\n"
        "print(parsed[\"NAME\"]);\n";
    EXPECT_EQ(run_code(code), "8080\ntrue\nRaven Server\n");
}

TEST(InterpreterTest, AdvancedCollectionsDequeAndPriorityQueue) {
    std::string code =
        "import \"stdlib/collections\";\n"
        "let dq = create_deque();\n"
        "dq.push_back(10);\n"
        "dq.push_front(5);\n"
        "dq.push_back(20);\n"
        "print(dq.pop_front());\n"
        "print(dq.pop_back());\n"
        "print(dq.size());\n"
        "let pq = create_priority_queue();\n"
        "pq.push(\"low\", 10);\n"
        "pq.push(\"critical\", 1);\n"
        "pq.push(\"medium\", 5);\n"
        "print(pq.pop());\n"
        "print(pq.pop());\n";
    EXPECT_EQ(run_code(code), "5\n20\n1\ncritical\nmedium\n");
}

TEST(InterpreterTest, AdvancedCollectionsLinkedListAndBST) {
    std::string code =
        "import \"stdlib/collections\";\n"
        "let ll = create_linked_list();\n"
        "ll.push_back(\"a\");\n"
        "ll.push_front(\"z\");\n"
        "ll.push_back(\"b\");\n"
        "print(ll.get(0));\n"
        "print(ll.get(1));\n"
        "print(ll.size());\n"
        "let bst = create_bst();\n"
        "bst.insert(50);\n"
        "bst.insert(20);\n"
        "bst.insert(80);\n"
        "bst.insert(10);\n"
        "print(bst.contains(20));\n"
        "print(bst.contains(99));\n"
        "print(bst.min());\n"
        "print(bst.max());\n";
    EXPECT_EQ(run_code(code), "z\na\n3\ntrue\nfalse\n10\n80\n");
}

TEST(InterpreterTest, AdvancedCollectionsTrieAndRingBuffer) {
    std::string code =
        "import \"stdlib/collections\";\n"
        "let trie = create_trie();\n"
        "trie.insert(\"raven\");\n"
        "trie.insert(\"ravine\");\n"
        "trie.insert(\"rapid\");\n"
        "print(trie.contains(\"raven\"));\n"
        "print(trie.starts_with(\"rav\"));\n"
        "let rb = create_ring_buffer(3);\n"
        "rb.push(1);\n"
        "rb.push(2);\n"
        "rb.push(3);\n"
        "rb.push(4);\n"
        "print(rb.size());\n"
        "print(rb.pop());\n"
        "print(rb.pop());\n";
    EXPECT_EQ(run_code(code), "true\ntrue\n3\n2\n3\n");
}

TEST(InterpreterTest, NativeHMACSHA256StandardVector) {
    std::string code =
        "import \"stdlib/crypto\";\n"
        "let sig = hmac_sha256(\"secret_key\", \"The quick brown fox jumps over the lazy dog\");\n"
        "print(sig);\n";
    EXPECT_EQ(run_code(code), "31fd15fa0f61fd40dc09d919d4aa5b4141a0b27c1d51e74a6789a890aaaa187c\n");
}

TEST(InterpreterTest, ModernCleanSyntaxOptionalSemicolonsAndParens) {
    std::string code =
        "let a = 10\n"
        "let b = 20\n"
        "let sum = a + b\n"
        "if sum > 25 {\n"
        "    print(\"Sum is\", sum)\n"
        "}\n"
        "let items = [1, 2, 3]\n"
        "for x in items {\n"
        "    if x == 2 {\n"
        "        print(\"found\", x)\n"
        "    }\n"
        "}\n";
    EXPECT_EQ(run_code(code), "Sum is 30\nfound 2\n");
}

TEST(InterpreterTest, VariadicPrintWithMultipleArguments) {
    std::string code =
        "print(\"Alpha\", 42, true, [1, 2]);\n"
        "print();\n"
        "print(\"End\");\n";
    EXPECT_EQ(run_code(code), "Alpha 42 true [1, 2]\n\nEnd\n");
}

TEST(InterpreterTest, NativeSliceSyntaxArraysAndStrings) {
    std::string code =
        "let arr = [10, 20, 30, 40, 50];\n"
        "let sub = arr[1:4];\n"
        "print(sub[0]);\n"
        "print(sub[2]);\n"
        "let front = arr[:2];\n"
        "print(len(front));\n"
        "let back = arr[3:];\n"
        "print(back[0]);\n"
        "let s = \"HelloRaven\";\n"
        "print(s[0:5]);\n"
        "print(s[5:]);\n";
    EXPECT_EQ(run_code(code), "20\n40\n2\n40\nHello\nRaven\n");
}

TEST(InterpreterTest, TypeAnnotationsGradualTyping) {
    std::string valid_code =
        "let n: Number = 100;\n"
        "let s: String = \"TypeSafe\";\n"
        "let b: Bool = true;\n"
        "let arr: Array = [1, 2];\n"
        "fn mult(x: Number, y: Number): Number {\n"
        "    return x * y;\n"
        "}\n"
        "print(n, s, b, len(arr), mult(3, 4));\n";
    EXPECT_EQ(run_code(valid_code), "100 TypeSafe true 2 12\n");

    std::string invalid_code = "let x: Number = \"bad\";\n";
    EXPECT_THROW(run_code(invalid_code), RuntimeError);
}

TEST(InterpreterTest, NativeWebFrameworkAndDSL) {
    std::string code =
        "import \"stdlib/web/html\";\n"
        "import \"stdlib/web/template\";\n"
        "let el = div_el({\"class\": \"card\"}, [h1_el({}, \"Title\"), p_el({}, \"Body\")]);\n"
        "print(el);\n"
        "let tmpl = render_template(\"Hello {{ user }}!\", {\"user\": \"World\"});\n"
        "print(tmpl);\n";
    EXPECT_EQ(run_code(code), "<div class=\"card\"><h1>Title</h1><p>Body</p></div>\nHello World!\n");
}

TEST(InterpreterTest, ResultAndOptionMonadTypes) {
    std::string code =
        "import \"stdlib/result\";\n"
        "import \"stdlib/option\";\n"
        "let ok_res = Ok(100);\n"
        "let err_res = Err(\"failed\");\n"
        "print(is_ok(ok_res));\n"
        "print(is_err(err_res));\n"
        "print(unwrap(ok_res));\n"
        "print(unwrap_or(err_res, 0));\n"
        "let opt_some = Some(\"Raven\");\n"
        "let opt_none = None();\n"
        "print(is_some(opt_some));\n"
        "print(is_none(opt_none));\n"
        "print(option_unwrap(opt_some));\n"
        "print(option_unwrap_or(opt_none, \"fallback\"));\n";
    EXPECT_EQ(run_code(code), "true\ntrue\n100\n0\ntrue\ntrue\nRaven\nfallback\n");
}















