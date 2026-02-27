/******************************************************************************/
// Dungeon Keeper - Renderer Abstraction Layer
/******************************************************************************/
/** @file RendererOpenGL.h
 *     OpenGL framebuffer blit renderer backend (Phase 1).
 */
/******************************************************************************/
#ifndef RENDERER_OPENGL_H
#define RENDERER_OPENGL_H

#include "IRenderer.h"

/******************************************************************************/

/**
 * OpenGL renderer backend — Phase 1 (framebuffer blit).
 *
 * The software rasteriser still runs on the CPU and writes into a staging
 * buffer.  EndFrame() uploads that buffer as an RGBA GL texture and renders
 * it as a fullscreen quad using a minimal shader, then calls
 * platform_swap_gl_buffers() to present the result.
 *
 * This gives a cross-platform display path (GL 3.3 Core) without changing
 * any game logic.  The full GPU geometry pipeline is Phase 2.
 */
class RendererOpenGL : public IRenderer {
public:
    RendererOpenGL();
    ~RendererOpenGL() override;

    bool     Init() override;
    void     Shutdown() override;
    bool     BeginFrame() override;
    void     EndFrame() override;
    uint8_t* LockFramebuffer(int* out_pitch) override;
    void     UnlockFramebuffer() override;
    const char* GetName() const override;
    bool     SupportsRuntimeSwitch() const override;

private:
    bool compile_shaders();
    void upload_palette_texture();

    // Staging buffer (CPU-side, 8-bit paletted, width * height bytes)
    uint8_t* m_stagingBuf  = nullptr;
    int      m_stagingW    = 0;
    int      m_stagingH    = 0;

    // GL objects
    unsigned int m_vao          = 0;
    unsigned int m_vbo          = 0;
    unsigned int m_shader       = 0;
    unsigned int m_texIndex     = 0; // 8-bit index texture
    unsigned int m_texPalette   = 0; // 256-entry RGBA palette texture
};

/******************************************************************************/
#endif // RENDERER_OPENGL_H
