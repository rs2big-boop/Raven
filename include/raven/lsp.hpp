#pragma once

#include <iostream>
#include <string>

namespace raven {

class LanguageServer {
public:
    static void run(std::istream& in, std::ostream& out);
};

} // namespace raven
