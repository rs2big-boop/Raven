// Raven DAP (Debug Adapter Protocol) Server
// Implements the full DAP spec over stdio for VS Code integration.
// Transport: Content-Length: N\r\n\r\n{json}

#include "raven/dap.hpp"
#include "raven/interpreter.hpp"
#include "raven/lexer.hpp"
#include "raven/parser.hpp"
#include "raven/environment.hpp"
#include "raven/debugger.hpp"  // for extract_stmt_line

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

namespace raven {

// ---------------------------------------------------------------------------
// JSON helpers
// ---------------------------------------------------------------------------

static std::string json_escape(const std::string& s) {
    std::string r;
    r.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  r += "\\\""; break;
            case '\\': r += "\\\\"; break;
            case '\n': r += "\\n";  break;
            case '\r': r += "\\r";  break;
            case '\t': r += "\\t";  break;
            case '\b': r += "\\b";  break;
            case '\f': r += "\\f";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    r += buf;
                } else {
                    r += static_cast<char>(c);
                }
        }
    }
    return r;
}

// Find closing quote respecting escape sequences; open_quote points at '"'
static size_t find_close_quote(const std::string& s, size_t open_quote) {
    size_t i = open_quote + 1;
    while (i < s.size()) {
        if (s[i] == '\\') { i += 2; }
        else if (s[i] == '"') { return i; }
        else { ++i; }
    }
    return std::string::npos;
}

// Extract a string value for a given JSON key (non-nested)
static std::string get_dap_str(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t')) pos++;
    if (pos >= json.size()) return "";
    if (json[pos] != '"') return "";
    size_t close = find_close_quote(json, pos);
    if (close == std::string::npos) return "";
    // Unescape
    std::string raw = json.substr(pos + 1, close - pos - 1);
    std::string result;
    result.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\\' && i + 1 < raw.size()) {
            ++i;
            switch (raw[i]) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                default:   result += raw[i]; break;
            }
        } else {
            result += raw[i];
        }
    }
    return result;
}

static int get_dap_int(const std::string& json, const std::string& key, int def = 0) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return def;
    pos += search.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t')) pos++;
    if (pos >= json.size()) return def;
    if (json[pos] == '-' || std::isdigit(static_cast<unsigned char>(json[pos]))) {
        int sign = 1;
        if (json[pos] == '-') { sign = -1; pos++; }
        int val = 0;
        while (pos < json.size() && std::isdigit(static_cast<unsigned char>(json[pos]))) {
            val = val * 10 + (json[pos] - '0');
            pos++;
        }
        return sign * val;
    }
    return def;
}

static bool get_dap_bool(const std::string& json, const std::string& key, bool def = false) {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) {
        // try with space: "key": true
        search = "\"" + key + "\": ";
        pos = json.find(search);
        if (pos == std::string::npos) return def;
        pos += search.size();
    } else {
        pos += search.size();
    }
    while (pos < json.size() && json[pos] == ' ') pos++;
    if (pos + 4 <= json.size() && json.substr(pos, 4) == "true") return true;
    if (pos + 5 <= json.size() && json.substr(pos, 5) == "false") return false;
    return def;
}

// Extract a string value from a nested object: "outerKey":{"innerKey":"value"}
static std::string get_dap_nested_str(const std::string& json,
                                       const std::string& outer,
                                       const std::string& inner) {
    std::string search = "\"" + outer + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t')) pos++;
    if (pos >= json.size() || json[pos] != '{') return "";
    size_t depth = 1;
    size_t block_start = pos;
    pos++;
    while (pos < json.size() && depth > 0) {
        if (json[pos] == '{') depth++;
        else if (json[pos] == '}') depth--;
        else if (json[pos] == '"') {
            size_t cl = find_close_quote(json, pos);
            if (cl != std::string::npos) pos = cl;
        }
        if (depth > 0) pos++;
    }
    std::string block = json.substr(block_start, pos - block_start + 1);
    return get_dap_str(block, inner);
}

// ---------------------------------------------------------------------------
// Transport
// ---------------------------------------------------------------------------

