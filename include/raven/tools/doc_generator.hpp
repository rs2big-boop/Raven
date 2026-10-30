#pragma once

#include <string>
#include <vector>

namespace raven {

struct DocItem {
    std::string kind;        // "fn", "struct"
    std::string name;
    std::vector<std::string> params;
    std::string comment;
    int line;
};

class DocGenerator {
public:
    static std::vector<DocItem> extract(const std::string& source);
    static std::string to_markdown(const std::string& module_name, const std::vector<DocItem>& items);
};

} // namespace raven
