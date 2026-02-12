/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file renderer_citro3d.c
 *     Nintendo 3DS citro3d renderer implementation.
 * @par Purpose:
 *     Implements hardware-accelerated rendering using citro3d for Nintendo 3DS.
 * @par Comment:
 *     Uses PICA200 GPU shaders for palette rendering.
 *     Optimized for 400×240 top screen resolution.
 * @author   KeeperFX Team
 * @date     12 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "../pre_inc.h"
#include "renderer_interface.h"

#ifdef PLATFORM_3DS

#include <3ds.h>
#include <citro3d.h>
#include <tex3ds.h>
#include <string.h>

#include "../bflib_render.h"
#include "../bflib_video.h"
#include "../globals.h"

#include "../post_inc.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
// Constants
#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

#define CLEAR_COLOR 0x000000FF  // Black with full alpha
#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 240
#define MAX_VERTICES 4096

/******************************************************************************/
// External declarations
extern unsigned char lbPalette[768];  // From bflib_video.h - RGB palette (256 * 3)

/******************************************************************************/
// PICA200 shader binary (compiled from PICA assembly)
// This would normally be compiled from .v.pica shader source
extern const u8 vshader_shbin[];
extern const u32 vshader_shbin_size;

/******************************************************************************/
// Vertex structure for PICA200
typedef struct {
    float position[3];  // X, Y, Z
    float texcoord[2];  // U, V
    u8 color[4];        // RGBA (shade in R channel)
} C3D_Vertex;

/******************************************************************************/
// Global state
static C3D_RenderTarget* s_target = NULL;
static C3D_Tex s_paletteTexture;
static C3D_Tex s_atlasTexture;
static DVLB_s* s_shaderDvlb = NULL;
static shaderProgram_s s_shaderProgram;
static int s_projectionUniform;
static C3D_Mtx s_projectionMatrix;

// Vertex buffer
static C3D_Vertex s_vertices[MAX_VERTICES];
static u32 s_vertexCount = 0;

/******************************************************************************/
// Helper functions

static void setup_projection_matrix(int width, int height)
{
    // Orthographic projection for 2D rendering
    Mtx_OrthoTilt(&s_projectionMatrix, 0.0f, (float)width, (float)height, 0.0f, 0.0f, 1.0f, true);
}

static void update_palette_texture(void)
{
    // Convert RGB palette to RGBA8 format for PICA200
    u32* palette_rgba = (u32*)linearAlloc(256 * sizeof(u32));
    if (!palette_rgba)
        return;

    for (int i = 0; i < 256; i++) {
        u8 r = lbPalette[i * 3 + 0];
        u8 g = lbPalette[i * 3 + 1];
        u8 b = lbPalette[i * 3 + 2];
        // RGBA8 format for 3DS
        palette_rgba[i] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
    }

    // Upload palette texture
    C3D_TexUpload(&s_paletteTexture, palette_rgba);
    linearFree(palette_rgba);
}

static void flush_vertices(void)
{
    if (s_vertexCount == 0)
        return;

    // Set up vertex buffer
    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, s_vertices, sizeof(C3D_Vertex), 3, 0x210);

    // Draw triangles
    C3D_DrawArrays(GPU_TRIANGLES, 0, s_vertexCount);

    s_vertexCount = 0;
}

/******************************************************************************/
// Renderer interface implementation

