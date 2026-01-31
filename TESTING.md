# Testing in KeeperFX

KeeperFX has a comprehensive C++ unit testing infrastructure that allows you to write standalone code backed by automated tests.

## Quick Start

### 1. Write Your Test

Create a test file in the `tests/` directory:

```cpp
#include "tst_main.h"

ADD_TEST(test_my_feature)
{
    // Your test code
    CU_ASSERT_EQUAL(1 + 1, 2);
}
```

### 2. Add to Build

Add your test file to `TESTS_OBJ` in the Makefile:

```makefile
TESTS_OBJ = obj/tests/tst_main.o \
    ... \
    obj/tests/tst_myfeature.o
```

### 3. Build and Run

```bash
# Build tests
make tests

# Run tests (requires MinGW or Wine on Linux)
./bin/tests.exe
```

## Complete Documentation

See **[tests/README.md](tests/README.md)** for:
- Complete testing guide
- CUnit assertion reference
- Example tests
- Best practices
- Troubleshooting tips

## Example Tests

Check out these examples:
- `tests/tst_example_standalone.cpp` - Comprehensive examples of testing standalone code
- `tests/001_test.cpp` - Simple test examples
- `tests/tst_fixes.cpp` - Bug fix validation tests

## Testing Infrastructure

- **Framework**: CUnit 2.1-3
- **Language**: C++ (C++20)
- **Location**: `tests/` directory
- **Build System**: Makefile target `make tests`

---

**Ready to write tests?** Start with the [full testing guide](tests/README.md).
