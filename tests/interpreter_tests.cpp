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