static TbResult renderer_citro3d_init(struct SDL_Window* window, int width, int height)
{
    // Initialize citro3d
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

    // Create render target for top screen
    s_target = C3D_RenderTargetCreate(SCREEN_HEIGHT, SCREEN_WIDTH, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    if (!s_target) {
        C3D_Fini();
        return Lb_FAIL;
    }

    C3D_RenderTargetSetOutput(s_target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    // Load shader
    s_shaderDvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
    if (!s_shaderDvlb) {
        C3D_RenderTargetDelete(s_target);
        C3D_Fini();
        return Lb_FAIL;
    }

    shaderProgramInit(&s_shaderProgram);
    shaderProgramSetVsh(&s_shaderProgram, &s_shaderDvlb->DVLE[0]);
    C3D_BindProgram(&s_shaderProgram);

    // Get uniform locations
    s_projectionUniform = shaderInstanceGetUniformLocation(s_shaderProgram.vertexShader, "projection");

    // Setup projection matrix
    setup_projection_matrix(SCREEN_WIDTH, SCREEN_HEIGHT);

    // Initialize palette texture (256x1 RGBA8)
    C3D_TexInit(&s_paletteTexture, 256, 1, GPU_RGBA8);
    C3D_TexSetFilter(&s_paletteTexture, GPU_NEAREST, GPU_NEAREST);
    update_palette_texture();

    // Initialize atlas texture (placeholder - would be populated with actual texture data)
    C3D_TexInit(&s_atlasTexture, 256, 256, GPU_L8);  // 8-bit luminance for palette indices
    C3D_TexSetFilter(&s_atlasTexture, GPU_NEAREST, GPU_NEAREST);

    // Configure attributes for vertex shader
    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3);  // v0 = position
    AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2);  // v1 = texcoord
    AttrInfo_AddLoader(attrInfo, 2, GPU_UNSIGNED_BYTE, 4);  // v2 = color

    // Configure texture environment
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, 0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);

    return Lb_SUCCESS;
}

static void renderer_citro3d_shutdown(void)
{
    // Clean up textures
    C3D_TexDelete(&s_atlasTexture);
    C3D_TexDelete(&s_paletteTexture);

    // Clean up shader
    if (s_shaderDvlb)
        DVLB_Free(s_shaderDvlb);
    shaderProgramFree(&s_shaderProgram);

    // Clean up render target
    if (s_target)
        C3D_RenderTargetDelete(s_target);

    // Finalize citro3d
    C3D_Fini();
}

static void renderer_citro3d_begin_frame(void)
{
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C3D_RenderTargetClear(s_target, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
    C3D_FrameDrawOn(s_target);

    // Update projection matrix uniform
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, s_projectionUniform, &s_projectionMatrix);

    // Bind textures
    C3D_TexBind(0, &s_atlasTexture);
    C3D_TexBind(1, &s_paletteTexture);

    s_vertexCount = 0;
}

static void renderer_citro3d_end_frame(void)
{
    flush_vertices();
    C3D_FrameEnd(0);
}

static void renderer_citro3d_draw_gpoly(struct PolyPoint* p1, struct PolyPoint* p2, 
                                        struct PolyPoint* p3)
{
    // Convert PolyPoint to C3D_Vertex
    // Note: This is a simplified implementation
    // In practice, you'd need to handle vec_mode, vec_map, vec_colour globals
    
    if (s_vertexCount + 3 >= MAX_VERTICES) {
        flush_vertices();
    }

    // Add three vertices for triangle
    for (int i = 0; i < 3; i++) {
        struct PolyPoint* p = (i == 0) ? p1 : (i == 1) ? p2 : p3;
        C3D_Vertex* v = &s_vertices[s_vertexCount++];
        
        v->position[0] = (float)p->X;
        v->position[1] = (float)p->Y;
        v->position[2] = 0.0f;
        v->texcoord[0] = (float)p->U / 65536.0f;
        v->texcoord[1] = (float)p->V / 65536.0f;
        
        // Pack shade value in red channel
        u8 shade = (u8)((p->S * 255) / 256);
        v->color[0] = shade;
        v->color[1] = shade;
        v->color[2] = shade;
        v->color[3] = 255;
    }
}

static void renderer_citro3d_draw_trig(struct PolyPoint* p1, struct PolyPoint* p2, 
                                       struct PolyPoint* p3)
{
    // For now, use same implementation as draw_gpoly
    renderer_citro3d_draw_gpoly(p1, p2, p3);
}

/******************************************************************************/
static RendererInterface renderer_citro3d_impl = {
    renderer_citro3d_init,
    renderer_citro3d_shutdown,
    renderer_citro3d_begin_frame,
    renderer_citro3d_end_frame,
    renderer_citro3d_draw_gpoly,
    renderer_citro3d_draw_trig,
};

RendererInterface* get_citro3d_renderer(void)
{
    return &renderer_citro3d_impl;
}

/******************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // PLATFORM_3DS
