/******************************************************************************/
// Dungeon Keeper - Renderer Abstraction Layer
/******************************************************************************/
/** @file RendererVita.h
 *     PlayStation Vita renderer backend declaration.
 * @par Purpose:
 *     IRenderer implementation for the PlayStation Vita.
 *
 *     When VITA_HAVE_VITAGL is defined (vitaGL + vitashark installed):
 *       Uses vitaGL directly.  The 8-bit indexed game framebuffer is uploaded
 *       as a GL_LUMINANCE texture.  A palette-lookup Cg fragment shader
 *       converts each index to RGBA using a 256×1 palette texture, entirely
 *       on the GPU.  The CPU ExpandPalette loop is eliminated.
 *
 *     Otherwise (fallback):
 *       SDL2 blit path — palette-expand on CPU, SDL_UpdateTexture, SDL_RenderCopy.
 */
/******************************************************************************/
#ifndef RENDERER_VITA_H
#define RENDERER_VITA_H

#ifdef PLATFORM_VITA

#include "IRenderer.h"

#ifdef VITA_HAVE_VITAGL
#include <vitaGL.h>
#endif

/**
 * Vita renderer backend.
 *
 * Resolution: 640×480 logical (game), letterboxed to 960×544 physical.
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

    const char* GetName() const override {
#ifdef VITA_HAVE_VITAGL
        return "Vita (vitaGL palette shader)";
#else
        return "Vita (SDL2 blit)";
#endif
    }
    bool SupportsRuntimeSwitch() const override { return false; }

private:
    static const int k_gameW = 640;
    static const int k_gameH = 480;

    bool m_initialized = false;

#ifdef VITA_HAVE_VITAGL
    // vitaGL GPU path — no CPU per-pixel work, no RGBA staging buffer
    GLuint m_index_tex   = 0;   /**< 640×480 GL_LUMINANCE: 8-bit palette indices */
    GLuint m_palette_tex = 0;   /**< 256×1  GL_RGBA:       expanded palette colours */
    GLuint m_vert_shader = 0;
    GLuint m_frag_shader = 0;
    GLuint m_program     = 0;
    GLint  m_loc_index   = -1;  /**< uniform "indexTex"   */
    GLint  m_loc_palette = -1;  /**< uniform "paletteTex" */
#else
    // SDL2 CPU blit fallback
    struct SDL_Renderer* m_renderer  = nullptr;
    struct SDL_Texture*  m_texture   = nullptr;
    uint8_t*             m_rgbaBuffer = nullptr;

    void ExpandPaletteFrom(const uint8_t* src);
#endif
};

#endif // PLATFORM_VITA
#endif // RENDERER_VITA_H
