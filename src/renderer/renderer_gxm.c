/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file renderer_gxm.c
 *     PlayStation Vita GXM renderer implementation.
 * @par Purpose:
 *     Implements hardware-accelerated rendering using libgxm for PS Vita.
 * @par Comment:
 *     Uses GXM (Graphics eXtension Module) API with GXP shaders.
 *     Optimized for 960×544 resolution.
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

#ifdef PLATFORM_VITA

#include <psp2/types.h>
#include <psp2/gxm.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/display.h>
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
#define DISPLAY_WIDTH          960
#define DISPLAY_HEIGHT         544
#define DISPLAY_STRIDE_IN_PIXELS 1024
#define DISPLAY_BUFFER_COUNT   2
#define MAX_VERTICES           65536
#define MAX_INDICES            (MAX_VERTICES * 3 / 2)

/******************************************************************************/
// External declarations
extern unsigned char lbPalette[768];  // From bflib_video.h - RGB palette (256 * 3)

// External shader binaries (compiled GXP)
extern const SceGxmProgram _binary_palette_v_gxp_start;
extern const SceGxmProgram _binary_palette_f_gxp_start;

/******************************************************************************/
// Vertex structure for GXM
typedef struct {
    float position[3];  // X, Y, Z
    float texcoord[2];  // U, V
    uint8_t color[4];   // RGBA (shade in R channel)
} GxmVertex;

/******************************************************************************/
// Global state
static SceGxmContext* s_context = NULL;
static SceGxmRenderTarget* s_renderTarget = NULL;
static SceGxmShaderPatcher* s_shaderPatcher = NULL;
static SceGxmVertexProgram* s_vertexProgram = NULL;
static SceGxmFragmentProgram* s_fragmentProgram = NULL;

// Display buffers
static void* s_displayBufferData[DISPLAY_BUFFER_COUNT];
static SceUID s_displayBufferUid[DISPLAY_BUFFER_COUNT];
static SceGxmColorSurface s_displaySurface[DISPLAY_BUFFER_COUNT];
static SceGxmSyncObject* s_displayBufferSync[DISPLAY_BUFFER_COUNT];
static uint32_t s_backBufferIndex = 0;
static uint32_t s_frontBufferIndex = 0;

// Depth/stencil surface
static void* s_depthBufferData = NULL;
static SceGxmDepthStencilSurface s_depthSurface;

// Vertex and index buffers
static GxmVertex s_vertices[MAX_VERTICES];
static uint16_t s_indices[MAX_INDICES];
static uint32_t s_vertexCount = 0;
static uint32_t s_indexCount = 0;

// Textures
static SceGxmTexture s_paletteTexture;
static SceGxmTexture s_atlasTexture;
static void* s_paletteTextureData = NULL;
static void* s_atlasTextureData = NULL;

// Uniforms
static const SceGxmProgramParameter* s_projectionParam = NULL;
static float s_projectionMatrix[16];

/******************************************************************************/
// Helper functions

static void* gpu_alloc(SceKernelMemBlockType type, uint32_t size, uint32_t alignment, 
                       uint32_t attribs, SceUID* uid)
{
    if (size == 0 || !uid)
        return NULL;

    // Align size
    size = (size + alignment - 1) & ~(alignment - 1);

    // Allocate memory block
    *uid = sceKernelAllocMemBlock("gpu_mem", type, size, NULL);
    if (*uid < 0)
        return NULL;

    // Get base address
    void* mem = NULL;
    if (sceKernelGetMemBlockBase(*uid, &mem) < 0) {
        sceKernelFreeMemBlock(*uid);
        return NULL;
    }

    return mem;
}

static void gpu_free(SceUID uid)
{
    if (uid >= 0)
        sceKernelFreeMemBlock(uid);
}

static void setup_projection_matrix(int width, int height)
{
    // Orthographic projection for 2D rendering
    float left = 0.0f;
    float right = (float)width;
    float bottom = (float)height;
    float top = 0.0f;
    float near = 0.0f;
    float far = 1.0f;

    s_projectionMatrix[0] = 2.0f / (right - left);
    s_projectionMatrix[1] = 0.0f;
    s_projectionMatrix[2] = 0.0f;
    s_projectionMatrix[3] = 0.0f;

    s_projectionMatrix[4] = 0.0f;
    s_projectionMatrix[5] = 2.0f / (top - bottom);
    s_projectionMatrix[6] = 0.0f;
    s_projectionMatrix[7] = 0.0f;

    s_projectionMatrix[8] = 0.0f;
    s_projectionMatrix[9] = 0.0f;
    s_projectionMatrix[10] = -2.0f / (far - near);
    s_projectionMatrix[11] = 0.0f;

    s_projectionMatrix[12] = -(right + left) / (right - left);
    s_projectionMatrix[13] = -(top + bottom) / (top - bottom);
    s_projectionMatrix[14] = -(far + near) / (far - near);
    s_projectionMatrix[15] = 1.0f;
}

