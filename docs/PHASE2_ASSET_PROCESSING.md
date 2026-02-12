# Phase 2: Asset Processing Integration

This document outlines the remaining work to fully integrate asset processing (graphics, language, sound) into the CMake build system.

## Current Status (Phase 1 Complete)

✅ **Completed in Phase 1:**
- Core CMake build system for executables (keeperfx, keeperfx_hvlog, tests)
- All dependencies integrated (SDL2, ffmpeg, OpenAL, enet, zlib, etc.)
- Tool downloads (png2ico, po2ngdat, sndbanker, pngpal2raw, png2bestpal, rnctools)
- Icon generation from PNG files
- Test target with CUnit
- Windows x64 release preset

## Remaining Work for Phase 2

### 1. Graphics Pipeline (`pkg_gfx.mk` → CMake)

**Current Makefile Process:**
- Converts PNG images to RAW format for game engine
- Processes landview graphics for campaigns (22+ files per campaign)
- Generates torture room, doors, frontend graphics
- Uses tools: `png2bestpal`, `pngpal2raw`

**CMake Migration Plan:**
```cmake
# Example pattern for PNG→RAW conversion
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/pkg/ldata/torture.raw
    COMMAND ${PNGTOBSPAL_PATH} -o torture.pal gfx/torture.png
    COMMAND ${PNGTORAW_PATH} -o pkg/ldata/torture.raw -p torture.pal gfx/torture.png
    DEPENDS gfx/torture.png
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
```

**Files to Process:**
- `LANDVIEWRAWS` (~200+ files across all campaigns)
- `LANDVIEWDATTABS` (6 files)
- `TOTRUREGFX` (11 files)
- `FRONTENDGFX` (13 files)
- `ENGINEGFX` (100+ files)

### 2. Language Pipeline (`pkg_lang.mk` → CMake)

**Current Makefile Process:**
- Converts PO/POT files to NGDAT/NCDAT format
- Supports 15+ languages (eng, chi, cht, cze, dut, fre, ger, ita, jpn, kor, lat, pol, rus, spa, swe)
- Uses character encoding tables for non-Latin languages
- Tool: `po2ngdat`

**CMake Migration Plan:**
```cmake
# Example pattern for PO→NGDAT conversion
add_custom_command(
    OUTPUT pkg/fxdata/gtext_eng.dat
    COMMAND ${POTONGDAT_PATH} -o pkg/fxdata/gtext_eng.dat 
            -e ${EU_CHAR_ENCODING} lang/gtext_eng.po
    DEPENDS lang/gtext_eng.po tools/po2ngdat/res/char_encoding_tbl_eu.txt
)
```

**Files to Process:**
- `NGTEXTDATS` (17 engine language files)
- `NCTEXTDATS` (100+ campaign text files)
- `MPTEXTDATS` (17 multiplayer map text files)

### 3. Sound Pipeline (`pkg_sfx.mk` → CMake)

**Current Makefile Process:**
- Converts WAV files to DAT sound banks
- Processes speech banks for 14 languages
- Handles landview campaign speeches
- Tool: `sndbanker`

**CMake Migration Plan:**
```cmake
# Example pattern for WAV→DAT conversion
add_custom_command(
    OUTPUT pkg/sound/sound.dat
    COMMAND ${SNDBANKER_PATH} -o pkg/sound/sound.dat sfx/sound/filelist.txt
    DEPENDS sfx/sound/filelist.txt ${SOUND_WAV_FILES}
)
```

**Files to Process:**
- `NGSOUNDDATS` (15 sound bank files)
- `LANDVIEWSPEECHDIRS` (40+ campaign speech directories)

### 4. Package Target (`package.mk` → CMake)

**Current Makefile Process:**
- Creates release package structure
- Copies executables, DLLs, configurations
- Packages campaigns, creatures, maps
- Creates 7z archive

**CMake Migration Plan:**
```cmake
add_custom_target(package
    COMMAND ${CMAKE_COMMAND} -E make_directory pkg
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:keeperfx> pkg/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:keeperfx_hvlog> pkg/
    # ... more packaging steps
    DEPENDS keeperfx keeperfx_hvlog pkg-gfx pkg-lang pkg-sfx
)
```

## Implementation Strategy

### Approach 1: Custom Commands (Full CMake)
**Pros:**
- Complete CMake integration
- Better dependency tracking
- Cross-platform support

**Cons:**
- Significant effort (~500+ custom commands)
- Complex dependency graphs
- Asset source files may not be available in all environments

### Approach 2: External Target (Hybrid)
**Pros:**
- Quick migration path
- Reuses existing Makefile logic
- Less risk of breaking builds

**Cons:**
- Still depends on Make
- Less integrated feel

```cmake
add_custom_target(assets
    COMMAND make pkg-gfx pkg-lang pkg-sfx
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
add_dependencies(keeperfx assets)
```

### Approach 3: Optional Assets (Recommended for Phase 2)
**Pros:**
- Core build works without assets
- Assets can be pre-built or downloaded
- Flexible for different build environments

**Cons:**
- Need to manage pre-built asset packages

```cmake
option(BUILD_ASSETS "Build game assets from source" OFF)
if(BUILD_ASSETS)
    # Include asset processing
else()
    # Download pre-built assets
endif()
```

## Recommended Next Steps

1. **Create asset processing module** (`cmake/AssetProcessing.cmake`)
2. **Start with graphics** (most complex, highest value)
3. **Add language processing** (well-structured, predictable patterns)
4. **Add sound processing** (requires external WAV files)
5. **Create package target** (ties everything together)
6. **Add CI/CD integration** (automated builds with assets)

## Timeline Estimate

- Graphics pipeline: 2-3 days
- Language pipeline: 1-2 days
- Sound pipeline: 1-2 days
- Package target: 1 day
- Testing & refinement: 2-3 days

**Total: ~1-2 weeks** for complete Phase 2 implementation

## Notes

- Asset source files (PNG, PO, WAV) are large and stored in separate repositories
- Many developers may want to build code without rebuilding all assets
- Consider a hybrid approach where assets are optional
- Pre-built asset packages could be distributed separately
