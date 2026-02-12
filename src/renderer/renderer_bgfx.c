/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file renderer_bgfx.c
 *     bgfx hardware renderer implementation.
 * @par Purpose:
 *     Implements hardware-accelerated rendering using bgfx library which
 *     supports OpenGL, Vulkan, DirectX, and Metal backends.
 * @par Comment:
 *     Uses palette lookup shaders for efficient 8-bit texture rendering.
 * @author   KeeperFX Team
 * @date     2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "../pre_inc.h"
#include "renderer_interface.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <bgfx/c99/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include "../bflib_render.h"
#include "../bflib_video.h"
#include "../bflib_vidraw.h"
#include "../globals.h"
#include "../engine_textures.h"

#include "../post_inc.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
// External declarations
extern unsigned char lbPalette[768];  // From bflib_video.h - RGB palette (256 * 3)

/******************************************************************************/
// Constants
#define MAX_BATCH_VERTICES 65536
#define MAX_BATCH_INDICES  (MAX_BATCH_VERTICES * 3 / 2)
#define TEXTURE_ATLAS_SIZE 8192  // 8192x8192 atlas for all texture blocks
#define TEXTURE_BLOCK_SIZE 32

// Palette mode configuration
typedef enum {
    PALETTE_MODE_GPU_LOOKUP = 0,    // Use palette lookup shader
    PALETTE_MODE_RGBA_EXPAND = 1,   // Pre-expand to RGBA textures
} PaletteMode;

/******************************************************************************/
// Global state
static bgfx_vertex_layout_t s_vertexLayout;
static bgfx_program_handle_t s_programPalette = BGFX_INVALID_HANDLE;
static bgfx_program_handle_t s_programRGBA = BGFX_INVALID_HANDLE;
static bgfx_texture_handle_t s_textureAtlas = BGFX_INVALID_HANDLE;
static bgfx_texture_handle_t s_paletteTexture = BGFX_INVALID_HANDLE;
static bgfx_uniform_handle_t s_texColorSampler = BGFX_INVALID_HANDLE;
static bgfx_uniform_handle_t s_paletteSampler = BGFX_INVALID_HANDLE;

static PaletteMode s_paletteMode = PALETTE_MODE_GPU_LOOKUP;
static int s_screenWidth = 0;
static int s_screenHeight = 0;

// Vertex batch buffer
typedef struct BgfxVertex {
    float x, y, z;      // Position
    float u, v;         // Texture coordinates
    uint32_t rgba;      // Color/shade packed as RGBA
} BgfxVertex;

static BgfxVertex s_vertices[MAX_BATCH_VERTICES];
static uint16_t s_indices[MAX_BATCH_INDICES];
static uint32_t s_vertexCount = 0;
static uint32_t s_indexCount = 0;
static bgfx_texture_handle_t s_currentTexture = BGFX_INVALID_HANDLE;

/******************************************************************************/
// Helper functions

static uint32_t pack_shade_to_rgba(long shade)
{
    // Convert shade (0-256) to 0-255 range and pack as white color with alpha
    uint8_t s = (uint8_t)((shade * 255) / 256);
    return (0xFF << 24) | (s << 16) | (s << 8) | s;
}

static void flush_batch(void)
{
    if (s_vertexCount == 0) {
        return;
    }

    // Create transient vertex and index buffers
    bgfx_transient_vertex_buffer_t tvb;
    bgfx_transient_index_buffer_t tib;
    
    if (!bgfx_alloc_transient_vertex_buffer(&tvb, s_vertexCount, &s_vertexLayout)) {
        ERRORLOG("Failed to allocate transient vertex buffer");
        s_vertexCount = 0;
        s_indexCount = 0;
        return;
    }
    
    if (!bgfx_alloc_transient_index_buffer(&tib, s_indexCount, false)) {
        ERRORLOG("Failed to allocate transient index buffer");
        s_vertexCount = 0;
        s_indexCount = 0;
        return;
    }

    // Copy vertex data
    memcpy(tvb.data, s_vertices, s_vertexCount * sizeof(BgfxVertex));
    
    // Copy index data
    memcpy(tib.data, s_indices, s_indexCount * sizeof(uint16_t));

    // Set vertex and index buffers
    bgfx_set_transient_vertex_buffer(0, &tvb, 0, s_vertexCount);
    bgfx_set_transient_index_buffer(&tib, 0, s_indexCount);

    // Set texture and uniforms
    if (s_paletteMode == PALETTE_MODE_GPU_LOOKUP) {
        bgfx_set_texture(0, s_texColorSampler, s_currentTexture, UINT32_MAX);
        bgfx_set_texture(1, s_paletteSampler, s_paletteTexture, UINT32_MAX);
        bgfx_submit(0, s_programPalette, 0, BGFX_DISCARD_ALL);
    } else {
        bgfx_set_texture(0, s_texColorSampler, s_currentTexture, UINT32_MAX);
        bgfx_submit(0, s_programRGBA, 0, BGFX_DISCARD_ALL);
    }

    // Reset batch
    s_vertexCount = 0;
    s_indexCount = 0;
}