static std::string read_dap_message(std::istream& in) {
    std::string header;
    size_t content_length = 0;

    while (std::getline(in, header)) {
        if (!header.empty() && header.back() == '\r') header.pop_back();
        if (header.empty()) break;
        if (header.rfind("Content-Length: ", 0) == 0) {
            try { content_length = std::stoul(header.substr(16)); } catch (...) {}
        }
    }

    if (content_length == 0 || !in.good()) return "";

    std::vector<char> buf(content_length);
    in.read(buf.data(), static_cast<std::streamsize>(content_length));
    if (static_cast<size_t>(in.gcount()) != content_length) return "";
    return std::string(buf.data(), content_length);
}

static void send_dap_message(std::ostream& out, const std::string& body) {
    out << "Content-Length: " << body.size() << "\r\n\r\n" << body;
    out.flush();
}

// ---------------------------------------------------------------------------
// DapServer class
// ---------------------------------------------------------------------------

struct DapBreakpoint {
    std::string source_path;
    int line = 0;
    std::string condition;
    bool verified = true;
};

struct DapVariable {
    std::string name;
    std::string value;
    std::string type;
    int variablesReference = 0;
};

class DapServer {
public:
    DapServer(std::istream& in, std::ostream& out, std::string file)
        : in_(in), out_(out), source_file_(std::move(file)) {}

    void run() {
        while (!disconnected_ && in_.good()) {
            std::string msg = read_dap_message(in_);
            if (msg.empty()) break;
            handle_message(msg);
        }
    }

private:
    std::istream& in_;
    std::ostream& out_;
    std::string source_file_;
    std::string source_;

    std::vector<DapBreakpoint> breakpoints_;
    std::unordered_map<int, Value> ref_map_;
    int next_ref_id_ = 1000;
    int event_seq_ = 100;

    // Interpreter (created on launch)
    std::unique_ptr<Interpreter> interp_;

    // Current execution position
    size_t current_line_ = 0;
    std::string current_func_ = "<main>";

    // Call depth tracking for step-over/step-out
    size_t call_depth_ = 0;

    // Step mode
    enum class StepMode { None, Over, In, Out };
    StepMode step_mode_ = StepMode::None;
    size_t step_start_depth_ = 0;

    // Control flags
    bool paused_       = false;
    bool stop_on_entry_= true;
    bool started_      = false;
    bool disconnected_ = false;
    bool launched_     = false;
    bool program_done_ = false;

    // --- Sending helpers ---

    void send_response(int req_seq, const std::string& command, const std::string& body,
                       bool success = true) {
        std::string msg = "{\"type\":\"response\","
                          "\"request_seq\":" + std::to_string(req_seq) + ","
                          "\"success\":" + (success ? "true" : "false") + ","
                          "\"command\":\"" + json_escape(command) + "\"";
        if (!body.empty() && body != "{}") {
            msg += ",\"body\":" + body;
        } else {
            msg += ",\"body\":{}";
        }
        msg += "}";
        send_dap_message(out_, msg);
    }

    void send_error_response(int req_seq, const std::string& command, const std::string& msg_text) {
        std::string msg = "{\"type\":\"response\","
                          "\"request_seq\":" + std::to_string(req_seq) + ","
                          "\"success\":false,"
                          "\"command\":\"" + json_escape(command) + "\","
                          "\"message\":\"" + json_escape(msg_text) + "\","
                          "\"body\":{}}";
        send_dap_message(out_, msg);
    }

    void send_event(const std::string& event_name, const std::string& body) {
        std::string msg = "{\"type\":\"event\","
                          "\"seq\":" + std::to_string(event_seq_++) + ","
                          "\"event\":\"" + json_escape(event_name) + "\"";
        if (!body.empty() && body != "{}") {
            msg += ",\"body\":" + body;
        } else {
            msg += ",\"body\":{}";
        }
        msg += "}";
        send_dap_message(out_, msg);
    }

    void send_stopped_event(const std::string& reason) {
        send_event("stopped",
            "{\"reason\":\"" + reason + "\","
            "\"threadId\":1,"
            "\"allThreadsStopped\":true}");
    }

