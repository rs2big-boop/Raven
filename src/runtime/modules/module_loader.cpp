// module_loader.cpp — Raven module loading and resolution
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdio>

namespace raven::runtime {

struct ModuleEntry {
    std::string name;
    std::string path;
    bool is_loaded = false;
};

class ModuleLoader {
public:
    static ModuleLoader& instance() {
        static ModuleLoader loader;
        return loader;
    }

    void add_search_path(const std::string& path) {
        search_paths_.push_back(path);
    }

    std::string resolve(const std::string& name) const {
        for (const auto& base : search_paths_) {
            std::string candidate = base + "/" + name + ".rvn";
            if (FILE* f = fopen(candidate.c_str(), "r")) {
                fclose(f);
                return candidate;
            }
        }
        for (const auto& base : search_paths_) {
            std::string candidate = base + "/" + name;
            if (FILE* f = fopen(candidate.c_str(), "r")) {
                fclose(f);
                return candidate;
            }
        }
        return "";
    }

    bool is_loaded(const std::string& name) const {
        return loaded_.count(name) > 0;
    }

    void mark_loaded(const std::string& name, const std::string& path) {
        loaded_[name] = {name, path, true};
    }

private:
    ModuleLoader() = default;
    std::vector<std::string> search_paths_;
    std::unordered_map<std::string, ModuleEntry> loaded_;
};

} // namespace raven::runtime
