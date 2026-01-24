# Local Development Configuration

## Quick Start

1. Copy the template:
   ```bash
   cp config.local.mk.template config.local.mk
   ```

2. Edit `config.local.mk` with your preferences

3. Build normally:
   ```bash
   make
   ```

## What is config.local.mk?

`config.local.mk` is a **gitignored** file for your personal development settings. It's automatically included by the main Makefile before any other configuration.

## Available Settings

### ImGui Debug Overlay
```makefile
ENABLE_IMGUI = 1  # Enable ImGui (requires deps/imgui/ to be vendored)
```

### Debug Build
```makefile
DEBUG = 1  # Enable debug symbols, disable optimizations
```

### Functional Testing
```makefile
FTEST_DEBUG = 1  # Enable functional testing framework
```

### Custom Flags
```makefile
EXTRA_CFLAGS = -DMY_FLAG=1
EXTRA_CXXFLAGS = -DOTHER_FLAG
```

### Build Metadata
```makefile
BUILD_NUMBER = 12345
PACKAGE_SUFFIX = MyBuild
```

## Why Use This?

- **No more command-line arguments**: Set your preferences once
- **Personal to you**: Won't affect other developers
- **Never committed**: Safe to put local paths or settings
- **Override everything**: Takes precedence over Makefile defaults

## Example Configurations

### Full Debug Build with ImGui
```makefile
ENABLE_IMGUI = 1
DEBUG = 1
```

### Fast Development Build
```makefile
ENABLE_IMGUI = 1
# Keep optimizations on for faster gameplay
```

### Testing Configuration
```makefile
FTEST_DEBUG = 1
BUILD_NUMBER = 9999
PACKAGE_SUFFIX = Test
```

## Notes

- This file is **optional** - the build works fine without it
- Settings in `config.local.mk` override Makefile defaults
- You can still override with command-line: `make DEBUG=0` 
- Don't commit `config.local.mk` - it's gitignored for a reason!
