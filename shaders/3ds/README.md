# Nintendo 3DS PICA200 Shaders

This directory contains shader source files for the Nintendo 3DS PICA200 GPU.

## Shader Files

- `palette_vshader.v.pica` - Vertex shader for palette-based rendering
- `palette_gshader.g.pica` - Geometry shader (optional, for additional processing)
- `README.md` - This file

## PICA200 GPU Overview

The Nintendo 3DS uses the PICA200 GPU which has:
- Programmable vertex and geometry shaders
- Fixed-function fragment pipeline
- Support for texture combiners
- 24-bit floating point vertex shader operations

## Shader Compilation

Shaders must be compiled using `picasso` (the PICA200 shader assembler) from devkitPro:

```bash
# Compile vertex shader
picasso -o palette_vshader.shbin palette_vshader.v.pica

# The .shbin file can then be embedded in the application
bin2s palette_vshader.shbin > palette_vshader_shbin.s
```

## Vertex Format

The vertex shader expects the following inputs:
- `v0` - Position (X, Y, Z as float)
- `v1` - Texture coordinates (U, V as float)
- `v2` - Color/Shade (RGBA as u8)

## Outputs

The vertex shader outputs:
- `o0` - Position (clip space)
- `o1` - Texture coordinates (for texture unit 0)
- `o2` - Color (for texture combiner)

## Texture Combiners

The fragment stage uses texture combiners to:
1. Sample the palette index texture (8-bit)
2. Use the index to look up the actual color in a 256x1 palette texture
3. Modulate the color with the vertex shade value

## Resolution Optimization

Shaders are optimized for the 3DS top screen resolution of 400×240 pixels.
The bottom screen (320×240) can also be used for UI elements.
