#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace raven::packages {

struct LockedPackage {
    std::string name;
    std::string version;
    std::string source;
    std::string hash;
};

class Lockfile {
public:
    static bool write(const std::string& filepath, const std::vector<LockedPackage>& packages);
    static std::vector<LockedPackage> read(const std::string& filepath);
    static bool exists(const std::string& filepath);
};

} // namespace raven::packages
