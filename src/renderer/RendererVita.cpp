/******************************************************************************/
// Dungeon Keeper - Renderer Abstraction Layer
/******************************************************************************/
/** @file RendererVita.cpp
 *     PlayStation Vita renderer backend implementation.
 * @par Purpose:
 *     IRenderer implementation using vitaGL (OpenGL ES over GXM) for Vita.
 *     Blit-based: 8-bit indexed framebuffer → RGBA → GL texture → fullscreen quad.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "renderer/RendererVita.h"

#ifdef PLATFORM_VITA

#include <vitaGL.h>
#include <string.h>
#include <stdlib.h>

#include "bflib_video.h"
#include "globals.h"
#include "post_inc.h"

/******************************************************************************/
// External palette (256 RGB triplets, 768 bytes total)
extern unsigned char lbPalette[768];

/******************************************************************************/
// Simple passthrough vertex shader (GLSL ES)
static const char* s_vert_src =
    "attribute vec2 a_pos;\n"
    "attribute vec2 a_uv;\n"
    "varying vec2 v_uv;\n"
    "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); v_uv = a_uv; }\n";

// Fragment shader: samples RGBA texture
static const char* s_frag_src =
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform sampler2D u_tex;\n"
    "void main() { gl_FragColor = texture2D(u_tex, v_uv); }\n";

// Fullscreen quad: pos(x,y) + uv(u,v)
static const float s_quad[] = {
    -1.f, -1.f,  0.f, 1.f,
     1.f, -1.f,  1.f, 1.f,
    -1.f,  1.f,  0.f, 0.f,
     1.f,  1.f,  1.f, 0.f,
};

/******************************************************************************/

RendererVita::RendererVita() = default;
RendererVita::~RendererVita() { Shutdown(); }

bool RendererVita::Init()
{
    if (m_initialized) return true;

    // Initialise vitaGL — this sets up the GXM context internally
    vglInit(0x800000);  // 8 MB for the VRAM heap

    // Allocate CPU-side buffers
    m_framebuffer = (uint8_t*)malloc(m_width * m_height);
    m_rgbaBuffer  = (uint8_t*)malloc(m_width * m_height * 4);
    if (!m_framebuffer || !m_rgbaBuffer) {
        ERRORLOG("RendererVita: failed to allocate framebuffers");
        return false;
    }

    // Build shader
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &s_vert_src, NULL);
    glCompileShader(vert);

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &s_frag_src, NULL);
    glCompileShader(frag);

    m_program = glCreateProgram();
    glAttachShader(m_program, vert);
    glAttachShader(m_program, frag);
    glLinkProgram(m_program);
    glDeleteShader(vert);
    glDeleteShader(frag);

    // Upload quad geometry
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_quad), s_quad, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Create framebuffer texture (RGBA, game resolution)
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glViewport(0, 0, m_width, m_height);
    glClearColor(0.f, 0.f, 0.f, 1.f);

    m_initialized = true;
    SYNCLOG("RendererVita: initialised (%dx%d)", m_width, m_height);
    return true;
}

void RendererVita::Shutdown()
{
    if (!m_initialized) return;

    if (m_texture) { glDeleteTextures(1, &m_texture); m_texture = 0; }
    if (m_vbo)     { glDeleteBuffers(1, &m_vbo);      m_vbo = 0;     }
    if (m_program) { glDeleteProgram(m_program);       m_program = 0; }
    free(m_framebuffer); m_framebuffer = nullptr;
    free(m_rgbaBuffer);  m_rgbaBuffer  = nullptr;

    vglEnd();
    m_initialized = false;
}

bool RendererVita::BeginFrame()
{
    glClear(GL_COLOR_BUFFER_BIT);
    return true;
}

void RendererVita::EndFrame()
{
    if (!m_initialized) return;

    ExpandPalette();

    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height,
                    GL_RGBA, GL_UNSIGNED_BYTE, m_rgbaBuffer);

    glUseProgram(m_program);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    GLint a_pos = glGetAttribLocation(m_program, "a_pos");
    GLint a_uv  = glGetAttribLocation(m_program, "a_uv");
    glEnableVertexAttribArray(a_pos);
    glVertexAttribPointer(a_pos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(a_uv);
    glVertexAttribPointer(a_uv, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glUniform1i(glGetUniformLocation(m_program, "u_tex"), 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);

    vglSwapBuffers(GL_FALSE);
}

uint8_t* RendererVita::LockFramebuffer(int* out_pitch)
{
    if (out_pitch) *out_pitch = m_width;
    return m_framebuffer;
}

void RendererVita::UnlockFramebuffer()
{
    // No-op: framebuffer stays valid until EndFrame
}

void RendererVita::ExpandPalette()
{
    const int n = m_width * m_height;
    for (int i = 0; i < n; i++) {
        int idx = m_framebuffer[i];
        m_rgbaBuffer[i * 4 + 0] = lbPalette[idx * 3 + 0] << 2;  // R (6-bit to 8-bit)
        m_rgbaBuffer[i * 4 + 1] = lbPalette[idx * 3 + 1] << 2;  // G
        m_rgbaBuffer[i * 4 + 2] = lbPalette[idx * 3 + 2] << 2;  // B
        m_rgbaBuffer[i * 4 + 3] = 0xFF;                           // A
    }
}

#endif // PLATFORM_VITA
