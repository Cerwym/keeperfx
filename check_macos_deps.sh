#!/bin/bash
# KeeperFX macOS Dependency Checker
# This script checks if all required dependencies for building KeeperFX on macOS are installed

echo "=============================================="
echo "KeeperFX macOS Dependency Checker"
echo "=============================================="
echo ""

all_good=true

# Check for Homebrew
echo -n "Checking for Homebrew... "
if command -v brew &> /dev/null; then
    echo "✓ Found ($(brew --version | head -1))"
else
    echo "✗ NOT FOUND"
    echo "  Install with: /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
    all_good=false
fi
echo ""

# Check for MinGW cross-compiler
echo -n "Checking for MinGW cross-compiler... "
if command -v i686-w64-mingw32-gcc &> /dev/null; then
    version=$(i686-w64-mingw32-gcc --version | head -1)
    echo "✓ Found ($version)"
else
    echo "✗ NOT FOUND"
    echo "  Install with: brew install mingw-w64"
    all_good=false
fi
echo ""

# Check for Make
echo -n "Checking for Make... "
if command -v make &> /dev/null; then
    version=$(make --version | head -1)
    echo "✓ Found ($version)"
else
    echo "✗ NOT FOUND"
    echo "  Install with: brew install make"
    all_good=false
fi
echo ""

# Check for Wine
echo -n "Checking for Wine... "
if command -v wine &> /dev/null; then
    version=$(wine --version 2>&1)
    echo "✓ Found ($version)"
else
    echo "⚠ NOT FOUND (optional, but needed to run the game)"
    echo "  Install with: brew install --cask wine-stable"
fi
echo ""

# Check for Rosetta (Apple Silicon only)
if [[ $(uname -m) == "arm64" ]]; then
    echo -n "Checking for Rosetta 2 (required for Wine on Apple Silicon)... "
    if /usr/bin/pgrep -q oahd; then
        echo "✓ Installed"
    else
        echo "⚠ NOT FOUND"
        echo "  Install with: softwareupdate --install-rosetta"
    fi
    echo ""
fi

# Check for Git
echo -n "Checking for Git... "
if command -v git &> /dev/null; then
    version=$(git --version)
    echo "✓ Found ($version)"
else
    echo "✗ NOT FOUND"
    echo "  Install with: xcode-select --install"
    all_good=false
fi
echo ""

# Summary
echo "=============================================="
if $all_good; then
    echo "✓ All required dependencies are installed!"
    echo ""
    echo "You can now build KeeperFX with:"
    echo "  make all -j\$(sysctl -n hw.ncpu)"
else
    echo "✗ Some required dependencies are missing."
    echo "Please install them using the commands above."
fi
echo "=============================================="
