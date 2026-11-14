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
        "println(\"Hello, \" + name);\n"
        "\n"
        "if (age >= 18) {\n"
        "    println(\"Adult\");\n"
        "}\n";

    EXPECT_EQ(run_code(source), "Hello, Ran\nAdult\n");
}

TEST(InterpreterTest, PrintVsPrintlnSemantics) {
    // print does not emit a newline, println does
    std::string print_code =
        "print(\"Hello, \");\n"
        "print(\"AninextCrew!\");\n";
    EXPECT_EQ(run_code(print_code), "Hello, AninextCrew!");

    std::string println_code =
        "println(\"Hello, \");\n"
        "println(\"AninextCrew!\");\n";
    EXPECT_EQ(run_code(println_code), "Hello, \nAninextCrew!\n");

    std::string mixed_code =
        "print(\"Score: \");\n"
        "println(100);\n";
    EXPECT_EQ(run_code(mixed_code), "Score: 100\n");

    // Also test bytecode VM
    EXPECT_EQ(run_vm_code(print_code), "Hello, AninextCrew!");
    EXPECT_EQ(run_vm_code(println_code), "Hello, \nAninextCrew!\n");
    EXPECT_EQ(run_vm_code(mixed_code), "Score: 100\n");
}

TEST(InterpreterTest, ArithmeticAndPrecedence) {
    std::string source =
        "println(2 + 3 * 4);\n"
        "println((2 + 3) * 4);\n"
        "println(10 - 4 / 2);\n";

    EXPECT_EQ(run_code(source), "14\n20\n8\n");
}

TEST(InterpreterTest, VariablesAndScopes) {
    std::string source =
        "let x = 10;\n"
        "{\n"
        "    let x = 20;\n"
        "    println(x);\n"
        "}\n"
        "println(x);\n";

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
        "println(sum);\n";

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
        "println(fib(7));\n";

    EXPECT_EQ(run_code(valid_source), "13\n");
}

TEST(InterpreterTest, DynamicArrays) {
    std::string source =
        "let arr = [10, 20, 30];\n"
        "push(arr, 40);\n"
        "println(len(arr));\n"
        "println(arr[0]);\n"
        "println(arr[-1]);\n"
        "arr[1] = 99;\n"
        "println(arr);\n";

    EXPECT_EQ(run_code(source), "4\n10\n40\n[10, 99, 30, 40]\n");
}

TEST(InterpreterTest, HashMapsAndObjects) {
    std::string source =
        "let user = {\"name\": \"Ran\", \"score\": 100};\n"
        "println(user.name);\n"
        "user.score = 150;\n"
        "println(user[\"score\"]);\n";

    EXPECT_EQ(run_code(source), "Ran\n150\n");
}

TEST(InterpreterTest, FunctionalPipelineAndLambdas) {
    std::string source =
        "let nums = [1, 2, 3, 4, 5];\n"
        "let double_val = fn(x) { return x * 2; };\n"
        "let is_even = fn(x) { return x % 2 == 0; };\n"
        "let evens = filter(nums, is_even);\n"
        "let doubled = map(evens, double_val);\n"
        "println(doubled);\n";

    EXPECT_EQ(run_code(source), "[4, 8]\n");
}

TEST(InterpreterTest, PipelineOperator) {
    std::string source =
        "let inc = fn(x) { return x + 10; };\n"
        "let res = 5 |> inc;\n"
        "println(res);\n";

    EXPECT_EQ(run_code(source), "15\n");
}

TEST(InterpreterTest, ForInLoop) {
    std::string source =
        "let sum = 0;\n"
        "for (let x in [10, 20, 30]) {\n"
        "    sum = sum + x;\n"
        "}\n"
        "println(sum);\n";

    EXPECT_EQ(run_code(source), "60\n");
}

TEST(InterpreterTest, TryCatchAndThrow) {
    std::string source =
        "try {\n"
        "    throw \"Custom failure\";\n"
        "} catch (err) {\n"
        "    println(\"Caught: \" + err);\n"
        "}\n";

    EXPECT_EQ(run_code(source), "Caught: Custom failure\n");
}

TEST(InterpreterTest, BytecodeVMExecution) {
    std::string source =
        "let a = 15;\n"
        "let b = 25;\n"
        "println(a + b);\n"
        "let arr = [1, 2, 3];\n"
        "println(arr[0] + arr[1]);\n";

    EXPECT_EQ(run_vm_code(source), "40\n3\n");
}

TEST(InterpreterTest, BytecodeSerializationRoundtrip) {
    std::string source =
        "let msg = \"Bytecode Binary Serializer\";\n"
        "println(msg);\n";

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
    std::string source = "println(notDefined);\n";
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
        "println(msg);\n";

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
        "println(m);\n"
        "println(p);\n"
        "println(f);\n";

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
        "println(rep);\n"
        "println(pad);\n";

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
        "println(s);\n"
        "println(r);\n"
        "println(upper);\n"
        "println(joined);\n";

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
        "println(parsed.name);\n"
        "println(parsed.version);\n"
        "println(parsed.active);\n";

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
        "println(stack.pop());\n"
        "println(stack.peek());\n"
        "let q = create_queue();\n"
        "q.enqueue(\"A\");\n"
        "q.enqueue(\"B\");\n"
        "println(q.dequeue());\n"
        "let set = create_set();\n"
        "set.add(\"x\");\n"
        "set.add(\"x\");\n"
        "println(set.size());\n";

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
        "println(paired[0][0] + \":\" + paired[0][1]);\n"
        "let chunks = chunk([1, 2, 3, 4], 2);\n"
        "println(len(chunks));\n"
        "let flat = flatten([[10, 20], [30]]);\n"
        "println(len(flat));\n";

    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    std::ostringstream out;
    Interpreter interpreter(out);
    interpreter.interpret(stmts);
    EXPECT_EQ(out.str(), "1:a\n2\n3\n");
}

