#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

namespace raven::platform {

// Low-level OS environment variable manipulation (POSIX & Windows)
std::optional<std::string> get_env_var(const std::string& name);
bool set_env_var(const std::string& name, const std::string& value, bool overwrite = true);
bool unset_env_var(const std::string& name);
bool has_env_var(const std::string& name);
std::unordered_map<std::string, std::string> get_all_env_vars();

// Dotenv interpolation: expands ${VAR}, ${VAR:-default}, $VAR
// Replaces escaped \$ with literal $
std::string interpolate_env_value(
    const std::string& raw_val,
    const std::unordered_map<std::string, std::string>& context
);

// Parses a .env format string into a map of key-value pairs.
// Handles comments (#), quotes ('', "", ``), escapes, multiline values, and interpolation.
std::unordered_map<std::string, std::string> parse_env_string(
    const std::string& content,
    const std::unordered_map<std::string, std::string>& base_env = {}
);

// Reads a .env file from disk, parses it, and applies it to the process environment.
// Returns the map of variables loaded.
std::unordered_map<std::string, std::string> load_env_file(
    const std::string& path,
    bool overwrite = true
);

// Searches for a .env file starting at start_dir and traversing parent directories.
std::optional<std::string> find_env_file(
    const std::string& start_dir,
    const std::string& filename = ".env"
);

// Formats a key-value map into standard .env format string.
std::string format_env_string(const std::unordered_map<std::string, std::string>& vars);

// Writes a key-value map to a .env file.
bool write_env_file(const std::string& path, const std::unordered_map<std::string, std::string>& vars);

} // namespace raven::platform
