/******************************************************************************/
// Dungeon Keeper - Renderer Abstraction Layer
/******************************************************************************/
/** @file RendererVita.cpp
 *     PlayStation Vita renderer backend implementation.
 * @par Purpose:
 *     IRenderer for PlayStation Vita.
 *
 *     When VITA_HAVE_VITAGL is compiled in AND vita_vitagl_preinit()
 *     succeeds (called before SDL_Init from LbScreenInitialize), the GPU
 *     palette-lookup path is used.  Otherwise falls back to SDL2 blit.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "renderer/RendererVita.h"

#ifdef PLATFORM_VITA

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bflib_video.h"
#include "bflib_vidsurface.h"
#include "globals.h"

#ifdef VITA_HAVE_VITAGL
#include <psp2/io/stat.h>    // sceIoMkdir
#include <psp2/kernel/sysmem.h>  // sceKernelAllocMemBlock probe
#include <psp2/kernel/processmgr.h>  // sceKernelExitProcess
#include <psp2/ctrl.h>
extern "C" {
#include "platform/debugscreen/debugScreen.h"
#include "platform/vita_sce_diag.h"
}
#endif

#include "post_inc.h"

/******************************************************************************/
// vitaGL pre-initialisation (VITA_HAVE_VITAGL path)
// Called from LbScreenInitialize() BEFORE SDL_Init(SDL_INIT_VIDEO).
/******************************************************************************/

#ifdef VITA_HAVE_VITAGL

static bool s_vitagl_ready = false;

extern "C" bool vita_is_vitagl_ready(void) { return s_vitagl_ready; }

// ---------------------------------------------------------------------------
// Show an on-screen error message using the debug framebuffer (no vitaGL needed)
// and wait for the user to press a button before exiting cleanly.
// If CDRAM allocation fails (e.g. after a partial vitaGL init), the message is
// written to kfx_fatal.log so it isn't silently lost, then the process exits.
static void vita_fatal_dialog(const char* msg)
{
    // Always persist the message to a log first, in case screen init fails.
    FILE* lf = fopen("ux0:data/keeperfx/kfx_fatal.log", "w");
    if (lf) { fprintf(lf, "KeeperFX Fatal Error:\n%s\n", msg); fclose(lf); }

    if (psvDebugScreenInit() < 0) {
        // CDRAM unavailable (likely leaked by a partial vitaGL init).
        // Message is in kfx_fatal.log; exit cleanly without crashing.
        sceKernelExitProcess(1);
        return;
    }

    psvDebugScreenSetFgColor(0xFF4040); // red text
    psvDebugScreenPrintf("\n\n  KeeperFX -- Fatal Error\n\n");
    psvDebugScreenSetFgColor(0xFFFFFF); // white body
    psvDebugScreenPrintf("  %s\n\n", msg);
    psvDebugScreenSetFgColor(0xAAAAAA); // grey hint
    psvDebugScreenPrintf("  Press any button to exit.\n");

    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    SceCtrlData pad;
    // Drain any held-down buttons first, then wait for a fresh press.
    do { sceCtrlReadBufferPositive(0, &pad, 1); } while (pad.buttons != 0);
    do { sceCtrlReadBufferPositive(0, &pad, 1); } while (pad.buttons == 0);

    sceKernelExitProcess(0);
}

// ---------------------------------------------------------------------------
// Preinit step log — strncat entries here during preinit (before keeperfx.log
// is open), then SYNCLOG the full string in RendererVita::Init().
// ---------------------------------------------------------------------------
#define PREINIT_LOG_MAX 256
static char s_preinit_log[PREINIT_LOG_MAX];

