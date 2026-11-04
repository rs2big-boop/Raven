#include "raven/lsp.hpp"
#include "raven/lexer.hpp"
#include "raven/parser.hpp"
#include "raven/linter.hpp"
#include "raven/formatter.hpp"
#include "raven/doc_generator.hpp"
#include "raven/environment.hpp"

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace raven {

static std::string get_hover_documentation(const std::string& symbol) {
    static const std::unordered_map<std::string, std::string> docs = {
        {"class", "### `class` Keyword\\nDefines an object-oriented class with fields, overloaded constructors, and methods.\\n\\n```raven\\nclass Node {\\n    data\\n    next\\n    Node(data) { this.data = data; }\\n}\\n```"},
        {"this", "### `this` Keyword\\nImplicit reference to the active object instance within a class or method.\\n\\n```raven\\nthis.data = value;\\n```"},
        {"let", "### `let` Keyword\\nDeclares a mutable or immutable variable scoped to the current block.\\n\\n```raven\\nlet x = 42;\\n```"},
        {"fn", "### `fn` Keyword\\nDeclares a named function or lambda closure.\\n\\n```raven\\nfn add(a, b) { return a + b; }\\n```"},
        {"struct", "### `struct` Keyword\\nDeclares a lightweight structured data record.\\n\\n```raven\\nstruct Point { x, y };\\n```"},
        {"match", "### `match` Expression\\nEvaluates pattern matching branches against a target value.\\n\\n```raven\\nlet res = match (status) { 200 => \"OK\", _ => \"Error\" };\\n```"},
        {"is", "### `is` Operator\\nRuntime type inspection operator (`expr is Type`).\\n\\n```raven\\nif (val is Number) { ... }\\n```"},
        {"loop", "### `loop` Statement\\nInfinite execution loop, equivalent to `while (true)`.\\n\\n```raven\\nloop { if (done) break; }\\n```"},
        {"spawn", "### `spawn(callable)` Builtin\\nSpawns a native OS thread executing concurrently.\\n\\n```raven\\nspawn(fn() { ... });\\n```"},
        {"map", "### `map(array, transform_fn)` Builtin\\nMaps each element of an array to a new transformed array.\\n\\n```raven\\nlet sq = map([1, 2, 3], x => x * x);\\n```"},
        {"filter", "### `filter(array, predicate_fn)` Builtin\\nFilters elements satisfying the predicate function.\\n\\n```raven\\nlet evens = filter(arr, x => x % 2 == 0);\\n```"},
        {"reduce", "### `reduce(array, fn, initial)` Builtin\\nFolds an array from left to right using accumulator.\\n\\n```raven\\nlet sum = reduce([1, 2, 3], (acc, x) => acc + x, 0);\\n```"},
        {"print", "### `print(value)` Builtin\\nPrints string representation of value to standard output.\\n\\n```raven\\nprint(\"Hello, Raven!\");\\n```"},
        {"len", "### `len(container)` Builtin\\nReturns number of elements in array, characters in string, or keys in map.\\n\\n```raven\\nlet n = len([1, 2, 3]); // 3\\n```"},
        {"to_json", "### `to_json(value)` Builtin\\nEncodes native Raven value into JSON string.\\n\\n```raven\\nlet s = to_json({\"name\": \"Raven\"});\\n```"},
        {"parse_json", "### `parse_json(string)` Builtin\\nParses JSON string into Raven maps, arrays, numbers, and strings.\\n\\n```raven\\nlet obj = parse_json(\"{\\\"a\\\": 1}\");\\n```"},
        {"read_file", "### `read_file(path)` Builtin\\nReads entire file at path into a string.\\n\\n```raven\\nlet content = read_file(\"data.txt\");\\n```"},
        {"write_file", "### `write_file(path, content)` Builtin\\nWrites content string to file at path.\\n\\n```raven\\nwrite_file(\"data.txt\", \"hello\");\\n```"}
    };
    auto it = docs.find(symbol);
    if (it != docs.end()) return it->second;
    return "";
}

static std::string extract_word_at_pos(const std::string& text, size_t target_line, size_t target_char) {
    std::istringstream stream(text);
    std::string line;
    size_t cur_line = 0;
    while (std::getline(stream, line)) {
        if (cur_line == target_line) {
            if (target_char >= line.size()) target_char = line.empty() ? 0 : line.size() - 1;
            if (line.empty()) return "";
            size_t start = target_char;
            while (start > 0 && (isalnum(line[start - 1]) || line[start - 1] == '_')) start--;
            size_t end = target_char;
            while (end < line.size() && (isalnum(line[end]) || line[end] == '_')) end++;
            return line.substr(start, end - start);
        }
        cur_line++;
    }
    return "";
}

static void send_rpc(std::ostream& out, const std::string& body) {
    out << "Content-Length: " << body.size() << "\r\n\r\n" << body;
    out.flush();
}

static std::string read_rpc(std::istream& in) {
    std::string header;
    size_t content_length = 0;

    while (std::getline(in, header)) {
        if (!header.empty() && header.back() == '\r') {
            header.pop_back();
        }
        if (header.empty()) {
            break;
        }
        if (header.rfind("Content-Length: ", 0) == 0) {
            content_length = std::stoul(header.substr(16));
        }
    }

    if (content_length == 0) return "";

    std::vector<char> buf(content_length);
    in.read(buf.data(), content_length);
    return std::string(buf.data(), content_length);
}

static std::string extract_json_field(const std::string& json, const std::string& field) {
    std::string target = "\"" + field + "\":";
    size_t pos = json.find(target);
    if (pos == std::string::npos) return "";
    pos += target.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    if (pos >= json.size()) return "";

    if (json[pos] == '"') {
        size_t end = json.find('"', pos + 1);
        if (end == std::string::npos) return "";
        return json.substr(pos + 1, end - pos - 1);
    } else {
        size_t end = json.find_first_of(",}\n\r", pos);
        if (end == std::string::npos) end = json.size();
        return json.substr(pos, end - pos);
    }
}

void LanguageServer::run(std::istream& in, std::ostream& out) {
    std::unordered_map<std::string, std::string> documents;

    while (in.good()) {
        std::string req = read_rpc(in);
        if (req.empty()) break;

        std::string method = extract_json_field(req, "method");
        std::string id = extract_json_field(req, "id");

        if (method == "initialize") {
            std::string resp =
                "{\"jsonrpc\":\"2.0\",\"id\":" + (id.empty() ? "1" : id) + ","
                "\"result\":{"
                "\"capabilities\":{"
                "\"textDocumentSync\":1,"
                "\"hoverProvider\":true,"
                "\"completionProvider\":{\"triggerCharacters\":[\".\",\":\"]},"
                "\"documentFormattingProvider\":true,"
                "\"documentSymbolProvider\":true,"
                "\"codeActionProvider\":true"
                "}}}";
            send_rpc(out, resp);
        } else if (method == "textDocument/didOpen" || method == "textDocument/didChange") {
            // Find uri and text
            std::string uri = extract_json_field(req, "uri");
            size_t text_pos = req.find("\"text\":");
            std::string doc_text;
            if (text_pos != std::string::npos) {
                text_pos += 7;
                while (text_pos < req.size() && req[text_pos] == ' ') text_pos++;
                if (text_pos < req.size() && req[text_pos] == '"') {
                    size_t text_end = req.rfind('"');
                    if (text_end > text_pos) {
                        doc_text = req.substr(text_pos + 1, text_end - text_pos - 1);
                    }
                }
            }

            if (!uri.empty() && !doc_text.empty()) {
                documents[uri] = doc_text;

                // Validate syntax and lint
                Lexer lexer(doc_text);
                auto tokens = lexer.tokenize();
                Parser parser(tokens);
                auto stmts = parser.parse();

                std::ostringstream diag_json;
                diag_json << "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\",\"params\":{"
                          << "\"uri\":\"" << uri << "\",\"diagnostics\":[";

                bool first = true;
                if (parser.has_error()) {
                    for (const auto& err : parser.errors()) {
                        if (!first) diag_json << ",";
                        first = false;
                        diag_json << "{\"range\":{\"start\":{\"line\":0,\"character\":0},"
                                  << "\"end\":{\"line\":0,\"character\":10}},"
                                  << "\"severity\":1,\"message\":\"" << err << "\"}";
                    }
                }

                Linter linter;
                auto issues = linter.analyze(stmts);
                for (const auto& issue : issues) {
                    if (!first) diag_json << ",";
                    first = false;
                    int l = std::max(0, issue.line - 1);
                    int c = std::max(0, issue.column - 1);
                    int sev = (issue.severity == LintSeverity::Error) ? 1 : 2;
                    diag_json << "{\"range\":{\"start\":{\"line\":" << l << ",\"character\":" << c << "},"
                              << "\"end\":{\"line\":" << l << ",\"character\":" << (c + 5) << "}},"
                              << "\"severity\":" << sev << ",\"message\":\"" << issue.message << "\"}";
                }

                diag_json << "]}}";
                send_rpc(out, diag_json.str());
            }
        } else if (method == "textDocument/completion") {
            std::string resp =
                "{\"jsonrpc\":\"2.0\",\"id\":" + (id.empty() ? "1" : id) + ","
                "\"result\":["
                "{\"label\":\"class\",\"kind\":7,\"detail\":\"class Name { fields, constructor, methods }\"},"
                "{\"label\":\"this\",\"kind\":6,\"detail\":\"Reference current class instance\"},"
                "{\"label\":\"fn\",\"kind\":14,\"detail\":\"Define a function\"},"
                "{\"label\":\"let\",\"kind\":14,\"detail\":\"Declare a variable\"},"
                "{\"label\":\"struct\",\"kind\":14,\"detail\":\"Define a struct\"},"
                "{\"label\":\"match\",\"kind\":14,\"detail\":\"Pattern matching\"},"
                "{\"label\":\"is\",\"kind\":15,\"detail\":\"Type checking operator (x is Number)\"},"
                "{\"label\":\"loop\",\"kind\":15,\"detail\":\"Infinite loop\"},"
                "{\"label\":\"print\",\"kind\":3,\"detail\":\"fn print(value)\"},"
                "{\"label\":\"len\",\"kind\":3,\"detail\":\"fn len(container)\"},"
                "{\"label\":\"map\",\"kind\":3,\"detail\":\"fn map(array, fn) -> array\"},"
                "{\"label\":\"filter\",\"kind\":3,\"detail\":\"fn filter(array, fn) -> array\"},"
                "{\"label\":\"reduce\",\"kind\":3,\"detail\":\"fn reduce(array, fn, init) -> value\"},"
                "{\"label\":\"keys\",\"kind\":3,\"detail\":\"fn keys(map) -> array\"},"
                "{\"label\":\"values\",\"kind\":3,\"detail\":\"fn values(map) -> array\"},"
                "{\"label\":\"spawn\",\"kind\":3,\"detail\":\"fn spawn(fn) -> thread\"},"
                "{\"label\":\"chan_create\",\"kind\":3,\"detail\":\"fn chan_create() -> channel\"},"
                "{\"label\":\"abs\",\"kind\":3,\"detail\":\"fn abs(number)\"},"
                "{\"label\":\"sort_arr\",\"kind\":3,\"detail\":\"fn sort_arr(array)\"},"
                "{\"label\":\"format\",\"kind\":3,\"detail\":\"fn format(template, ...args)\"},"
                "{\"label\":\"to_json\",\"kind\":3,\"detail\":\"fn to_json(value)\"},"
                "{\"label\":\"parse_json\",\"kind\":3,\"detail\":\"fn parse_json(json_str)\"},"
                "{\"label\":\"read_file\",\"kind\":3,\"detail\":\"fn read_file(path) -> string\"},"
                "{\"label\":\"write_file\",\"kind\":3,\"detail\":\"fn write_file(path, content) -> bool\"},"
                "{\"label\":\"assert\",\"kind\":3,\"detail\":\"fn assert(cond, msg)\"},"
                "{\"label\":\"panic\",\"kind\":3,\"detail\":\"fn panic(msg)\"},"
                "{\"label\":\"unique\",\"kind\":3,\"detail\":\"fn unique(array)\"},"
                "{\"label\":\"slice\",\"kind\":3,\"detail\":\"fn slice(sequence, start, end)\"},"
                "{\"label\":\"regex_match\",\"kind\":3,\"detail\":\"fn regex_match(str, pattern)\"},"
                "{\"label\":\"http_get\",\"kind\":3,\"detail\":\"fn http_get(url)\"}"
                "]}";
            send_rpc(out, resp);
        } else if (method == "textDocument/formatting") {
            std::string uri = extract_json_field(req, "uri");
            std::string formatted = "";
            if (documents.find(uri) != documents.end()) {
                formatted = Formatter::format(documents[uri]);
            }

            // Escape formatted string for JSON
            std::ostringstream esc;
            for (char ch : formatted) {
                if (ch == '"') esc << "\\\"";
                else if (ch == '\\') esc << "\\\\";
                else if (ch == '\n') esc << "\\n";
                else if (ch == '\r') esc << "\\r";
                else if (ch == '\t') esc << "\\t";
                else esc << ch;
            }

            std::string resp =
                "{\"jsonrpc\":\"2.0\",\"id\":" + (id.empty() ? "1" : id) + ","
                "\"result\":[{\"range\":{\"start\":{\"line\":0,\"character\":0},"
                "\"end\":{\"line\":99999,\"character\":0}},\"newText\":\"" + esc.str() + "\"}]}";
            send_rpc(out, resp);
        } else if (method == "textDocument/hover") {
            std::string uri = extract_json_field(req, "uri");
            std::string line_str = extract_json_field(req, "line");
            std::string char_str = extract_json_field(req, "character");
            size_t target_line = line_str.empty() ? 0 : std::stoul(line_str);
            size_t target_char = char_str.empty() ? 0 : std::stoul(char_str);

            std::string hover_text = "**Raven Language Server**\\n\\nType-inferred expression";
            if (documents.find(uri) != documents.end()) {
                std::string word = extract_word_at_pos(documents[uri], target_line, target_char);
                if (!word.empty()) {
                    std::string doc = get_hover_documentation(word);
                    if (!doc.empty()) {
                        hover_text = doc;
                    } else {
                        hover_text = "### `" + word + "`\\n\\nRaven identifier";
                    }
                }
            }

            std::string resp =
                "{\"jsonrpc\":\"2.0\",\"id\":" + (id.empty() ? "1" : id) + ","
                "\"result\":{\"contents\":{\"kind\":\"markdown\",\"value\":\"" + hover_text + "\"}}}";
            send_rpc(out, resp);
        } else if (method == "textDocument/codeAction") {
            std::string uri = extract_json_field(req, "uri");
            static const std::vector<std::string> known_symbols = {
                "class", "this", "print", "len", "abs", "sort_arr", "format", "to_json", "parse_json",
                "map", "filter", "reduce", "keys", "values", "is",
                "unique", "slice", "regex_match", "http_get", "spawn", "chan_create",
                "chan_send", "chan_recv", "chan_close", "let", "fn", "struct", "match",
                "return", "if", "else", "while", "for", "import", "loop", "true", "false"
            };

            std::string word = "";
            size_t undef_pos = req.find("Undefined variable '");
            if (undef_pos != std::string::npos) {
                undef_pos += 20;
                size_t end_quote = req.find("'", undef_pos);
                if (end_quote != std::string::npos) {
                    word = req.substr(undef_pos, end_quote - undef_pos);
                }
            }

            std::ostringstream actions_json;
            actions_json << "{\"jsonrpc\":\"2.0\",\"id\":" << (id.empty() ? "1" : id) << ",\"result\":[";

            if (!word.empty()) {
                std::string best_match;
                int best_dist = 4;
                for (const auto& sym : known_symbols) {
                    int dist = levenshtein_distance(word, sym);
                    if (dist < best_dist) {
                        best_dist = dist;
                        best_match = sym;
                    }
                }

                if (!best_match.empty()) {
                    actions_json << "{"
                                 << "\"title\":\"Change '" << word << "' to '" << best_match << "'\","
                                 << "\"kind\":\"quickfix\","
                                 << "\"isPreferred\":true,"
                                 << "\"edit\":{"
                                 << "\"changes\":{\"" << uri << "\":[{"
                                 << "\"range\":{\"start\":{\"line\":0,\"character\":0},\"end\":{\"line\":0,\"character\":" << word.size() << "}},"
                                 << "\"newText\":\"" << best_match << "\""
                                 << "}]}}"
                                 << "}";
                }
            }

            actions_json << "]}";
            send_rpc(out, actions_json.str());
        } else if (method == "shutdown") {
            send_rpc(out, "{\"jsonrpc\":\"2.0\",\"id\":" + (id.empty() ? "1" : id) + ",\"result\":null}");
        } else if (method == "exit") {
            break;
        }
    }
}

} // namespace raven
