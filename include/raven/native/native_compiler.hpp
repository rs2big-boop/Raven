#pragma once

#include <memory>
#include <string>
#include <vector>

#include "raven/ast.hpp"

namespace raven::native {

struct NativeCompilerOptions {
    std::string output_path;
    bool optimize = true;
    bool emit_c = false;
    std::string c_output_path;
    std::string host_compiler;
    std::vector<std::string> extra_include_dirs;
    std::vector<std::string> extra_lib_dirs;
    std::vector<std::string> extra_cxx_flags;
    bool verbose = false;
};

class NativeCompiler {
public:
    static std::string compile_to_c(const std::vector<std::shared_ptr<Stmt>>& statements,
                                    const std::string& module_name = "main");

    static bool compile_to_file(const std::string& input_path,
                                const std::string& output_path,
                                const NativeCompilerOptions& options = {});

    static std::string find_host_compiler();
    static std::string find_raven_lib();
    static std::string find_raven_include_dir();
};

} // namespace raven::native
