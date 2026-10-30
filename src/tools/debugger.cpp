#include "raven/tools/debugger.hpp"
#include <fstream>
#include <sstream>

namespace raven {

int run_debugger(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filepath << "' for debugging.\n";
        return 1;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    Debugger dbg(filepath, ss.str());
    return dbg.start_interactive();
}

} // namespace raven