TEST(InterpreterTest, NativeAbsAndSortBuiltins) {
    EXPECT_EQ(run_code("println(abs(-5));\n"), "5\n");
    EXPECT_EQ(run_code("println(abs(3.14));\n"), "3.14\n");
    EXPECT_EQ(run_code(
        "let arr = [3, 1, 4, 1, 5, 9, 2, 6];\n"
        "sort_arr(arr);\n"
        "println(arr[0]);\n"
        "println(arr[7]);\n"
    ), "1\n9\n");
}

TEST(InterpreterTest, NativeCharBuiltins) {
    EXPECT_EQ(run_code("println(char_code(\"A\"));\n"), "65\n");
    EXPECT_EQ(run_code("println(char(65));\n"), "A\n");
}

TEST(InterpreterTest, StandardLibrarySortAndStringUtils) {
    std::string source =
        "import \"stdlib/sort\";\n"
        "import \"stdlib/string_utils\";\n"
        "let sorted = bubble_sort([5, 2, 8, 1]);\n"
        "println(sorted[0]);\n"
        "println(sorted[3]);\n"
        "println(min_of_arr(sorted));\n"
        "println(max_of_arr(sorted));\n"
        "println(starts_with(\"raven-lang\", \"raven\"));\n"
        "println(ends_with(\"code.rvn\", \".rvn\"));\n"
        "println(count_occurrences(\"banana\", \"an\"));\n";

    EXPECT_EQ(run_code(source), "1\n8\n1\n8\ntrue\ntrue\n2\n");
}

TEST(InterpreterTest, StringInterpolation) {
    std::string source =
        "let name = \"Raven\";\n"
        "let ver = 2;\n"
        "let msg = f\"Welcome to {name} v{ver + 0.5}!\";\n"
        "println(msg);\n";
    EXPECT_EQ(run_code(source), "Welcome to Raven v2.5!\n");
}

TEST(InterpreterTest, StructDeclarationAndAccess) {
    std::string source =
        "struct Vector2D { x, y };\n"
        "let v = Vector2D(3, 4);\n"
        "println(v.__type__);\n"
        "println(v.x);\n"
        "println(v.y);\n"
        "v.x = 100;\n"
        "println(v.x);\n";
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
        "println(sum);\n";
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
        "println(x);\n";
    // 10 + 5 = 15; 15 - 3 = 12; 12 * 2 = 24; 24 / 4 = 6
    EXPECT_EQ(run_code(source), "6\n");
}

TEST(InterpreterTest, NativeRegexAndSliceAndHash) {
    std::string source =
        "let matched = regex_match(\"user@example.com\", \".*@.*\\\\.com\");\n"
        "println(matched);\n"
        "let s = slice(\"Hello, World\", 0, 5);\n"
        "println(s);\n"
        "let arr_s = slice([10, 20, 30, 40], 1, 3);\n"
        "println(arr_s[0]);\n"
        "println(arr_s[1]);\n"
        "let b64 = base64_encode(\"Raven\");\n"
        "println(b64);\n"
        "println(base64_decode(b64));\n";
    EXPECT_EQ(run_code(source), "true\nHello\n20\n30\nUmF2ZW4=\nRaven\n");
}

TEST(InterpreterTest, NewStringAndArrayBuiltins) {
    std::string source =
        "println(str_repeat(\"ab\", 3));\n"
        "println(pad_left(\"42\", 5, \"0\"));\n"
        "println(is_digit(\"7\"));\n"
        "println(is_alpha(\"k\"));\n"
        "println(has_prefix(\"golang\", \"go\"));\n"
        "println(has_suffix(\"golang\", \"lang\"));\n"
        "let a = [3, 1, 4, 1, 5];\n"
        "println(sum(a));\n"
        "println(arr_min(a));\n"
        "println(arr_max(a));\n"
        "println(len(unique(a)));\n"
        "println(format(\"Raven v{} is {}\", 2, \"modern\"));\n";
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
        "println(str_title(\"hello world raven\"));\n"
        "println(atoi(\"42\") + 8);\n"
        "println(slices_sum([10, 20, 30]));\n"
        "let m1 = {\"a\": 1};\n"
        "let m2 = {\"b\": 2};\n"
        "let m = map_merge(m1, m2);\n"
        "println(m[\"a\"] + m[\"b\"]);\n"
        "println(compare(5, 10));\n"
        "let h = create_min_heap();\n"
        "h.push(40);\n"
        "h.push(10);\n"
        "h.push(20);\n"
        "println(h.pop());\n"
        "println(h.pop());\n";
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
        "println(len(u));\n"
        "let x = xid_new();\n"
        "println(len(x));\n"
        "let y = yaml_parse(\"name: Raven\\nversion: 3\\nready: true\\n\");\n"
        "println(y[\"name\"]);\n"
        "println(y[\"ready\"]);\n"
        "let cfg = ini_parse(\"[server]\\nhost = 127.0.0.1\\nport = 8080\\n\");\n"
        "println(cfg[\"server\"][\"host\"]);\n"
        "println(type_by_extension(\".json\"));\n"
        "let rendered = template_render(\"Hello {{name}}!\", {\"name\": \"Gopher\"});\n"
        "println(rendered);\n"
        "let atom = create_atomic_int(10);\n"
        "atom.add(5);\n"
        "println(atom.load());\n"
        "let ar = create_arena();\n"
        "let p1 = ar.allocate(\"item\");\n"
        "println(ar.count());\n"
        "println(unique_make(\"test_val\"));\n"
        "embed_file(\"/virtual/config.txt\", \"data\");\n"
        "println(embed_read(\"/virtual/config.txt\"));\n";

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
        "println(chan_len(ch));\n"
        "println(chan_recv(ch));\n"
        "println(chan_recv(ch));\n"
        "chan_close(ch);\n"
        "\n"
        "import \"stdlib/sync\";\n"
        "let sc = create_channel(1);\n"
        "go(fn() { sc.send(\"from_go\"); });\n"
        "println(sc.recv());\n"
        "\n"
        "let wg = create_wait_group();\n"
        "wg.add(2);\n"
        "go(fn() { wg.done(); });\n"
        "go(fn() { wg.done(); });\n"
        "wg.wait();\n"
        "println(\"wg_ok\");\n";

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
                       "println(greet());\n"
                       "println(greet(\"Raven\"));\n"), "Hello, World!\nHello, Raven!\n");
}

