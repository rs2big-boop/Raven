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
                "{\"label\":\"fn\",\"kind\":14,\"detail\":\"Define a function\"},"
                "{\"label\":\"let\",\"kind\":14,\"detail\":\"Declare a variable\"},"
                "{\"label\":\"struct\",\"kind\":14,\"detail\":\"Define a struct\"},"
                "{\"label\":\"match\",\"kind\":14,\"detail\":\"Pattern matching\"},"
                "{\"label\":\"print\",\"kind\":3,\"detail\":\"fn print(value)\"},"
                "{\"label\":\"len\",\"kind\":3,\"detail\":\"fn len(container)\"},"
                "{\"label\":\"abs\",\"kind\":3,\"detail\":\"fn abs(number)\"},"
                "{\"label\":\"sort_arr\",\"kind\":3,\"detail\":\"fn sort_arr(array)\"},"
                "{\"label\":\"format\",\"kind\":3,\"detail\":\"fn format(template, ...args)\"},"
                "{\"label\":\"to_json\",\"kind\":3,\"detail\":\"fn to_json(value)\"},"
                "{\"label\":\"parse_json\",\"kind\":3,\"detail\":\"fn parse_json(json_str)\"},"
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
            std::string resp =
                "{\"jsonrpc\":\"2.0\",\"id\":" + (id.empty() ? "1" : id) + ","
                "\"result\":{\"contents\":{\"kind\":\"markdown\",\"value\":\"**Raven Language Server**\\n\\nType-inferred expression\"}}}";
            send_rpc(out, resp);
        } else if (method == "textDocument/codeAction") {
            std::string uri = extract_json_field(req, "uri");
            static const std::vector<std::string> known_symbols = {
                "print", "len", "abs", "sort_arr", "format", "to_json", "parse_json",
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