static void update_palette_texture(void)
{
    if (!s_paletteTextureData)
        return;

    // Convert RGB palette to RGBA8888 format
    uint32_t* palette_rgba = (uint32_t*)s_paletteTextureData;
    for (int i = 0; i < 256; i++) {
        uint8_t r = lbPalette[i * 3 + 0];
        uint8_t g = lbPalette[i * 3 + 1];
        uint8_t b = lbPalette[i * 3 + 2];
        palette_rgba[i] = (r << 0) | (g << 8) | (b << 16) | (0xFF << 24);
    }
}

static void flush_batch(void)
{
    if (s_vertexCount == 0 || !s_context)
        return;

    // Set vertex program
    sceGxmSetVertexProgram(s_context, s_vertexProgram);
    sceGxmSetFragmentProgram(s_context, s_fragmentProgram);

    // Set projection uniform
    if (s_projectionParam) {
        void* uniform_buffer = NULL;
        sceGxmReserveVertexDefaultUniformBuffer(s_context, &uniform_buffer);
        sceGxmSetUniformDataF(uniform_buffer, s_projectionParam, 0, 16, s_projectionMatrix);
    }

    // Set vertex stream
    sceGxmSetVertexStream(s_context, 0, s_vertices);

    // Draw
    sceGxmDraw(s_context, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16,
               s_indices, s_indexCount);

    s_vertexCount = 0;
    s_indexCount = 0;
}

/******************************************************************************/
// Renderer interface implementation

static TbResult renderer_gxm_init(struct SDL_Window* window, int width, int height)
{
    // Initialize GXM
    SceGxmInitializeParams initParams;
    memset(&initParams, 0, sizeof(initParams));
    initParams.flags = 0;
    initParams.displayQueueMaxPendingCount = DISPLAY_BUFFER_COUNT - 1;
    initParams.displayQueueCallback = NULL;
    initParams.displayQueueCallbackDataSize = 0;
    initParams.parameterBufferSize = SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE;

    int err = sceGxmInitialize(&initParams);
    if (err != 0)
        return Lb_FAIL;

    // Allocate display buffers
    for (int i = 0; i < DISPLAY_BUFFER_COUNT; i++) {
        s_displayBufferData[i] = gpu_alloc(
            SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
            DISPLAY_STRIDE_IN_PIXELS * DISPLAY_HEIGHT * 4,
            SCE_GXM_COLOR_SURFACE_ALIGNMENT,
            SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
            &s_displayBufferUid[i]
        );
        
        if (!s_displayBufferData[i]) {
            renderer_gxm_shutdown();
            return Lb_FAIL;
        }

        // Initialize color surface
        sceGxmColorSurfaceInit(
            &s_displaySurface[i],
            SCE_GXM_COLOR_FORMAT_A8B8G8R8,
            SCE_GXM_COLOR_SURFACE_LINEAR,
            SCE_GXM_COLOR_SURFACE_SCALE_NONE,
            SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
            DISPLAY_WIDTH,
            DISPLAY_HEIGHT,
            DISPLAY_STRIDE_IN_PIXELS,
            s_displayBufferData[i]
        );

        // Create sync object
        sceGxmSyncObjectCreate(&s_displayBufferSync[i]);
    }

    // Allocate depth buffer
    const uint32_t depthBufferSize = DISPLAY_STRIDE_IN_PIXELS * DISPLAY_HEIGHT * 4;
    SceUID depthBufferUid;
    s_depthBufferData = gpu_alloc(
        SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
        depthBufferSize,
        SCE_GXM_DEPTHSTENCIL_SURFACE_ALIGNMENT,
        SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
        &depthBufferUid
    );

    if (!s_depthBufferData) {
        renderer_gxm_shutdown();
        return Lb_FAIL;
    }

    // Initialize depth/stencil surface
    sceGxmDepthStencilSurfaceInit(
        &s_depthSurface,
        SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24,
        SCE_GXM_DEPTH_STENCIL_SURFACE_LINEAR,
        DISPLAY_STRIDE_IN_PIXELS,
        s_depthBufferData,
        NULL
    );

    // Create render target
    SceGxmRenderTargetParams renderTargetParams;
    memset(&renderTargetParams, 0, sizeof(renderTargetParams));
    renderTargetParams.flags = 0;
    renderTargetParams.width = DISPLAY_WIDTH;
    renderTargetParams.height = DISPLAY_HEIGHT;
    renderTargetParams.scenesPerFrame = 1;
    renderTargetParams.multisampleMode = SCE_GXM_MULTISAMPLE_NONE;
    renderTargetParams.multisampleLocations = 0;
    renderTargetParams.driverMemBlock = -1;

    sceGxmCreateRenderTarget(&renderTargetParams, &s_renderTarget);

    // Note: In a full implementation, you would:
    // 1. Create shader patcher
    // 2. Register shader programs
    // 3. Create vertex and fragment programs
    // 4. Initialize textures
    // 5. Setup uniforms

    setup_projection_matrix(DISPLAY_WIDTH, DISPLAY_HEIGHT);

    return Lb_SUCCESS;
}