static void batch_triangle(struct PolyPoint* p1, struct PolyPoint* p2, struct PolyPoint* p3)
{
    // Check if we need to flush
    if (s_vertexCount + 3 > MAX_BATCH_VERTICES || s_indexCount + 3 > MAX_BATCH_INDICES) {
        flush_batch();
    }

    // Add vertices
    uint32_t baseVertex = s_vertexCount;
    
    // Vertex 1
    s_vertices[s_vertexCount].x = (float)p1->X;
    s_vertices[s_vertexCount].y = (float)p1->Y;
    s_vertices[s_vertexCount].z = 0.0f;
    s_vertices[s_vertexCount].u = (float)p1->U / 256.0f;  // Normalize UV coords
    s_vertices[s_vertexCount].v = (float)p1->V / 256.0f;
    s_vertices[s_vertexCount].rgba = pack_shade_to_rgba(p1->S);
    s_vertexCount++;
    
    // Vertex 2
    s_vertices[s_vertexCount].x = (float)p2->X;
    s_vertices[s_vertexCount].y = (float)p2->Y;
    s_vertices[s_vertexCount].z = 0.0f;
    s_vertices[s_vertexCount].u = (float)p2->U / 256.0f;
    s_vertices[s_vertexCount].v = (float)p2->V / 256.0f;
    s_vertices[s_vertexCount].rgba = pack_shade_to_rgba(p2->S);
    s_vertexCount++;
    
    // Vertex 3
    s_vertices[s_vertexCount].x = (float)p3->X;
    s_vertices[s_vertexCount].y = (float)p3->Y;
    s_vertices[s_vertexCount].z = 0.0f;
    s_vertices[s_vertexCount].u = (float)p3->U / 256.0f;
    s_vertices[s_vertexCount].v = (float)p3->V / 256.0f;
    s_vertices[s_vertexCount].rgba = pack_shade_to_rgba(p3->S);
    s_vertexCount++;

    // Add indices
    s_indices[s_indexCount++] = baseVertex + 0;
    s_indices[s_indexCount++] = baseVertex + 1;
    s_indices[s_indexCount++] = baseVertex + 2;
}