    void send_output_event(const std::string& text, bool is_stderr = false) {
        send_event("output",
            "{\"category\":\"" + std::string(is_stderr ? "stderr" : "stdout") + "\","
            "\"output\":\"" + json_escape(text) + "\"}");
    }

    // --- Message dispatch ---

    void handle_message(const std::string& msg) {
        std::string type = get_dap_str(msg, "type");
        if (type != "request") return;

        std::string cmd = get_dap_str(msg, "command");
        int seq = get_dap_int(msg, "seq");

        if      (cmd == "initialize")              handle_initialize(seq, msg);
        else if (cmd == "launch")                  handle_launch(seq, msg);
        else if (cmd == "setBreakpoints")          handle_set_breakpoints(seq, msg);
        else if (cmd == "setExceptionBreakpoints") send_response(seq, cmd, "{}");
        else if (cmd == "setFunctionBreakpoints")  send_response(seq, cmd, "{\"breakpoints\":[]}");
        else if (cmd == "configurationDone")       handle_configuration_done(seq);
        else if (cmd == "threads")                 handle_threads(seq);
        else if (cmd == "stackTrace")              handle_stack_trace(seq, msg);
        else if (cmd == "scopes")                  handle_scopes(seq, msg);
        else if (cmd == "variables")               handle_variables(seq, msg);
        else if (cmd == "continue")                handle_continue(seq);
        else if (cmd == "next")                    handle_next(seq);
        else if (cmd == "stepIn")                  handle_step_in(seq);
        else if (cmd == "stepOut")                 handle_step_out(seq);
        else if (cmd == "pause")                   send_response(seq, cmd, "{}");
        else if (cmd == "evaluate")                handle_evaluate(seq, msg);
        else if (cmd == "setVariable")             handle_set_variable(seq, msg);
        else if (cmd == "terminate")               { disconnected_ = true; send_response(seq, cmd, "{}"); }
        else if (cmd == "disconnect")              { disconnected_ = true; send_response(seq, cmd, "{}"); }
        else                                       send_response(seq, cmd, "{}");
    }

    // --- Individual handlers ---

