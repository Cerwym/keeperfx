/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file renderer_software.c
 *     Software renderer implementation.
 * @par Purpose:
 *     Wraps existing software rendering functions (trig, draw_gpoly) into
 *     the renderer interface.
 * @par Comment:
 *     This maintains backward compatibility with the existing software renderer.
 * @author   KeeperFX Team
 * @date     2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "../pre_inc.h"
#include "renderer_interface.h"

#include "../bflib_render.h"
#include "../bflib_video.h"
#include "../globals.h"

#include "../post_inc.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
// External functions from existing rendering system
extern void draw_gpoly(struct PolyPoint *point_a, struct PolyPoint *point_b, struct PolyPoint *point_c);
extern void trig(struct PolyPoint *point_a, struct PolyPoint *point_b, struct PolyPoint *point_c);

/******************************************************************************/
static TbResult software_renderer_init(struct SDL_Window* window, int width, int height)
{
    // Software renderer initialization is already handled by existing code
    // in bflib_video and bflib_render setup functions
    return 1; // Success
}

static void software_renderer_shutdown(void)
{
    // Software renderer cleanup is already handled by existing code
}

static void software_renderer_begin_frame(void)
{
    // Frame begin is handled by existing video system
}

static void software_renderer_end_frame(void)
{
    // Frame end is handled by existing video system
}

static void software_renderer_draw_gpoly(struct PolyPoint* p1, struct PolyPoint* p2, 
                                         struct PolyPoint* p3)
{
    // Call the original software renderer function
    draw_gpoly(p1, p2, p3);
}

static void software_renderer_draw_trig(struct PolyPoint* p1, struct PolyPoint* p2, 
                                        struct PolyPoint* p3)
{
    // Call the original software renderer function
    trig(p1, p2, p3);
}

/******************************************************************************/
static RendererInterface software_renderer = {
    .init = software_renderer_init,
    .shutdown = software_renderer_shutdown,
    .begin_frame = software_renderer_begin_frame,
    .end_frame = software_renderer_end_frame,
    .draw_gpoly = software_renderer_draw_gpoly,
    .draw_trig = software_renderer_draw_trig,
};

RendererInterface* get_software_renderer(void)
{
    return &software_renderer;
}

/******************************************************************************/
#ifdef __cplusplus
}
#endif