static TbResult create_texture_atlas(void)
{
    // Create texture atlas from block_ptrs
    // Each block is 32x32, we have TEXTURE_VARIATIONS_COUNT (32) × TEXTURE_BLOCKS_COUNT (1088)
    // Total textures: 32 * 1088 = 34816 blocks
    // At 32x32 each, we need roughly sqrt(34816) * 32 = 5920x5920 pixels
    // Using 8192x8192 atlas to have margin
    
    const uint32_t atlasSize = TEXTURE_ATLAS_SIZE;
    const uint32_t blockSize = TEXTURE_BLOCK_SIZE;
    const uint32_t blocksPerRow = atlasSize / blockSize; // 256 blocks per row
    
    SYNCLOG("Creating texture atlas: %dx%d pixels for %d texture blocks", 
            atlasSize, atlasSize, TEXTURE_VARIATIONS_COUNT * TEXTURE_BLOCKS_COUNT);
    
    // Allocate atlas memory
    uint8_t* atlasData = (uint8_t*)calloc(atlasSize * atlasSize, 1);
    if (!atlasData) {
        ERRORLOG("Failed to allocate texture atlas memory (%d bytes)", atlasSize * atlasSize);
        return 0;
    }
    
    // Copy block_ptrs textures into atlas
    // We iterate through all blocks because block_ptrs is a flat array indexed by
    // (variation * TEXTURE_BLOCKS_COUNT + block), and we need to maintain correct
    // spatial packing in the atlas regardless of null entries
    uint32_t blockIdx = 0;
    uint32_t totalBlocks = TEXTURE_VARIATIONS_COUNT * TEXTURE_BLOCKS_COUNT;
    
    for (uint32_t i = 0; i < totalBlocks; i++) {
        if (block_ptrs[i] == NULL) {
            continue; // Skip null blocks
        }
        
        // Ensure we don't overflow the atlas
        if (blockIdx >= blocksPerRow * blocksPerRow) {
            WARNLOG("Atlas full, cannot add more blocks (added %d/%d)", blockIdx, totalBlocks);
            break;
        }
        
        uint32_t blockX = (blockIdx % blocksPerRow) * blockSize;
        uint32_t blockY = (blockIdx / blocksPerRow) * blockSize;
        
        // Copy 32x32 block into atlas
        for (uint32_t y = 0; y < blockSize; y++) {
            for (uint32_t x = 0; x < blockSize; x++) {
                uint32_t srcIdx = y * blockSize + x;
                uint32_t dstIdx = (blockY + y) * atlasSize + (blockX + x);
                atlasData[dstIdx] = block_ptrs[i][srcIdx];
            }
        }
        
        blockIdx++;
    }
    
    SYNCLOG("Copied %d texture blocks into atlas", blockIdx);
    
    // Create bgfx texture
    const bgfx_memory_t* mem = bgfx_make_ref(atlasData, atlasSize * atlasSize);
    s_textureAtlas = bgfx_create_texture_2d(atlasSize, atlasSize, false, 1,
                                            BGFX_TEXTURE_FORMAT_R8,
                                            BGFX_TEXTURE_NONE | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT,
                                            mem);
    
    free(atlasData);
    
    if (!bgfx_is_valid(s_textureAtlas)) {
        ERRORLOG("Failed to create texture atlas");
        return 0;
    }
    
    return 1;
}

static TbResult create_palette_texture(void)
{
    // Create 256x1 palette texture from current palette
    // lbPalette is defined in bflib_video.h - 768 bytes (256 * 3 RGB)
    
    uint8_t paletteData[256 * 4]; // RGBA format
    
    // Convert RGB palette to RGBA
    for (int i = 0; i < 256; i++) {
        // Each palette entry is 3 bytes (R, G, B) - values are 0-63 in original
        // Need to scale from 0-63 to 0-255
        paletteData[i * 4 + 0] = (lbPalette[i * 3 + 0] * 255) / 63; // R
        paletteData[i * 4 + 1] = (lbPalette[i * 3 + 1] * 255) / 63; // G
        paletteData[i * 4 + 2] = (lbPalette[i * 3 + 2] * 255) / 63; // B
        paletteData[i * 4 + 3] = 255;                                 // A
    }
    
    const bgfx_memory_t* mem = bgfx_make_ref(paletteData, 256 * 4);
    s_paletteTexture = bgfx_create_texture_2d(256, 1, false, 1,
                                              BGFX_TEXTURE_FORMAT_RGBA8,
                                              BGFX_TEXTURE_NONE | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT,
                                              mem);
    
    if (!bgfx_is_valid(s_paletteTexture)) {
        ERRORLOG("Failed to create palette texture");
        return 0;
    }
    
    return 1;
}

static bgfx_shader_handle_t load_shader(const char* name)
{
    // Try to load compiled shader binary
    // Shaders should be pre-compiled for the target platform
    // File format: shaders/<name>.bin
    
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "shaders/%s.bin", name);
    
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        WARNLOG("Failed to open shader file: %s", filepath);
        return BGFX_INVALID_HANDLE;
    }
    
    // Get file size - using fseek/ftell for compatibility
    // Note: For files larger than 2GB, this may need platform-specific handling
    if (fseek(file, 0, SEEK_END) != 0) {
        ERRORLOG("Failed to seek in shader file: %s", filepath);
        fclose(file);
        return BGFX_INVALID_HANDLE;
    }
    
    long size = ftell(file);
    if (size <= 0 || size > 10 * 1024 * 1024) { // Sanity check: shader > 10MB is suspicious
        ERRORLOG("Invalid shader file size (%ld bytes): %s", size, filepath);
        fclose(file);
        return BGFX_INVALID_HANDLE;
    }
    
    if (fseek(file, 0, SEEK_SET) != 0) {
        ERRORLOG("Failed to seek in shader file: %s", filepath);
        fclose(file);
        return BGFX_INVALID_HANDLE;
    }
    
    // Read shader data
    uint8_t* data = (uint8_t*)malloc(size);
    if (!data) {
        ERRORLOG("Failed to allocate memory for shader: %s", filepath);
        fclose(file);
        return BGFX_INVALID_HANDLE;
    }
    
    size_t read = fread(data, 1, size, file);
    fclose(file);
    
    if (read != (size_t)size) {
        ERRORLOG("Failed to read shader file: %s", filepath);
        free(data);
        return BGFX_INVALID_HANDLE;
    }
    
    // Create shader from memory
    const bgfx_memory_t* mem = bgfx_copy(data, size);
    free(data);
    
    bgfx_shader_handle_t shader = bgfx_create_shader(mem);
    
    if (!bgfx_is_valid(shader)) {
        ERRORLOG("Failed to create shader: %s", filepath);
        return BGFX_INVALID_HANDLE;
    }
    
    SYNCLOG("Loaded shader: %s", filepath);
    return shader;
}

