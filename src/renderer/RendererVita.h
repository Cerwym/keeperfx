/******************************************************************************/
// Dungeon Keeper - Renderer Abstraction Layer
/******************************************************************************/
/** @file RendererVita.h
 *     PlayStation Vita renderer backend declaration.
 * @par Purpose:
 *     C++ IRenderer implementation using vitaGL (OpenGL ES over GXM).
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
 * Uses vitaGL as an OpenGL ES 2.0 wrapper over the PSP2/GXM hardware.
 * The 8-bit paletted framebuffer is uploaded each frame as a GL texture
 * and rendered as a fullscreen quad — the same blit-based approach as
 * the desktop OpenGL backend.
 *
 * Resolution: 960x544 (native Vita OLED / LCD).
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

    const char* GetName() const override { return "Vita (vitaGL)"; }
    bool SupportsRuntimeSwitch() const override { return false; }

private:
    bool m_initialized = false;
    uint8_t* m_framebuffer = nullptr;   /**< CPU-side staging buffer (8-bit indexed) */
    uint8_t* m_rgbaBuffer  = nullptr;   /**< RGBA expanded buffer for GL upload */
    int m_width  = 960;
    int m_height = 544;

    unsigned int m_texture   = 0;
    unsigned int m_program   = 0;
    unsigned int m_vao       = 0;
    unsigned int m_vbo       = 0;

    void ExpandPalette();   /**< Convert 8-bit indexed to RGBA using lbPalette */
};

#endif // PLATFORM_VITA
#endif // RENDERER_VITA_H