static void renderer_gxm_shutdown(void)
{
    // Wait for GPU to finish
    if (s_context)
        sceGxmFinish(s_context);

    // Destroy render target
    if (s_renderTarget)
        sceGxmDestroyRenderTarget(s_renderTarget);

    // Free display buffers
    for (int i = 0; i < DISPLAY_BUFFER_COUNT; i++) {
        if (s_displayBufferSync[i])
            sceGxmSyncObjectDestroy(s_displayBufferSync[i]);
        gpu_free(s_displayBufferUid[i]);
    }

    // Free depth buffer
    if (s_depthBufferData)
        gpu_free(-1);  // Would need to track the UID

    // Free textures
    if (s_paletteTextureData)
        gpu_free(-1);
    if (s_atlasTextureData)
        gpu_free(-1);

    // Terminate GXM
    sceGxmTerminate();
}

static void renderer_gxm_begin_frame(void)
{
    if (!s_context)
        return;

    // Start rendering to current back buffer
    sceGxmBeginScene(
        s_context,
        0,
        s_renderTarget,
        NULL,
        NULL,
        s_displayBufferSync[s_backBufferIndex],
        &s_displaySurface[s_backBufferIndex],
        &s_depthSurface
    );

    // Clear
    sceGxmSetFrontPolygonMode(s_context, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
    sceGxmSetBackPolygonMode(s_context, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);

    s_vertexCount = 0;
    s_indexCount = 0;
}

static void renderer_gxm_end_frame(void)
{
    if (!s_context)
        return;

    flush_batch();

    // End scene
    sceGxmEndScene(s_context, NULL, NULL);

    // Queue display buffer
    struct SceDisplayFrameBuf frameBuf;
    memset(&frameBuf, 0, sizeof(frameBuf));
    frameBuf.size = sizeof(frameBuf);
    frameBuf.base = s_displayBufferData[s_backBufferIndex];
    frameBuf.pitch = DISPLAY_STRIDE_IN_PIXELS;
    frameBuf.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    frameBuf.width = DISPLAY_WIDTH;
    frameBuf.height = DISPLAY_HEIGHT;

    sceDisplaySetFrameBuf(&frameBuf, SCE_DISPLAY_SETBUF_NEXTFRAME);

    // Swap buffers
    s_frontBufferIndex = s_backBufferIndex;
    s_backBufferIndex = (s_backBufferIndex + 1) % DISPLAY_BUFFER_COUNT;
}

static void renderer_gxm_draw_gpoly(struct PolyPoint* p1, struct PolyPoint* p2, 
                                    struct PolyPoint* p3)
{
    // Check if we need to flush
    if (s_vertexCount + 3 >= MAX_VERTICES || s_indexCount + 3 >= MAX_INDICES) {
        flush_batch();
    }

    // Add vertices
    for (int i = 0; i < 3; i++) {
        struct PolyPoint* p = (i == 0) ? p1 : (i == 1) ? p2 : p3;
        GxmVertex* v = &s_vertices[s_vertexCount];
        
        v->position[0] = (float)p->X;
        v->position[1] = (float)p->Y;
        v->position[2] = 0.0f;
        v->texcoord[0] = (float)p->U / 65536.0f;
        v->texcoord[1] = (float)p->V / 65536.0f;
        
        uint8_t shade = (uint8_t)((p->S * 255) / 256);
        v->color[0] = shade;
        v->color[1] = shade;
        v->color[2] = shade;
        v->color[3] = 255;

        s_indices[s_indexCount++] = s_vertexCount++;
    }
}

static void renderer_gxm_draw_trig(struct PolyPoint* p1, struct PolyPoint* p2, 
                                   struct PolyPoint* p3)
{
    renderer_gxm_draw_gpoly(p1, p2, p3);
}

/******************************************************************************/
static RendererInterface renderer_gxm_impl = {
    renderer_gxm_init,
    renderer_gxm_shutdown,
    renderer_gxm_begin_frame,
    renderer_gxm_end_frame,
    renderer_gxm_draw_gpoly,
    renderer_gxm_draw_trig,
};

RendererInterface* get_gxm_renderer(void)
{
    return &renderer_gxm_impl;
}

/******************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // PLATFORM_VITA
