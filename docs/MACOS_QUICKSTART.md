# KeeperFX on macOS - Quick Start Guide

**TL;DR**: Cross-compile Windows executable on macOS, run with Wine

## Check Your Setup

```bash
# Run dependency checker (after cloning the repo)
./check_macos_deps.sh
```

## One-Time Setup (5 minutes)

```bash
# Install Homebrew (if needed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install build tools
brew install mingw-w64 make coreutils

# Install Wine (for running the game)
brew install --cask wine-stable

# For Apple Silicon Macs, also install Rosetta 2
softwareupdate --install-rosetta
```

## Build & Run

```bash
# Clone repository
git clone https://github.com/dkfans/keeperfx.git
cd keeperfx

# Build (uses all CPU cores)
make all -j$(sysctl -n hw.ncpu)

# Setup game directory
mkdir -p ~/.wine/drive_c/dk
# Copy your original Dungeon Keeper files to ~/.wine/drive_c/dk/

# Copy built game
cp bin/* ~/.wine/drive_c/dk/

# Run with Wine
cd ~/.wine/drive_c/dk
wine keeperfx.exe
```

## VSCode Users

1. Open project in VSCode
2. Edit `.vscode/macscript.sh` (created automatically on first run)
3. Press `Cmd+Shift+B` to build and run

## Common Issues

**"i686-w64-mingw32-gcc not found"**
→ Run: `brew install mingw-w64`

**Wine won't start on Apple Silicon**
→ Run: `softwareupdate --install-rosetta`

**Build fails with missing headers**
→ Run: `make clean && make all`

## Full Documentation

See `docs/macos_build_instructions.md` for complete instructions, troubleshooting, and advanced usage.
