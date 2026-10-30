#include "raven/platform/system_info.hpp"
#include <unistd.h>
#include <sys/utsname.h>
#include <thread>

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
