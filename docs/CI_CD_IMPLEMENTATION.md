# GitHub Actions CI/CD Implementation (Phase 4)

This document describes the GitHub Actions workflows implemented as part of Phase 4 infrastructure improvements.

## Overview

The CI/CD system supports building KeeperFX across 8 target platforms:
- Windows x86
- Windows x64
- Linux x64
- macOS Universal (x86_64 + arm64)
- WebAssembly
- Nintendo 3DS
- PlayStation Vita
- Nintendo Switch

## Workflows

### ci-build.yml
Main CI workflow for desktop platforms (Windows, Linux, macOS).

**Features:**
- Matrix strategy for parallel builds across 4 platforms
- vcpkg package caching for faster builds
- CMake presets for reproducible configuration
- CTest integration (currently disabled pending full CMake support)
- Artifact uploads for build outputs

**Triggers:**
- Push to any branch
- Pull requests

### ci-wasm.yml
WebAssembly build workflow using Emscripten.

**Features:**
- Emscripten SDK setup
- WebAssembly CMake preset
- Artifact uploads (.wasm, .js, .html files)

**Triggers:**
- Push to any branch
- Pull requests

### ci-homebrew.yml
Console homebrew builds for 3DS, Vita, and Switch.

**Features:**
- DevKit container environments
- Platform-specific CMake presets
- Parallel matrix builds for all three consoles

**Triggers:**
- Push to any branch
- Pull requests

### release.yml
Multi-platform release workflow.

**Features:**
- Builds all 7 platforms (desktop + WASM + consoles)
- Platform-specific artifact packaging
- Automatic upload to GitHub releases
- Manual trigger with version input

**Triggers:**
- GitHub release creation
- Manual workflow dispatch

## CMake Presets

All platform configurations are defined in `CMakePresets.json`:

```json
{
  "configurePresets": [
    "windows-x86-release",
    "windows-x64-release",
    "linux-x64-release",
    "macos-universal",
    "wasm-release",
    "3ds-release",
    "vita-release",
    "switch-release"
  ]
}
```

Each preset includes:
- Generator (Ninja)
- Build type (RelWithDebInfo)
- Toolchain configuration
- vcpkg integration

## vcpkg Configuration

vcpkg is pinned to commit `9558037875497b9db8cf38fcd7db68ec661bffe7` for reproducible builds.

Dependencies defined in `vcpkg.json`:
- SDL2
- SDL2_image
- SDL2_mixer
- SDL2_net
- bgfx

## Testing

CUnit test support has been added to CMakeLists.txt:
- Test executable: `keeperfx_tests`
- Run via: `ctest --preset <preset-name>`
- Currently disabled in CI pending full CMake migration

## Current Limitations

1. **CMake Migration In Progress**: The project is transitioning from Makefile to CMake. Current CMake support is Windows-specific.

2. **Tests Disabled**: CTest execution is disabled in CI workflows (`test: false`) until CMake builds are fully functional across all platforms.

3. **Platform-Specific Code**: CMakeLists.txt currently has Windows-specific configurations (e.g., `-mwindows`, `imagehlp`, `dbghelp`). These need to be made conditional for cross-platform builds.

4. **Console Builds Untested**: 3DS, Vita, and Switch builds require:
   - DevKit SDK environment variables
   - Platform-specific toolchain files
   - Testing on actual hardware or emulators

## Next Steps

### Short Term (Phase 4 Completion)
- [ ] Test workflows on actual pushes/PRs
- [ ] Verify artifact uploads work correctly
- [ ] Validate vcpkg caching effectiveness
- [ ] Add build time monitoring

### Medium Term (Phase 5+)
- [ ] Complete CMake cross-platform support
- [ ] Enable CTest execution in CI
- [ ] Add code coverage reporting
- [ ] Implement build status badges
- [ ] Add platform-specific test suites

### Long Term
- [ ] Add deployment workflows
- [ ] Implement automatic version bumping
- [ ] Add performance regression testing
- [ ] Create nightly build schedule
- [ ] Add security scanning (SAST/DAST)

## Usage

### Running Builds Locally

**Desktop platforms:**
```bash
cmake --preset windows-x86-release
cmake --build --preset windows-x86-release
```

**Running tests:**
```bash
ctest --preset windows-x86-release --output-on-failure
```

### Triggering Manual Release

1. Go to Actions → Release workflow
2. Click "Run workflow"
3. Enter version (e.g., `v1.2.0`)
4. Click "Run workflow"

## Build Time Expectations

Target: < 15 minutes total (parallel)

Current estimates:
- Windows builds: ~8-10 minutes each
- Linux builds: ~6-8 minutes
- macOS builds: ~10-12 minutes
- WASM builds: ~5-7 minutes
- Console builds: ~8-10 minutes each

With parallelization and caching, total time should be under 15 minutes.

## Artifact Storage

Artifacts are retained for 90 days by default. Release artifacts are permanent.

**Artifact naming:**
- CI builds: `keeperfx-{platform}`
- Releases: `keeperfx-{version}-{platform}.{ext}`

## Contributing

When adding new platforms or modifying workflows:
1. Update CMakePresets.json
2. Test locally if possible
3. Update this documentation
4. Add to appropriate workflow matrix
5. Verify vcpkg dependencies

## Resources

- [CMake Presets Documentation](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)
- [vcpkg Documentation](https://vcpkg.io/)
- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [lukka/run-vcpkg](https://github.com/lukka/run-vcpkg)
- [lukka/run-cmake](https://github.com/lukka/run-cmake)
