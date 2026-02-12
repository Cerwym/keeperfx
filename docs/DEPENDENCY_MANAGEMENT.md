# Dependency Management

KeeperFX uses **CMake FetchContent** for automatic dependency management, inspired by successful cross-platform projects like [isle-portable](https://github.com/isledecomp/isle-portable).

## Overview

### Why FetchContent?

1. **Self-contained builds**: No external package managers required
2. **Consistent versions**: Same dependency versions across all platforms and developers
3. **CI/CD reliability**: No dependency on external registries or vcpkg baseline updates
4. **Cross-platform**: Works identically on Windows, Linux, macOS, and homebrew consoles
5. **Transparent**: Easy to see exactly which version of each dependency is being used

### Dependencies Managed by FetchContent

- **SDL2** (2.30.9): Core windowing, input, and audio
- **SDL2_image** (2.8.2): Image loading
- **SDL2_mixer** (2.8.0): Audio mixing
- **SDL2_net** (2.2.0): Networking
- **bgfx** (master): Cross-platform rendering abstraction
- **bx** (master): Base library for bgfx
- **bimg** (master): Image library for bgfx

## Usage

### Default Behavior (FetchContent)

By default, CMake will automatically download and build all dependencies:

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

The first build will take longer as dependencies are downloaded and built. Subsequent builds will be faster as dependencies are cached.

### Using System Packages

If you have dependencies installed system-wide and want to use them instead:

```bash
mkdir build && cd build
cmake .. -DDOWNLOAD_DEPENDENCIES=OFF
cmake --build .
```

This requires:
- SDL2, SDL2_image, SDL2_mixer, SDL2_net
- bgfx with bx and bimg

to be installed and discoverable via CMake's `find_package()`.

## Platform-Specific Behavior

### Desktop Platforms (Windows, Linux, macOS)

Uses FetchContent by default. Dependencies are downloaded from their official Git repositories.

### Homebrew Consoles (3DS, Vita, Switch)

Always use SDK-provided libraries, bypassing FetchContent entirely:

- **Nintendo 3DS**: Uses devkitPro libraries (citro3d, ctru)
- **PlayStation Vita**: Uses vitasdk libraries (vitaGL, SDL2, Sce* stubs)
- **Nintendo Switch**: Uses devkitPro libraries (SDL2, EGL, nx)

## Build Cache

FetchContent downloads and builds dependencies in the build directory under `_deps/`. This directory can be cached in CI/CD:

```yaml
- name: Cache FetchContent dependencies
  uses: actions/cache@v4
  with:
    path: ${{ github.workspace }}/out/build/${{ matrix.preset }}/_deps
    key: fetchcontent-${{ matrix.os }}-${{ hashFiles('CMakeLists.txt') }}
```

## Comparison with vcpkg

| Feature | FetchContent | vcpkg |
|---------|-------------|-------|
| Setup required | None | Install vcpkg, set VCPKG_ROOT |
| Version control | Direct Git tags in CMakeLists.txt | Baseline + registry |
| Build time (first) | ~10-15 min | ~10-15 min |
| Build time (cached) | < 1 min | < 1 min |
| Cross-platform | Native CMake support | Requires triplets configuration |
| Transparency | All versions visible in CMakeLists.txt | Hidden in vcpkg.json |
| Maintenance | Update Git tags as needed | Update baseline hash |

## Migration Notes

Previous versions of KeeperFX used vcpkg for dependency management. The migration to FetchContent provides:

1. **Simpler CI/CD**: No vcpkg setup or caching needed
2. **Better reproducibility**: Exact versions locked in CMakeLists.txt
3. **Easier contributions**: Contributors don't need to install vcpkg
4. **Consistency with similar projects**: Follows patterns from isle-portable and other successful cross-platform ports

### For Existing Developers

If you previously used vcpkg:
- You can remove the VCPKG_ROOT environment variable
- Delete your `vcpkg/` directory if it was in the project
- Run `cmake ..` as usual - dependencies will be fetched automatically

## Troubleshooting

### Long Initial Build Time

The first build downloads and compiles all dependencies. This is normal and only happens once. Subsequent builds reuse the cached dependencies.

### Download Failures

If a download fails (network issues, GitHub down):
- CMake will report which dependency failed
- Simply re-run `cmake ..` to retry
- Downloads are cached, so successful downloads won't be repeated

### Switching Between FetchContent and System Packages

When switching between `-DDOWNLOAD_DEPENDENCIES=ON` and `OFF`:
- Clear your build directory: `rm -rf build/`
- Configure fresh: `cmake .. -DDOWNLOAD_DEPENDENCIES=OFF`

## Advanced Configuration

### Using Specific Dependency Versions

Edit `CMakeLists.txt` and change the `GIT_TAG` for the desired dependency:

```cmake
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.9  # Change this to a different tag
    GIT_SHALLOW TRUE
)
```

### Adding New Dependencies

Follow the FetchContent pattern:

```cmake
FetchContent_Declare(
    my_library
    GIT_REPOSITORY https://github.com/user/my_library.git
    GIT_TAG v1.0.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(my_library)
```

## References

- [CMake FetchContent documentation](https://cmake.org/cmake/help/latest/module/FetchContent.html)
- [isle-portable build system](https://github.com/isledecomp/isle-portable/blob/master/CMakeLists.txt)
- [SDL2 releases](https://github.com/libsdl-org/SDL/releases)
- [bgfx GitHub](https://github.com/bkaradzic/bgfx)
