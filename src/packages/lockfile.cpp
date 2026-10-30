#include "raven/packages/lockfile.hpp"
#include <fstream>
#include <sstream>

namespace raven::packages {

bool Lockfile::write(const std::string& filepath, const std::vector<LockedPackage>& packages) {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "# Raven Lockfile (auto-generated, do not edit manually)\n\n";
    for (const auto& pkg : packages) {
        out << "package " << pkg.name << "\n";
        out << "  version: " << pkg.version << "\n";
        out << "  source: " << pkg.source << "\n";
        if (!pkg.hash.empty()) {
            out << "  hash: " << pkg.hash << "\n";
        }
        out << "\n";
    }
    return true;
}

std::vector<LockedPackage> Lockfile::read(const std::string& filepath) {
    std::vector<LockedPackage> pkgs;
    std::ifstream in(filepath);
    if (!in.is_open()) return pkgs;

    std::string line;
    LockedPackage current;
    bool in_pkg = false;

    while (std::getline(in, line)) {
        // Strip whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos || line[start] == '#') continue;
        std::string trimmed = line.substr(start);

        if (trimmed.rfind("package ", 0) == 0) {
            if (in_pkg && !current.name.empty()) {
                pkgs.push_back(current);
            }
            current = LockedPackage{};
            current.name = trimmed.substr(8);
            in_pkg = true;
        } else if (in_pkg) {
            if (trimmed.rfind("version:", 0) == 0) {
                std::string v = trimmed.substr(8);
                size_t vs = v.find_first_not_of(" \t");
                current.version = (vs != std::string::npos) ? v.substr(vs) : "";
            } else if (trimmed.rfind("source:", 0) == 0) {
                std::string s = trimmed.substr(7);
                size_t ss = s.find_first_not_of(" \t");
                current.source = (ss != std::string::npos) ? s.substr(ss) : "";
            } else if (trimmed.rfind("hash:", 0) == 0) {
                std::string h = trimmed.substr(5);
                size_t hs = h.find_first_not_of(" \t");
                current.hash = (hs != std::string::npos) ? h.substr(hs) : "";
            }
        }
    }

    if (in_pkg && !current.name.empty()) {
        pkgs.push_back(current);
    }

    return pkgs;
}

bool Lockfile::exists(const std::string& filepath) {
    std::ifstream in(filepath);
    return in.good();
}

} // namespace raven::packages
