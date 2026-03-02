/******************************************************************************/
// Dungeon Keeper - Renderer Abstraction Layer
/******************************************************************************/
/** @file RendererVita.h
 *     PlayStation Vita renderer backend declaration.
 * @par Purpose:
 *     IRenderer implementation for the PlayStation Vita.
 *
 *     When VITA_HAVE_VITAGL is defined at compile time AND vitaGL
 *     pre-initialises successfully at runtime (vita_vitagl_preinit()),
 *     the GPU palette-lookup path is used:
 *       8-bit indexed framebuffer → GL_LUMINANCE texture → Cg palette
 *       shader → 960×544 screen.  Zero CPU per-pixel cost.
 *
 *     If vitaGL is not available or pre-init fails, falls back to:
 *       SDL2 blit path — palette-expand on CPU, SDL_UpdateTexture,
 *       SDL_RenderCopy.
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
 * Vita renderer backend.  Tries vitaGL at runtime; falls back to SDL2.
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
        return m_use_vitagl ? "Vita (vitaGL palette shader)"
                            : "Vita (SDL2 blit fallback)";
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
    bool   m_use_vitagl  = false; /**< true when vitaGL preinit succeeded */

    // vitaGL GPU path members
    GLuint m_index_tex   = 0;   /**< 640×480 GL_LUMINANCE: 8-bit palette indices */
    GLuint m_palette_tex = 0;   /**< 256×1  GL_RGBA:       expanded palette colours */
    GLuint m_vert_shader = 0;
    GLuint m_frag_shader = 0;
    GLuint m_program     = 0;
    GLint  m_loc_index   = -1;  /**< uniform "indexTex"   */
    GLint  m_loc_palette = -1;  /**< uniform "paletteTex" */
#endif

    // SDL2 CPU blit path members (always compiled in; used when vitaGL unavailable)
    struct SDL_Renderer* m_renderer   = nullptr;
    struct SDL_Texture*  m_texture    = nullptr;
    uint8_t*             m_rgbaBuffer = nullptr;
    int                  m_surfW      = 0;
    int                  m_surfH      = 0;
    uint32_t             m_paletteLut[256] = {};

    bool EnsureSurface(int w, int h);
    void RebuildPaletteLut();
    void ExpandPaletteFrom(const uint8_t* src);
};

#endif // PLATFORM_VITA
#endif // RENDERER_VITA_H
