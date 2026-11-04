#include "raven/platform/filesystem.hpp"
#include <filesystem>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

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

std::string fs_executable_path() {
#if defined(_WIN32) || defined(_WIN64)
    char buffer[MAX_PATH] = {};
    DWORD len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        return std::string(buffer);
    }
    return {};
#else
    std::error_code ec;
    fs::path exe = "/proc/self/exe";
    if (fs::exists(exe, ec)) {
        fs::path resolved = fs::canonical(exe, ec);
        if (!ec) {
            return resolved.string();
        }
    }
    return {};
#endif
}

std::string fs_executable_dir() {
    std::string exe = fs_executable_path();
    if (exe.empty()) {
        return {};
    }
    return fs::path(exe).parent_path().string();
}

} // namespace raven::platform