/******************************************************************************/
// Renderer interface implementation

static TbResult bgfx_renderer_init(struct SDL_Window* window, int width, int height)
{
    if (!window) {
        ERRORLOG("Invalid SDL window");
        return 0;
    }

    s_screenWidth = width;
    s_screenHeight = height;

    // Get native window handle from SDL
    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(window, &wmi)) {
        ERRORLOG("Failed to get window info: %s", SDL_GetError());
        return 0;
    }

    // Set platform data for bgfx
    bgfx_platform_data_t pd;
    memset(&pd, 0, sizeof(pd));
    
#if defined(_WIN32) || defined(_WIN64)
    pd.nwh = wmi.info.win.window;
#elif defined(__linux__)
    pd.ndt = wmi.info.x11.display;
    pd.nwh = (void*)(uintptr_t)wmi.info.x11.window;
#elif defined(__APPLE__)
    pd.nwh = wmi.info.cocoa.window;
#endif

    bgfx_set_platform_data(&pd);

    // Initialize bgfx
    bgfx_init_t init;
    bgfx_init_ctor(&init);
    init.type = BGFX_RENDERER_TYPE_COUNT; // Auto-select best renderer
    init.resolution.width = width;
    init.resolution.height = height;
    init.resolution.reset = BGFX_RESET_VSYNC;

    if (!bgfx_init(&init)) {
        ERRORLOG("Failed to initialize bgfx");
        return 0;
    }

    // Get actual renderer type after initialization
    bgfx_renderer_type_t rendererType = bgfx_get_renderer_type();
    SYNCLOG("bgfx initialized with renderer: %d", rendererType);

    // Setup vertex layout using the actual renderer type
    bgfx_vertex_layout_begin(&s_vertexLayout, rendererType);
    bgfx_vertex_layout_add(&s_vertexLayout, BGFX_ATTRIB_POSITION, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&s_vertexLayout, BGFX_ATTRIB_TEXCOORD0, 2, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&s_vertexLayout, BGFX_ATTRIB_COLOR0, 4, BGFX_ATTRIB_TYPE_UINT8, true, false);
    bgfx_vertex_layout_end(&s_vertexLayout);

    // Create samplers
    s_texColorSampler = bgfx_create_uniform("s_texColor", BGFX_UNIFORM_TYPE_SAMPLER, 1);
    s_paletteSampler = bgfx_create_uniform("s_palette", BGFX_UNIFORM_TYPE_SAMPLER, 1);

    // Load shaders
    bgfx_shader_handle_t vsh_palette = load_shader("vs_palette");
    bgfx_shader_handle_t fsh_palette = load_shader("fs_palette");
    bgfx_shader_handle_t vsh_rgba = load_shader("vs_rgba");
    bgfx_shader_handle_t fsh_rgba = load_shader("fs_rgba");
    
    // Create programs
    if (bgfx_is_valid(vsh_palette) && bgfx_is_valid(fsh_palette)) {
        s_programPalette = bgfx_create_program(vsh_palette, fsh_palette, true);
        if (!bgfx_is_valid(s_programPalette)) {
            ERRORLOG("Failed to create palette shader program");
        }
    } else {
        ERRORLOG("Palette shaders not loaded - cannot render in palette mode");
    }
    
    if (bgfx_is_valid(vsh_rgba) && bgfx_is_valid(fsh_rgba)) {
        s_programRGBA = bgfx_create_program(vsh_rgba, fsh_rgba, true);
        if (!bgfx_is_valid(s_programRGBA)) {
            ERRORLOG("Failed to create RGBA shader program");
        }
    } else {
        ERRORLOG("RGBA shaders not loaded - cannot render in RGBA mode");
    }
    
    // Check if at least one program is valid
    if (!bgfx_is_valid(s_programPalette) && !bgfx_is_valid(s_programRGBA)) {
        ERRORLOG("No valid shader programs loaded - bgfx renderer cannot function");
        ERRORLOG("Please compile shaders using shaderc (see shaders/README.md)");
        bgfx_shutdown();
        return 0;
    }

    // Create texture atlas
    if (!create_texture_atlas()) {
        ERRORLOG("Failed to create texture atlas");
        bgfx_shutdown();
        return 0;
    }

    // Create palette texture
    if (!create_palette_texture()) {
        ERRORLOG("Failed to create palette texture");
        bgfx_shutdown();
        return 0;
    }

    s_currentTexture = s_textureAtlas;

    // Setup orthographic projection
    float ortho[16];
    // Create ortho matrix manually for 2D rendering
    // Left=0, Right=width, Bottom=height, Top=0, Near=0, Far=100
    memset(ortho, 0, sizeof(ortho));
    ortho[0] = 2.0f / (float)width;
    ortho[5] = -2.0f / (float)height;
    ortho[10] = -2.0f / 100.0f;
    ortho[12] = -1.0f;
    ortho[13] = 1.0f;
    ortho[14] = -1.0f;
    ortho[15] = 1.0f;
    
    bgfx_set_view_transform(0, NULL, ortho);
    bgfx_set_view_rect(0, 0, 0, width, height);

    SYNCLOG("bgfx renderer initialized successfully");
    return 1;
}

