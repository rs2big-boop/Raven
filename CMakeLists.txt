cmake_minimum_required(VERSION 3.20)
project(MiniLang VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Core Library
add_library(minilang_core
    src/lexer.cpp
)

target_include_directories(minilang_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# CLI Binary
add_executable(minilang
    src/main.cpp
)

target_link_libraries(minilang PRIVATE minilang_core)

# Tests
find_package(GTest REQUIRED)
include(GoogleTest)
enable_testing()

add_executable(lexer_tests
    tests/lexer_tests.cpp
)

target_link_libraries(lexer_tests PRIVATE
    minilang_core
    GTest::gtest_main
)

gtest_discover_tests(lexer_tests)
