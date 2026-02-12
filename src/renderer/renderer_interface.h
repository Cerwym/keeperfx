/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file renderer_interface.h
 *     Header file for renderer abstraction layer.
 * @par Purpose:
 *     Defines the renderer interface for runtime selection between
 *     software and hardware renderers.
 * @par Comment:
 *     None.
 * @author   KeeperFX Team
 * @date     2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef RENDERER_INTERFACE_H
#define RENDERER_INTERFACE_H

#include "../bflib_basics.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PolyPoint;
struct SDL_Window;

/******************************************************************************/
/**
 * Renderer interface for abstracting different rendering backends.
 * All renderers (software, hardware) must implement this interface.
 */
typedef struct RendererInterface {
    /** Initialize the renderer with given window and dimensions */
    TbResult (*init)(struct SDL_Window* window, int width, int height);
    
    /** Shutdown and clean up the renderer */
    void (*shutdown)(void);
    
    /** Begin a new frame */
    void (*begin_frame)(void);
    
    /** End and present the current frame */
    void (*end_frame)(void);
    
    /** Draw a textured/colored polygon (quad)
     * Uses vec_mode, vec_map, vec_colour globals for rendering state
     */
    void (*draw_gpoly)(struct PolyPoint* p1, struct PolyPoint* p2, 
                       struct PolyPoint* p3);
    
    /** Draw a triangle with gouraud shading or texture
     * Uses vec_mode, vec_map, vec_colour globals for rendering state
     */
    void (*draw_trig)(struct PolyPoint* p1, struct PolyPoint* p2, 
                      struct PolyPoint* p3);
} RendererInterface;

/******************************************************************************/
/** Global renderer instance pointer - set at startup based on config */
extern RendererInterface* g_renderer;

/** Get the software renderer implementation */
RendererInterface* get_software_renderer(void);

/** Get the hardware renderer implementation (stub - not yet implemented) */
RendererInterface* get_hardware_renderer(void);

/** Get the bgfx renderer implementation */
RendererInterface* get_bgfx_renderer(void);

/** Get the citro3d renderer implementation (Nintendo 3DS) */
RendererInterface* get_citro3d_renderer(void);

/** Get the GXM renderer implementation (PlayStation Vita) */
RendererInterface* get_gxm_renderer(void);

/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif // RENDERER_INTERFACE_H
