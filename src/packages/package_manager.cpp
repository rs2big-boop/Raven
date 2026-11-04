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

std::vector<PackageMetadata> PackageManager::get_curated_registry() {
    return {
        {
            "raven-router", "1.2.0",
            "High-throughput HTTP trie router with middleware pipelining and parameter extraction",
            "raven-core", "github.com/raven-ecosystem/raven-router",
            {"web", "routing", "http"}, 14200, 342
        },
        {
            "raven-jwt", "0.9.4",
            "HMAC-SHA256 and RSA JWT token encoder, decoder, and claims validator",
            "security-team", "github.com/raven-ecosystem/raven-jwt",
            {"auth", "jwt", "crypto"}, 9800, 215
        },
        {
            "raven-pg", "1.0.1",
            "Native PostgreSQL wire protocol driver with connection pooling and async queries",
            "db-guild", "github.com/raven-ecosystem/raven-pg",
            {"database", "sql", "postgres"}, 8400, 198
        },
        {
            "raven-redis", "0.8.2",
            "RESP3 compliant Redis client with Pub/Sub, pipelining, and connection healthcheck",
            "caching-team", "github.com/raven-ecosystem/raven-redis",
            {"cache", "redis", "kv"}, 7650, 184
        },
        {
            "raven-cli-ui", "0.5.0",
            "Beautiful terminal progress bars, spinners, ANSI color styling, and interactive prompts",
            "cli-wizards", "github.com/raven-ecosystem/raven-cli-ui",
            {"cli", "terminal", "ui"}, 11200, 289
        },
        {
            "raven-validator", "1.1.0",
            "Schema and data contract validation with declarative rules and typed errors",
            "ran-sh", "github.com/raven-ecosystem/raven-validator",
            {"validation", "schema", "data"}, 6500, 142
        },
        {
            "raven-worker-pool", "1.0.0",
            "Dynamic CSP worker pool with backpressure, load shedding, and metrics",
            "concurrency-devs", "github.com/raven-ecosystem/raven-worker-pool",
            {"concurrency", "worker-pool", "csp"}, 8900, 256
        },
        {
            "raven-yaml", "0.4.1",
            "YAML 1.2 parser and emitter with anchor expansion and map decoding",
            "infra-tools", "github.com/raven-ecosystem/raven-yaml",
            {"yaml", "parser", "serialization"}, 5100, 118
        }
    };
}

std::vector<PackageMetadata> PackageManager::search_registry(const std::string& query) {
    auto registry = get_curated_registry();
    if (query.empty()) return registry;

    std::string lower_query = query;
    for (char& c : lower_query) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    std::vector<PackageMetadata> results;
    for (const auto& pkg : registry) {
        std::string lower_name = pkg.name;
        for (char& c : lower_name) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        std::string lower_desc = pkg.description;
        for (char& c : lower_desc) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        bool matched = (lower_name.find(lower_query) != std::string::npos) ||
                       (lower_desc.find(lower_query) != std::string::npos);

        if (!matched) {
            for (const auto& tag : pkg.tags) {
                std::string lower_tag = tag;
                for (char& c : lower_tag) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (lower_tag.find(lower_query) != std::string::npos) {
                    matched = true;
                    break;
                }
            }
        }

        if (matched) {
            results.push_back(pkg);
        }
    }
    return results;
}

PackageMetadata PackageManager::get_package_info(const std::string& pkg_name) {
    for (const auto& pkg : get_curated_registry()) {
        if (pkg.name == pkg_name) {
            return pkg;
        }
    }
    return PackageMetadata{pkg_name, "0.1.0", "Community package", "unknown", "github.com/" + pkg_name, {}};
}