TEST(InterpreterTest, NullCoalescing) {
    EXPECT_EQ(run_code("let x = nil;\n"
                       "let y = x ?? \"default\";\n"
                       "println(y);\n"
                       "let z = 42 ?? \"default\";\n"
                       "println(z);\n"), "default\n42\n");
}

TEST(InterpreterTest, IsTypeOperator) {
    EXPECT_EQ(run_code("println(42 is Number);\n"
                       "println(\"hi\" is String);\n"
                       "println(true is Bool);\n"
                       "println([1,2] is Array);\n"
                       "println(nil is Nil);\n"), "true\ntrue\ntrue\ntrue\ntrue\n");
}

TEST(InterpreterTest, OptionalChaining) {
    EXPECT_EQ(run_code("let obj = nil;\n"
                       "println(obj?.name);\n"
                       "println(obj?.[0]);\n"
                       "let person = {\"name\": \"Alice\"};\n"
                       "println(person?.name);\n"), "nil\nnil\nAlice\n");
}

TEST(InterpreterTest, SpreadOperator) {
    EXPECT_EQ(run_code("let a = [1, 2, 3];\n"
                       "let b = [...a, 4, 5];\n"
                       "println(len(b));\n"
                       "println(b[3]);\n"
                       "let m1 = {\"x\": 10};\n"
                       "let m2 = {...m1, \"y\": 20};\n"
                       "println(m2[\"x\"] + m2[\"y\"]);\n"), "5\n4\n30\n");
}

TEST(InterpreterTest, LoopKeyword) {
    EXPECT_EQ(run_code("let i = 0;\n"
                       "loop {\n"
                       "    i += 1;\n"
                       "    if (i >= 3) { break; }\n"
                       "}\n"
                       "println(i);\n"), "3\n");
}

TEST(InterpreterTest, MultiLineStrings) {
    std::string source =
        "let msg = \"\"\"line 1\n"
        "line 2\"\"\";\n"
        "println(msg);\n";
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
        "println(received);\n"
        "\n"
        "let cache = create_lru_cache(2);\n"
        "cache.set(\"a\", 1);\n"
        "cache.set(\"b\", 2);\n"
        "println(cache.get(\"a\"));\n"
        "println(cache.size());\n"
        "\n"
        "let errs = validate_schema({\"name\": \"Ran\", \"age\": 25}, {\n"
        "    \"name\": {\"required\": true, \"type\": \"string\"},\n"
        "    \"age\": {\"required\": true, \"type\": \"number\"}\n"
        "});\n"
        "println(len(errs));\n";

    EXPECT_EQ(run_code(source), "42\n1\n2\n0\n");
}

