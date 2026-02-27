/******************************************************************************/
// Dungeon Keeper - Renderer Abstraction Layer
/******************************************************************************/
/** @file platform_gl_sdl2.cpp
 *     OpenGL context management for SDL2-based desktop platforms.
 * @par Purpose:
 *     Implements platform_create_gl_context / platform_destroy_gl_context /
 *     platform_swap_gl_buffers for any platform using SDL2 as the windowing
 *     layer (Windows, Linux, macOS).
 *
 *     For platforms with custom windowing (Vita/vitaGL, 3DS/citro3d, Wii/GX),
 *     provide a separate implementation file instead of this one.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "platform.h"

#include <SDL2/SDL.h>
#include "post_inc.h"

/******************************************************************************/

static SDL_GLContext s_glContext = nullptr;

extern "C" int platform_create_gl_context(void *sdl_window)
{
    SDL_Window *window = static_cast<SDL_Window*>(sdl_window);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    s_glContext = SDL_GL_CreateContext(window);
    if (!s_glContext)
        return 0;

    SDL_GL_MakeCurrent(window, s_glContext);
    return 1;
}

extern "C" void platform_destroy_gl_context(void)
{
    if (s_glContext)
    {
        SDL_GL_DeleteContext(s_glContext);
        s_glContext = nullptr;
    }
}

extern "C" void platform_swap_gl_buffers(void *sdl_window)
{
    SDL_GL_SwapWindow(static_cast<SDL_Window*>(sdl_window));
}
