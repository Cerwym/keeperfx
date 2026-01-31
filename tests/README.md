# KeeperFX Unit Testing Guide

## Overview

KeeperFX has a C++ unit testing infrastructure using **CUnit 2.1-3** framework. This allows you to write standalone code backed by automated tests.

## Test Infrastructure

### Framework
- **Test Framework**: CUnit 2.1-3 (included in `deps/CUnit-2.1-3/`)
- **Language**: C++ (C++20 standard)
- **Build System**: Makefile with MinGW cross-compilation
- **Test Runner**: Custom wrapper using CUnit's registry system

### Directory Structure
```
tests/
├── README.md           # This file
├── tst_main.h          # Test registration macros
├── tst_main.cpp        # Test runner main entry point
├── 001_test.cpp        # Example simple tests
├── tst_fixes.cpp       # Bug fix validation tests
├── tst_enet_client.cpp # Network client tests
└── tst_enet_server.cpp # Network server tests
```

## Writing Tests

### Basic Test Structure

Use the `ADD_TEST()` macro to create test functions. Each test is automatically registered with CUnit:

```cpp
#include "tst_main.h"

ADD_TEST(test_my_feature)
{
    // Your test code here
    CU_ASSERT(1 + 1 == 2);
    CU_ASSERT_EQUAL(42, calculate_answer());
}

ADD_TEST(test_another_feature)
{
    int result = some_function();
    CU_ASSERT_NOT_EQUAL(result, 0);
    CU_ASSERT_PTR_NOT_NULL(get_pointer());
}
```

### CUnit Assertions

Common CUnit assertion macros you can use:

| Assertion | Description |
|-----------|-------------|
| `CU_ASSERT(condition)` | Assert condition is true |
| `CU_ASSERT_TRUE(value)` | Assert value is true |
| `CU_ASSERT_FALSE(value)` | Assert value is false |
| `CU_ASSERT_EQUAL(actual, expected)` | Assert equality |
| `CU_ASSERT_NOT_EQUAL(actual, expected)` | Assert inequality |
| `CU_ASSERT_PTR_NULL(ptr)` | Assert pointer is NULL |
| `CU_ASSERT_PTR_NOT_NULL(ptr)` | Assert pointer is not NULL |
| `CU_ASSERT_STRING_EQUAL(actual, expected)` | Assert string equality |
| `CU_ASSERT_DOUBLE_EQUAL(actual, expected, epsilon)` | Assert doubles are equal within epsilon |

For more assertions, see the [CUnit documentation](http://cunit.sourceforge.net/doc/writing_tests.html).

### Test File Naming Convention

- Prefix test files with `tst_` (e.g., `tst_fixes.cpp`, `tst_player.cpp`)
- Use descriptive names that indicate what's being tested
- Group related tests in the same file

## Building Tests

### Prerequisites

You need a MinGW cross-compiler toolchain installed:

```bash
# On Ubuntu/Debian
sudo apt-get install mingw-w64

# On Fedora
sudo dnf install mingw64-gcc mingw64-gcc-c++
```

### Build Command

```bash
make tests
```

This will:
1. Download and extract dependencies (SDL2, CUnit, etc.)
2. Compile all test files in `tests/`
3. Link with CUnit and required libraries
4. Create `bin/tests.exe` (or `bin/tests` on Linux)

### Adding New Test Files

1. Create your test file in the `tests/` directory (e.g., `tst_myfeature.cpp`)
2. Add it to the `TESTS_OBJ` variable in the Makefile:

```makefile
TESTS_OBJ = obj/tests/tst_main.o \
obj/tests/tst_fixes.o \
obj/tests/001_test.o \
obj/tests/tst_enet_server.o \
obj/tests/tst_enet_client.o \
obj/tests/tst_myfeature.o  # <-- Add your new test here
```

## Running Tests

### Basic Execution

```bash
./bin/tests.exe  # On Windows or Wine
wine ./bin/tests.exe  # On Linux with Wine
```

The test runner will:
- Automatically discover all registered tests
- Run them in sequence
- Report pass/fail status for each test
- Print a summary

### Example Output

```
CUnit - A Unit testing framework for C - Version 2.1-3
http://cunit.sourceforge.net/

Suite: tests/001_test.cpp
  Test: test_good ... passed
  Test: test_good2 ... passed

Run Summary:    Type  Total    Ran Passed Failed Inactive
              suites      1      1    n/a      0        0
               tests      2      2      2      0        0
             asserts      2      2      2      0        0

Elapsed time =    0.001 seconds
```

## Best Practices

### 1. Test Organization
- Group related tests in the same file
- Use descriptive test names that explain what's being tested
- Keep tests focused on a single behavior

### 2. Test Independence
- Each test should be independent and not rely on state from other tests
- Clean up resources created during tests
- Don't assume test execution order

### 3. Assertions
- Use specific assertions (e.g., `CU_ASSERT_EQUAL` rather than `CU_ASSERT`)
- Add multiple assertions when testing complex behavior
- Check both success and error conditions

### 4. Testing Game Code
When testing KeeperFX game code:
- Mock or stub external dependencies when possible
- Test logic separately from rendering/SDL code
- Focus on testable units (algorithms, data structures, utilities)

## Example: Testing a Utility Function

```cpp
// In your source: src/utilities.cpp
int clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// In tests/tst_utilities.cpp
#include "tst_main.h"
extern "C" {
    int clamp(int value, int min, int max);
}

ADD_TEST(test_clamp_within_range)
{
    CU_ASSERT_EQUAL(clamp(5, 0, 10), 5);
}

ADD_TEST(test_clamp_below_min)
{
    CU_ASSERT_EQUAL(clamp(-5, 0, 10), 0);
}

ADD_TEST(test_clamp_above_max)
{
    CU_ASSERT_EQUAL(clamp(15, 0, 10), 10);
}

ADD_TEST(test_clamp_boundary_values)
{
    CU_ASSERT_EQUAL(clamp(0, 0, 10), 0);
    CU_ASSERT_EQUAL(clamp(10, 0, 10), 10);
}
```

## Troubleshooting

### Build Errors

**Error: `i686-w64-mingw32-g++: not found`**
- Solution: Install MinGW cross-compiler (see Prerequisites)

**Error: Missing dependencies**
- Solution: Run `make -f libexterns.mk` to download dependencies

### Runtime Errors

**Error: Test binary doesn't run**
- On Linux: Try running with Wine: `wine ./bin/tests.exe`
- On Windows: Ensure SDL2.dll is in the same directory or in PATH

## Alternative Testing Approaches

While CUnit is the current framework, you could also consider:
- **Google Test (gtest)**: Modern C++ testing framework with rich assertions
- **Catch2**: Header-only C++ testing framework with BDD-style tests
- **Unity**: Lightweight C testing framework

To add a different framework, you would need to:
1. Add the framework to `deps/`
2. Update the Makefile to link against it
3. Create a new test runner or adapt `tst_main.cpp`

## Contributing

When adding new tests:
1. Follow the existing code style
2. Add tests for bug fixes to prevent regressions
3. Test both success and failure cases
4. Update this README if adding new patterns or tools

## Resources

- [CUnit Documentation](http://cunit.sourceforge.net/)
- [CUnit API Reference](http://cunit.sourceforge.net/doc/writing_tests.html)
- KeeperFX Build Instructions: `docs/build_instructions.txt`
