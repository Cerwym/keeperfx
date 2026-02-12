# Homebrew Console Ports

This directory contains platform-specific implementations for homebrew console platforms.

## Dependency Management

KeeperFX uses **CMake FetchContent** to automatically download and build dependencies. This provides:
- Consistent dependency versions across all platforms
- No need for external package managers (vcpkg, conan, etc.)
- Self-contained build system
- Better CI/CD reliability

### Using FetchContent (Default)

By default, dependencies are downloaded and built automatically:

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Using System Packages (Optional)

If you prefer to use system-installed packages:

```bash
mkdir build && cd build
cmake .. -DDOWNLOAD_DEPENDENCIES=OFF
cmake --build .
```

This requires SDL2, SDL2_image, SDL2_mixer, SDL2_net, and bgfx to be installed on your system.

## Supported Platforms

### Nintendo 3DS
- **Renderer**: citro3d (PICA200 GPU)
- **Audio**: NDSP (Nintendo DSP)
- **Input**: Physical buttons, Circle Pad, and touchscreen
- **Resolution**: 400×240 (top screen)
- **SDK**: devkitARM with citro3d

Files:
- `src/renderer/renderer_citro3d.c`
- `src/audio/audio_ndsp.c`
- `src/input/input_3ds.c`
- `shaders/3ds/palette_vshader.v.pica`

### PlayStation Vita
- **Renderer**: bgfx with vitaGL backend (OpenGL ES wrapper)
- **Audio**: SceAudio
- **Input**: Physical buttons, analog sticks, front and rear touchscreens
- **Resolution**: 960×544
- **SDK**: vitasdk

Files:
- `src/audio/audio_vita.c`
- `src/input/input_vita.c`
- Note: Rendering uses existing bgfx implementation via vitaGL (OpenGL ES on Vita)

