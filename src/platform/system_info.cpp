#include "raven/platform/system_info.hpp"
#include <thread>
#include <algorithm>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>

namespace raven::platform {

std::string os_name() {
    return "Windows";
}

std::string os_arch() {
#if defined(_M_ARM64) || defined(__aarch64__)
    return "arm64";
#elif defined(_M_X64) || defined(__x86_64__)
    return "x86_64";
#else
    return "x86";
#endif
}

int cpu_cores() {
    return static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
}

std::string hostname() {
    char h[256];
    DWORD size = sizeof(h);
    if (GetComputerNameA(h, &size)) return std::string(h);
    return "localhost";
}

} // namespace raven::platform

#else

#include <unistd.h>
#include <sys/utsname.h>

namespace raven::platform {

std::string os_name() {
    struct utsname u;
    if (uname(&u) == 0) return u.sysname;
    return "Linux";
}

std::string os_arch() {
    struct utsname u;
    if (uname(&u) == 0) return u.machine;
    return "x86_64";
}

int cpu_cores() {
    return static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
}

std::string hostname() {
    char h[256];
    if (gethostname(h, sizeof(h)) == 0) return h;
    return "localhost";
}

} // namespace raven::platform

#endif
