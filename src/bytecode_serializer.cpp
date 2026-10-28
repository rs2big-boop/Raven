#include "raven/bytecode_serializer.hpp"

#include <cstring>
#include <fstream>

namespace raven {

enum class SerializedType : uint8_t {
    Nil = 0,
    Bool = 1,
    Number = 2,
    String = 3
};

template <typename T>
static void write_raw(std::ostream& out, const T& value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template <typename T>
static bool read_raw(std::istream& in, T& value) {
    in.read(reinterpret_cast<char*>(&value), sizeof(T));
    return in.good();
}

bool BytecodeSerializer::serialize(const Chunk& chunk, std::ostream& out) {
    // 1. Magic & Version
    write_raw(out, MAGIC);
    write_raw(out, VERSION);

    // 2. Constant pool
    uint32_t const_count = static_cast<uint32_t>(chunk.constants.size());
    write_raw(out, const_count);

    for (const auto& val : chunk.constants) {
        if (val.is_nil()) {
            write_raw(out, SerializedType::Nil);
        } else if (val.is_bool()) {
            write_raw(out, SerializedType::Bool);
            uint8_t b = val.as_bool() ? 1 : 0;
            write_raw(out, b);
        } else if (val.is_number()) {
            write_raw(out, SerializedType::Number);
            double num = val.as_number();
            write_raw(out, num);
        } else if (val.is_string()) {
            write_raw(out, SerializedType::String);
            const std::string& str = val.as_string();
            uint32_t len = static_cast<uint32_t>(str.size());
            write_raw(out, len);
            out.write(str.data(), len);
        } else {
            // Unserializable complex type -> emit nil
            write_raw(out, SerializedType::Nil);
        }
    }

    // 3. Bytecode instructions
    uint32_t code_size = static_cast<uint32_t>(chunk.code.size());
    write_raw(out, code_size);
    if (code_size > 0) {
        out.write(reinterpret_cast<const char*>(chunk.code.data()), code_size);
    }

    // 4. Source line numbers
    uint32_t lines_size = static_cast<uint32_t>(chunk.lines.size());
    write_raw(out, lines_size);
    for (int line : chunk.lines) {
        int32_t l = line;
        write_raw(out, l);
    }

    return out.good();
}

bool BytecodeSerializer::deserialize(std::istream& in, Chunk& chunk) {
    chunk.code.clear();
    chunk.constants.clear();
    chunk.lines.clear();

    // 1. Magic & Version
    uint32_t magic = 0;
    if (!read_raw(in, magic) || magic != MAGIC) {
        return false;
    }

    uint32_t version = 0;
    if (!read_raw(in, version) || version != VERSION) {
        return false;
    }

    // 2. Constant pool
    uint32_t const_count = 0;
    if (!read_raw(in, const_count)) return false;

    for (uint32_t i = 0; i < const_count; ++i) {
        SerializedType type;
        if (!read_raw(in, type)) return false;

        switch (type) {
            case SerializedType::Nil:
                chunk.constants.push_back(Value());
                break;
            case SerializedType::Bool: {
                uint8_t b = 0;
                if (!read_raw(in, b)) return false;
                chunk.constants.push_back(Value(b != 0));
                break;
            }
            case SerializedType::Number: {
                double num = 0.0;
                if (!read_raw(in, num)) return false;
                chunk.constants.push_back(Value(num));
                break;
            }
            case SerializedType::String: {
                uint32_t len = 0;
                if (!read_raw(in, len)) return false;
                std::string str(len, '\0');
                in.read(&str[0], len);
                if (!in.good()) return false;
                chunk.constants.push_back(Value(std::move(str)));
                break;
            }
        }
    }

    // 3. Bytecode instructions
    uint32_t code_size = 0;
    if (!read_raw(in, code_size)) return false;
    chunk.code.resize(code_size);
    if (code_size > 0) {
        in.read(reinterpret_cast<char*>(chunk.code.data()), code_size);
        if (!in.good()) return false;
    }

    // 4. Source lines
    uint32_t lines_size = 0;
    if (!read_raw(in, lines_size)) return false;
    chunk.lines.resize(lines_size);
    for (uint32_t i = 0; i < lines_size; ++i) {
        int32_t line = 0;
        if (!read_raw(in, line)) return false;
        chunk.lines[i] = line;
    }

    return true;
}

bool BytecodeSerializer::save_to_file(const std::string& path, const Chunk& chunk) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    return serialize(chunk, file);
}

bool BytecodeSerializer::load_from_file(const std::string& path, Chunk& chunk) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    return deserialize(file, chunk);
}

} // namespace raven
