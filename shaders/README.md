# KeeperFX bgfx Shaders

This directory contains shader files for the bgfx hardware renderer.

## Shader Files

### Palette Lookup Mode
- `vs_palette.sc` - Vertex shader for 8-bit palette mode
- `fs_palette.sc` - Fragment shader for 8-bit palette mode
  - Uses palette texture lookup for efficient rendering
  - Supports per-vertex shading (S component from PolyPoint)

### RGBA Mode  
- `vs_rgba.sc` - Vertex shader for pre-expanded RGBA mode
- `fs_rgba.sc` - Fragment shader for pre-expanded RGBA mode
  - Uses pre-converted RGBA textures
  - Supports per-vertex shading

### Common
- `varying.def.sc` - Vertex attribute and varying definitions

## Shader Compilation

Shaders need to be compiled to platform-specific formats using `shaderc` tool from bgfx.

Example compilation commands:
```bash
# For OpenGL
shaderc -f vs_palette.sc -o vs_palette.bin --type vertex --platform linux -i /path/to/bgfx/src
shaderc -f fs_palette.sc -o fs_palette.bin --type fragment --platform linux -i /path/to/bgfx/src

# For DirectX 11
shaderc -f vs_palette.sc -o vs_palette.bin --type vertex --platform windows -p vs_5_0 -i /path/to/bgfx/src
shaderc -f fs_palette.sc -o fs_palette.bin --type fragment --platform windows -p ps_5_0 -i /path/to/bgfx/src

# For Vulkan
shaderc -f vs_palette.sc -o vs_palette.bin --type vertex --platform linux -p spirv -i /path/to/bgfx/src
shaderc -f fs_palette.sc -o fs_palette.bin --type fragment --platform linux -p spirv -i /path/to/bgfx/src
```

## Vertex Format

The shaders expect vertices with the following layout (matching PolyPoint structure):
- **Position (a_position)**: 3 floats (X, Y, Z) - screen-space coordinates
- **TexCoord0 (a_texcoord0)**: 2 floats (U, V) - texture coordinates normalized to [0, 1]
- **Color0 (a_color0)**: 4 bytes RGBA - shade value packed in red channel

## Texture Setup

### Palette Mode
- **s_texColor**: 8-bit texture atlas (R8 format)
- **s_palette**: 256x1 RGBA texture containing the color palette

### RGBA Mode
- **s_texColor**: RGBA texture atlas (RGBA8 format)

## Configuration

Set `PALETTE_MODE=GPU_LOOKUP` or `PALETTE_MODE=RGBA_EXPAND` in `config/keeperfx.cfg` to choose rendering mode.