static void bgfx_renderer_shutdown(void)
{
    // Flush any pending draws
    flush_batch();

    // Cleanup resources
    if (bgfx_is_valid(s_textureAtlas)) {
        bgfx_destroy_texture(s_textureAtlas);
        s_textureAtlas = BGFX_INVALID_HANDLE;
    }
    
    if (bgfx_is_valid(s_paletteTexture)) {
        bgfx_destroy_texture(s_paletteTexture);
        s_paletteTexture = BGFX_INVALID_HANDLE;
    }
    
    if (bgfx_is_valid(s_programPalette)) {
        bgfx_destroy_program(s_programPalette);
        s_programPalette = BGFX_INVALID_HANDLE;
    }
    
    if (bgfx_is_valid(s_programRGBA)) {
        bgfx_destroy_program(s_programRGBA);
        s_programRGBA = BGFX_INVALID_HANDLE;
    }
    
    if (bgfx_is_valid(s_texColorSampler)) {
        bgfx_destroy_uniform(s_texColorSampler);
        s_texColorSampler = BGFX_INVALID_HANDLE;
    }
    
    if (bgfx_is_valid(s_paletteSampler)) {
        bgfx_destroy_uniform(s_paletteSampler);
        s_paletteSampler = BGFX_INVALID_HANDLE;
    }

    bgfx_shutdown();
    
    SYNCLOG("bgfx renderer shutdown");
}

static void bgfx_renderer_begin_frame(void)
{
    // Reset batch state
    s_vertexCount = 0;
    s_indexCount = 0;
    
    // Set view clear state
    bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000FF, 1.0f, 0);
    
    // Touch view to ensure it's processed even if no draw calls
    bgfx_touch(0);
}

static void bgfx_renderer_end_frame(void)
{
    // Flush any remaining batched triangles
    flush_batch();
    
    // Present frame
    bgfx_frame(false);
}

static void bgfx_renderer_draw_gpoly(struct PolyPoint* p1, struct PolyPoint* p2, 
                                     struct PolyPoint* p3)
{
    // Draw as triangle
    batch_triangle(p1, p2, p3);
}

static void bgfx_renderer_draw_trig(struct PolyPoint* p1, struct PolyPoint* p2, 
                                    struct PolyPoint* p3)
{
    // Draw as triangle
    batch_triangle(p1, p2, p3);
}

/******************************************************************************/
static RendererInterface bgfx_renderer = {
    .init = bgfx_renderer_init,
    .shutdown = bgfx_renderer_shutdown,
    .begin_frame = bgfx_renderer_begin_frame,
    .end_frame = bgfx_renderer_end_frame,
    .draw_gpoly = bgfx_renderer_draw_gpoly,
    .draw_trig = bgfx_renderer_draw_trig,
};

RendererInterface* get_bgfx_renderer(void)
{
    return &bgfx_renderer;
}

/******************************************************************************/
#ifdef __cplusplus
}
#endif
