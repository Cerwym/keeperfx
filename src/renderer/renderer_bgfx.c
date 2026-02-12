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

#include <bgfx/c99/bgfx.h>
#include <bgfx/platform.h>
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
// Constants
#define MAX_BATCH_VERTICES 65536
#define MAX_BATCH_INDICES  (MAX_BATCH_VERTICES * 3 / 2)
#define TEXTURE_ATLAS_SIZE 1024
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
    // For now, create a simple placeholder texture atlas
    // In full implementation, this would pack all block_ptrs textures into an atlas
    
    // TODO: Implement full texture atlas creation from block_ptrs
    // For now, create a minimal test texture
    const uint32_t size = 256;
    uint8_t* data = (uint8_t*)malloc(size * size);
    if (!data) {
        ERRORLOG("Failed to allocate texture atlas memory");
        return 0;
    }
    
    // Fill with test pattern (palette indices)
    for (uint32_t i = 0; i < size * size; i++) {
        data[i] = (uint8_t)(i % 256);
    }
    
    const bgfx_memory_t* mem = bgfx_make_ref(data, size * size);
    s_textureAtlas = bgfx_create_texture_2d(size, size, false, 1,
                                            BGFX_TEXTURE_FORMAT_R8,
                                            BGFX_TEXTURE_NONE | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT,
                                            mem);
    
    free(data);
    
    if (!bgfx_is_valid(s_textureAtlas)) {
        ERRORLOG("Failed to create texture atlas");
        return 0;
    }
    
    return 1;
}

static TbResult create_palette_texture(void)
{
    // Create 256x1 palette texture from current palette
    // TODO: Get actual palette data from the game
    uint8_t paletteData[256 * 4]; // RGBA format
    
    // Fill with test gradient for now
    for (int i = 0; i < 256; i++) {
        paletteData[i * 4 + 0] = (uint8_t)i; // R
        paletteData[i * 4 + 1] = (uint8_t)i; // G
        paletteData[i * 4 + 2] = (uint8_t)i; // B
        paletteData[i * 4 + 3] = 255;        // A
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

static bgfx_shader_handle_t load_shader_stub(const char* name)
{
    // For now, return invalid handle as shaders need to be compiled
    // In full implementation, load pre-compiled shader binaries
    WARNLOG("Shader loading not yet implemented: %s", name);
    return BGFX_INVALID_HANDLE;
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

    // Setup vertex layout
    bgfx_vertex_layout_begin(&s_vertexLayout, BGFX_RENDERER_TYPE_NOOP);
    bgfx_vertex_layout_add(&s_vertexLayout, BGFX_ATTRIB_POSITION, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&s_vertexLayout, BGFX_ATTRIB_TEXCOORD0, 2, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&s_vertexLayout, BGFX_ATTRIB_COLOR0, 4, BGFX_ATTRIB_TYPE_UINT8, true, false);
    bgfx_vertex_layout_end(&s_vertexLayout);

    // Create samplers
    s_texColorSampler = bgfx_create_uniform("s_texColor", BGFX_UNIFORM_TYPE_SAMPLER, 1);
    s_paletteSampler = bgfx_create_uniform("s_palette", BGFX_UNIFORM_TYPE_SAMPLER, 1);

    // Load shaders (stub for now - shaders need to be pre-compiled)
    bgfx_shader_handle_t vsh_palette = load_shader_stub("vs_palette");
    bgfx_shader_handle_t fsh_palette = load_shader_stub("fs_palette");
    bgfx_shader_handle_t vsh_rgba = load_shader_stub("vs_rgba");
    bgfx_shader_handle_t fsh_rgba = load_shader_stub("fs_rgba");
    
    // Create programs (will be invalid until shaders are loaded)
    if (bgfx_is_valid(vsh_palette) && bgfx_is_valid(fsh_palette)) {
        s_programPalette = bgfx_create_program(vsh_palette, fsh_palette, true);
    }
    if (bgfx_is_valid(vsh_rgba) && bgfx_is_valid(fsh_rgba)) {
        s_programRGBA = bgfx_create_program(vsh_rgba, fsh_rgba, true);
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
    bx_mtx_ortho(ortho, 0.0f, (float)width, (float)height, 0.0f, 0.0f, 100.0f, 0.0f, bgfx_get_caps()->homogeneousDepth);
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
