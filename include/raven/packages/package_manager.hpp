#pragma once
#include <string>
#include <vector>
#include "lockfile.hpp"
#include "dependency_resolver.hpp"

namespace raven::packages {

class PackageManager {
public:
    static bool init_manifest(const std::string& package_name, const std::string& manifest_path = "raven.json");
    static bool install_from_manifest(const std::string& manifest_path = "raven.json");
    static bool install_package(const std::string& repo_url, const std::string& version = "latest");
    static std::vector<std::string> list_installed(const std::string& modules_dir = "raven_modules");
};

} // namespace raven::packages
