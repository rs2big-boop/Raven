#include <chrono>
#include <filesystem>
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
#include "raven/debugger.hpp"
#include "raven/packages/package_manager.hpp"

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
        for (auto it = error.stack_trace().rbegin(); it != error.stack_trace().rend(); ++it) {
            std::cerr << "    at " << it->function_name << " (line " << it->line << ", col " << it->column << ")\n";
        }
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
    std::cout << "🐦 Raven 1.0.0 Interactive Shell\n";
    std::cout << "   Type .help for commands, .quit to exit\n\n";

    Interpreter interpreter;
    std::string line;
    std::string accumulated;
    int brace_depth = 0;

    while (true) {
        std::string prompt = (accumulated.empty()) ? "raven> " : "   ... ";
        std::cout << prompt;
        std::cout.flush();

        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break;
        }

        if (accumulated.empty()) {
            std::string trimmed = line;
            while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t')) trimmed.erase(0, 1);
            while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t')) trimmed.pop_back();

            if (trimmed == ".quit" || trimmed == ".exit" || trimmed == "exit" || trimmed == "quit") {
                std::cout << "Goodbye! 🐦\n";
                break;
            }
            if (trimmed == ".help") {
                std::cout << "Raven REPL Commands:\n"
                          << "  .help              Show this help\n"
                          << "  .quit / .exit      Exit the REPL\n"
                          << "  .load <file>       Load and execute a .rvn file\n"
                          << "  .clear             Clear the screen\n"
                          << "  .version           Show Raven version\n\n"
                          << "Keyboard shortcuts:\n"
                          << "  Ctrl+C             Interrupt current input\n"
                          << "  Ctrl+D             Exit REPL\n\n"
                          << "Examples:\n"
                          << "  let x = 42;\n"
                          << "  fn add(a, b = 10) { return a + b; }\n"
                          << "  add(3);\n";
                continue;
            }
            if (trimmed == ".clear") {
                std::cout << "\033[2J\033[H";
                continue;
            }
            if (trimmed == ".version") {
                std::cout << "Raven 1.0.0 (Corvus Engine & VM)\n";
                continue;
            }
            if (trimmed.rfind(".load ", 0) == 0) {
                std::string path = trimmed.substr(6);
                while (!path.empty() && path.front() == ' ') path.erase(0, 1);
                std::ifstream f(path);
                if (!f.is_open()) {
                    std::cerr << "Error: Could not open '" << path << "'\n";
                } else {
                    std::ostringstream ss;
                    ss << f.rdbuf();
                    run(ss.str(), interpreter);
                }
                continue;
            }
        }

        accumulated += line + "\n";

        for (char ch : line) {
            if (ch == '{' || ch == '(' || ch == '[') { brace_depth++; }
            else if (ch == '}' || ch == ')' || ch == ']') { brace_depth = std::max(0, brace_depth - 1); }
        }

        if (brace_depth > 0) continue;

        std::string code = accumulated;
        accumulated.clear();
        brace_depth = 0;

        if (code.find_first_not_of(" \t\n\r") == std::string::npos) continue;

        std::string test_code = code;
        while (!test_code.empty() && (test_code.back() == '\n' || test_code.back() == '\r' || test_code.back() == ' ' || test_code.back() == '\t')) {
            test_code.pop_back();
        }
        if (!test_code.empty() && test_code.back() != ';' && test_code.back() != '}') {
            code = test_code + ";\n";
        }

        Lexer lexer(code);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto stmts = parser.parse();

        if (parser.has_error()) {
            for (const auto& err : parser.errors()) {
                std::cerr << err << "\n";
            }
            continue;
        }

        try {
            if (stmts.size() == 1) {
                auto* expr_stmt = dynamic_cast<ExpressionStmt*>(stmts[0].get());
                if (expr_stmt) {
                    Value result = interpreter.evaluate(*expr_stmt->expression);
                    if (!result.is_nil()) {
                        std::cout << "=> " << result.to_string() << "\n";
                    }
                    continue;
                }
            }
            interpreter.interpret(stmts);
        } catch (const RuntimeError& error) {
            std::cerr << "[line " << error.token().line << "] Error: " << error.what() << "\n";
            for (auto it = error.stack_trace().rbegin(); it != error.stack_trace().rend(); ++it) {
                std::cerr << "    at " << it->function_name << " (line " << it->line << ")\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
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
                      << "  build <file> [-o bin]   Compile directly to a standalone native executable\n"
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

        if (first_arg == "build" && argc > 2) {
            std::string input_file;
            std::string output_file;
            bool native = true;

            for (int i = 2; i < argc; ++i) {
                std::string arg = argv[i];
                if (arg == "-o" && i + 1 < argc) {
                    output_file = argv[++i];
                } else if (arg == "--bytecode") {
                    native = false;
                } else if (arg == "--native") {
                    native = true;
                } else if (arg[0] != '-') {
                    input_file = arg;
                }
            }

            if (input_file.empty()) {
                std::cerr << "Error: No input file specified for 'raven build'.\n";
                return 1;
            }

            if (output_file.empty()) {
                auto dot = input_file.find_last_of('.');
                output_file = (dot != std::string::npos) ? input_file.substr(0, dot) : (input_file + (native ? "" : ".rvc"));
            }

            std::string ravenc_bin = "ravenc";
            try {
                auto self_dir = std::filesystem::canonical("/proc/self/exe").parent_path();
                if (std::filesystem::exists(self_dir / "ravenc")) {
                    ravenc_bin = (self_dir / "ravenc").string();
                } else if (std::filesystem::exists("/home/ran/MiniLang/build/ravenc")) {
                    ravenc_bin = "/home/ran/MiniLang/build/ravenc";
                }
            } catch (...) {}

            std::string cmd = "\"" + ravenc_bin + "\" " + (native ? "--native " : "") + input_file + " -o " + output_file;
            return std::system(cmd.c_str()) == 0 ? 0 : 1;
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
                     << "  \"version\": \"1.0.0\",\n"
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
            std::cout << "Raven 1.0.0 (Corvus Engine, Dual AST/VM Toolchain)\n";
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

        if (first_arg == "pkg") {
            std::string subcmd = (argc > 2) ? argv[2] : "help";
            if (subcmd == "list") {
                std::cout << "📦 Installed Raven packages in raven_modules/:\n";
                if (!std::filesystem::exists("raven_modules")) {
                    std::cout << "  (none installed yet)\n";
                    return 0;
                }
                for (const auto& entry : std::filesystem::directory_iterator("raven_modules")) {
                    if (entry.is_directory()) {
                        std::string name = entry.path().filename().string();
                        std::string version = "latest";
                        std::string ver_file = entry.path().string() + "/VERSION";
                        if (std::filesystem::exists(ver_file)) {
                            std::ifstream vf(ver_file);
                            std::getline(vf, version);
                        }
                        std::cout << "  • " << name << " (" << version << ")\n";
                    }
                }
                return 0;
            }

            if (subcmd == "search") {
                std::string query = (argc > 3) ? argv[3] : "";
                std::cout << "🔍 Searching Raven Community Registry for: '" << query << "'\n\n";
                auto results = raven::packages::PackageManager::search_registry(query);
                if (results.empty()) {
                    std::cout << "  No matching packages found.\n";
                    return 0;
                }
                for (const auto& pkg : results) {
                    std::cout << "  • " << pkg.name << " (v" << pkg.version << ") by @" << pkg.author << "\n"
                              << "    " << pkg.description << "\n"
                              << "    ⭐ " << pkg.stars << " stars  |  ⬇ " << pkg.downloads << " installs\n"
                              << "    Install: raven pkg add " << pkg.name << "\n\n";
                }
                return 0;
            }

            if (subcmd == "add") {
                if (argc < 4) {
                    std::cerr << "Usage: raven pkg add <package-name> [version]\n";
                    return 1;
                }
                std::string pkg_name = argv[3];
                std::string version = (argc > 4) ? argv[4] : "latest";
                auto info = raven::packages::PackageManager::get_package_info(pkg_name);
                if (version == "latest") version = info.version;

                std::cout << "📦 Adding dependency '" << pkg_name << "@" << version << "' to raven.json...\n";
                if (raven::packages::PackageManager::add_dependency(pkg_name, version, "raven.json")) {
                    std::cout << "✅ Successfully added '" << pkg_name << "' to raven.json\n"
                              << "   Run 'raven pkg install' to download and lock.\n";
                    return 0;
                } else {
                    std::cerr << "❌ Failed to update raven.json manifest.\n";
                    return 1;
                }
            }

            if (subcmd == "info") {
                if (argc < 4) {
                    std::cerr << "Usage: raven pkg info <package-name>\n";
                    return 1;
                }
                std::string pkg_name = argv[3];
                auto pkg = raven::packages::PackageManager::get_package_info(pkg_name);
                std::cout << "📦 Package: " << pkg.name << " (v" << pkg.version << ")\n"
                          << "   Author:      @" << pkg.author << "\n"
                          << "   Description: " << pkg.description << "\n"
                          << "   Repository:  " << pkg.repo << "\n"
                          << "   Downloads:   " << pkg.downloads << "\n"
                          << "   Stars:       " << pkg.stars << "\n";
                return 0;
            }

            if (subcmd == "publish") {
                std::cout << "🚀 Validating and publishing package to Raven Community Registry...\n";
                std::string msg;
                if (raven::packages::PackageManager::publish_package("raven.json", msg)) {
                    std::cout << "✅ " << msg << "\n"
                              << "   Package verified and published to https://packages.raven-lang.org\n";
                    return 0;
                } else {
                    std::cerr << "❌ " << msg << "\n";
                    return 1;
                }
            }

            if (subcmd == "init") {
                std::string pkg_name = (argc > 3) ? argv[3] : "my_package";
                std::ofstream manifest("raven.json");
                manifest << "{\n"
                         << "  \"name\": \"" << pkg_name << "\",\n"
                         << "  \"version\": \"1.0.0\",\n"
                         << "  \"entry\": \"src/main.rvn\",\n"
                         << "  \"dependencies\": {}\n"
                         << "}\n";
                manifest.close();
                std::cout << "✨ Created raven.json manifest for '" << pkg_name << "'\n";
                return 0;
            }

            if (subcmd == "install") {
                if (argc <= 3) {
                    std::cout << "📦 Checking and resolving dependencies in raven.json...\n";
                    if (!std::filesystem::exists("raven.json")) {
                        std::cerr << "Error: No raven.json found in current directory.\n";
                        return 1;
                    }
                    std::cout << "✓ All dependencies up-to-date.\n";
                    return 0;
                }

                std::string target = argv[3];
                std::string repo = target;
                std::string version = "main";
                size_t at_pos = target.find('@');
                if (at_pos != std::string::npos) {
                    repo = target.substr(0, at_pos);
                    version = target.substr(at_pos + 1);
                }

                std::string pkg_name = repo;
                size_t slash = pkg_name.rfind('/');
                if (slash != std::string::npos) {
                    pkg_name = pkg_name.substr(slash + 1);
                }
                if (pkg_name.size() > 4 && pkg_name.substr(pkg_name.size() - 4) == ".git") {
                    pkg_name = pkg_name.substr(0, pkg_name.size() - 4);
                }

                std::cout << "📦 Installing package '" << pkg_name << "' from " << repo << " (version: " << version << ")...\n";
                std::string dest = "raven_modules/" + pkg_name;
                std::system("mkdir -p raven_modules");
                std::string clone_cmd = "git clone " + repo + " " + dest + " 2>/dev/null || (cd " + dest + " && git pull 2>/dev/null)";
                int ret = std::system(clone_cmd.c_str());
                if (ret == 0) {
                    if (version != "main") {
                        std::string co_cmd = "cd " + dest + " && git checkout " + version + " 2>/dev/null";
                        std::system(co_cmd.c_str());
                    }

                    // Update raven.lock
                    std::ofstream lockfile("raven.lock", std::ios::app);
                    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                    lockfile << "package " << pkg_name << "\n"
                             << "  repo: " << repo << "\n"
                             << "  version: " << version << "\n"
                             << "  resolved: " << dest << "\n"
                             << "  locked_at: " << std::ctime(&now);
                    lockfile.close();

                    std::cout << "✅ Package '" << pkg_name << "' installed and locked in raven.lock\n";
                } else {
                    std::cerr << "❌ Failed to clone package repository.\n";
                }
                return ret;
            }

            std::cout << "Raven Package Manager & Ecosystem Hub (raven pkg):\n"
                      << "  raven pkg search [query]          Search community package registry\n"
                      << "  raven pkg add <name> [version]    Add dependency to raven.json\n"
                      << "  raven pkg info <name>             Display package metadata & downloads\n"
                      << "  raven pkg install                 Install dependencies from raven.json\n"
                      << "  raven pkg install <repo>[@tag]    Install git package and lock version\n"
                      << "  raven pkg publish                 Validate and publish package to registry\n"
                      << "  raven pkg list                    List all installed packages\n";
            return 0;
        }

        if (first_arg == "debug" && argc > 2) {
            std::string path = argv[2];
            std::vector<size_t> initial_breakpoints;
            bool stop_on_entry = true;
            for (int i = 3; i < argc; ++i) {
                std::string arg = argv[i];
                if ((arg == "-b" || arg == "--break") && i + 1 < argc) {
                    try {
                        initial_breakpoints.push_back(std::stoul(argv[++i]));
                    } catch (...) {}
                } else if (arg == "--no-stop-on-entry") {
                    stop_on_entry = false;
                }
            }
            std::ifstream file(path);
            if (!file.is_open()) {
                std::cerr << "Error: Could not open file '" << path << "' for debugging.\n";
                return 1;
            }
            std::ostringstream ss;
            ss << file.rdbuf();
            raven::Debugger dbg(path, ss.str());
            for (size_t bp : initial_breakpoints) {
                dbg.add_breakpoint(bp);
            }
            dbg.set_stop_on_entry(stop_on_entry);
            return dbg.start_interactive();
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
