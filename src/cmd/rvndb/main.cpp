#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "raven/debugger.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: rvndb <source.rvn> [-b <line>]... [--no-stop-on-entry]\n";
        return 1;
    }

    std::string path = argv[1];
    std::vector<size_t> initial_breakpoints;
    bool stop_on_entry = true;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-b" || arg == "--break") && i + 1 < argc) {
            try {
                initial_breakpoints.push_back(std::stoul(argv[++i]));
            } catch (...) {}
        } else if (arg == "--no-stop-on-entry") {
            stop_on_entry = false;
        }
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << path << "' for debugging.\n";
        return 1;
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    raven::Debugger dbg(path, ss.str());
    for (size_t bp : initial_breakpoints) {
        dbg.add_breakpoint(bp);
    }
    dbg.set_stop_on_entry(stop_on_entry);
    return dbg.start_interactive();
}
