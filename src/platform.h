#ifndef PLATFORM_H
#define PLATFORM_H

// Platform-specific stuff is declared here

#ifdef __cplusplus
extern "C" {
#endif

int kfxmain(int argc, char *argv[]);
const char * get_os_version(void);
const void * get_image_base(void);
const char * get_wine_version(void);
const char * get_wine_host(void);

/******************************************************************************/
/* OpenGL context management (implemented per-platform for the GL backend)    */
/******************************************************************************/

/** Create an OpenGL context for the given SDL_Window.
 *  Sets the GL context as current on success.
 *  @return Non-zero on success, 0 on failure. */
int platform_create_gl_context(void *sdl_window);

/** Destroy the active OpenGL context. */
void platform_destroy_gl_context(void);

/** Present the back buffer to screen (SDL_GL_SwapWindow equivalent).
 *  @param sdl_window  The SDL_Window to swap. */
void platform_swap_gl_buffers(void *sdl_window);

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_H