#### Why vitaGL?
vitaGL (https://github.com/Rinnegatamante/vitaGL) provides an OpenGL ES 2.0 wrapper on top of the Vita's native GXM API. This allows:
- **Code reuse**: The existing bgfx renderer works without platform-specific changes
- **Proven approach**: Used by successful ports like sm64-vita (https://github.com/bythos14/sm64-vita)
- **Maintainability**: Single renderer codebase for multiple platforms
- **Community support**: Active development and homebrew community adoption

### Nintendo Switch
- **Renderer**: bgfx with deko3d backend (already supported)
- **Audio**: audren (Audio Renderer)
- **Input**: Joy-Con buttons, analog sticks, and touchscreen
- **Resolution**: 1280×720 (handheld), 1920×1080 (docked)
- **SDK**: libnx

Files:
- `src/audio/audio_switch.c`
- `src/input/input_switch.c`
- Note: Rendering uses existing bgfx implementation with deko3d backend

## Architecture

All implementations follow the established interface pattern:

### Renderer Interface
```c
typedef struct RendererInterface {
    TbResult (*init)(struct SDL_Window* window, int width, int height);
    void (*shutdown)(void);
    void (*begin_frame)(void);
    void (*end_frame)(void);
    void (*draw_gpoly)(struct PolyPoint* p1, struct PolyPoint* p2, struct PolyPoint* p3);
    void (*draw_trig)(struct PolyPoint* p1, struct PolyPoint* p2, struct PolyPoint* p3);
} RendererInterface;
```

### Audio Interface
```c
typedef struct AudioInterface {
    TbResult (*init)(void);
    void (*shutdown)(void);
    void (*play_sample)(int id, long x, long y, long z, int volume);
    void (*play_music)(int track);
    void (*set_listener)(long x, long y, long z, int angle);
    void (*set_volume)(int master, int music, int sfx);
} AudioInterface;
```

### Input Interface
```c
typedef struct InputInterface {
    void (*poll_events)(void);
    TbBool (*is_key_down)(int keycode);
    void (*get_mouse)(int* x, int* y, int* buttons);
    TbBool (*get_gamepad_axis)(int axis, int* value);
} InputInterface;
```

## Building for Homebrew Platforms

### Nintendo 3DS
```bash
# Install devkitARM and citro3d
# Set up environment
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=$DEVKITPRO/devkitARM

# Build
mkdir build-3ds && cd build-3ds
cmake .. -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/3DS.cmake -DPLATFORM_3DS=ON
make
```

### PlayStation Vita
```bash
# Install vitasdk
# Set up environment
export VITASDK=/usr/local/vitasdk

# Install vitaGL dependency
# vitaGL provides OpenGL ES compatibility layer for bgfx
# See: https://github.com/Rinnegatamante/vitaGL
# Usually pre-installed in vitasdk, but can be installed with:
# vdpm vitaGL

# Build
mkdir build-vita && cd build-vita
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake
make

# Or use CMake preset
cmake --preset vita-release
cmake --build --preset vita-release
```

**Dependencies**: 
- vitaGL (OpenGL ES wrapper for Vita) - installed via vdpm
- SDL2 (Vita port) - included in vitasdk
- bgfx will work through vitaGL's OpenGL ES implementation

### Nintendo Switch
```bash
# Install devkitA64 and libnx
# Set up environment
export DEVKITPRO=/opt/devkitpro
export DEVKITA64=$DEVKITPRO/devkitA64

# Build
mkdir build-switch && cd build-switch
cmake .. -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake -DPLATFORM_SWITCH=ON
make
```

## Implementation Status

### Completed
- ✅ Renderer interface for 3DS (citro3d)
- ✅ Audio interfaces for 3DS, Vita, Switch
- ✅ Input interfaces for 3DS, Vita, Switch
- ✅ Shader templates for 3DS (PICA assembly)
- ✅ Interface declarations in header files
- ✅ Vita uses bgfx renderer via vitaGL (no custom renderer needed)

### Needs Implementation
- ⚠️ Actual sample loading and playback logic
- ⚠️ Music streaming implementation
- ⚠️ Texture atlas management for homebrew platforms
- ⚠️ Platform-specific build system integration
- ⚠️ Shader compilation pipeline
- ⚠️ Memory management optimization for limited RAM

### Future Enhancements
- 🔮 Stereoscopic 3D support for Nintendo 3DS
- 🔮 Gyroscope support for all platforms
- 🔮 Multi-touch gesture recognition
- 🔮 Platform-specific UI scaling and adaptation
- 🔮 Performance profiling and optimization

## Platform-Specific Considerations

### Nintendo 3DS
- **Memory**: Limited to 128MB (64MB app + 64MB system)
- **CPU**: ARM11 MPCore @ 268MHz
- **GPU**: PICA200 @ 268MHz
- **Challenges**: 
  - Small screen resolution requires UI adaptation
  - Limited RAM requires aggressive asset management
  - Power management for battery life

### PlayStation Vita
- **Memory**: 512MB shared RAM, 128MB VRAM
- **CPU**: ARM Cortex-A9 quad-core @ 444MHz
- **GPU**: SGX543MP4+ @ 222MHz
- **Rendering**: Uses vitaGL (OpenGL ES wrapper) + bgfx for consistency with other platforms
- **Challenges**:
  - vitaGL provides OpenGL ES 2.0 compatibility layer over native GXM
  - Proper VRAM management through vitaGL
  - Performance tuning for OpenGL ES emulation overhead
- **References**:
  - vitaGL: https://github.com/Rinnegatamante/vitaGL
  - sm64-vita: https://github.com/bythos14/sm64-vita (example port using vitaGL)

### Nintendo Switch
- **Memory**: 4GB shared RAM
- **CPU**: ARM Cortex-A57 quad-core @ 1020MHz
- **GPU**: NVIDIA Maxwell @ 307-768MHz
- **Challenges**:
  - Handheld vs docked mode switching
  - Dynamic resolution scaling
  - Touch screen only in handheld mode

## Testing

Each platform should be tested for:
- ✓ Rendering performance (target 30+ FPS)
- ✓ Audio playback and 3D positioning
- ✓ Input responsiveness
- ✓ Memory usage within platform limits
- ✓ Battery life (for portable platforms)
- ✓ Stability (no crashes during extended play)

## Contributing

When adding support for additional homebrew platforms:
1. Follow the established interface pattern
2. Create platform-specific implementations in separate files
3. Use `#ifdef PLATFORM_XXX` guards
4. Document platform-specific quirks and limitations
5. Provide shader sources in platform-native formats
6. Test thoroughly on real hardware

## Resources

- **Nintendo 3DS**: https://github.com/devkitPro/3dstools
- **PlayStation Vita**: https://vitasdk.org/
- **Nintendo Switch**: https://github.com/switchbrew/libnx

## License

These implementations are part of KeeperFX and licensed under GPL v2.
See LICENSE file in repository root for details.