TEST(InterpreterTest, BytecodeVerifierValidChunk) {
    std::string source =
        "let x = 100;\n"
        "let y = 200;\n"
        "println(x + y);\n";

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

TEST(InterpreterTest, BytecodeVerifierLongInstructionsBeyond256Constants) {
    // Generate 300 variables to force emission of ConstantLong, DefineGlobalLong, GetGlobalLong
    std::string source;
    for (int i = 0; i < 300; ++i) {
        source += "let v_" + std::to_string(i) + " = " + std::to_string(i) + ";\n";
    }
    source += "println(v_299);\n";

    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    auto stmts = parser.parse();

    Chunk chunk;
    Compiler compiler(chunk);
    ASSERT_TRUE(compiler.compile(stmts));

    // BytecodeVerifier must properly handle ConstantLong/DefineGlobalLong/GetGlobalLong
    // without misinterpreting 24-bit operand bytes as invalid opcodes or bad jumps.
    auto res = BytecodeVerifier::verify(chunk);
    EXPECT_TRUE(res.is_valid);
    EXPECT_TRUE(res.error_message.empty());

    // VM must also execute it correctly
    std::ostringstream out;
    VM vm(out);
    auto run_res = vm.run(chunk);
    EXPECT_EQ(run_res, InterpretResult::Ok);
    EXPECT_EQ(out.str(), "299\n");
}

TEST(InterpreterTest, DualEngineDifferentialParityArithmetic) {
    std::string source =
        "let a = 12 * 8 + 4;\n"
        "let b = (a - 10) / 2;\n"
        "println(a);\n"
        "println(b);\n"
        "println(a > b);\n"
        "println(a == b);\n";

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
        "println(sum);\n";

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
        "println(b);\n"
        "println(c);\n"
        "println(b is Number);\n"
        "println(\"hello\" is String);\n"
        "println(nil is Nil);\n";

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
        "println(res1 == nil);\n"
        "// 2. Send value, expect immediate recv with timeout\n"
        "ch.send(\"hello\");\n"
        "let res2 = ch.recv_timeout(100);\n"
        "println(res2);\n";

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
        "println(broken);\n"
        "let after_stats = gc_stats();\n"
        "println(after_stats[\"total_collections\"] > initial_stats[\"total_collections\"]);\n";

    EXPECT_EQ(run_code(source), "0\ntrue\n");
}

TEST(InterpreterTest, DualEngineDifferentialParityMapsAndMatch) {
    std::string source =
        "let user = {\"name\": \"Alice\", \"age\": 30};\n"
        "println(user[\"name\"]);\n"
        "println(user.age);\n"
        "user.age = 31;\n"
        "println(user.age);\n"
        "let status = match (user.age) {\n"
        "    20 => \"junior\",\n"
        "    31 => \"senior\",\n"
        "    _ => \"unknown\"\n"
        "};\n"
        "println(status);\n";

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
        "println(len);\n"
        "let pid = libc.call(\"getpid\", \"int\", [], []);\n"
        "println(pid > 0);\n"
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
        "println(total);\n";

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
        "println(sum);\n";

    std::string interp_out = run_code(source);
    std::string vm_out = run_vm_code(source);
    EXPECT_EQ(interp_out, vm_out);
    EXPECT_EQ(interp_out, "8\n"); // 1 + 3 + 4 = 8
}

TEST(InterpreterTest, DualEngineDifferentialParityLambdas) {
    std::string source =
        "let square = fn(x) { return x * x; };\n"
        "println(square(5));\n"
        "let apply = fn(f, val) { return f(val); };\n"
        "println(apply(square, 6));\n";

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
    // Calling done() when counter is already 0 must throw negative counter exception
    EXPECT_THROW(wgm.done(wg_id), std::runtime_error);

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

    EXPECT_EQ(run_code("let x = [1, 2, 3]; println(len(x));\n"), "3\n");
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

    EXPECT_EQ(run_code("let x = 10; println(x);\n"), "10\n");
}

TEST(InterpreterTest, NativeCryptographySHA256AndMD5) {
    EXPECT_EQ(run_code("println(native_sha256(\"hello\"));\n"),
              "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824\n");
    EXPECT_EQ(run_code("println(native_md5(\"hello\"));\n"),
              "5d41402abc4b2a76b9719d911017c592\n");
    EXPECT_EQ(run_code("import \"stdlib/crypto\"; println(sha256_sum(\"hello\"));\n"),
              "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824\n");
}

TEST(InterpreterTest, ModularNetHttpPackage) {
    // Verify HTTP modular structures compile and execute in Raven
    std::string code =
        "import \"stdlib/net/http/status\";\n"
        "import \"stdlib/net/http/headers\";\n"
        "import \"stdlib/net/http/response\";\n"
        "println(status_text(STATUS_OK));\n"
        "let h = new_headers();\n"
        "header_set(h, \"Authorization\", \"Bearer test-token\");\n"
        "println(header_get(h, \"authorization\", \"none\"));\n";

    EXPECT_EQ(run_code(code), "OK\nBearer test-token\n");
}

TEST(InterpreterTest, NativeSocketsUDPDatagrams) {
    std::string code =
        "import \"stdlib/net/udp\";\n"
        "let sock = udp_listen(\"127.0.0.1\", 28123);\n"
        "println(sock != nil);\n"
        "let sent = sock.send_to(\"127.0.0.1\", 28123, \"ping-udp\");\n"
        "println(sent > 0);\n"
        "let pkt = sock.recv_from(1024);\n"
        "println(pkt[\"data\"]);\n"
        "println(pkt[\"sender_ip\"]);\n"
        "sock.close();\n";
    EXPECT_EQ(run_code(code), "true\ntrue\nping-udp\n127.0.0.1\n");
}

TEST(InterpreterTest, NativeSocketsTCPCommunication) {
    std::string code =
        "import \"stdlib/net/tcp\";\n"
        "let srv = tcp_listen(\"127.0.0.1\", 28124);\n"
        "println(srv != nil);\n"
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
        "println(cli != nil);\n"
        "cli.write(\"hello\");\n"
        "let resp = cli.read(1024);\n"
        "println(resp);\n"
        "cli.close();\n"
        "srv.close();\n";
    EXPECT_EQ(run_code(code), "true\ntrue\npong:hello\n");
}

TEST(InterpreterTest, NetHttpCookiesAndErrors) {
    std::string code =
        "import \"stdlib/net/http/cookies\";\n"
        "import \"stdlib/net/http/errors\";\n"
        "let c = parse_cookies(\"session=xyz123; user=alice\");\n"
        "println(cookie_get(c, \"session\", \"\"));\n"
        "println(cookie_get(c, \"user\", \"\"));\n"
        "let formatted = format_cookie(\"token\", \"secret\", {\"path\": \"/api\", \"http_only\": true});\n"
        "println(formatted);\n"
        "let err = not_found_error(\"User missing\");\n"
        "println(err[\"status\"]);\n"
        "println(err[\"message\"]);\n";
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
        "println(req[\"method\"]);\n"
        "println(req[\"path\"]);\n"
        "println(req[\"query\"]);\n"
        "println(req[\"cookies\"][\"sid\"]);\n"
        "println(req[\"json\"][\"name\"]);\n";
    EXPECT_EQ(run_code(code), "POST\n/items\nsort=desc\n123\nraven\n");
}

TEST(InterpreterTest, StdlibCoreResultAndOption) {
    std::string code =
        "import \"stdlib/core/result\";\n"
        "import \"stdlib/core/option\";\n"
        "let r1 = Ok(100);\n"
        "println(r1.is_ok());\n"
        "println(r1.unwrap());\n"
        "let r2 = Err(\"failed\");\n"
        "println(r2.is_err());\n"
        "println(r2.unwrap_or(42));\n"
        "let o1 = Some(\"raven\");\n"
        "println(o1.is_some());\n"
        "println(o1.unwrap());\n"
        "let o2 = None();\n"
        "println(o2.is_none());\n"
        "println(o2.unwrap_or(\"fallback\"));\n";
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
        "println(st.pop());\n"
        "println(st.peek());\n"
        "let q = new_queue();\n"
        "q.enqueue(\"first\");\n"
        "q.enqueue(\"second\");\n"
        "println(q.dequeue());\n"
        "println(q.peek());\n"
        "let s = new_set();\n"
        "s.add(\"alpha\");\n"
        "s.add(\"beta\");\n"
        "println(s.has(\"alpha\"));\n"
        "println(s.size());\n";
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
        "println(x);\n"
        "println(y);\n"
        "println(z);\n"
        "let {name, role} = {\"name\": \"Raven\", \"role\": \"Compiler\"};\n"
        "println(name);\n"
        "println(role);\n";
    EXPECT_EQ(run_code(code), "10\n20\n30\nRaven\nCompiler\n");
}

TEST(InterpreterTest, VariadicFunctionsAndLambdas) {
    std::string code =
        "fn collect_all(...items) {\n"
        "    return items;\n"
        "}\n"
        "let res1 = collect_all(1, 2, 3);\n"
        "println(len(res1));\n"
        "println(res1[0] + res1[1] + res1[2]);\n"
        "fn prefix_log(prefix, ...words) {\n"
        "    return prefix + \": \" + join(words, \" \");\n"
        "}\n"
        "println(prefix_log(\"INFO\", \"System\", \"online\", \"and\", \"ready\"));\n"
        "let lam = fn(first, ...rest) {\n"
        "    return first + len(rest);\n"
        "};\n"
        "println(lam(10, 1, 2, 3));\n";
    EXPECT_EQ(run_code(code), "3\n6\nINFO: System online and ready\n13\n");
}

TEST(InterpreterTest, StdlibConfigEnvParsing) {
    std::string code =
        "import \"stdlib/config\";\n"
        "let env_str = \"# database config\\nPORT=8080\\nDEBUG=true\\nNAME=\\\"Raven Server\\\"\\n\";\n"
        "let parsed = parse_env_content(env_str);\n"
        "println(parsed[\"PORT\"]);\n"
        "println(parsed[\"DEBUG\"]);\n"
        "println(parsed[\"NAME\"]);\n";
    EXPECT_EQ(run_code(code), "8080\ntrue\nRaven Server\n");
}

TEST(InterpreterTest, AdvancedCollectionsDequeAndPriorityQueue) {
    std::string code =
        "import \"stdlib/collections\";\n"
        "let dq = create_deque();\n"
        "dq.push_back(10);\n"
        "dq.push_front(5);\n"
        "dq.push_back(20);\n"
        "println(dq.pop_front());\n"
        "println(dq.pop_back());\n"
        "println(dq.size());\n"
        "let pq = create_priority_queue();\n"
        "pq.push(\"low\", 10);\n"
        "pq.push(\"critical\", 1);\n"
        "pq.push(\"medium\", 5);\n"
        "println(pq.pop());\n"
        "println(pq.pop());\n";
    EXPECT_EQ(run_code(code), "5\n20\n1\ncritical\nmedium\n");
}

TEST(InterpreterTest, AdvancedCollectionsLinkedListAndBST) {
    std::string code =
        "import \"stdlib/collections\";\n"
        "let ll = create_linked_list();\n"
        "ll.push_back(\"a\");\n"
        "ll.push_front(\"z\");\n"
        "ll.push_back(\"b\");\n"
        "println(ll.get(0));\n"
        "println(ll.get(1));\n"
        "println(ll.size());\n"
        "let bst = create_bst();\n"
        "bst.insert(50);\n"
        "bst.insert(20);\n"
        "bst.insert(80);\n"
        "bst.insert(10);\n"
        "println(bst.contains(20));\n"
        "println(bst.contains(99));\n"
        "println(bst.min());\n"
        "println(bst.max());\n";
    EXPECT_EQ(run_code(code), "z\na\n3\ntrue\nfalse\n10\n80\n");
}

TEST(InterpreterTest, AdvancedCollectionsTrieAndRingBuffer) {
    std::string code =
        "import \"stdlib/collections\";\n"
        "let trie = create_trie();\n"
        "trie.insert(\"raven\");\n"
        "trie.insert(\"ravine\");\n"
        "trie.insert(\"rapid\");\n"
        "println(trie.contains(\"raven\"));\n"
        "println(trie.starts_with(\"rav\"));\n"
        "let rb = create_ring_buffer(3);\n"
        "rb.push(1);\n"
        "rb.push(2);\n"
        "rb.push(3);\n"
        "rb.push(4);\n"
        "println(rb.size());\n"
        "println(rb.pop());\n"
        "println(rb.pop());\n";
    EXPECT_EQ(run_code(code), "true\ntrue\n3\n2\n3\n");
}

TEST(InterpreterTest, NativeHMACSHA256StandardVector) {
    std::string code =
        "import \"stdlib/crypto\";\n"
        "let sig = hmac_sha256(\"secret_key\", \"The quick brown fox jumps over the lazy dog\");\n"
        "println(sig);\n";
    EXPECT_EQ(run_code(code), "31fd15fa0f61fd40dc09d919d4aa5b4141a0b27c1d51e74a6789a890aaaa187c\n");
}

TEST(InterpreterTest, ModernCleanSyntaxOptionalSemicolonsAndParens) {
    std::string code =
        "let a = 10\n"
        "let b = 20\n"
        "let sum = a + b\n"
        "if sum > 25 {\n"
        "    println(\"Sum is\", sum)\n"
        "}\n"
        "let items = [1, 2, 3]\n"
        "for x in items {\n"
        "    if x == 2 {\n"
        "        println(\"found\", x)\n"
        "    }\n"
        "}\n";
    EXPECT_EQ(run_code(code), "Sum is 30\nfound 2\n");
}

TEST(InterpreterTest, VariadicPrintWithMultipleArguments) {
    std::string code =
        "println(\"Alpha\", 42, true, [1, 2]);\n"
        "println();\n"
        "println(\"End\");\n";
    EXPECT_EQ(run_code(code), "Alpha 42 true [1, 2]\n\nEnd\n");
}

TEST(InterpreterTest, NativeSliceSyntaxArraysAndStrings) {
    std::string code =
        "let arr = [10, 20, 30, 40, 50];\n"
        "let sub = arr[1:4];\n"
        "println(sub[0]);\n"
        "println(sub[2]);\n"
        "let front = arr[:2];\n"
        "println(len(front));\n"
        "let back = arr[3:];\n"
        "println(back[0]);\n"
        "let s = \"HelloRaven\";\n"
        "println(s[0:5]);\n"
        "println(s[5:]);\n";
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
        "println(n, s, b, len(arr), mult(3, 4));\n";
    EXPECT_EQ(run_code(valid_code), "100 TypeSafe true 2 12\n");

    std::string invalid_code = "let x: Number = \"bad\";\n";
    EXPECT_THROW(run_code(invalid_code), RuntimeError);
}

TEST(InterpreterTest, NativeWebFrameworkAndDSL) {
    std::string code =
        "import \"stdlib/web/html\";\n"
        "import \"stdlib/web/template\";\n"
        "let el = div_el({\"class\": \"card\"}, [h1_el({}, \"Title\"), p_el({}, \"Body\")]);\n"
        "println(el);\n"
        "let tmpl = render_template(\"Hello {{ user }}!\", {\"user\": \"World\"});\n"
        "println(tmpl);\n";
    EXPECT_EQ(run_code(code), "<div class=\"card\"><h1>Title</h1><p>Body</p></div>\nHello World!\n");
}

TEST(InterpreterTest, ResultAndOptionMonadTypes) {
    std::string code =
        "import \"stdlib/result\";\n"
        "import \"stdlib/option\";\n"
        "let ok_res = Ok(100);\n"
        "let err_res = Err(\"failed\");\n"
        "println(is_ok(ok_res));\n"
        "println(is_err(err_res));\n"
        "println(unwrap(ok_res));\n"
        "println(unwrap_or(err_res, 0));\n"
        "let opt_some = Some(\"Raven\");\n"
        "let opt_none = None();\n"
        "println(is_some(opt_some));\n"
        "println(is_none(opt_none));\n"
        "println(option_unwrap(opt_some));\n"
        "println(option_unwrap_or(opt_none, \"fallback\"));\n";
    EXPECT_EQ(run_code(code), "true\ntrue\n100\n0\ntrue\ntrue\nRaven\nfallback\n");
}

TEST(InterpreterTest, UnquotedMapKeysAndShorthand) {
    std::string source =
        "let data = 1;\n"
        "let next = 2;\n"
        "let node = { data, next };\n"
        "println(node.data);\n"
        "println(node.next);\n"
        "let named = { data: 10, next: 20 };\n"
        "println(named.data);\n"
        "println(named.next);\n"
        "let quoted = { \"data\": 7 };\n"
        "println(quoted.data);\n";
    EXPECT_EQ(run_code(source), "1\n2\n10\n20\n7\n");
}

TEST(InterpreterTest, ThisInObjectMethods) {
    std::string source =
        "fn make_box(value) {\n"
        "    return {\n"
        "        data: value,\n"
        "        get: fn() { return this.data; },\n"
        "        set: fn(v) { this.data = v; }\n"
        "    };\n"
        "}\n"
        "let box = make_box(42);\n"
        "println(box.get());\n"
        "box.set(99);\n"
        "println(box.data);\n"
        "println(box.get());\n";
    EXPECT_EQ(run_code(source), "42\n99\n99\n");
}

TEST(InterpreterTest, JavaStyleClass) {
    std::string source =
        "class IntNode {\n"
        "    data\n"
        "    next\n"
        "    IntNode(data) {\n"
        "        this.data = data\n"
        "        this.next = nil\n"
        "    }\n"
        "    IntNode(data, next) {\n"
        "        this.data = data\n"
        "        this.next = next\n"
        "    }\n"
        "    setNext(next) { this.next = next }\n"
        "    getNext() { return this.next }\n"
        "    getData() { return this.data }\n"
        "    setData(data) { this.data = data }\n"
        "    toString() {\n"
        "        return \"Node{data=\" + str(this.data) + \", next=\" + str(this.next) + \"}\"\n"
        "    }\n"
        "}\n"
        "let tail = IntNode(2)\n"
        "let head = IntNode(1, tail)\n"
        "println(head.getData())\n"
        "println(head.getNext().getData())\n"
        "head.setData(9)\n"
        "println(head.getData())\n"
        "println(head is IntNode)\n"
        "println(head.toString())\n";
    EXPECT_EQ(run_code(source), "1\n2\n9\ntrue\nNode{data=9, next=Node{data=2, next=nil}}\n");
}

TEST(InterpreterTest, ArrowFunctionsAndMethodShorthand) {
    std::string source =
        "let double = x => x * 2;\n"
        "println(double(21));\n"
        "let add = (a, b) => a + b;\n"
        "println(add(2, 3));\n"
        "let box = {\n"
        "    data: 1,\n"
        "    get() { return this.data; },\n"
        "    set(v) { this.data = v; }\n"
        "};\n"
        "println(box.get());\n"
        "box.set(9);\n"
        "println(box.get());\n"
        "let evens = filter([1, 2, 3, 4], x => x % 2 == 0);\n"
        "println(evens[0]);\n"
        "println(evens[1]);\n";
    EXPECT_EQ(run_code(source), "42\n5\n1\n9\n2\n4\n");
}

TEST(InterpreterTest, DotAccessInsteadOfBrackets) {
    std::string source =
        "let node = { val: 5, left: nil, right: nil };\n"
        "println(node.val);\n"
        "node.val = 8;\n"
        "println(node.val);\n";
    EXPECT_EQ(run_code(source), "5\n8\n");
}

TEST(InterpreterTest, ClassInheritanceBasic) {
    std::string source =
        "class Animal {\n"
        "    fn speak() { return this.name + \" makes a sound\"; }\n"
        "    Animal(name) { this.name = name; }\n"
        "}\n"
        "class Dog extends Animal {\n"
        "    fn speak() { return this.name + \" barks!\"; }\n"
        "    Dog(name) { super(name); }\n"
        "}\n"
        "let a = Animal(\"Cat\");\n"
        "println(a.speak());\n"
        "let d = Dog(\"Rex\");\n"
        "println(d.speak());\n"
        "println(d.name);\n";
    EXPECT_EQ(run_code(source), "Cat makes a sound\nRex barks!\nRex\n");
}

TEST(InterpreterTest, ClassInheritanceMultiLevel) {
    std::string source =
        "class A {\n"
        "    A(x) { this.x = x; }\n"
        "    fn val() { return this.x; }\n"
        "}\n"
        "class B extends A {\n"
        "    B(x, y) { super(x); this.y = y; }\n"
        "    fn val() { return this.x + this.y; }\n"
        "}\n"
        "class C extends B {\n"
        "    C(x, y, z) { super(x, y); this.z = z; }\n"
        "    fn val() { return this.x + this.y + this.z; }\n"
        "}\n"
        "let c = C(1, 2, 3);\n"
        "println(c.val());\n"
        "println(c.x);\n"
        "println(c.y);\n"
        "println(c.z);\n";
    EXPECT_EQ(run_code(source), "6\n1\n2\n3\n");
}

TEST(InterpreterTest, ClassStaticMethods) {
    std::string source =
        "class MathHelper {\n"
        "    static fn square(x) { return x * x; }\n"
        "    static fn cube(x) { return x * x * x; }\n"
        "    static fn add(a, b) { return a + b; }\n"
        "}\n"
        "println(MathHelper.square(5));\n"
        "println(MathHelper.cube(3));\n"
        "println(MathHelper.add(10, 20));\n";
    EXPECT_EQ(run_code(source), "25\n27\n30\n");
}

TEST(InterpreterTest, SuperMethodCall) {
    std::string source =
        "class Shape {\n"
        "    Shape(color) { this.color = color; }\n"
        "    fn describe() { return \"A \" + this.color + \" shape\"; }\n"
        "}\n"
        "class Circle extends Shape {\n"
        "    Circle(color, radius) { super(color); this.radius = radius; }\n"
        "    fn describe() { return super.describe() + \" (circle)\"; }\n"
        "}\n"
        "let c = Circle(\"red\", 5);\n"
        "println(c.describe());\n"
        "println(c.color);\n"
        "println(c.radius);\n";
    EXPECT_EQ(run_code(source), "A red shape (circle)\nred\n5\n");
}

TEST(InterpreterTest, InputPromptAndIntParsing) {
    std::string source =
        "let n = int(\"42\");\n"
        "println(n * 2);\n"
        "let f = int(\"123.45\");\n"
        "println(f);\n";
    EXPECT_EQ(run_code(source), "84\n123\n");
}

TEST(InterpreterTest, TypeConversionsPythonStyle) {
    std::string source =
        "let f = float(\"3.14\");\n"
        "println(f);\n"
        "let b1 = bool(\"true\");\n"
        "let b2 = bool(\"false\");\n"
        "println(b1);\n"
        "println(b2);\n"
        "let a = array(\"cat\");\n"
        "println(len(a));\n"
        "println(a[0]);\n"
        "let d = dict([[\"x\", 10], [\"y\", 20]]);\n"
        "println(d.x + d.y);\n";
    EXPECT_EQ(run_code(source), "3.14\ntrue\nfalse\n3\nc\n30\n");
}

TEST(InterpreterTest, AsyncAwaitBasic) {
    EXPECT_EQ(run_code(
        "async fn compute(x) {\n"
        "    return x * 2;\n"
        "}\n"
        "let ch = compute(21);\n"
        "let result = await ch;\n"
        "println(result);\n"
    ), "42\n");
}

TEST(InterpreterTest, AsyncFnReturnsString) {
    EXPECT_EQ(run_code(
        "async fn greet(name) {\n"
        "    return \"Hello, \" + name + \"!\";\n"
        "}\n"
        "let ch = greet(\"Raven\");\n"
        "println(await ch);\n"
    ), "Hello, Raven!\n");
}

TEST(InterpreterTest, MultipleAsyncCalls) {
    EXPECT_EQ(run_code(
        "async fn double(x) {\n"
        "    return x * 2;\n"
        "}\n"
        "let a = double(10);\n"
        "let b = double(20);\n"
        "println(await a + await b);\n"
    ), "60\n");
}

TEST(InterpreterTest, UsingNormalExit) {
    std::string source =
        "let closed = false;\n"
        "let res = {\n"
        "    \"close\": fn() { closed = true; }\n"
        "};\n"
        "using r = res {\n"
        "    println(\"inside\");\n"
        "}\n"
        "println(closed);\n";
    EXPECT_EQ(run_code(source), "inside\ntrue\n");
}

TEST(InterpreterTest, UsingEarlyReturn) {
    std::string source =
        "let closed = false;\n"
        "let res = {\n"
        "    \"close\": fn() { closed = true; }\n"
        "};\n"
        "fn test_ret() {\n"
        "    using r = res {\n"
        "        return \"early\";\n"
        "    }\n"
        "}\n"
        "let res_val = test_ret();\n"
        "println(res_val);\n"
        "println(closed);\n";
    EXPECT_EQ(run_code(source), "early\ntrue\n");
}

TEST(InterpreterTest, UsingExceptionUnwinding) {
    std::string source =
        "let closed = false;\n"
        "let res = {\n"
        "    \"close\": fn() { closed = true; }\n"
        "};\n"
        "try {\n"
        "    using r = res {\n"
        "        throw \"something failed\";\n"
        "    }\n"
        "} catch (e) {\n"
        "    println(e);\n"
        "}\n"
        "println(closed);\n";
    EXPECT_EQ(run_code(source), "something failed\ntrue\n");
}

TEST(InterpreterTest, UsingNestedMultiCleanupContinuesOnFailure) {
    std::string source =
        "let log = [];\n"
        "let res1 = { \"close\": fn() { push(log, \"closed 1\"); } };\n"
        "let res2 = { \"close\": fn() { push(log, \"closed 2\"); throw \"fail 2\"; } };\n"
        "try {\n"
        "    using r1 = res1 {\n"
        "        using r2 = res2 {\n"
        "            push(log, \"inside\");\n"
        "        }\n"
        "    }\n"
        "} catch (e) {\n"
        "    push(log, \"caught: \" + str(e));\n"
        "}\n"
        "println(len(log));\n"
        "println(log[0]);\n"
        "println(log[1]);\n"
        "println(log[2]);\n"
        "println(log[3]);\n";
    EXPECT_EQ(run_code(source), "4\ninside\nclosed 2\nclosed 1\ncaught: fail 2\n");
}

TEST(InterpreterTest, UsingLoopBreakAndContinue) {
    std::string source =
        "let close_count = 0;\n"
        "fn make_res() {\n"
        "    return { \"close\": fn() { close_count += 1; } };\n"
        "}\n"
        "let i = 0;\n"
        "while (i < 3) {\n"
        "    i += 1;\n"
        "    using r = make_res() {\n"
        "        if (i == 1) { continue; }\n"
        "        if (i == 2) { break; }\n"
        "    }\n"
        "}\n"
        "println(close_count);\n";
    EXPECT_EQ(run_code(source), "2\n");
}

TEST(InterpreterTest, UsingMultipleCleanupBothFailPreservesPrimaryAndRunsAll) {
    std::string source =
        "let log = [];\n"
        "let res1 = { \"close\": fn() { push(log, \"res1 close attempt\"); throw \"err 1\"; } };\n"
        "let res2 = { \"close\": fn() { push(log, \"res2 close attempt\"); throw \"err 2\"; } };\n"
        "try {\n"
        "    using r1 = res1 {\n"
        "        using r2 = res2 {\n"
        "            push(log, \"work done\");\n"
        "        }\n"
        "    }\n"
        "} catch (e) {\n"
        "    push(log, \"primary: \" + str(e));\n"
        "}\n"
        "println(len(log));\n"
        "println(log[0]);\n"
        "println(log[1]);\n"
        "println(log[2]);\n"
        "println(log[3]);\n";
    EXPECT_EQ(run_code(source), "4\nwork done\nres2 close attempt\nres1 close attempt\nprimary: err 2\n");
}

TEST(InterpreterTest, MoveTransfersValueAndInvalidatesSource) {
    std::string source =
        "let a = \"hello\";\n"
        "let b = move a;\n"
        "println(b);\n";
    EXPECT_EQ(run_code(source), "hello\n");
}

TEST(InterpreterTest, MoveUseAfterMoveThrowsRuntimeError) {
    std::string source =
        "let a = 123;\n"
        "let b = move a;\n"
        "println(a);\n";
    EXPECT_THROW(run_code(source), RuntimeError);
}

TEST(InterpreterTest, DoubleMoveThrowsRuntimeError) {
    std::string source =
        "let a = 123;\n"
        "let b = move a;\n"
        "let c = move a;\n";
    EXPECT_THROW(run_code(source), RuntimeError);
}

TEST(InterpreterTest, MoveReassignmentRevivesVariable) {
    std::string source =
        "let a = 10;\n"
        "let b = move a;\n"
        "a = 20;\n"
        "println(a + b);\n";
    EXPECT_EQ(run_code(source), "30\n");
}

TEST(InterpreterTest, MoveInsideUsingSuppressesRAIICleanup) {
    std::string source =
        "let closed = false;\n"
        "let res = { \"close\": fn() { closed = true; } };\n"
        "fn take_res() {\n"
        "    using r = res {\n"
        "        return move r;\n"
        "    }\n"
        "}\n"
        "let r2 = take_res();\n"
        "println(closed);\n"
        "r2.close();\n"
        "println(closed);\n";
    EXPECT_EQ(run_code(source), "false\ntrue\n");
}