    void handle_initialize(int seq, const std::string& /*msg*/) {
        send_response(seq, "initialize", R"({
            "supportsConfigurationDoneRequest": true,
            "supportsFunctionBreakpoints": false,
            "supportsConditionalBreakpoints": true,
            "supportsHitConditionalBreakpoints": false,
            "supportsEvaluateForHovers": true,
            "supportsSetVariable": true,
            "supportsStepInTargetsRequest": false,
            "supportsCompletionsRequest": false,
            "supportsRestartRequest": false,
            "supportsExceptionOptions": false,
            "supportsValueFormattingOptions": false,
            "supportsExceptionInfoRequest": false,
            "supportTerminateDebuggee": true,
            "supportsDelayedStackTraceLoading": false,
            "supportsLoadedSourcesRequest": false,
            "supportsLogPoints": false,
            "supportsTerminateThreadsRequest": false,
            "supportsSetExpression": false,
            "supportsTerminateRequest": true,
            "supportsDataBreakpoints": false,
            "supportsReadMemoryRequest": false,
            "supportsDisassembleRequest": false,
            "supportsInstructionBreakpoints": false
        })");
        send_event("initialized", "{}");
    }

    void handle_launch(int seq, const std::string& msg) {
        // Extract program path from arguments
        std::string program = get_dap_nested_str(msg, "arguments", "program");
        if (program.empty()) program = get_dap_str(msg, "program");
        if (!program.empty()) source_file_ = program;

        // stopOnEntry
        // Check in arguments sub-object first, then top-level
        size_t args_start = msg.find("\"arguments\"");
        if (args_start != std::string::npos) {
            size_t brace = msg.find('{', args_start);
            size_t bend  = std::string::npos;
            if (brace != std::string::npos) {
                size_t depth = 1;
                size_t i = brace + 1;
                while (i < msg.size() && depth > 0) {
                    if (msg[i] == '{') depth++;
                    else if (msg[i] == '}') depth--;
                    if (depth > 0) i++;
                }
                bend = i;
                std::string args_block = msg.substr(brace, bend - brace + 1);
                stop_on_entry_ = get_dap_bool(args_block, "stopOnEntry", true);
            }
        } else {
            stop_on_entry_ = get_dap_bool(msg, "stopOnEntry", true);
        }

        // Load source file
        if (!source_file_.empty()) {
            std::ifstream f(source_file_);
            if (!f.is_open()) {
                send_error_response(seq, "launch", "Cannot open file: " + source_file_);
                return;
            }
            source_ = std::string((std::istreambuf_iterator<char>(f)),
                                   std::istreambuf_iterator<char>());
        }

        launched_ = true;
        send_response(seq, "launch", "{}");
    }

    void handle_set_breakpoints(int seq, const std::string& msg) {
        // Extract source path
        std::string src_path = get_dap_nested_str(msg, "source", "path");
        if (src_path.empty()) src_path = get_dap_str(msg, "path");

        // Remove old breakpoints for this source
        breakpoints_.erase(
            std::remove_if(breakpoints_.begin(), breakpoints_.end(),
                [&](const DapBreakpoint& bp) { return bp.source_path == src_path; }),
            breakpoints_.end()
        );

        // Parse breakpoints array
        std::vector<std::pair<int,std::string>> new_bps;
        size_t arr_pos = msg.find("\"breakpoints\":");
        if (arr_pos != std::string::npos) {
            size_t arr_start = msg.find('[', arr_pos);
            if (arr_start != std::string::npos) {
                // Find matching ]
                size_t depth = 1;
                size_t i = arr_start + 1;
                while (i < msg.size() && depth > 0) {
                    if (msg[i] == '[') depth++;
                    else if (msg[i] == ']') depth--;
                    else if (msg[i] == '"') {
                        size_t cl = find_close_quote(msg, i);
                        if (cl != std::string::npos) i = cl;
                    }
                    if (depth > 0) i++;
                }
                std::string arr_str = msg.substr(arr_start, i - arr_start + 1);

                // Parse each {line:N, condition:"..."} object
                size_t pos = 0;
                while (pos < arr_str.size()) {
                    size_t obj_start = arr_str.find('{', pos);
                    if (obj_start == std::string::npos) break;
                    // Find matching }
                    size_t d2 = 1;
                    size_t j = obj_start + 1;
                    while (j < arr_str.size() && d2 > 0) {
                        if (arr_str[j] == '{') d2++;
                        else if (arr_str[j] == '}') d2--;
                        else if (arr_str[j] == '"') {
                            size_t cl = find_close_quote(arr_str, j);
                            if (cl != std::string::npos) j = cl;
                        }
                        if (d2 > 0) j++;
                    }
                    std::string obj = arr_str.substr(obj_start, j - obj_start + 1);
                    int line = get_dap_int(obj, "line");
                    std::string cond = get_dap_str(obj, "condition");
                    if (line > 0) new_bps.push_back({line, cond});
                    pos = j + 1;
                }
            }
        }

        // Build response and store breakpoints
        std::string bp_json = "[";
        for (size_t i = 0; i < new_bps.size(); i++) {
            DapBreakpoint bp;
            bp.source_path = src_path;
            bp.line = new_bps[i].first;
            bp.condition = new_bps[i].second;
            bp.verified = true;
            breakpoints_.push_back(bp);

            if (i > 0) bp_json += ",";
            bp_json += "{\"verified\":true,\"line\":" + std::to_string(bp.line) + "}";
        }
        bp_json += "]";

        send_response(seq, "setBreakpoints", "{\"breakpoints\":" + bp_json + "}");
    }

    void handle_configuration_done(int seq) {
        send_response(seq, "configurationDone", "{}");

        if (!launched_) return;
        if (source_.empty() && !source_file_.empty()) {
            std::ifstream f(source_file_);
            if (!f.is_open()) {
                send_output_event("[Raven] Cannot open file: " + source_file_ + "\n", true);
                send_event("terminated", "{}");
                send_event("exited", "{\"exitCode\":1}");
                disconnected_ = true;
                return;
            }
            source_ = std::string((std::istreambuf_iterator<char>(f)),
                                   std::istreambuf_iterator<char>());
        }

        // Parse
        Lexer lexer(source_);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto statements = parser.parse();

        if (parser.has_error()) {
            for (const auto& err : parser.errors()) {
                send_output_event("[Raven] Parse error: " + err + "\n", true);
            }
            send_event("terminated", "{}");
            send_event("exited", "{\"exitCode\":1}");
            disconnected_ = true;
            return;
        }

        // Create interpreter with output capture
        // We use a custom streambuf that sends output events
        class DapStreambuf : public std::streambuf {
        public:
            DapServer* server;
            std::string buf;
            int overflow(int c) override {
                if (c != EOF) {
                    buf += static_cast<char>(c);
                    if (c == '\n') {
                        server->send_output_event(buf);
                        buf.clear();
                    }
                }
                return c;
            }
            std::streamsize xsputn(const char* s, std::streamsize n) override {
                for (std::streamsize i = 0; i < n; ++i) overflow(s[i]);
                return n;
            }
        };

        DapStreambuf dap_buf;
        dap_buf.server = this;
        std::ostream dap_out(&dap_buf);

        interp_ = std::make_unique<Interpreter>(dap_out);

        // Set step callback — fires before each statement
        interp_->set_step_callback([this](Interpreter& /*interp*/, const Stmt& stmt) {
            on_step(stmt);
        });

        // Handle stopOnEntry
        if (stop_on_entry_ && !started_) {
            // Will pause at first statement via on_step
        }

        // Execute — this may block inside pause_and_wait()
        try {
            for (const auto& stmt : statements) {
                if (disconnected_) break;
                interp_->execute(*stmt);
            }
        } catch (const RuntimeError& err) {
            // Flush any remaining buffered output
            if (!dap_buf.buf.empty()) {
                send_output_event(dap_buf.buf);
                dap_buf.buf.clear();
            }
            std::string msg = "[line " + std::to_string(err.token().line) + "] Runtime Error: " +
                              std::string(err.what()) + "\n";
            for (auto it = err.stack_trace().rbegin(); it != err.stack_trace().rend(); ++it) {
                msg += "    at " + it->function_name + " (line " + std::to_string(it->line) + ")\n";
            }
            send_output_event(msg, true);
        } catch (const std::exception& e) {
            if (!dap_buf.buf.empty()) {
                send_output_event(dap_buf.buf);
                dap_buf.buf.clear();
            }
            send_output_event(std::string("[Raven Fatal Error] ") + e.what() + "\n", true);
        } catch (...) {
            // Ignore other exceptions (break/continue escaping, etc.)
        }

        // Flush any remaining buffered output
        if (!dap_buf.buf.empty()) {
            send_output_event(dap_buf.buf);
        }

        if (!disconnected_) {
            program_done_ = true;
            send_event("terminated", "{}");
            send_event("exited", "{\"exitCode\":0}");
            disconnected_ = true;
        }
    }

    // Called by the step callback before each statement executes
    void on_step(const Stmt& stmt) {
        if (disconnected_) return;

        size_t line = extract_stmt_line(stmt);
        if (line == 0) return;

        current_line_ = line;
        // Determine call depth from call stack
        if (interp_) {
            call_depth_ = interp_->call_stack().size();
            current_func_ = call_depth_ > 0
                ? interp_->call_stack().back().function_name
                : "<main>";
        }

        bool should_pause = false;
        std::string stop_reason;

        if (stop_on_entry_ && !started_) {
            should_pause = true;
            started_ = true;
            stop_reason = "entry";
        } else if (step_mode_ != StepMode::None) {
            switch (step_mode_) {
                case StepMode::In:
                    // Always stop at next statement
                    should_pause = true;
                    stop_reason = "step";
                    step_mode_ = StepMode::None;
                    break;
                case StepMode::Over:
                    // Stop when call depth is <= depth at step-over start
                    if (call_depth_ <= step_start_depth_) {
                        should_pause = true;
                        stop_reason = "step";
                        step_mode_ = StepMode::None;
                    }
                    break;
                case StepMode::Out:
                    // Stop when call depth is < depth at step-out start
                    if (call_depth_ < step_start_depth_) {
                        should_pause = true;
                        stop_reason = "step";
                        step_mode_ = StepMode::None;
                    }
                    break;
                default: break;
            }
        }

        // Check breakpoints (can override step logic to also pause)
        if (!should_pause) {
            for (const auto& bp : breakpoints_) {
                if (bp.line == static_cast<int>(line) &&
                    (bp.source_path.empty() || bp.source_path == source_file_))
                {
                    if (bp.condition.empty() || eval_condition(bp.condition)) {
                        should_pause = true;
                        stop_reason = "breakpoint";
                        break;
                    }
                }
            }
        }

        if (should_pause) {
            // Clear ref map on each pause (variables may have changed)
            ref_map_.clear();
            next_ref_id_ = 1000;
            send_stopped_event(stop_reason);
            pause_and_wait();
        }
    }

    // Block and handle incoming DAP messages until we should resume
    void pause_and_wait() {
        paused_ = true;
        while (paused_ && !disconnected_ && in_.good()) {
            std::string msg = read_dap_message(in_);
            if (msg.empty()) {
                disconnected_ = true;
                break;
            }
            handle_while_paused(msg);
        }
    }

    void handle_while_paused(const std::string& msg) {
        std::string cmd = get_dap_str(msg, "command");
        int seq = get_dap_int(msg, "seq");

        if (cmd == "continue") {
            step_mode_ = StepMode::None;
            paused_ = false;
            send_response(seq, "continue", "{\"allThreadsContinued\":true}");
            send_event("continued", "{\"allThreadsContinued\":true}");
        } else if (cmd == "next") {
            step_mode_ = StepMode::Over;
            step_start_depth_ = call_depth_;
            paused_ = false;
            send_response(seq, "next", "{}");
        } else if (cmd == "stepIn") {
            step_mode_ = StepMode::In;
            paused_ = false;
            send_response(seq, "stepIn", "{}");
        } else if (cmd == "stepOut") {
            step_mode_ = StepMode::Out;
            step_start_depth_ = call_depth_;
            paused_ = false;
            send_response(seq, "stepOut", "{}");
        } else if (cmd == "stackTrace") {
            handle_stack_trace(seq, msg);
        } else if (cmd == "scopes") {
            handle_scopes(seq, msg);
        } else if (cmd == "variables") {
            handle_variables(seq, msg);
        } else if (cmd == "evaluate") {
            handle_evaluate(seq, msg);
        } else if (cmd == "setVariable") {
            handle_set_variable(seq, msg);
        } else if (cmd == "threads") {
            handle_threads(seq);
        } else if (cmd == "setBreakpoints") {
            handle_set_breakpoints(seq, msg);
        } else if (cmd == "setExceptionBreakpoints") {
            send_response(seq, cmd, "{}");
        } else if (cmd == "pause") {
            send_response(seq, "pause", "{}");
        } else if (cmd == "terminate" || cmd == "disconnect") {
            paused_ = false;
            disconnected_ = true;
            send_response(seq, cmd, "{}");
        } else {
            // Unknown during pause — respond with empty success
            send_response(seq, cmd, "{}");
        }
    }

    void handle_threads(int seq) {
        send_response(seq, "threads", "{\"threads\":[{\"id\":1,\"name\":\"main\"}]}");
    }

    void handle_stack_trace(int seq, const std::string& /*msg*/) {
        if (!interp_) {
            send_response(seq, "stackTrace", "{\"stackFrames\":[],\"totalFrames\":0}");
            return;
        }

        const auto& cs = interp_->call_stack();
        std::string frames = "[";

        // Current frame (top)
        frames += "{\"id\":0,\"name\":\"" + json_escape(current_func_) + "\","
                  "\"source\":{\"path\":\"" + json_escape(source_file_) + "\","
                  "\"name\":\"" + json_escape(source_file_) + "\"},"
                  "\"line\":" + std::to_string(current_line_) + ","
                  "\"column\":1}";

        // Call stack frames (innermost = most recent)
        for (int i = static_cast<int>(cs.size()) - 1; i >= 0; --i) {
            const auto& frame = cs[static_cast<size_t>(i)];
            int frame_id = static_cast<int>(cs.size()) - i;
            frames += ",{\"id\":" + std::to_string(frame_id) + ","
                      "\"name\":\"" + json_escape(frame.function_name) + "\","
                      "\"source\":{\"path\":\"" + json_escape(source_file_) + "\","
                      "\"name\":\"" + json_escape(source_file_) + "\"},"
                      "\"line\":" + std::to_string(frame.line) + ","
                      "\"column\":" + std::to_string(frame.column) + "}";
        }

        frames += "]";
        int total = static_cast<int>(cs.size()) + 1;
        send_response(seq, "stackTrace",
            "{\"stackFrames\":" + frames + ",\"totalFrames\":" + std::to_string(total) + "}");
    }

    static constexpr int LOCAL_SCOPE_REF  = 1;
    static constexpr int GLOBAL_SCOPE_REF = 2;

    void handle_scopes(int seq, const std::string& /*msg*/) {
        send_response(seq, "scopes",
            "{\"scopes\":["
              "{\"name\":\"Locals\",\"variablesReference\":" + std::to_string(LOCAL_SCOPE_REF) + ","
               "\"expensive\":false},"
              "{\"name\":\"Globals\",\"variablesReference\":" + std::to_string(GLOBAL_SCOPE_REF) + ","
               "\"expensive\":false}"
            "]}");
    }

    std::vector<DapVariable> collect_vars_from_env(std::shared_ptr<Environment> env,
                                                     bool only_this_scope) {
        std::vector<DapVariable> result;
        if (!env) return result;

        auto walk = [&](std::shared_ptr<Environment> e) {
            for (const auto& [name, val] : e->values()) {
                DapVariable dv;
                dv.name  = name;
                dv.value = val.to_string();
                dv.type  = val.type_name();
                dv.variablesReference = 0;
                if (val.is_array() || val.is_map()) {
                    int ref = next_ref_id_++;
                    ref_map_[ref] = val;
                    dv.variablesReference = ref;
                }
                result.push_back(std::move(dv));
            }
        };

        if (only_this_scope) {
            walk(env);
        } else {
            // Walk entire chain
            auto cur = env;
            while (cur) {
                walk(cur);
                cur = cur->enclosing();
            }
        }

        // Sort by name for readability
        std::sort(result.begin(), result.end(),
            [](const DapVariable& a, const DapVariable& b) { return a.name < b.name; });
        return result;
    }

    void handle_variables(int seq, const std::string& msg) {
        int ref = get_dap_int(msg, "variablesReference");
        std::vector<DapVariable> vars;

        if (!interp_) {
            send_response(seq, "variables", "{\"variables\":[]}");
            return;
        }

        if (ref == LOCAL_SCOPE_REF) {
            vars = collect_vars_from_env(interp_->environment(), true);
        } else if (ref == GLOBAL_SCOPE_REF) {
            vars = collect_vars_from_env(interp_->globals(), true);
        } else if (ref_map_.count(ref)) {
            const Value& val = ref_map_.at(ref);
            if (val.is_array()) {
                const auto& arr = *val.as_array();
                for (size_t i = 0; i < arr.size(); i++) {
                    DapVariable dv;
                    dv.name  = "[" + std::to_string(i) + "]";
                    dv.value = arr[i].to_string();
                    dv.type  = arr[i].type_name();
                    dv.variablesReference = 0;
                    if (arr[i].is_array() || arr[i].is_map()) {
                        int new_ref = next_ref_id_++;
                        ref_map_[new_ref] = arr[i];
                        dv.variablesReference = new_ref;
                    }
                    vars.push_back(std::move(dv));
                }
            } else if (val.is_map()) {
                const auto& mp = *val.as_map();
                for (const auto& [k, v] : mp) {
                    DapVariable dv;
                    dv.name  = k;
                    dv.value = v.to_string();
                    dv.type  = v.type_name();
                    dv.variablesReference = 0;
                    if (v.is_array() || v.is_map()) {
                        int new_ref = next_ref_id_++;
                        ref_map_[new_ref] = v;
                        dv.variablesReference = new_ref;
                    }
                    vars.push_back(std::move(dv));
                }
            }
        }

        std::string json = "[";
        for (size_t i = 0; i < vars.size(); i++) {
            if (i > 0) json += ",";
            json += "{\"name\":\"" + json_escape(vars[i].name) + "\","
                    "\"value\":\"" + json_escape(vars[i].value) + "\","
                    "\"type\":\"" + json_escape(vars[i].type) + "\","
                    "\"variablesReference\":" + std::to_string(vars[i].variablesReference) + "}";
        }
        json += "]";
        send_response(seq, "variables", "{\"variables\":" + json + "}");
    }

    void handle_evaluate(int seq, const std::string& msg) {
        std::string expr_str = get_dap_nested_str(msg, "arguments", "expression");
        if (expr_str.empty()) expr_str = get_dap_str(msg, "expression");

        if (!interp_ || expr_str.empty()) {
            send_response(seq, "evaluate",
                "{\"result\":\"(no expression)\",\"variablesReference\":0}");
            return;
        }

        bool eval_ok = false;
        std::string eval_result;
        int eval_ref = 0;

        try {
            Lexer l(expr_str + ";");
            auto toks = l.tokenize();
            Parser p(toks);
            auto stmts = p.parse();
            if (!p.has_error() && !stmts.empty()) {
                if (auto* es = dynamic_cast<ExpressionStmt*>(stmts[0].get())) {
                    Value res = interp_->evaluate(*es->expression);
                    eval_result = res.to_string();
                    eval_ok = true;
                    if (res.is_array() || res.is_map()) {
                        eval_ref = next_ref_id_++;
                        ref_map_[eval_ref] = res;
                    }
                }
            }
        } catch (const std::exception& e) {
            eval_result = std::string("Error: ") + e.what();
        } catch (...) {
            eval_result = "Error evaluating expression";
        }

        if (eval_ok) {
            send_response(seq, "evaluate",
                "{\"result\":\"" + json_escape(eval_result) + "\","
                "\"variablesReference\":" + std::to_string(eval_ref) + "}");
        } else {
            send_response(seq, "evaluate",
                "{\"result\":\"" + json_escape(eval_result) + "\","
                "\"variablesReference\":0}");
        }
    }

    void handle_set_variable(int seq, const std::string& /*msg*/) {
        // Not fully implemented — return success but don't actually change
        send_response(seq, "setVariable", "{\"value\":\"(read-only)\",\"variablesReference\":0}");
    }

    void handle_continue(int seq) {
        step_mode_ = StepMode::None;
        paused_ = false;
        send_response(seq, "continue", "{\"allThreadsContinued\":true}");
        send_event("continued", "{\"allThreadsContinued\":true}");
    }

    void handle_next(int seq) {
        step_mode_ = StepMode::Over;
        step_start_depth_ = call_depth_;
        paused_ = false;
        send_response(seq, "next", "{}");
    }

    void handle_step_in(int seq) {
        step_mode_ = StepMode::In;
        paused_ = false;
        send_response(seq, "stepIn", "{}");
    }

    void handle_step_out(int seq) {
        step_mode_ = StepMode::Out;
        step_start_depth_ = call_depth_;
        paused_ = false;
        send_response(seq, "stepOut", "{}");
    }

    // Evaluate a condition string in the current interpreter scope
    bool eval_condition(const std::string& condition) {
        if (!interp_ || condition.empty()) return true;
        try {
            Lexer l(condition + ";");
            auto toks = l.tokenize();
            Parser p(toks);
            auto stmts = p.parse();
            if (!p.has_error() && !stmts.empty()) {
                if (auto* es = dynamic_cast<ExpressionStmt*>(stmts[0].get())) {
                    Value res = interp_->evaluate(*es->expression);
                    return res.is_truthy();
                }
            }
        } catch (...) {}
        return true; // On error, treat as true (always break)
    }
};

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

void run_dap_server(const std::string& source_file, std::istream& in, std::ostream& out) {
    DapServer server(in, out, source_file);
    server.run();
}

} // namespace raven
