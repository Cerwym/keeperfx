/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file renderer_hardware.c
 *     Hardware renderer stub implementation.
 * @par Purpose:
 *     Provides a stub OpenGL hardware renderer for future implementation.
 *     Currently not functional - placeholder for build system.
 * @par Comment:
 *     This is a stub to avoid unused variable warnings and support future
 *     hardware rendering implementation.
 * @author   KeeperFX Team
 * @date     2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "renderer_interface.h"

#include "bflib_render.h"
#include "bflib_video.h"
#include "globals.h"

#include "post_inc.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
static TbResult hardware_renderer_init(struct SDL_Window* window, int width, int height)
{
    // TODO: Initialize OpenGL context
    ERRORLOG("Hardware renderer not yet implemented");
    return 0; // Failure
}

static void hardware_renderer_shutdown(void)
{
    // TODO: Cleanup OpenGL context
}

static void hardware_renderer_begin_frame(void)
{
    // TODO: Begin OpenGL frame rendering
}

static void hardware_renderer_end_frame(void)
{
    // TODO: Present OpenGL frame buffer
}

static void hardware_renderer_draw_gpoly(struct PolyPoint* p1, struct PolyPoint* p2, 
                                         struct PolyPoint* p3)
{
    // TODO: Render polygon using OpenGL
}

static void hardware_renderer_draw_trig(struct PolyPoint* p1, struct PolyPoint* p2, 
                                        struct PolyPoint* p3)
{
    // TODO: Render triangle using OpenGL
}

/******************************************************************************/
static RendererInterface hardware_renderer = {
    .init = hardware_renderer_init,
    .shutdown = hardware_renderer_shutdown,
    .begin_frame = hardware_renderer_begin_frame,
    .end_frame = hardware_renderer_end_frame,
    .draw_gpoly = hardware_renderer_draw_gpoly,
    .draw_trig = hardware_renderer_draw_trig,
};

RendererInterface* get_hardware_renderer(void)
{
    return &hardware_renderer;
}

/******************************************************************************/
#ifdef __cplusplus
}
#endif
