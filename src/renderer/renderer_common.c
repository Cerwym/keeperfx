/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file renderer_common.c
 *     Common renderer functionality.
 * @par Purpose:
 *     Provides common renderer functions and global state.
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
#include "pre_inc.h"
#include "renderer_interface.h"

#include "post_inc.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/** Global renderer instance - initialized at startup */
RendererInterface* g_renderer = NULL;

/******************************************************************************/
#ifdef __cplusplus
}
#endif
