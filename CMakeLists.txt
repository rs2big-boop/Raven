cmake_minimum_required(VERSION 3.20)
project(Raven VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Core Library
add_library(raven_core
    # Frontend (Lexer, Parser, AST, Diagnostics)
    src/frontend/lexer/lexer.cpp
    src/frontend/lexer/token.cpp
    src/frontend/parser/parser.cpp
    src/frontend/parser/parser_error.cpp
    src/frontend/ast/expressions.cpp
    src/frontend/ast/statements.cpp
    src/frontend/diagnostics/diagnostic.cpp
    src/frontend/diagnostics/source_location.cpp

    # Semantic Analysis (Resolver, Scopes, Symbol Table, Types, Analyzer)
    src/semantic/symbols.cpp
    src/semantic/scopes.cpp
    src/semantic/resolver.cpp
    src/semantic/analyzer.cpp
    src/semantic/types.cpp

    # Compiler & Optimization (AST -> Bytecode, Constant Folding, Serialization)
    src/compiler/compiler.cpp
    src/compiler/bytecode_generator.cpp
    src/compiler/optimizer.cpp
    src/compiler/bytecode_serializer.cpp

    # Intermediate Representation (IR) Subsystem & Passes
    src/ir/ir_node.cpp
    src/ir/ir_builder.cpp
    src/ir/passes/constant_folding.cpp
    src/ir/passes/dead_code_elimination.cpp
    src/ir/passes/common_subexpression.cpp

    # Tree-Walk Execution Engine
    src/interpreter/interpreter.cpp
    src/interpreter/evaluator.cpp
    src/interpreter/environment.cpp

    # Corvus Virtual Machine Engine
    src/vm/vm.cpp
    src/vm/engine.cpp
    src/vm/stack.cpp
    src/vm/frames.cpp
    src/vm/opcodes.cpp
    src/vm/loader.cpp
    src/vm/verifier.cpp

    # Concurrency Subsystem
    src/concurrency/channel.cpp
    src/concurrency/wait_group.cpp
    src/concurrency/mutex.cpp
    src/concurrency/spawn_registry.cpp
    src/concurrency/thread.cpp
    src/concurrency/task.cpp
    src/concurrency/scheduler.cpp
    src/concurrency/cancellation.cpp

    # Runtime & Memory Management
    src/runtime/values/value.cpp
    src/runtime/values/array.cpp
    src/runtime/values/map.cpp
    src/runtime/values/string.cpp
    src/runtime/values/callable.cpp
    src/runtime/exceptions/runtime_error.cpp
    src/runtime/exceptions/stack_trace.cpp
    src/runtime/modules/module_loader.cpp
    src/runtime/builtins/builtin_registry.cpp
    src/runtime/memory/memory_manager.cpp
    src/runtime/memory/root_registry.cpp
    src/runtime/memory/managed_heap.cpp
    src/runtime/memory/object_header.cpp
    src/runtime/memory/reference_counter.cpp
    src/runtime/memory/cycle_collector.cpp

    # Package Management Subsystem
    src/packages/lockfile.cpp
    src/packages/dependency_resolver.cpp
    src/packages/package_manager.cpp

    # Platform Subsystem (Sockets, HTTP Client, Crypto, Filesystem, System Info)
    src/platform/crypto.cpp
    src/platform/sockets.cpp
    src/platform/http_client.cpp
    src/platform/filesystem.cpp
    src/platform/system_info.cpp

    # Developer Tooling (Linter/Vet, Formatter, Documentation Generator, Language Server, Debugger)
    src/tools/linter.cpp
    src/tools/formatter.cpp
    src/tools/doc_generator.cpp
    src/tools/lsp.cpp
    src/tools/debugger.cpp
)

target_include_directories(raven_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Windows: suppress min/max macros and enable Unicode
if(WIN32)
    target_compile_definitions(raven_core PUBLIC
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
        UNICODE
        _UNICODE
    )
endif()

# CLI Binaries (like Go's cmd/)
add_executable(raven src/cmd/raven/main.cpp)
target_link_libraries(raven PRIVATE raven_core)

add_executable(ravenc src/cmd/ravenc/main.cpp)
target_link_libraries(ravenc PRIVATE raven_core)

add_executable(rvm src/cmd/rvm/main.cpp)
target_link_libraries(rvm PRIVATE raven_core)

add_executable(rvndb src/cmd/rvndb/main.cpp)
target_link_libraries(rvndb PRIVATE raven_core)

# Tests
option(RAVEN_BUILD_TESTS "Build Raven test suite" ON)
if(CMAKE_CROSSCOMPILING)
    set(RAVEN_BUILD_TESTS OFF)
endif()

if(RAVEN_BUILD_TESTS)
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        message(STATUS "GTest not found; skipping test targets.")
        set(RAVEN_BUILD_TESTS OFF)
    endif()
endif()

if(RAVEN_BUILD_TESTS)
    include(GoogleTest)
    enable_testing()

    add_executable(lexer_tests tests/lexer_tests.cpp)
    target_link_libraries(lexer_tests PRIVATE raven_core GTest::gtest_main)
    gtest_discover_tests(lexer_tests)

    add_executable(parser_tests tests/parser_tests.cpp)
    target_link_libraries(parser_tests PRIVATE raven_core GTest::gtest_main)
    gtest_discover_tests(parser_tests)

    add_executable(interpreter_tests tests/interpreter_tests.cpp)
    target_link_libraries(interpreter_tests PRIVATE raven_core GTest::gtest_main)
    gtest_discover_tests(interpreter_tests)

    add_executable(memory_tests tests/memory/memory_tests.cpp)
    target_link_libraries(memory_tests PRIVATE raven_core GTest::gtest_main)
    gtest_discover_tests(memory_tests)

    add_executable(semantic_tests tests/semantic/semantic_tests.cpp)
    target_link_libraries(semantic_tests PRIVATE raven_core GTest::gtest_main)
    gtest_discover_tests(semantic_tests)

    add_executable(package_tests tests/packages/package_tests.cpp)
    target_link_libraries(package_tests PRIVATE raven_core GTest::gtest_main)
    gtest_discover_tests(package_tests)

    add_executable(ir_tests tests/ir/ir_tests.cpp)
    target_link_libraries(ir_tests PRIVATE raven_core GTest::gtest_main)
    gtest_discover_tests(ir_tests)

    add_executable(differential_tests tests/differential/differential_tests.cpp)
    target_link_libraries(differential_tests PRIVATE raven_core GTest::gtest_main)
    gtest_discover_tests(differential_tests)
endif()

# Installation targets
include(GNUInstallDirs)
install(TARGETS raven ravenc rvm
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY stdlib/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/raven/stdlib
    FILES_MATCHING PATTERN "*.rvn"
)
install(DIRECTORY include/raven/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/raven
    FILES_MATCHING PATTERN "*.hpp"
)
install(TARGETS raven_core
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
