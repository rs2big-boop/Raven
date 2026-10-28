#include "raven/doc_generator.hpp"
#include "raven/lexer.hpp"
#include <sstream>

namespace raven {

std::vector<DocItem> DocGenerator::extract(const std::string& source) {
    std::vector<DocItem> items;
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    std::string accumulated_comments;

    // Scan source line by line for comments and signatures
    std::istringstream stream(source);
    std::string line;
    std::string current_doc;
    int line_num = 1;

    while (std::getline(stream, line)) {
        size_t first = line.find_first_not_of(" \t\r");
        if (first != std::string::npos && line.substr(first, 2) == "//") {
            std::string c = line.substr(first + 2);
            if (!c.empty() && c[0] == ' ') c = c.substr(1);
            if (!current_doc.empty()) current_doc += "\n";
            current_doc += c;
        } else if (first != std::string::npos) {
            std::string trimmed = line.substr(first);
            if (trimmed.rfind("fn ", 0) == 0) {
                // Parse function name and params
                size_t pstart = trimmed.find('(');
                size_t pend = trimmed.find(')');
                if (pstart != std::string::npos && pend != std::string::npos) {
                    std::string fn_name = trimmed.substr(3, pstart - 3);
                    size_t f_first = fn_name.find_first_not_of(" \t");
                    size_t f_last = fn_name.find_last_not_of(" \t");
                    if (f_first != std::string::npos) fn_name = fn_name.substr(f_first, f_last - f_first + 1);

                    std::string raw_params = trimmed.substr(pstart + 1, pend - pstart - 1);
                    std::vector<std::string> params;
                    std::istringstream pstream(raw_params);
                    std::string p;
                    while (std::getline(pstream, p, ',')) {
                        size_t p1 = p.find_first_not_of(" \t");
                        size_t p2 = p.find_last_not_of(" \t");
                        if (p1 != std::string::npos) params.push_back(p.substr(p1, p2 - p1 + 1));
                    }

                    DocItem item;
                    item.kind = "fn";
                    item.name = fn_name;
                    item.params = params;
                    item.comment = current_doc;
                    item.line = line_num;
                    items.push_back(item);
                }
                current_doc.clear();
            } else if (trimmed.rfind("struct ", 0) == 0) {
                size_t bstart = trimmed.find('{');
                if (bstart != std::string::npos) {
                    std::string struct_name = trimmed.substr(7, bstart - 7);
                    size_t s1 = struct_name.find_first_not_of(" \t");
                    size_t s2 = struct_name.find_last_not_of(" \t");
                    if (s1 != std::string::npos) struct_name = struct_name.substr(s1, s2 - s1 + 1);

                    DocItem item;
                    item.kind = "struct";
                    item.name = struct_name;
                    item.comment = current_doc;
                    item.line = line_num;
                    items.push_back(item);
                }
                current_doc.clear();
            } else {
                current_doc.clear();
            }
        } else {
            current_doc.clear();
        }
        line_num++;
    }

    return items;
}

std::string DocGenerator::to_markdown(const std::string& module_name, const std::vector<DocItem>& items) {
    std::ostringstream ss;
    ss << "# Module `" << module_name << "`\n\n";

    if (items.empty()) {
        ss << "*No exported functions or structs found.*\n";
        return ss.str();
    }

    ss << "## Index\n\n";
    for (const auto& item : items) {
        if (item.kind == "fn") {
            ss << "- [`fn " << item.name << "(";
            for (size_t i = 0; i < item.params.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << item.params[i];
            }
            ss << ")`](#fn-" << item.name << ")\n";
        } else {
            ss << "- [`struct " << item.name << "`](#struct-" << item.name << ")\n";
        }
    }

    ss << "\n## Documentation\n\n";
    for (const auto& item : items) {
        if (item.kind == "fn") {
            ss << "### `fn " << item.name << "(";
            for (size_t i = 0; i < item.params.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << item.params[i];
            }
            ss << ")`\n\n";
        } else {
            ss << "### `struct " << item.name << "`\n\n";
        }

        if (!item.comment.empty()) {
            ss << item.comment << "\n\n";
        } else {
            ss << "*No documentation provided.*\n\n";
        }
    }

    return ss.str();
}

} // namespace raven
