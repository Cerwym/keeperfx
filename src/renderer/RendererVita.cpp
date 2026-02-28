/******************************************************************************/
// Dungeon Keeper - Renderer Abstraction Layer
/******************************************************************************/
/** @file RendererVita.cpp
 *     PlayStation Vita renderer backend implementation.
 * @par Purpose:
 *     IRenderer for PlayStation Vita.
 *
 *     VITA_HAVE_VITAGL path (vitaGL + vitashark):
 *       vglInitExtended owns the display.  Game framebuffer (8bpp indexed) is
 *       uploaded as a GL_LUMINANCE texture.  A Cg palette-lookup shader maps
 *       each index to RGBA in the fragment stage — zero CPU per-pixel cost.
 *       vglSwapBuffers presents to the Vita screen.
 *
 *     Fallback (no vitaGL):
 *       SDL2 blit path unchanged from the original implementation.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "renderer/RendererVita.h"

#ifdef PLATFORM_VITA

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>

#include "bflib_video.h"
#include "bflib_vidsurface.h"
#include "globals.h"

#ifdef VITA_HAVE_VITAGL
#include <vitashark.h>
#include <psp2/gxm.h>
#endif

#include "post_inc.h"

/******************************************************************************/
#ifdef VITA_HAVE_VITAGL
/******************************************************************************/

// ---------------------------------------------------------------------------
// Cg shader sources (compiled at runtime by vitashark)
// ---------------------------------------------------------------------------

/** Fullscreen quad passthrough — positions already in NDC, no matrix needed. */
static const char* k_vert_src =
    "void main("
    "    float2 aPos : POSITION,"
    "    float2 aUV  : TEXCOORD0,"
    "    out float4 oPos : POSITION,"
    "    out float2 oUV  : TEXCOORD0"
    ") {"
    "    oPos = float4(aPos.x, aPos.y, 0.0f, 1.0f);"
    "    oUV = aUV;"
    "}";

/** Palette lookup: sample 8-bit index → RGBA from 256×1 palette texture. */
static const char* k_frag_src =
    "void main("
    "    float2 vUV : TEXCOORD0,"
    "    out float4 fragColor : COLOR,"
    "    uniform sampler2D indexTex   : TEXUNIT0,"
    "    uniform sampler2D paletteTex : TEXUNIT1"
    ") {"
    "    float idx = tex2D(indexTex, vUV).r;"
    "    fragColor = tex2D(paletteTex, float2(idx, 0.5f));"
    "}";

