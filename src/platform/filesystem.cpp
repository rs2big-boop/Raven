#include "raven/platform/filesystem.hpp"
#include <filesystem>

namespace raven::platform {

namespace fs = std::filesystem;

bool fs_exists(const std::string& path) {
    std::error_code ec;
    return fs::exists(path, ec);
}

bool fs_is_directory(const std::string& path) {
    std::error_code ec;
    return fs::is_directory(path, ec);
}

bool fs_create_dir(const std::string& path) {
    std::error_code ec;
    return fs::create_directories(path, ec);
}

bool fs_remove(const std::string& path) {
    std::error_code ec;
    return fs::remove_all(path, ec) > 0;
}

std::vector<std::string> fs_list_dir(const std::string& path) {
    std::vector<std::string> entries;
    std::error_code ec;
    if (!fs::exists(path, ec)) return entries;
    for (const auto& entry : fs::directory_iterator(path, ec)) {
        entries.push_back(entry.path().filename().string());
    }
    return entries;
}

uint64_t fs_file_size(const std::string& path) {
    std::error_code ec;
    return fs::file_size(path, ec);
}

} // namespace raven::platform
