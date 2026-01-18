cmake_minimum_required(VERSION 3.14)
project(kotlin-lite VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. 定义核心库 ---
add_library(kotlin_lite_lib 
    src/compiler.cpp
    src/lexer/lexer.cpp
    src/parser/parser.cpp
    src/semantic/semantic_analyzer.cpp
)
target_include_directories(kotlin_lite_lib PUBLIC src)

# --- 2. 定义主程序 ---
add_executable(kotlin-lite main.cpp)
target_link_libraries(kotlin-lite PRIVATE kotlin_lite_lib)

# --- 3. GTest 集成 ---
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# --- 4. 定义测试程序 ---
add_executable(unit_tests 
    tests/test_main.cpp
    tests/lexer/test_lexer.cpp
    tests/parser/test_parser.cpp
    tests/semantic/test_semantic.cpp
)
target_link_libraries(unit_tests 
    PRIVATE 
    kotlin_lite_lib 
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unit_tests)