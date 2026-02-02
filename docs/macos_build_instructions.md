# Building KeeperFX on macOS

This guide explains how to build KeeperFX on macOS (both Intel and Apple Silicon) using cross-compilation and run it with Wine.

## Table of Contents
1. [Prerequisites](#prerequisites)
2. [Installing Dependencies](#installing-dependencies)
3. [Building KeeperFX](#building-keeperfx)
4. [Running with Wine](#running-with-wine)
5. [VSCode Integration](#vscode-integration)
6. [Troubleshooting](#troubleshooting)

## Prerequisites

- macOS 10.15 (Catalina) or later
- [Homebrew](https://brew.sh/) package manager
- Xcode Command Line Tools: `xcode-select --install`

### Quick Dependency Check

After cloning the repository, you can run the dependency checker to see what's installed:
```bash
./check_macos_deps.sh
```

This script will check for all required dependencies and provide installation instructions for any that are missing.

## Installing Dependencies

### 1. Install Homebrew (if not already installed)
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### 2. Install MinGW Cross-Compiler
The MinGW cross-compiler allows you to compile Windows executables on macOS:
```bash
brew install mingw-w64
```

This will install the cross-compiler toolchain including:
- `i686-w64-mingw32-gcc` - C compiler
- `i686-w64-mingw32-g++` - C++ compiler
- `i686-w64-mingw32-windres` - Resource compiler

### 3. Install Wine (Latest Version)
Wine allows you to run Windows applications on macOS:

**For Intel Macs:**
```bash
brew install --cask wine-stable
```

**For Apple Silicon Macs:**
Wine requires Rosetta 2 for best compatibility:
```bash
# Install Rosetta 2 if not already installed
softwareupdate --install-rosetta

# Install Wine
brew install --cask wine-stable
```

### 4. Install Additional Build Tools
```bash
brew install make coreutils
```

### 5. Verify Installation
```bash
# Check MinGW
i686-w64-mingw32-gcc --version

# Check Wine
wine --version

# Check Make
make --version
```

## Building KeeperFX

### Using the Command Line

1. **Clone the repository** (if you haven't already):
   ```bash
   git clone https://github.com/dkfans/keeperfx.git
   cd keeperfx
   ```

2. **Build the standard version**:
   ```bash
   make all -j$(sysctl -n hw.ncpu)
   ```
   
   Or build specific versions:
   ```bash
   # Standard release version
   make standard -j$(sysctl -n hw.ncpu)
   
   # Heavy logging version (for debugging)
   make heavylog -j$(sysctl -n hw.ncpu)
   
   # Debug build with symbols
   make standard DEBUG=1 -j$(sysctl -n hw.ncpu)
   ```

3. **The compiled executables** will be in the `bin/` directory:
   - `bin/keeperfx.exe` - Standard version
   - `bin/keeperfx_hvlog.exe` - Heavy logging version

### Clean Build
If you need to clean previous build artifacts:
```bash
make clean
```

## Running with Wine

### Setup Game Directory

1. **Create Wine prefix and game directory**:
   ```bash
   # Wine will create ~/.wine directory on first run
   mkdir -p ~/.wine/drive_c/dk
   ```

2. **Copy original Dungeon Keeper files**:
   Copy your original Dungeon Keeper game files to `~/.wine/drive_c/dk/`
   
   You need files from the original game such as:
   - `data/` directory
   - Sound and music files
   - Other original game assets

3. **Copy KeeperFX binaries**:
   ```bash
   cp bin/* ~/.wine/drive_c/dk/
   ```

### Running the Game

```bash
cd ~/.wine/drive_c/dk
wine keeperfx.exe
```

With command-line arguments:
```bash
wine keeperfx.exe -level 00001 -campaign keeporig -nointro -alex -altinput
```

### Debugging with Wine

For debugging, use `winedbg`:
```bash
winedbg --gdb keeperfx.exe
# When prompted, press 'C' then Enter to continue
```

## VSCode Integration

The project includes VSCode configuration for macOS development.

### Initial Setup

1. **Open the project in VSCode**:
   ```bash
   code .
   ```

2. **The first time you open the project**, VSCode will automatically:
   - Create `.vscode/settings.json`
   - Create `.vscode/launch.json`
   - Create `.vscode/macscript.sh` from the template

3. **Edit `.vscode/macscript.sh`** to customize:
   - Game directory path (default: `~/.wine/drive_c/dk/`)
   - Command-line arguments for the game

### Building with VSCode

- **Press `Cmd+Shift+B`** (or select "Terminal" > "Run Build Task")
- This will:
  1. Cross-compile KeeperFX for Windows
  2. Copy binaries to your game directory
  3. Launch the game with Wine (if installed)

### Other VSCode Tasks

Available tasks (access via `Cmd+Shift+P` > "Tasks: Run Task"):
- **Default task** - Build and run the game
- **Clean** - Clean build artifacts
- **Generate compile_commands.json** - For IntelliSense support

## Troubleshooting

### MinGW Compiler Not Found

**Error**: `i686-w64-mingw32-gcc: command not found`

**Solution**:
```bash
brew install mingw-w64
# Verify installation
which i686-w64-mingw32-gcc
```

### Wine Not Starting

**Error**: Wine fails to start or shows errors

**Solutions**:
1. **For Apple Silicon Macs**, ensure Rosetta 2 is installed:
   ```bash
   softwareupdate --install-rosetta
   ```

2. **Try initializing Wine**:
   ```bash
   wine wineboot
   ```

3. **Check Wine configuration**:
   ```bash
   winecfg
   ```

### Missing Dependencies During Build

**Error**: Missing headers or libraries during compilation

**Solution**: The Makefile automatically downloads required dependencies. If you encounter issues:
```bash
make clean
make libexterns  # Download external libraries
make all
```

### Performance Issues with Wine

Wine performance can vary. For best results:
1. Use the latest Wine version: `brew upgrade wine-stable`
2. Try Wine Staging for better gaming support: `brew install --cask wine-staging`
3. Consider using [CrossOver](https://www.codeweavers.com/crossover) (commercial Wine distribution)

### Game Crashes or Graphics Issues

1. **Try different Wine versions**:
   ```bash
   brew uninstall --cask wine-stable
   brew install --cask wine-staging
   ```

2. **Configure Wine settings**:
   ```bash
   winecfg
   ```
   - Try different Windows versions
   - Adjust graphics settings

3. **Check game logs**:
   ```bash
   tail -f ~/.wine/drive_c/dk/keeperfx.log
   ```

### Compilation Errors

1. **Ensure all dependencies are installed**:
   ```bash
   brew list mingw-w64
   ```

2. **Update Homebrew and packages**:
   ```bash
   brew update
   brew upgrade
   ```

3. **Clean and rebuild**:
   ```bash
   make clean
   make all -j$(sysctl -n hw.ncpu)
   ```

## Additional Notes

### Architecture Support

- **Intel Macs**: Native support, best performance
- **Apple Silicon (M1/M2/M3)**: Requires Rosetta 2, performance may vary

### Wine Alternatives

If Wine doesn't work well for you, consider:
- **CrossOver**: Commercial Wine distribution with better game support
- **Parallels Desktop**: Run Windows in a virtual machine
- **Boot Camp**: Dual-boot Windows (Intel Macs only)

### Contributing

If you improve the macOS build process or Wine integration, please consider contributing back to the project!

## Resources

- [KeeperFX Wiki](https://github.com/dkfans/keeperfx/wiki)
- [Wine HQ](https://www.winehq.org/)
- [Homebrew Documentation](https://docs.brew.sh/)
- [MinGW-w64](https://www.mingw-w64.org/)

## Support

For issues specific to macOS builds:
1. Check the [GitHub Issues](https://github.com/dkfans/keeperfx/issues)
2. Join the [Keeper Klan Discord](https://discord.gg/hE4p7vy2Hb)
3. Ask in the KeeperFX development channel
