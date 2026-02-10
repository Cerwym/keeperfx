# PlayStation Vita Dependencies

This directory contains precompiled dependencies for PlayStation Vita builds.

## Purpose

If the VitaSDK container doesn't have SDL2 libraries pre-installed or if `vdpm` doesn't work in CI, you can manually download and place SDL2 library tarballs here.

## Usage

### Manual SDL2 Installation (Fallback)

1. Download SDL2 libraries for Vita from:
   - https://github.com/vitasdk/packages/releases
   - Or build from: https://github.com/libsdl-org/SDL/releases

2. Place tarballs in this directory:
   ```
   deps/vita/sdl2.tar.gz
   deps/vita/sdl2_image.tar.gz
   deps/vita/sdl2_mixer.tar.gz
   deps/vita/sdl2_net.tar.gz
   ```

3. The CI workflow will automatically extract and use these if vdpm fails.

## Current Status

Currently using VitaSDK container's built-in SDL2 or vdpm installation.
Tarballs are only needed if those methods fail.

## Notes

- This is a temporary fallback solution
- Ideally, dependencies should be installed via vdpm or come pre-installed in the container
- Only commit tarballs if absolutely necessary for CI to work
