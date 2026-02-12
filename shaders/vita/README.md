# PlayStation Vita GXP Shaders

This directory contains shader source files for the PlayStation Vita GXM (Graphics eXtension Module) API.

## Shader Files

- `palette_v.cg` - Vertex shader for palette-based rendering
- `palette_f.cg` - Fragment shader for palette-based rendering
- `README.md` - This file

## GXM Overview

The PlayStation Vita uses the GXM graphics API which:
- Is based on the SGX543MP4+ GPU
- Supports programmable vertex and fragment shaders written in Cg
- Uses deferred rendering architecture
- Supports texture sampling in both vertex and fragment stages

## Shader Compilation

Shaders must be compiled using `psp2cgc` (the Vita Cg compiler) from the Vita SDK:

```bash
# Compile vertex shader
psp2cgc -profile sce_vp_psp2 -o palette_v.gxp palette_v.cg

# Compile fragment shader
psp2cgc -profile sce_fp_psp2 -o palette_f.gxp palette_f.cg

# The .gxp files can then be embedded in the application or loaded at runtime
```

## Vertex Format

The vertex shader expects the following inputs:
- `float3 position` - Position (X, Y, Z)
- `float2 texcoord` - Texture coordinates (U, V)
- `float4 color` - Color/Shade (shade value in R channel)

## Outputs

The vertex shader outputs:
- `float4 position : POSITION` - Clip space position
- `float2 texcoord : TEXCOORD0` - Texture coordinates
- `float shade : TEXCOORD1` - Shade value for fragment shader

## Fragment Shader

The fragment shader:
1. Samples the palette index texture (8-bit luminance)
2. Uses the index to look up the actual color in a 256x1 RGBA palette texture
3. Applies the shade value to modulate the final color

## Resolution Optimization

Shaders are optimized for the Vita's 960×544 resolution.
Performance considerations:
- Minimize texture lookups
- Use dependent texture reads efficiently
- Consider using VRAM for frequently accessed textures

## Memory Layout

- Vertex shaders run on the USSE (Unified Shader Scalar Engine)
- Fragment shaders also run on the USSE with tile-based rendering
- Textures should be swizzled for optimal memory access patterns
