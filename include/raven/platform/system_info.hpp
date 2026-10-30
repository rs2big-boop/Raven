#pragma once
#include <string>

namespace raven::platform {

std::string os_name();
std::string os_arch();
int cpu_cores();
std::string hostname();

} // namespace raven::platform
