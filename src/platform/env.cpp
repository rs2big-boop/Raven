#include "raven/platform/env.hpp"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cctype>
#include <algorithm>

#ifndef _WIN32
extern char** environ;
#else
#include <windows.h>
#endif

namespace raven::platform {

namespace {

std::string trim_left(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        ++start;
    }
    return s.substr(start);
}

std::string trim_right(const std::string& s) {
    if (s.empty()) return s;
    size_t end = s.size();
    while (end > 0 && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
        --end;
    }
    return s.substr(0, end);
}

std::string trim(const std::string& s) {
    return trim_right(trim_left(s));
}

std::string unescape_double_quoted(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char next = s[i + 1];
            switch (next) {
                case 'n': out.push_back('\n'); ++i; break;
                case 'r': out.push_back('\r'); ++i; break;
                case 't': out.push_back('\t'); ++i; break;
                case '\\': out.push_back('\\'); ++i; break;
                case '"': out.push_back('"'); ++i; break;
                case '$': out.push_back('$'); ++i; break;
                default:
                    out.push_back(s[i]);
                    break;
            }
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

} // anonymous namespace

std::optional<std::string> get_env_var(const std::string& name) {
    if (name.empty()) return std::nullopt;
    const char* val = std::getenv(name.c_str());
    if (!val) return std::nullopt;
    return std::string(val);
}

bool set_env_var(const std::string& name, const std::string& value, bool overwrite) {
    if (name.empty()) return false;
#ifdef _WIN32
    if (!overwrite) {
        char buf[2];
        size_t len = 0;
        if (getenv_s(&len, buf, sizeof(buf), name.c_str()) == 0 && len > 0) {
            return true;
        }
    }
    return _putenv_s(name.c_str(), value.c_str()) == 0;
#else
    return ::setenv(name.c_str(), value.c_str(), overwrite ? 1 : 0) == 0;
#endif
}

bool unset_env_var(const std::string& name) {
    if (name.empty()) return false;
#ifdef _WIN32
    return _putenv_s(name.c_str(), "") == 0;
#else
    return ::unsetenv(name.c_str()) == 0;
#endif
}

bool has_env_var(const std::string& name) {
    if (name.empty()) return false;
    return std::getenv(name.c_str()) != nullptr;
}

std::unordered_map<std::string, std::string> get_all_env_vars() {
    std::unordered_map<std::string, std::string> result;
#ifndef _WIN32
    if (environ) {
        for (char** env = environ; *env != nullptr; ++env) {
            std::string s(*env);
            size_t eq = s.find('=');
            if (eq != std::string::npos && eq > 0) {
                result[s.substr(0, eq)] = s.substr(eq + 1);
            }
        }
    }
#else
    if (_environ) {
        for (char** env = _environ; *env != nullptr; ++env) {
            std::string s(*env);
            size_t eq = s.find('=');
            if (eq != std::string::npos && eq > 0) {
                result[s.substr(0, eq)] = s.substr(eq + 1);
            }
        }
    }
#endif
    return result;
}

std::string interpolate_env_value(
    const std::string& raw_val,
    const std::unordered_map<std::string, std::string>& context
) {
    std::string out;
    out.reserve(raw_val.size());
    size_t i = 0;
    while (i < raw_val.size()) {
        if (raw_val[i] == '\\' && i + 1 < raw_val.size() && raw_val[i + 1] == '$') {
            out.push_back('$');
            i += 2;
            continue;
        }

        if (raw_val[i] == '$') {
            if (i + 1 < raw_val.size() && raw_val[i + 1] == '{') {
                size_t end_brace = raw_val.find('}', i + 2);
                if (end_brace != std::string::npos) {
                    std::string expr = raw_val.substr(i + 2, end_brace - (i + 2));
                    std::string var_name = expr;
                    std::string def_val = "";
                    bool has_default = false;

                    size_t colon_dash = expr.find(":-");
                    if (colon_dash != std::string::npos) {
                        var_name = expr.substr(0, colon_dash);
                        def_val = expr.substr(colon_dash + 2);
                        has_default = true;
                    }

                    std::string found_val;
                    bool resolved = false;

                    auto it = context.find(var_name);
                    if (it != context.end() && !it->second.empty()) {
                        found_val = it->second;
                        resolved = true;
                    } else {
                        auto sys_val = get_env_var(var_name);
                        if (sys_val && !sys_val->empty()) {
                            found_val = *sys_val;
                            resolved = true;
                        }
                    }

                    if (resolved) {
                        out += found_val;
                    } else if (has_default) {
                        out += def_val;
                    }

                    i = end_brace + 1;
                    continue;
                }
            } else if (i + 1 < raw_val.size() && (std::isalpha(static_cast<unsigned char>(raw_val[i + 1])) || raw_val[i + 1] == '_')) {
                size_t start = i + 1;
                size_t len = 0;
                while (start + len < raw_val.size() &&
                       (std::isalnum(static_cast<unsigned char>(raw_val[start + len])) || raw_val[start + len] == '_')) {
                    ++len;
                }
                std::string var_name = raw_val.substr(start, len);
                auto it = context.find(var_name);
                if (it != context.end()) {
                    out += it->second;
                } else {
                    auto sys_val = get_env_var(var_name);
                    if (sys_val) out += *sys_val;
                }
                i = start + len;
                continue;
            }
        }

        out.push_back(raw_val[i]);
        ++i;
    }
    return out;
}

std::unordered_map<std::string, std::string> parse_env_string(
    const std::string& content,
    const std::unordered_map<std::string, std::string>& base_env
) {
    std::unordered_map<std::string, std::string> result = base_env;
    std::unordered_map<std::string, std::string> file_vars;

    std::string text = content;
    // Strip UTF-8 BOM if present
    if (text.size() >= 3 &&
        static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF) {
        text.erase(0, 3);
    }

    std::istringstream stream(text);
    std::string line;

    bool in_multiline = false;
    char quote_char = 0;
    std::string current_key;
    std::string current_val;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (in_multiline) {
            size_t close_pos = std::string::npos;
            for (size_t i = 0; i < line.size(); ++i) {
                if (line[i] == '\\' && quote_char == '"' && i + 1 < line.size()) {
                    ++i; // skip escaped
                    continue;
                }
                if (line[i] == quote_char) {
                    close_pos = i;
                    break;
                }
            }

            if (close_pos != std::string::npos) {
                current_val += line.substr(0, close_pos);
                in_multiline = false;

                std::string final_val;
                if (quote_char == '"') {
                    final_val = unescape_double_quoted(current_val);
                    final_val = interpolate_env_value(final_val, result);
                } else {
                    final_val = current_val; // single quote or backtick: raw
                }

                result[current_key] = final_val;
                file_vars[current_key] = final_val;
                current_key.clear();
                current_val.clear();
            } else {
                current_val += line + "\n";
            }
            continue;
        }

        std::string trimmed = trim_left(line);
        if (trimmed.empty() || trimmed.front() == '#') {
            continue;
        }

        // Support 'export KEY=VAL'
        if (trimmed.rfind("export ", 0) == 0) {
            trimmed = trim_left(trimmed.substr(7));
        }

        size_t eq_pos = trimmed.find('=');
        if (eq_pos == std::string::npos) {
            continue;
        }

        std::string key = trim(trimmed.substr(0, eq_pos));
        if (key.empty()) continue;

        std::string val_part = trim_left(trimmed.substr(eq_pos + 1));
        if (val_part.empty()) {
            result[key] = "";
            file_vars[key] = "";
            continue;
        }

        char first_char = val_part.front();
        if (first_char == '"' || first_char == '\'' || first_char == '`') {
            quote_char = first_char;
            std::string content_after_quote = val_part.substr(1);

            size_t close_pos = std::string::npos;
            for (size_t i = 0; i < content_after_quote.size(); ++i) {
                if (content_after_quote[i] == '\\' && quote_char == '"' && i + 1 < content_after_quote.size()) {
                    ++i; // skip escaped
                    continue;
                }
                if (content_after_quote[i] == quote_char) {
                    close_pos = i;
                    break;
                }
            }

            if (close_pos != std::string::npos) {
                std::string inside = content_after_quote.substr(0, close_pos);
                std::string final_val;
                if (quote_char == '"') {
                    final_val = unescape_double_quoted(inside);
                    final_val = interpolate_env_value(final_val, result);
                } else {
                    final_val = inside;
                }
                result[key] = final_val;
                file_vars[key] = final_val;
            } else {
                in_multiline = true;
                current_key = key;
                current_val = content_after_quote + "\n";
            }
        } else {
            // Unquoted value: strip inline comments (separated by space/tab)
            std::string raw_val = val_part;
            for (size_t i = 0; i < raw_val.size(); ++i) {
                if (raw_val[i] == '#' && i > 0 && (raw_val[i - 1] == ' ' || raw_val[i - 1] == '\t')) {
                    raw_val = raw_val.substr(0, i - 1);
                    break;
                }
            }
            raw_val = trim_right(raw_val);
            std::string final_val = interpolate_env_value(raw_val, result);
            result[key] = final_val;
            file_vars[key] = final_val;
        }
    }

