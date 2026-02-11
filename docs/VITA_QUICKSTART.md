# Quick Start: Building KeeperFX for PlayStation Vita

This is a quick reference guide for building KeeperFX for PlayStation Vita. For detailed setup instructions, see [VITA_SETUP.md](VITA_SETUP.md).

## Prerequisites

- VitaSDK installed and configured (see [VITA_SETUP.md](VITA_SETUP.md))
- VITASDK environment variable set to `/usr/local/vitasdk`
- vdpm repository cloned
- SDL2 libraries for Vita installed via vdpm

## First-Time Setup

```bash
# Clone vdpm and bootstrap VitaSDK
git clone https://github.com/vitasdk/vdpm
cd vdpm
./bootstrap-vitasdk.sh

# Set environment variables
export VITASDK=/usr/local/vitasdk
export PATH=$VITASDK/bin:$PATH

# Install SDL2 dependencies
./vdpm sdl2
./vdpm sdl2_image
./vdpm sdl2_mixer
./vdpm sdl2_net
```

## Building

```bash
# Configure
cmake -DCMAKE_TOOLCHAIN_FILE=vita.cmake -B build-vita

# Build
cmake --build build-vita

# Create VPK package
cd build-vita
mkdir -p sce_sys
vita-mksfoex -s TITLE_ID=KPFX00001 "KeeperFX" sce_sys/param.sfo
vita-make-fself -c keeperfx.elf eboot.bin
vita-pack-vpk -s sce_sys/param.sfo -b eboot.bin keeperfx.vpk
```

## Installing on Vita

1. Transfer keeperfx.vpk to your Vita
2. Install via VitaShell
3. Launch from LiveArea

## Troubleshooting

- Ensure VITASDK environment variable is set correctly
- Check that all SDL2 libraries are installed
- Verify VitaSDK bin directory is in PATH

For more help, see the full [Vita Setup Guide](VITA_SETUP.md).