extern "C" void vita_vitagl_preinit(void)
{
    if (s_vitagl_ready) return;

    s_preinit_log[0] = '\0';
    strncat(s_preinit_log, "start;", PREINIT_LOG_MAX - 1);

    // Ensure log directory exists and write "start" before any vgl calls.
    // If the process dies inside any vgl call, the last line in preinit.log
    // tells us exactly which step was fatal.
    sceIoMkdir("ux0:data/keeperfx", 0777);  // no-op if already exists
    { FILE* _tr = fopen("ux0:data/keeperfx/kfx_preinit.log", "w"); if (_tr) fclose(_tr); }
#define PREINIT_LOG(step) \
    strncat(s_preinit_log, " " step ";", PREINIT_LOG_MAX - 1); \
    { FILE* _pf = fopen("ux0:data/keeperfx/kfx_preinit.log", "a"); \
      if (_pf) { fprintf(_pf, step "\n"); fclose(_pf); } }

    PREINIT_LOG("start")

    // -----------------------------------------------------------------------
    // Diagnostic probe — log memory state before touching vitaGL.
    // -----------------------------------------------------------------------
    FILE* _diag = fopen("ux0:data/keeperfx/kfx_preinit.log", "a");
    if (_diag) {
        // CDRAM (GPU-visible RAM) availability
        SceUID cdram_blk = sceKernelAllocMemBlock("probe_cdram", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, 256*1024, NULL);
        fprintf(_diag, "sceKernelAllocMemBlock(CDRAM) uid=0x%08X\n", (unsigned)cdram_blk);
        if (cdram_blk >= 0) sceKernelFreeMemBlock(cdram_blk);

        SceUID cdram_big = sceKernelAllocMemBlock("probe_cdram_big", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, 4*1024*1024, NULL);
        fprintf(_diag, "sceKernelAllocMemBlock(CDRAM 4MB) uid=0x%08X\n", (unsigned)cdram_big);
        if (cdram_big >= 0) sceKernelFreeMemBlock(cdram_big);

        // Heap headroom before vitaGL allocates
        void* heap_probe_128 = malloc(100 * 1024 * 1024);
        fprintf(_diag, "malloc(100MB) = %s\n", heap_probe_128 ? "OK" : "FAIL");
        if (heap_probe_128) free(heap_probe_128);
        void* heap_probe_32 = malloc(32 * 1024 * 1024);
        fprintf(_diag, "malloc(32MB) = %s\n", heap_probe_32 ? "OK" : "FAIL");
        if (heap_probe_32) free(heap_probe_32);

        fclose(_diag);
    }
    strncat(s_preinit_log, " probed;", PREINIT_LOG_MAX - 1);
    PREINIT_LOG("gxmProbeOK")

    // Follow VitaQuake2: only set VDM buffer size; do NOT override vertex pool
    // (default 32 MB is sufficient for 2D blit workload and conserves heap).
    // Use vglInitExtended(0,...) auto-detect for GXM buffer size, same as Q2.
    vglSetVDMBufferSize(1024 * 1024);
    PREINIT_LOG("vdmBuf")

#ifdef VITA_SCE_DIAG
    vita_diag_install_hooks();
#endif
    GLboolean vgl_ok = vglInitExtended(0, 960, 544, 0x1000000, SCE_GXM_MULTISAMPLE_NONE);
#ifdef VITA_SCE_DIAG
    vita_diag_unhook_all();
    { FILE* _pf = fopen("ux0:data/keeperfx/kfx_preinit.log", "a");
      if (_pf) { fprintf(_pf, "[SCE_DIAG] vglInitExtended returned %s\n",
                         vgl_ok == GL_TRUE ? "GL_TRUE" : "GL_FALSE"); fclose(_pf); } }
#endif
    if (vgl_ok == GL_FALSE) {
        PREINIT_LOG("vglInitFAILED")
        vita_fatal_dialog(
            "KeeperFX: GPU initialisation failed.\n\n"
            "vitaGL (vglInitExtended) returned GL_FALSE.\n\n"
            "Ensure ur0:data/external/libshacccg.suprx is installed\n"
            "and that no other GXM application is running.");
        // vita_fatal_dialog does not return.
    }
    PREINIT_LOG("vglInit")

    // Mirror VitaQuake2: route all textures through VRAM.
    vglUseVram(GL_TRUE);
    PREINIT_LOG("vram")

    s_vitagl_ready = true;
    strncat(s_preinit_log, " ready", PREINIT_LOG_MAX - 1);
    { FILE* _pf = fopen("ux0:data/keeperfx/kfx_preinit.log", "a");
      if (_pf) { fprintf(_pf, "ready\n"); fclose(_pf); } }

#undef PREINIT_LOG
}

