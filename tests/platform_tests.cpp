#include <filesystem>
#include <gtest/gtest.h>

#include "raven/platform/filesystem.hpp"

TEST(PlatformFilesystemTest, ExecutablePathIsPortable) {
    std::string exe_path = raven::platform::fs_executable_path();
    std::string exe_dir = raven::platform::fs_executable_dir();

    EXPECT_FALSE(exe_path.empty());
    EXPECT_FALSE(exe_dir.empty());
    EXPECT_EQ(std::filesystem::path(exe_path).parent_path().string(), exe_dir);
}
