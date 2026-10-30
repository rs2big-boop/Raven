#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace raven::packages {

struct Dependency {
    std::string name;
    std::string version_constraint;
    std::vector<std::string> dependencies;
};

class DependencyResolver {
public:
    void add_package(const Dependency& dep);
    bool resolve(std::vector<std::string>& install_order, std::string& error_message);
    void clear();

private:
    std::unordered_map<std::string, Dependency> registry_;
    bool visit(const std::string& node,
               std::unordered_set<std::string>& visited,
               std::unordered_set<std::string>& visiting,
               std::vector<std::string>& order,
               std::string& error_message);
};

} // namespace raven::packages
