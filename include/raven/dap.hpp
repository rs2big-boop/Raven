#pragma once
#include <iostream>
#include <string>

namespace raven {

void run_dap_server(const std::string& source_file, std::istream& in, std::ostream& out);

} // namespace raven
