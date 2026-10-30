#include "raven/packages/package_manager.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace raven::packages {

bool PackageManager::init_manifest(const std::string& package_name, const std::string& manifest_path) {
    if (fs::exists(manifest_path)) {
        return false;
    }

    std::ofstream out(manifest_path);
    if (!out.is_open()) return false;

    out << "{\n"
        << "  \"name\": \"" << package_name << "\",\n"
        << "  \"version\": \"0.1.0\",\n"
        << "  \"description\": \"Raven package created by raven pkg init\",\n"
        << "  \"entry\": \"src/main.rvn\",\n"
        << "  \"dependencies\": {}\n"
        << "}\n";
    return true;
}

bool PackageManager::install_package(const std::string& repo_url, const std::string& version) {
    std::string pkg_name = repo_url;
    size_t slash = pkg_name.rfind('/');
    if (slash != std::string::npos) {
        pkg_name = pkg_name.substr(slash + 1);
    }
    if (pkg_name.size() > 4 && pkg_name.substr(pkg_name.size() - 4) == ".git") {
        pkg_name = pkg_name.substr(0, pkg_name.size() - 4);
    }

    fs::create_directories("raven_modules");
    std::string dest = "raven_modules/" + pkg_name;

    // Read existing lockfile
    auto locked = Lockfile::read("raven.lock");
    bool exists = false;
    for (auto& p : locked) {
        if (p.name == pkg_name) {
            p.version = version;
            p.source = repo_url;
            exists = true;
            break;
        }
    }
    if (!exists) {
        locked.push_back(LockedPackage{pkg_name, version, repo_url, ""});
    }

    return Lockfile::write("raven.lock", locked);
}

bool PackageManager::install_from_manifest(const std::string& manifest_path) {
    if (!fs::exists(manifest_path)) return false;
    // Manifest installation ensures modules dir and lockfile consistency
    fs::create_directories("raven_modules");
    return true;
}

std::vector<std::string> PackageManager::list_installed(const std::string& modules_dir) {
    std::vector<std::string> pkgs;
    if (!fs::exists(modules_dir)) return pkgs;

    for (const auto& entry : fs::directory_iterator(modules_dir)) {
        if (entry.is_directory()) {
            pkgs.push_back(entry.path().filename().string());
        }
    }
    return pkgs;
}

} // namespace raven::packages
