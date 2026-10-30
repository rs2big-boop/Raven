#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace raven::platform {

bool fs_exists(const std::string& path);
bool fs_is_directory(const std::string& path);
bool fs_create_dir(const std::string& path);
bool fs_remove(const std::string& path);
std::vector<std::string> fs_list_dir(const std::string& path);
uint64_t fs_file_size(const std::string& path);

} // namespace raven::platform
