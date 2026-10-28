#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "raven/compiler.hpp"
#include "raven/interpreter.hpp"
#include "raven/lexer.hpp"
#include "raven/parser.hpp"
#include "raven/vm.hpp"
#include "raven/linter.hpp"
#include "raven/formatter.hpp"
#include "raven/doc_generator.hpp"
#include "raven/lsp.hpp"

namespace raven {

static int run_disasm(const std::string& source, const std::string& name) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto statements = parser.parse();

    if (parser.has_error()) {
        for (const auto& err : parser.errors()) {
            std::cerr << err << "\n";
        }
        return 65;
    }

    Chunk chunk;
    Compiler compiler(chunk);
    compiler.compile(statements);

    chunk.disassemble(std::cout, name);
    return 0;
}

static int run_vm(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto statements = parser.parse();

    if (parser.has_error()) {
        for (const auto& err : parser.errors()) {
            std::cerr << err << "\n";
        }
        return 65;
    }

    Chunk chunk;
    Compiler compiler(chunk);
    compiler.compile(statements);

    VM vm(std::cout);
    auto result = vm.run(chunk);
    return result == InterpretResult::Ok ? 0 : 70;
}

static int run(const std::string& source, Interpreter& interpreter) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto statements = parser.parse();

    if (parser.has_error()) {
        for (const auto& err : parser.errors()) {
            std::cerr << err << "\n";
        }
        return 65;
    }

    try {
        interpreter.interpret(statements);
    } catch (const RuntimeError& error) {
        std::cerr << "[line " << error.token().line << ", col "
                  << error.token().column << "] Runtime Error: "
                  << error.what() << "\n";
        return 70;
    }

    return 0;
}

static int run_file(const std::string& path, bool use_vm = false, bool disasm = false) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << path << "'\n";
        return 1;
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    if (disasm) {
        return run_disasm(ss.str(), path);
    }
    if (use_vm) {
        return run_vm(ss.str());
    }

    Interpreter interpreter;
    return run(ss.str(), interpreter);
}

