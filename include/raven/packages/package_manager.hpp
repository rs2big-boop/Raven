#pragma once
#include <string>
#include <vector>
#include "lockfile.hpp"
#include "dependency_resolver.hpp"

namespace raven::packages {

struct PackageMetadata {
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::string repo;
    std::vector<std::string> tags;
    size_t downloads = 0;
    size_t stars = 0;
};

class PackageManager {
public:
    static bool init_manifest(const std::string& package_name, const std::string& manifest_path = "raven.json");
    static bool install_from_manifest(const std::string& manifest_path = "raven.json");
    static bool install_package(const std::string& repo_url, const std::string& version = "latest");
    static std::vector<std::string> list_installed(const std::string& modules_dir = "raven_modules");

    // Community Registry & Ecosystem Extensions
    static std::vector<PackageMetadata> search_registry(const std::string& query);
    static bool add_dependency(const std::string& pkg_name, const std::string& version = "latest", const std::string& manifest_path = "raven.json");
    static bool publish_package(const std::string& manifest_path, std::string& out_message);
    static PackageMetadata get_package_info(const std::string& pkg_name);
    static std::vector<PackageMetadata> get_curated_registry();
};

} // namespace raven::packages
