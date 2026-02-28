/******************************************************************************/
// Dungeon Keeper - Renderer Abstraction Layer
/******************************************************************************/
/** @file RendererVita.cpp
 *     PlayStation Vita renderer backend implementation.
 * @par Purpose:
 *     IRenderer implementation using SDL2 hardware presentation for Vita.
 *     Blit-based: 8-bit indexed game framebuffer → RGBA → SDL streaming texture
 *     → SDL_RenderCopy (auto-letterboxed to 960×544 by SDL_RenderSetLogicalSize).
 */
/******************************************************************************/
#include "pre_inc.h"
#include "renderer/RendererVita.h"

#ifdef PLATFORM_VITA

#include <SDL2/SDL.h>
#include <stdlib.h>

#include "bflib_video.h"
#include "bflib_vidsurface.h"
#include "globals.h"
#include "post_inc.h"

/******************************************************************************/

RendererVita::RendererVita() = default;
RendererVita::~RendererVita() { Shutdown(); }

bool RendererVita::Init()
{
    if (m_initialized) return true;

    // lbWindow is created by LbScreenSetup() before Init() is called.
    // SDL_GetWindowSurface has NOT been called on this window (that happens only
    // in RendererSoftware::Init()), so SDL_CreateRenderer succeeds here.
    m_renderer = SDL_CreateRenderer(lbWindow, -1,
                                    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) {
        ERRORLOG("RendererVita: SDL_CreateRenderer failed: %s", SDL_GetError());
        return false;
    }

    // Logical size 640×480 — SDL2 auto-letterboxes into the 960×544 Vita display.
    SDL_RenderSetLogicalSize(m_renderer, k_gameW, k_gameH);

    // Streaming texture at game resolution; updated each frame from lbDrawSurface.
    m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA32,
                                  SDL_TEXTUREACCESS_STREAMING, k_gameW, k_gameH);
    if (!m_texture) {
        ERRORLOG("RendererVita: SDL_CreateTexture failed: %s", SDL_GetError());
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
        return false;
    }

    m_rgbaBuffer = (uint8_t*)malloc((size_t)k_gameW * k_gameH * 4);
    if (!m_rgbaBuffer) {
        ERRORLOG("RendererVita: failed to allocate RGBA buffer");
        SDL_DestroyTexture(m_texture);  m_texture  = nullptr;
        SDL_DestroyRenderer(m_renderer); m_renderer = nullptr;
        return false;
    }

    m_initialized = true;
    SYNCLOG("RendererVita: initialised %dx%d logical -> 960x544 physical", k_gameW, k_gameH);
    return true;
}

void RendererVita::Shutdown()
{
    if (!m_initialized) return;

    free(m_rgbaBuffer);  m_rgbaBuffer  = nullptr;
    if (m_texture)  { SDL_DestroyTexture(m_texture);   m_texture  = nullptr; }
    if (m_renderer) { SDL_DestroyRenderer(m_renderer); m_renderer = nullptr; }

    m_initialized = false;
}

bool RendererVita::BeginFrame()
{
    return m_initialized;
}

void RendererVita::EndFrame()
{
    if (!m_initialized) return;

    // Read pixels from lbDrawSurface (the canonical 8bpp game buffer).
    SDL_LockSurface(lbDrawSurface);
    ExpandPaletteFrom(static_cast<const uint8_t*>(lbDrawSurface->pixels));
    SDL_UnlockSurface(lbDrawSurface);

    SDL_UpdateTexture(m_texture, NULL, m_rgbaBuffer, k_gameW * 4);
    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, NULL, NULL);
    SDL_RenderPresent(m_renderer);
}

uint8_t* RendererVita::LockFramebuffer(int* out_pitch)
{
    if (SDL_LockSurface(lbDrawSurface) < 0)
        return nullptr;
    if (out_pitch) *out_pitch = lbDrawSurface->pitch;
    return static_cast<uint8_t*>(lbDrawSurface->pixels);
}

void RendererVita::UnlockFramebuffer()
{
    SDL_UnlockSurface(lbDrawSurface);
}

void RendererVita::ExpandPaletteFrom(const uint8_t* src)
{
    // lbPalette: 256 RGB triplets, each component 0–63 (6-bit). Shift to 8-bit.
    // SDL_PIXELFORMAT_RGBA32 byte layout: R, G, B, A (platform-invariant).
    const int n = k_gameW * k_gameH;
    for (int i = 0; i < n; i++) {
        int idx = src[i];
        m_rgbaBuffer[i * 4 + 0] = lbPalette[idx * 3 + 0] << 2;  // R
        m_rgbaBuffer[i * 4 + 1] = lbPalette[idx * 3 + 1] << 2;  // G
        m_rgbaBuffer[i * 4 + 2] = lbPalette[idx * 3 + 2] << 2;  // B
        m_rgbaBuffer[i * 4 + 3] = 0xFF;                           // A
    }
}

#endif // PLATFORM_VITA