static void run_prompt() {
    std::cout << "Raven 0.3.0 Interactive Shell (Tree-Walk & Bytecode VM)\n";
    std::cout << "Type 'exit' or press Ctrl+D to quit.\n";

    Interpreter interpreter;
    std::string line;

    while (true) {
        std::cout << "raven> ";
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break;
        }

        if (line == "exit" || line == "quit") {
            break;
        }

        if (line.empty()) {
            continue;
        }

        if (line.back() != ';' && line.back() != '}') {
            line += ";";
        }

        run(line, interpreter);
    }
}

} // namespace raven

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string first_arg = argv[1];

        if (first_arg == "--help" || first_arg == "-h") {
            std::cout << "Raven Programming Language (C++20)\n\n"
                      << "Usage: raven [command|options] [arguments]\n\n"
                      << "Commands:\n"
                      << "  init [name]             Initialize a new Raven project with raven.json\n"
                      << "  run <file.rvn>          Execute a Raven script file\n"
                      << "  fmt <file.rvn> [-w]     Format Raven source code (like gofmt)\n"
                      << "  vet <file.rvn>          Static analysis & linting (like go vet)\n"
                      << "  doc <file.rvn>          Generate Markdown documentation (like godoc)\n"
                      << "  bench <file.rvn>        Benchmark script execution time and memory\n"
                      << "  lsp                     Start the Language Server Protocol (LSP) server\n"
                      << "  test                    Execute test suite with ctest\n"
                      << "  pkg install <repo-url>  Clone/install a package into raven_modules/\n\n"
                      << "Options:\n"
                      << "  -e, -c <code>           Execute inline Raven code\n"
                      << "  --vm <file.rvn>         Execute using the Raven Bytecode Virtual Machine (Corvus)\n"
                      << "  --disasm <file.rvn>     Compile and display the Bytecode disassembly\n"
                      << "  --version, -v           Print Raven version\n"
                      << "  --help, -h              Show this help message\n";
            return 0;
        }

        if (first_arg == "lsp") {
            raven::LanguageServer::run(std::cin, std::cout);
            return 0;
        }

        if (first_arg == "init") {
            std::string proj_name = (argc > 2) ? argv[2] : "raven_app";
            std::string prefix = (argc > 2) ? (proj_name + "/") : "";

            if (!prefix.empty()) {
                std::system(("mkdir -p " + proj_name + "/src").c_str());
            } else {
                std::system("mkdir -p src");
            }

            // Write raven.json
            std::ofstream manifest(prefix + "raven.json");
            manifest << "{\n"
                     << "  \"name\": \"" << proj_name << "\",\n"
                     << "  \"version\": \"0.1.0\",\n"
                     << "  \"entry\": \"src/main.rvn\",\n"
                     << "  \"dependencies\": {}\n"
                     << "}\n";
            manifest.close();

            // Write src/main.rvn
            std::ofstream main_file(prefix + "src/main.rvn");
            main_file << "// " << proj_name << " - Powered by Raven\n"
                      << "import \"stdlib/fmt\";\n\n"
                      << "fn main() {\n"
                      << "    println(\"Hello, World from " << proj_name << "!\");\n"
                      << "}\n\n"
                      << "main();\n";
            main_file.close();

            // Write .gitignore
            std::ofstream gitignore(prefix + ".gitignore");
            gitignore << "build/\n*.rvc\nraven_modules/\n.raven_cache/\n";
            gitignore.close();

            std::cout << "✨ Initialized new Raven project '" << proj_name << "'\n\n"
                      << "To get started:\n";
            if (!prefix.empty()) {
                std::cout << "  cd " << proj_name << "\n";
            }
            std::cout << "  raven run src/main.rvn\n";
            return 0;
        }

        if (first_arg == "fmt" && argc > 2) {
            std::string path = argv[2];
            bool in_place = (argc > 3 && std::string(argv[3]) == "-w");
            std::ifstream file(path);
            if (!file.is_open()) {
                std::cerr << "Error: Could not open file: " << path << "\n";
                return 1;
            }
            std::stringstream buf;
            buf << file.rdbuf();
            file.close();

            std::string formatted = raven::Formatter::format(buf.str());
            if (in_place) {
                std::ofstream out(path);
                out << formatted;
                std::cout << "Formatted " << path << "\n";
            } else {
                std::cout << formatted;
            }
            return 0;
        }

        if ((first_arg == "vet" || first_arg == "lint") && argc > 2) {
            std::string path = argv[2];
            std::ifstream file(path);
            if (!file.is_open()) {
                std::cerr << "Error: Could not open file: " << path << "\n";
                return 1;
            }
            std::stringstream buf;
            buf << file.rdbuf();

            raven::Lexer lexer(buf.str());
            raven::Parser parser(lexer.tokenize());
            auto stmts = parser.parse();
            if (parser.has_error()) {
                for (const auto& err : parser.errors()) {
                    std::cerr << err << "\n";
                }
                return 65;
            }

            raven::Linter linter;
            auto issues = linter.analyze(stmts);
            if (issues.empty()) {
                std::cout << "✓ No lint issues found in " << path << ".\n";
                return 0;
            }
            for (const auto& issue : issues) {
                std::cout << issue.to_string() << "\n";
            }
            return 0;
        }

        if (first_arg == "doc" && argc > 2) {
            std::string path = argv[2];
            std::ifstream file(path);
            if (!file.is_open()) {
                std::cerr << "Error: Could not open file: " << path << "\n";
                return 1;
            }
            std::stringstream buf;
            buf << file.rdbuf();

            auto items = raven::DocGenerator::extract(buf.str());
            std::cout << raven::DocGenerator::to_markdown(path, items);
            return 0;
        }

        if (first_arg == "bench" && argc > 2) {
            std::string path = argv[2];
            std::ifstream file(path);
            if (!file.is_open()) {
                std::cerr << "Error: Could not open file: " << path << "\n";
                return 1;
            }
            std::stringstream buf;
            buf << file.rdbuf();
            std::string source = buf.str();

            std::cout << "⚡ Benchmarking " << path << " ...\n";

            auto start = std::chrono::high_resolution_clock::now();
            raven::Interpreter interp;
            raven::run(source, interp);
            auto end = std::chrono::high_resolution_clock::now();

            auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            double ms = us / 1000.0;
            std::cout << "\n[Benchmark Result] Elapsed Time: " << ms << " ms (" << us << " µs)\n";
            return 0;
        }

        if (first_arg == "--version" || first_arg == "-v") {
            std::cout << "Raven 0.3.0 (Corvus Engine, Dual AST/VM Toolchain)\n";
            return 0;
        }

        if ((first_arg == "-e" || first_arg == "-c") && argc > 2) {
            std::string code = argv[2];
            raven::Interpreter interpreter;
            return raven::run(code, interpreter);
        }

        if (first_arg == "run" && argc > 2) {
            return raven::run_file(argv[2]);
        }

        if (first_arg == "pkg" && argc > 3 && std::string(argv[2]) == "install") {
            std::string repo = argv[3];
            std::cout << "📦 Installing package '" << repo << "' into raven_modules/...\n";
            std::string cmd = "mkdir -p raven_modules && cd raven_modules && git clone " + repo;
            int ret = std::system(cmd.c_str());
            if (ret == 0) {
                std::cout << "✅ Package installed successfully!\n";
            } else {
                std::cerr << "❌ Failed to install package.\n";
            }
            return ret;
        }

        if (first_arg == "test") {
            std::cout << "🧪 Running Raven test suite...\n";
            int ret = std::system("ctest --test-dir build --output-on-failure");
            return ret;
        }

        if (first_arg == "--disasm" && argc > 2) {
            return raven::run_file(argv[2], false, true);
        }

        if (first_arg == "--vm" && argc > 2) {
            return raven::run_file(argv[2], true, false);
        }

        return raven::run_file(first_arg);
    }

    raven::run_prompt();
    return 0;
}
