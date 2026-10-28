cmake_minimum_required(VERSION 3.20)
project(Raven VERSION 0.3.0 LANGUAGES CXX)

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
    src/lexer.cpp
    src/parser.cpp
    src/interpreter.cpp
    src/compiler.cpp
    src/optimizer.cpp
    src/bytecode_serializer.cpp
    src/vm.cpp
    src/linter.cpp
    src/formatter.cpp
    src/doc_generator.cpp
)

target_include_directories(raven_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Unified CLI Tool (raven)
add_executable(raven src/main.cpp)
target_link_libraries(raven PRIVATE raven_core)

# Dedicated Bytecode Compiler (ravenc)
add_executable(ravenc src/ravenc_main.cpp)
target_link_libraries(ravenc PRIVATE raven_core)

# Dedicated Raven Virtual Machine (rvm)
add_executable(rvm src/rvm_main.cpp)
target_link_libraries(rvm PRIVATE raven_core)

# Tests
find_package(GTest REQUIRED)
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
