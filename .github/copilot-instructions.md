# KeeperFX - Copilot Instructions

## Project Overview
KeeperFX (Dungeon Keeper Fan eXpansion) is an open-source project that enhances and modernizes the classic dungeon management game, Dungeon Keeper. The codebase is a complete rewrite of the original game, providing improved performance, modern features, and extensive modding capabilities.

## Technology Stack
- **Languages**: C (C11), C++ (C++20)
- **Build Systems**: 
  - Make (primary, using Makefile with MinGW)
  - CMake (modern build system with CMakePresets.json)
- **Key Dependencies**:
  - SDL2, SDL2_image, SDL2_mixer, SDL2_net
  - Lua (for scripting)
  - ENet (for networking)
- **Target Platform**: Windows (32-bit executable, cross-platform development supported)

## Building the Project

### Using Make (Windows or Linux cross-compile)
```bash
# Standard release build
make standard

# Verbose logging version
make heavylog

# Debug build
make standard DEBUG=1

# Clean build artifacts
make clean

# Build package for release
make clean && make standard && make heavylog && make package
```

### Using CMake
```bash
# Configure and build (don't build in root directory)
cmake --preset <preset-name>
cmake --build <build-dir>
```

## Code Style and Conventions

### Formatting
- **Indentation**: 4 spaces (no tabs), as defined in `.editorconfig`
- **Line Length**: Maximum 120 characters
- **Braces**: Opening brace on new line for function declarations
- **Spacing**: Space after control flow keywords (`if`, `for`, `while`, etc.)
- **File Organization**: Header files (`.h`) and implementation files (`.c`/`.cpp`)

### Naming Conventions
- **Functions**: `snake_case` (e.g., `creature_battle_init`, `map_block_get`)
- **Variables**: `snake_case` (e.g., `player_id`, `creature_health`)
- **Structs/Types**: `PascalCase` with descriptive names
- **Constants/Macros**: `UPPER_CASE` (e.g., `MAX_CREATURES`, `DEBUG_LEVEL`)
- **File Names**: `snake_case` with descriptive module names (e.g., `creature_battle.c`, `map_blocks.h`)

### Code Organization
- Source files organized by module/feature (e.g., `creature_*.c`, `room_*.c`, `config_*.c`)
- Header guards using `#ifndef` / `#define` / `#endif`
- Include `pre_inc.h` and `post_inc.h` for common definitions
- Keep platform-specific code isolated (e.g., `windows.cpp`, `linux.cpp`)

### Memory Management
- Manual memory management (C-style with `malloc`/`free`)
- Be careful with pointer arithmetic and buffer boundaries
- Use appropriate data structures from `bflib_*` modules

## Testing

### Test Infrastructure
- Unit tests located in `/tests` directory
- Main test runner: `tst_main.cpp`
- Test files follow `tst_*.cpp` naming convention
- Run tests after making changes to validate behavior

### Testing Commands
```bash
# Run test suite (if configured)
# Check CMakeLists.txt or Makefile for test targets
```

## Development Workflow

### Key Directories
- `/src` - Main source code (C/C++ files)
- `/tests` - Unit tests
- `/tools` - Build tools (png2ico, po2ngdat, sndbanker, etc.)
- `/res` - Resource files
- `/docs` - Documentation
- `/config` - Game configuration files
- `/campgns`, `/levels`, `/lang` - Game data

### Making Changes
1. **Understand the Module**: Code is organized by feature (creatures, rooms, maps, etc.)
2. **Maintain Compatibility**: This is a game with existing save files and mods
3. **Test Thoroughly**: Changes can affect gameplay, multiplayer, and mod compatibility
4. **Follow Conventions**: Match existing code style in the files you modify
5. **Document Complex Logic**: Add comments for non-obvious game mechanics or algorithms

### Compiler Warnings
- Code compiles with strict warning flags: `-Wall -W -Wshadow -Werror`
- Address all compiler warnings before committing
- No unused parameters (`-Wno-unused-parameter` is set but avoid adding new ones)

### Debug Builds
- Standard build: `BFDEBUG_LEVEL=0` (minimal logging)
- Heavy log build: `BFDEBUG_LEVEL=10` (verbose logging for debugging)
- Debug symbols: Add `DEBUG=1` to make command

## Important Notes

### Game-Specific Considerations
- This is a real-time strategy game with complex AI and pathfinding
- Performance is critical (game logic runs at specific FPS)
- Multiplayer support requires deterministic behavior
- Modding support means configuration must remain flexible
- Save game compatibility should be maintained when possible

### Cross-Platform Development
- Primary target is Windows (32-bit)
- Can be cross-compiled from Linux using MinGW
- Use platform-independent types and functions where possible
- Test on both compilation environments if available

### External Tools
Several command-line tools are built alongside the main game:
- `sndbanker` - Sound file processing
- `po2ngdat` - Language file conversion
- `png2ico`, `png2bestpal`, `pngpal2raw` - Graphics processing
- `rnctools` - RNC compression
- `dkillconv` - Map conversion (unfinished)

## Contributing
- Report bugs via GitHub issues
- Submit pull requests for bug fixes and features
- Discuss major changes in the Discord (Keeper Klan) development channel
- Code signing provided by SignPath.io for releases

## License
GNU General Public License v2.0 - Be mindful of license compatibility when adding dependencies.