// ---------------------------------------------------------------------------
// Fullscreen quad — NDC positions, UV (0,0)=top-left to (1,1)=bottom-right
// Drawn as GL_TRIANGLE_STRIP: TL → TR → BL → BR
// ---------------------------------------------------------------------------
static const float k_quad_pos[4][2] = {
    { -1.0f,  1.0f },   // top-left
    {  1.0f,  1.0f },   // top-right
    { -1.0f, -1.0f },   // bottom-left
    {  1.0f, -1.0f },   // bottom-right
};
static const float k_quad_uv[4][2] = {
    { 0.0f, 0.0f },
    { 1.0f, 0.0f },
    { 0.0f, 1.0f },
    { 1.0f, 1.0f },
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static GLuint compile_shader(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        ERRORLOG("RendererVita: shader compile failed (type %d)", (int)type);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

/******************************************************************************/

RendererVita::RendererVita() = default;
RendererVita::~RendererVita() { Shutdown(); }

bool RendererVita::Init()
{
    if (m_initialized) return true;

    // vitashark must be initialised before vglInitExtended so that
    // runtime Cg compilation (via glShaderSource/glCompileShader) is available.
    // NULL = use the system-provided SceShaccCg shader compiler.
    shark_init(NULL);

    // Initialise vitaGL — takes ownership of the GXM context and the display.
    // SDL_CreateWindow has already been called by bflib_video.c (logical window
    // only; SDL does NOT init GXM on Vita until SDL_CreateRenderer is called).
    // We never call SDL_CreateRenderer, so this is the first and only GXM init.
    // 0x800000 (8 MB) vertex/fragment pool; no MSAA at 960×544.
    vglInitExtended(0, 960, 544, 0x800000, SCE_GXM_MULTISAMPLE_NONE);

    // Index texture: 640×480 GL_LUMINANCE — stores raw 8-bit palette indices.
    glGenTextures(1, &m_index_tex);
    glBindTexture(GL_TEXTURE_2D, m_index_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, k_gameW, k_gameH, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

    // Palette texture: 256×1 GL_RGBA — expanded colours for each index.
    glGenTextures(1, &m_palette_tex);
    glBindTexture(GL_TEXTURE_2D, m_palette_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    // Compile palette shader.
    m_vert_shader = compile_shader(GL_VERTEX_SHADER,   k_vert_src);
    m_frag_shader = compile_shader(GL_FRAGMENT_SHADER, k_frag_src);
    if (!m_vert_shader || !m_frag_shader) {
        Shutdown();
        return false;
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, m_vert_shader);
    glAttachShader(m_program, m_frag_shader);
    glBindAttribLocation(m_program, 0, "aPos");
    glBindAttribLocation(m_program, 1, "aUV");
    glLinkProgram(m_program);

    GLint ok = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
    if (!ok) {
        ERRORLOG("RendererVita: palette shader link failed");
        Shutdown();
        return false;
    }

    m_loc_index   = glGetUniformLocation(m_program, "indexTex");
    m_loc_palette = glGetUniformLocation(m_program, "paletteTex");

    glUseProgram(m_program);
    glUniform1i(m_loc_index,   0);
    glUniform1i(m_loc_palette, 1);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    m_initialized = true;
    SYNCLOG("RendererVita: vitaGL palette shader initialised (%dx%d -> 960x544)", k_gameW, k_gameH);
    return true;
}

void RendererVita::Shutdown()
{
    if (!m_initialized) return;

    if (m_program)     { glDeleteProgram(m_program);     m_program     = 0; }
    if (m_vert_shader) { glDeleteShader(m_vert_shader);  m_vert_shader = 0; }
    if (m_frag_shader) { glDeleteShader(m_frag_shader);  m_frag_shader = 0; }
    if (m_index_tex)   { glDeleteTextures(1, &m_index_tex);   m_index_tex   = 0; }
    if (m_palette_tex) { glDeleteTextures(1, &m_palette_tex); m_palette_tex = 0; }
    // vitaGL has no explicit shutdown — GXM is released when the process exits.

    m_initialized = false;
}

bool RendererVita::BeginFrame()
{
    return m_initialized;
}

void RendererVita::EndFrame()
{
    if (!m_initialized) return;

    // Upload 8-bit index buffer (300 KB, direct from game framebuffer).
    SDL_LockSurface(lbDrawSurface);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_index_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, k_gameW, k_gameH,
                    GL_LUMINANCE, GL_UNSIGNED_BYTE, lbDrawSurface->pixels);
    SDL_UnlockSurface(lbDrawSurface);

    // Expand 6-bit lbPalette → 8-bit RGBA and upload (1 KB).
    uint8_t rgba[256 * 4];
    for (int i = 0; i < 256; i++) {
        rgba[i*4+0] = (uint8_t)(lbPalette[i*3+0] << 2);
        rgba[i*4+1] = (uint8_t)(lbPalette[i*3+1] << 2);
        rgba[i*4+2] = (uint8_t)(lbPalette[i*3+2] << 2);
        rgba[i*4+3] = 0xFF;
    }
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_palette_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1,
                    GL_RGBA, GL_UNSIGNED_BYTE, rgba);

    // Draw fullscreen quad — the shader does the palette lookup per-pixel.
    glUseProgram(m_program);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    vglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 4, k_quad_pos);
    vglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 4, k_quad_uv);
    vglDrawObjects(GL_TRIANGLE_STRIP, 4, GL_TRUE);

    vglSwapBuffers(GL_FALSE);
}

uint8_t* RendererVita::LockFramebuffer(int* out_pitch)
{
    if (SDL_LockSurface(lbDrawSurface) < 0) return nullptr;
    if (out_pitch) *out_pitch = lbDrawSurface->pitch;
    return static_cast<uint8_t*>(lbDrawSurface->pixels);
}

void RendererVita::UnlockFramebuffer()
{
    SDL_UnlockSurface(lbDrawSurface);
}

/******************************************************************************/
#else  // VITA_HAVE_VITAGL — SDL2 CPU blit fallback
/******************************************************************************/

RendererVita::RendererVita() = default;
RendererVita::~RendererVita() { Shutdown(); }

bool RendererVita::Init()
{
    if (m_initialized) return true;

    m_renderer = SDL_CreateRenderer(lbWindow, -1,
                                    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) {
        ERRORLOG("RendererVita: SDL_CreateRenderer failed: %s", SDL_GetError());
        return false;
    }

    SDL_RenderSetLogicalSize(m_renderer, k_gameW, k_gameH);

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
        SDL_DestroyTexture(m_texture);   m_texture  = nullptr;
        SDL_DestroyRenderer(m_renderer); m_renderer = nullptr;
        return false;
    }

    m_initialized = true;
    SYNCLOG("RendererVita: SDL2 blit fallback initialised %dx%d -> 960x544", k_gameW, k_gameH);
    return true;
}

void RendererVita::Shutdown()
{
    if (!m_initialized) return;

    free(m_rgbaBuffer);  m_rgbaBuffer = nullptr;
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
    if (SDL_LockSurface(lbDrawSurface) < 0) return nullptr;
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
    const int n = k_gameW * k_gameH;
    for (int i = 0; i < n; i++) {
        int idx = src[i];
        m_rgbaBuffer[i*4+0] = lbPalette[idx*3+0] << 2;
        m_rgbaBuffer[i*4+1] = lbPalette[idx*3+1] << 2;
        m_rgbaBuffer[i*4+2] = lbPalette[idx*3+2] << 2;
        m_rgbaBuffer[i*4+3] = 0xFF;
    }
}

#endif // VITA_HAVE_VITAGL
#endif // PLATFORM_VITA
