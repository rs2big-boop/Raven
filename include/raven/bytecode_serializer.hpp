#pragma once

#include <cstdint>
#include <iostream>
#include <string>

#include "raven/bytecode.hpp"

namespace raven {

class BytecodeSerializer {
public:
    inline static constexpr uint32_t MAGIC = 0x4356527F; // "\x7fRVC" in little-endian
    inline static constexpr uint32_t VERSION = 1;

    static bool serialize(const Chunk& chunk, std::ostream& out);
    static bool deserialize(std::istream& in, Chunk& chunk);

    static bool save_to_file(const std::string& path, const Chunk& chunk);
    static bool load_from_file(const std::string& path, Chunk& chunk);
};

} // namespace raven
