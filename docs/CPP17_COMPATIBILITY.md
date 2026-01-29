# C++17 Compatibility Version

This document shows required changes if targeting C++17 instead of C++20.

## Changes Required

### 1. Replace std::span with vector references

**File: config_container.hpp**

```cpp
// Remove this include:
// #include <span>

// Replace std::span methods with vector access:

// OLD (C++20):
std::span<T> items() {
    return {items_.data(), items_.size()};
}

std::span<const T> items() const {
    return {items_.data(), items_.size()};
}

std::span<const std::string> names() const {
    return {names_.data(), names_.size()};
}

// NEW (C++17):
const std::vector<T>& items() const {
    return items_;
}

std::vector<T>& items() {
    return items_;
}

const std::vector<std::string>& names() const {
    return names_;
}
```

### 2. Update usage in config_creature_modern.hpp

```cpp
// OLD (C++20):
std::span<const CreatureModelConfig> get_creatures() const {
    return creatures_.items();
}

// NEW (C++17):
const std::vector<CreatureModelConfig>& get_creatures() const {
    return creatures_.items();
}
```

### 3. Update demo program

**File: config_migration_demo.cpp**

```cpp
// OLD (C++20):
std::span<const NewCreatureConfig> creatures = new_creatures.items();

// NEW (C++17):
const std::vector<NewCreatureConfig>& creatures = new_creatures.items();
```

## Trade-offs

### What You Lose
- **std::span** - Zero-cost non-owning array view
  - Must return vector by reference instead
  - Slightly less flexible API

### What You Keep
- ✅ **std::optional** - Type-safe optional values
- ✅ **std::string_view** - Efficient string views
- ✅ **std::filesystem** - Modern path handling
- ✅ **std::vector** - Dynamic arrays with RAII
- ✅ **std::unordered_map** - Hash tables
- ✅ All core functionality

## Performance Impact

**Minimal** - Returning vector by reference has same performance as std::span in most cases:
- No copying (reference semantics)
- Same cache locality
- Same iteration performance
- Only difference: Can't create sub-spans

## Compiler Requirements

- GCC 7+ (released May 2017)
- Clang 5+ (released September 2017)
- MSVC 2017 15.3+ (released August 2017)

Much broader compatibility than C++20 (requires GCC 10+).

## Makefile Change

```makefile
# Line 429
CXXFLAGS = $(CXXINCS) -c -std=gnu++17 -fmessage-length=0 $(WARNFLAGS) $(DEPFLAGS) $(OPTFLAGS) $(DBGFLAGS) $(FTEST_DBGFLAGS) $(INCFLAGS)
```

## When to Choose C++17

Choose C++17 if:
- ✓ Target compiler is GCC 7-9 or Clang 5-12
- ✓ Need broader compatibility
- ✓ Can accept slight API differences

Choose C++20 if:
- ✓ Target compiler is GCC 10+ or Clang 13+
- ✓ Want perfect API consistency
- ✓ Want zero-cost span semantics