    return file_vars;
}

std::unordered_map<std::string, std::string> load_env_file(
    const std::string& path,
    bool overwrite
) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return {};
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    file.close();

    auto current_sys = get_all_env_vars();
    auto parsed = parse_env_string(ss.str(), current_sys);

    for (const auto& [k, v] : parsed) {
        set_env_var(k, v, overwrite);
    }

    return parsed;
}

std::optional<std::string> find_env_file(
    const std::string& start_dir,
    const std::string& filename
) {
    std::error_code ec;
    std::filesystem::path cur = std::filesystem::absolute(start_dir, ec);
    if (ec) cur = std::filesystem::current_path();

    while (true) {
        std::filesystem::path candidate = cur / filename;
        if (std::filesystem::exists(candidate, ec) && !std::filesystem::is_directory(candidate, ec)) {
            return candidate.string();
        }
        if (!cur.has_parent_path() || cur == cur.parent_path()) {
            break;
        }
        cur = cur.parent_path();
    }
    return std::nullopt;
}

std::string format_env_string(const std::unordered_map<std::string, std::string>& vars) {
    std::ostringstream ss;
    for (const auto& [k, v] : vars) {
        bool needs_quotes = false;
        if (v.empty() || v.find(' ') != std::string::npos || v.find('\t') != std::string::npos ||
            v.find('#') != std::string::npos || v.find('\n') != std::string::npos ||
            v.find('"') != std::string::npos || v.find('$') != std::string::npos) {
            needs_quotes = true;
        }

        ss << k << "=";
        if (needs_quotes) {
            ss << "\"";
            for (char c : v) {
                switch (c) {
                    case '\n': ss << "\\n"; break;
                    case '\r': ss << "\\r"; break;
                    case '\t': ss << "\\t"; break;
                    case '\\': ss << "\\\\"; break;
                    case '"': ss << "\\\""; break;
                    case '$': ss << "\\$"; break;
                    default: ss << c; break;
                }
            }
            ss << "\"\n";
        } else {
            ss << v << "\n";
        }
    }
    return ss.str();
}

bool write_env_file(const std::string& path, const std::unordered_map<std::string, std::string>& vars) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << format_env_string(vars);
    return file.good();
}

} // namespace raven::platform
