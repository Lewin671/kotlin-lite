# Development Guide

This document provides guidelines for developing and testing the `kotlin-lite` project.

## Project Structure

- `src/`: Core library logic (`kotlin_lite_lib`).
- `main.cpp`: Main executable entry point.
- `tests/`: Unit tests using Google Test.

## How to Write Unit Tests

We use [Google Test (GTest)](https://github.com/google/googletest) for our unit testing framework.

### 1. Add a Test Case
Tests are located in the `tests/` directory. You can add new test cases to existing files or create new `.cpp` files.

Example of a test case:
```cpp
#include <gtest/gtest.h>
#include "compiler.hpp"

TEST(TestSuiteName, TestName) {
    // Arrange
    std::string expected = "Hello from kotlin-lite library!";
    
    // Act
    std::string actual = kotlin_lite::getGreeting();
    
    // Assert
    EXPECT_EQ(actual, expected);
}
```

### 2. Register New Test Files
If you create a new test file (e.g., `tests/test_lexer.cpp`), add it to the `unit_tests` executable in `CMakeLists.txt`:

```cmake
add_executable(unit_tests 
    tests/test_main.cpp 
    tests/test_lexer.cpp  # Add your new file here
)
```

## How to Run Tests

### Using CMake/CTest
From the root of the project:

```bash
# 1. Configure and build
mkdir -p build && cd build
cmake ..
make unit_tests

# 2. Run all tests
ctest --output-on-failure
```

### Running the Test Binary Directly
For more detailed output (like individual test results and timing):

```bash
./build/unit_tests
```

## Best Practices
- **Test Logic, Not Entry Points**: Keep your business logic in `src/` so it can be linked to the `unit_tests` target. Avoid putting logic in `main.cpp`.
- **Use Meaningful Names**: Test suite names should represent the component (e.g., `LexerTest`), and test names should represent the behavior (e.g., `HandlesEmptyInput`).
- **ASSERT vs EXPECT**: 
    - Use `EXPECT_*` (e.g., `EXPECT_EQ`) for non-fatal failures; the test continues.
    - Use `ASSERT_*` (e.g., `ASSERT_NE`) for fatal failures; the test stops immediately if it fails.
