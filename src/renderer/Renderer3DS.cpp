/******************************************************************************/
// Dungeon Keeper - Renderer Abstraction Layer
/******************************************************************************/
/** @file Renderer3DS.cpp
 *     Nintendo 3DS renderer backend implementation.
 * @par Purpose:
 *     IRenderer implementation using citro3d (PICA200 hardware) for 3DS.
 *     Blit-based: 8-bit indexed framebuffer → RGBA8 → C3D texture → fullscreen quad.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "renderer/Renderer3DS.h"

#ifdef PLATFORM_3DS

#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include <stdlib.h>

#include "bflib_video.h"
#include "globals.h"
#include "post_inc.h"

/******************************************************************************/
// External palette (256 RGB triplets, 768 bytes total)
extern unsigned char lbPalette[768];

/******************************************************************************/
// 3DS screen constants
#define SCREEN_TOP_WIDTH  400
#define SCREEN_TOP_HEIGHT 240
#define CLEAR_COLOR       0x000000FF

/******************************************************************************/

static C3D_RenderTarget* s_renderTarget = NULL;
static C3D_Tex           s_fbTexture;

/******************************************************************************/

Renderer3DS::Renderer3DS() = default;
Renderer3DS::~Renderer3DS() { Shutdown(); }

bool Renderer3DS::Init()
{
    if (m_initialized) return true;

    // Initialise graphics subsystems
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

    // Create render target for the top screen
    s_renderTarget = C3D_RenderTargetCreate(SCREEN_TOP_HEIGHT, SCREEN_TOP_WIDTH,
                                             GPU_RB_RGBA8, GPU_RB_DEPTH16);
    C3D_RenderTargetSetOutput(s_renderTarget, GFX_TOP, GFX_LEFT,
                              GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
                              GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8));

    // Allocate CPU-side framebuffer
    m_framebuffer = (uint8_t*)malloc(m_width * m_height);
    m_rgbaBuffer  = (uint8_t*)linearAlloc(m_width * m_height * 4);  // linearAlloc for DMA
    if (!m_framebuffer || !m_rgbaBuffer) {
        ERRORLOG("Renderer3DS: failed to allocate framebuffers");
        return false;
    }

    // Create texture for framebuffer upload
    C3D_TexInit(&s_fbTexture, m_width, m_height, GPU_RGBA8);
    C3D_TexSetFilter(&s_fbTexture, GPU_NEAREST, GPU_NEAREST);

    m_initialized = true;
    SYNCLOG("Renderer3DS: initialised (%dx%d, top screen)", m_width, m_height);
    return true;
}

void Renderer3DS::Shutdown()
{
    if (!m_initialized) return;

    C3D_TexDelete(&s_fbTexture);
    if (s_renderTarget) { C3D_RenderTargetDelete(s_renderTarget); s_renderTarget = NULL; }
    free(m_framebuffer);      m_framebuffer = nullptr;
    linearFree(m_rgbaBuffer); m_rgbaBuffer  = nullptr;
    C3D_Fini();
    gfxExit();
    m_initialized = false;
}

bool Renderer3DS::BeginFrame()
{
    return C3D_FrameBegin(C3D_FRAME_SYNCDRAW) != 0;
}

void Renderer3DS::EndFrame()
{
    if (!m_initialized) return;

    ExpandPalette();

    // Upload RGBA8 data to citro3d texture
    C3D_TexUpload(&s_fbTexture, m_rgbaBuffer);

    // Render fullscreen quad to top screen target
    C3D_FrameDrawOn(s_renderTarget);
    C3D_FrameEnd(0);
}

uint8_t* Renderer3DS::LockFramebuffer(int* out_pitch)
{
    if (out_pitch) *out_pitch = m_width;
    return m_framebuffer;
}

void Renderer3DS::UnlockFramebuffer()
{
    // No-op
}

void Renderer3DS::ExpandPalette()
{
    const int n = m_width * m_height;
    for (int i = 0; i < n; i++) {
        int idx = m_framebuffer[i];
        // PICA200 expects RGBA8 in big-endian layout
        m_rgbaBuffer[i * 4 + 0] = lbPalette[idx * 3 + 0] << 2;  // R
        m_rgbaBuffer[i * 4 + 1] = lbPalette[idx * 3 + 1] << 2;  // G
        m_rgbaBuffer[i * 4 + 2] = lbPalette[idx * 3 + 2] << 2;  // B
        m_rgbaBuffer[i * 4 + 3] = 0xFF;                           // A
    }
}

#endif // PLATFORM_3DS