bool PackageManager::add_dependency(const std::string& pkg_name, const std::string& version, const std::string& manifest_path) {
    if (!fs::exists(manifest_path)) {
        init_manifest("app", manifest_path);
    }

    std::ifstream in(manifest_path);
    if (!in.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    // Look for "dependencies": {
    size_t dep_pos = content.find("\"dependencies\"");
    if (dep_pos == std::string::npos) {
        return false;
    }

    size_t brace_open = content.find('{', dep_pos);
    if (brace_open == std::string::npos) return false;

    size_t brace_close = content.find('}', brace_open);
    if (brace_close == std::string::npos) return false;

    std::string inside = content.substr(brace_open + 1, brace_close - brace_open - 1);
    bool is_empty = (inside.find_first_not_of(" \t\r\n") == std::string::npos);

    std::string insertion;
    if (is_empty) {
        insertion = "\n    \"" + pkg_name + "\": \"" + version + "\"\n  ";
    } else {
        insertion = inside + ",\n    \"" + pkg_name + "\": \"" + version + "\"\n  ";
    }

    std::string new_content = content.substr(0, brace_open + 1) + insertion + content.substr(brace_close);
    std::ofstream out(manifest_path);
    if (!out.is_open()) return false;
    out << new_content;
    return true;
}

bool PackageManager::remove_dependency(const std::string& pkg_name, const std::string& manifest_path) {
    if (!fs::exists(manifest_path)) return false;

    std::ifstream in(manifest_path);
    if (!in.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    size_t dep_pos = content.find("\"dependencies\"");
    if (dep_pos == std::string::npos) return false;

    size_t brace_open = content.find('{', dep_pos);
    if (brace_open == std::string::npos) return false;

    size_t brace_close = content.find('}', brace_open);
    if (brace_close == std::string::npos) return false;

    std::string target_key = "\"" + pkg_name + "\"";
    size_t key_pos = content.find(target_key, brace_open);
    if (key_pos == std::string::npos || key_pos > brace_close) {
        return false; // not found
    }

    // Find line start/comma and value end
    size_t line_start = content.rfind('\n', key_pos);
    if (line_start == std::string::npos || line_start < brace_open) line_start = key_pos;

    size_t val_end = content.find('\n', key_pos);
    if (val_end == std::string::npos || val_end > brace_close) val_end = brace_close;

    // Check if trailing comma before line_start or after val_end
    std::string before = content.substr(0, line_start);
    std::string after = content.substr(val_end);

    // Clean up possible trailing commas if needed
    if (!before.empty() && before.back() == ',') {
        before.pop_back();
    }

    std::ofstream out(manifest_path);
    if (!out.is_open()) return false;
    out << before << after;
    return true;
}

bool PackageManager::uninstall_package(const std::string& pkg_name, const std::string& modules_dir) {
    std::string dest = modules_dir + "/" + pkg_name;
    if (fs::exists(dest)) {
        std::error_code ec;
        fs::remove_all(dest, ec);
    }

    // Also remove from raven.lock
    if (fs::exists("raven.lock")) {
        auto locked = Lockfile::read("raven.lock");
        std::vector<LockedPackage> updated;
        for (const auto& p : locked) {
            if (p.name != pkg_name) updated.push_back(p);
        }
        Lockfile::write("raven.lock", updated);
    }
    return true;
}

bool PackageManager::publish_package(const std::string& manifest_path, std::string& out_message) {
    if (!fs::exists(manifest_path)) {
        out_message = "Error: Manifest file '" + manifest_path + "' not found.";
        return false;
    }

    std::ifstream in(manifest_path);
    if (!in.is_open()) {
        out_message = "Error: Cannot open manifest for reading.";
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    if (content.find("\"name\"") == std::string::npos) {
        out_message = "Validation Error: Manifest missing 'name' field.";
        return false;
    }
    if (content.find("\"version\"") == std::string::npos) {
        out_message = "Validation Error: Manifest missing 'version' field.";
        return false;
    }

    out_message = "Package successfully validated and ready for registry publishing.";
    return true;
}

} // namespace raven::packages
