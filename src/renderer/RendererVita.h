/******************************************************************************/
// Dungeon Keeper - Renderer Abstraction Layer
/******************************************************************************/
/** @file RendererVita.h
 *     PlayStation Vita renderer backend declaration.
 * @par Purpose:
 *     C++ IRenderer implementation using SDL2 (hardware-accelerated presentation).
 *     Only compiled when PLATFORM_VITA is defined.
 */
/******************************************************************************/
#ifndef RENDERER_VITA_H
#define RENDERER_VITA_H

#ifdef PLATFORM_VITA

#include "IRenderer.h"

/**
 * Vita renderer backend.
 *
 * Uses SDL2 hardware presentation: the 8-bit paletted game framebuffer is
 * palette-expanded to RGBA each frame, uploaded as a streaming SDL_Texture,
 * and presented via SDL_RenderCopy.  SDL_RenderSetLogicalSize(640, 480) makes
 * SDL2 auto-letterbox into the Vita's native 960×544 display.
 *
 * Resolution: 640×480 logical (rendered), 960×544 physical (Vita screen).
 */
class RendererVita : public IRenderer {
public:
    RendererVita();
    ~RendererVita() override;

    bool Init() override;
    void Shutdown() override;

    bool BeginFrame() override;
    void EndFrame() override;

    uint8_t* LockFramebuffer(int* out_pitch) override;
    void UnlockFramebuffer() override;

    const char* GetName() const override { return "Vita (SDL2)"; }
    bool SupportsRuntimeSwitch() const override { return false; }

private:
    static const int k_gameW = 640;
    static const int k_gameH = 480;

    bool          m_initialized = false;
    struct SDL_Renderer* m_renderer  = nullptr;
    struct SDL_Texture*  m_texture   = nullptr;
    uint8_t*      m_rgbaBuffer  = nullptr;   /**< RGBA staging buffer for texture upload */

    void ExpandPaletteFrom(const uint8_t* src);   /**< Convert 8-bit indexed → RGBA using lbPalette */
};

#endif // PLATFORM_VITA
#endif // RENDERER_VITA_H