// ---------------------------------------------------------------------------
// Shader loading: pre-compiled .gxp (when psp2cgc is available at build time)
// OR inline Cg compiled by vitashark at runtime (fallback when psp2cgc absent).
// Both VitaQuake1 and VitaQuake2 use the pre-compiled path (glShaderBinary).
// ---------------------------------------------------------------------------

#ifdef VITA_USE_PRECOMPILED_SHADERS
static GLuint load_shader_binary(GLenum type, const char* path)
{
    GLuint sh = glCreateShader(type);
    FILE* f = fopen(path, "rb");
    if (!f) {
        ERRORLOG("RendererVita: cannot open shader binary: %s", path);
        glDeleteShader(sh);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    void* buf = malloc((size_t)sz);
    fread(buf, 1, (size_t)sz, f);
    fclose(f);
    glShaderBinary(1, &sh, 0, buf, (GLsizei)sz);
    free(buf);
    return sh;
}
#else
// Inline Cg source strings — compiled at runtime by vitashark.
// Switch to the pre-compiled path by installing psp2cgc (official Sony SDK).
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

#include <vitashark.h>

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
#endif

static const float k_quad_pos[4][2] = {
    { -1.0f,  1.0f },
    {  1.0f,  1.0f },
    { -1.0f, -1.0f },
    {  1.0f, -1.0f },
};

#endif // VITA_HAVE_VITAGL

/******************************************************************************/
// RendererVita
/******************************************************************************/

RendererVita::RendererVita() = default;
RendererVita::~RendererVita() { Shutdown(); }

bool RendererVita::Init()
{
    if (m_initialized) return true;

#ifdef VITA_HAVE_VITAGL
    // Log preinit steps and report ready state.
    SYNCLOG("[vitaGL] preinit steps: %s", s_preinit_log);
    SYNCLOG("[vitaGL] ready: %s", s_vitagl_ready ? "yes" : "no");

    if (s_vitagl_ready) {
        SYNCLOG("Init: RendererVita: vitaGL ready — GPU palette path active");
        // vitaGL context is up — set up GL resources.
        glGenTextures(1, &m_index_tex);
        glBindTexture(GL_TEXTURE_2D, m_index_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, k_gameW, k_gameH, 0,
                     GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
        { GLenum _e = glGetError(); if (_e != GL_NO_ERROR) ERRORLOG("[vitaGL] GL error 0x%x after index glTexImage2D", _e); }

        glGenTextures(1, &m_palette_tex);
        glBindTexture(GL_TEXTURE_2D, m_palette_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        { GLenum _e = glGetError(); if (_e != GL_NO_ERROR) ERRORLOG("[vitaGL] GL error 0x%x after palette glTexImage2D", _e); }

#ifdef VITA_USE_PRECOMPILED_SHADERS
        m_vert_shader = load_shader_binary(GL_VERTEX_SHADER,   "app0:shaders/vita_blit_v.gxp");
        m_frag_shader = load_shader_binary(GL_FRAGMENT_SHADER, "app0:shaders/vita_blit_f.gxp");
#else
        m_vert_shader = compile_shader(GL_VERTEX_SHADER,   k_vert_src);
        m_frag_shader = compile_shader(GL_FRAGMENT_SHADER, k_frag_src);
#endif
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
        { GLenum _e = glGetError(); if (_e != GL_NO_ERROR) ERRORLOG("[vitaGL] GL error 0x%x after glLinkProgram", _e); }

        m_loc_index   = glGetUniformLocation(m_program, "indexTex");
        m_loc_palette = glGetUniformLocation(m_program, "paletteTex");
        glUseProgram(m_program);
        glUniform1i(m_loc_index,   0);
        glUniform1i(m_loc_palette, 1);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        m_use_vitagl  = true;
        m_initialized = true;
        SYNCLOG("RendererVita: vitaGL palette shader initialised (%dx%d -> 960x544)", k_gameW, k_gameH);
        return true;
    }
    ERRORLOG("RendererVita: vitaGL preinit failed — no SDL2 fallback on Vita");
    return false;
#endif // VITA_HAVE_VITAGL

    // Should not be reached: VITA_HAVE_VITAGL is required for a Vita build.
    ERRORLOG("RendererVita: no renderer available (VITA_HAVE_VITAGL not set?)");
    return false;
}

void RendererVita::Shutdown()
{
    if (!m_initialized) return;

#ifdef VITA_HAVE_VITAGL
    if (m_use_vitagl) {
        if (m_program)     { glDeleteProgram(m_program);               m_program     = 0; }
        if (m_vert_shader) { glDeleteShader(m_vert_shader);            m_vert_shader = 0; }
        if (m_frag_shader) { glDeleteShader(m_frag_shader);            m_frag_shader = 0; }
        if (m_index_tex)   { glDeleteTextures(1, &m_index_tex);        m_index_tex   = 0; }
        if (m_palette_tex) { glDeleteTextures(1, &m_palette_tex);      m_palette_tex = 0; }
        m_use_vitagl  = false;
        m_initialized = false;
        return;
    }
#endif

    free(m_rgbaBuffer);  m_rgbaBuffer = nullptr;
    m_surfW = 0;  m_surfH = 0;
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

#ifdef VITA_HAVE_VITAGL
    if (m_use_vitagl) {
        const int w = lbDrawSurface->w;
        const int h = lbDrawSurface->h;

        SDL_LockSurface(lbDrawSurface);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_index_tex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h,
                        GL_LUMINANCE, GL_UNSIGNED_BYTE, lbDrawSurface->pixels);
        SDL_UnlockSurface(lbDrawSurface);

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

        const float u1 = (float)w / (float)k_gameW;
        const float v1 = (float)h / (float)k_gameH;
        const float dyn_uv[4][2] = {
            { 0.0f, 0.0f }, { u1, 0.0f }, { 0.0f, v1 }, { u1, v1 },
        };

        glUseProgram(m_program);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        vglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 4, k_quad_pos);
        vglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 4, dyn_uv);
        vglDrawObjects(GL_TRIANGLE_STRIP, 4, GL_TRUE);
        vglSwapBuffers(GL_FALSE);
        return;
    }
#endif // VITA_HAVE_VITAGL

    // SDL2 blit path.
    if (!EnsureSurface(lbDrawSurface->w, lbDrawSurface->h)) return;
    RebuildPaletteLut();
    SDL_LockSurface(lbDrawSurface);
    ExpandPaletteFrom(static_cast<const uint8_t*>(lbDrawSurface->pixels));
    SDL_UnlockSurface(lbDrawSurface);
    SDL_UpdateTexture(m_texture, NULL, m_rgbaBuffer, m_surfW * 4);
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

bool RendererVita::EnsureSurface(int w, int h)
{
    if (w == m_surfW && h == m_surfH) return true;

    free(m_rgbaBuffer);  m_rgbaBuffer = nullptr;
    if (m_texture) { SDL_DestroyTexture(m_texture); m_texture = nullptr; }

    m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA32,
                                  SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!m_texture) {
        ERRORLOG("RendererVita: SDL_CreateTexture %dx%d failed: %s", w, h, SDL_GetError());
        return false;
    }

    m_rgbaBuffer = (uint8_t*)malloc((size_t)w * h * 4);
    if (!m_rgbaBuffer) {
        ERRORLOG("RendererVita: failed to allocate RGBA buffer %dx%d", w, h);
        SDL_DestroyTexture(m_texture);  m_texture = nullptr;
        return false;
    }

    SDL_RenderSetLogicalSize(m_renderer, w, h);
    m_surfW = w;  m_surfH = h;
    SYNCLOG("RendererVita: SDL2 surface %dx%d -> 960x544", w, h);
    return true;
}

void RendererVita::RebuildPaletteLut()
{
    for (int i = 0; i < 256; i++) {
        uint8_t r = (uint8_t)(lbPalette[i*3+0] << 2);
        uint8_t g = (uint8_t)(lbPalette[i*3+1] << 2);
        uint8_t b = (uint8_t)(lbPalette[i*3+2] << 2);
        m_paletteLut[i] = (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | 0xFF000000u;
    }
}

void RendererVita::ExpandPaletteFrom(const uint8_t* src)
{
    const int n = m_surfW * m_surfH;
    uint32_t* dst = (uint32_t*)m_rgbaBuffer;
    for (int i = 0; i < n; i++) {
        dst[i] = m_paletteLut[src[i]];
    }
}

#endif // PLATFORM_VITA
