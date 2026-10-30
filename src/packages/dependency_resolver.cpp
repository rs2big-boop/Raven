#include "raven/packages/dependency_resolver.hpp"

namespace raven::packages {

void DependencyResolver::add_package(const Dependency& dep) {
    registry_[dep.name] = dep;
}

void DependencyResolver::clear() {
    registry_.clear();
}

bool DependencyResolver::visit(const std::string& node,
                               std::unordered_set<std::string>& visited,
                               std::unordered_set<std::string>& visiting,
                               std::vector<std::string>& order,
                               std::string& error_message) {
    if (visiting.count(node)) {
        error_message = "Circular dependency detected involving: " + node;
        return false;
    }
    if (visited.count(node)) {
        return true;
    }

    visiting.insert(node);

    auto it = registry_.find(node);
    if (it != registry_.end()) {
        for (const auto& dep : it->second.dependencies) {
            if (!visit(dep, visited, visiting, order, error_message)) {
                return false;
            }
        }
    }

    visiting.erase(node);
    visited.insert(node);
    order.push_back(node);
    return true;
}

bool DependencyResolver::resolve(std::vector<std::string>& install_order, std::string& error_message) {
    install_order.clear();
    error_message.clear();

    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> visiting;

    for (const auto& [name, _] : registry_) {
        if (!visited.count(name)) {
            if (!visit(name, visited, visiting, install_order, error_message)) {
                return false;
            }
        }
    }

    return true;
}

} // namespace raven::packages
