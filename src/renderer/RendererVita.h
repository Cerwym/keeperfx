/******************************************************************************/
// Dungeon Keeper - Renderer Abstraction Layer
/******************************************************************************/
/** @file RendererVita.h
 *     PlayStation Vita renderer backend declaration.
 * @par Purpose:
 *     IRenderer for the PlayStation Vita — vitaGL GPU palette shader path.
 *
 *     8-bit indexed framebuffer → GL_LUMINANCE texture → Cg palette
 *     shader → 960×544 screen.  Zero CPU per-pixel cost.
 */
/******************************************************************************/
#ifndef RENDERER_VITA_H
#define RENDERER_VITA_H

#ifdef PLATFORM_VITA

#include "IRenderer.h"

#include <vitaGL.h>
#include "renderer/vita/VitaBlitShader.h"
#include "renderer/vita/VitaPassthroughPass.h"
#include "renderer/vita/VitaTileAtlas.h"
#include "renderer/vita/VitaSpriteLayer.h"

/**
 * Vita renderer backend — vitaGL GPU palette shader path.
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

    const char* GetName() const override { return "Vita (vitaGL palette shader)"; }
    bool SupportsRuntimeSwitch() const override { return false; }
    bool SupportsGPUPasses() const override { return true; }

    bool PresentRawPalFrame(const uint8_t* pixels, int width, int height,
                            int linesize, const uint8_t* palette_6bit) override;

    VitaTileAtlas&   GetTileAtlas()   { return m_tile_atlas; }
    VitaSpriteLayer& GetSpriteLayer() { return m_sprite_layer; }

private:
    static const int k_gameW = 640;
    static const int k_gameH = 480;

    bool m_initialized = false;

    GLuint m_index_tex   = 0;   /**< 640×480 GL_LUMINANCE: 8-bit palette indices */
    GLuint m_palette_tex = 0;   /**< 256×1  GL_RGBA:       expanded palette colours */

    VitaBlitShader m_blit;      /**< fullscreen palette-decode blit shader */
    VitaPassthroughPass m_passthrough; /**< stage-3 final blit to screen (960x544) */
    VitaTileAtlas m_tile_atlas; /**< GPU tile atlas — 32 RGBA8 2048x1024 textures */
    VitaSpriteLayer m_sprite_layer; /**< GPU UI sprite batch — Tier 1 */

    /** Post-process FBOs — ping-pong between m_pass_fbo_a/b for GPU passes.
     *  m_scene_fbo holds the RGBA-decoded scene produced by m_blit.
     *  Each IPostProcessPass reads from the src texture and writes to the dst FBO. */
    GLuint m_scene_fbo   = 0;   /**< decoded RGBA 640×480 scene render target */
    GLuint m_scene_tex   = 0;
    GLuint m_pass_fbo_a  = 0;   /**< ping-pong FBO A */
    GLuint m_pass_tex_a  = 0;
    GLuint m_pass_fbo_b  = 0;   /**< ping-pong FBO B */
    GLuint m_pass_tex_b  = 0;
};

#endif // PLATFORM_VITA
#endif // RENDERER_VITA_H
