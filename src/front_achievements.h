/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file front_achievements.h
 *     Header file for front_achievements.c.
 * @par Purpose:
 *     Achievements screen displaying routines.
 * @par Comment:
 *     Just a header file - #defines, typedefs, function prototypes etc.
 * @author   KeeperFX Team
 * @date     04 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef DK_FRONT_ACHIEVEMENTS_H
#define DK_FRONT_ACHIEVEMENTS_H

#include "globals.h"
#include "bflib_basics.h"
#include "bflib_guibtns.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
extern int achievements_scroll_offset;
extern int frontend_achievements_items_visible;
/******************************************************************************/
void frontend_draw_achievements_list(struct GuiButton *gbtn);
void frontend_quit_achievements_screen(struct GuiButton *gbtn);
void frontend_maintain_achievements_ok_button(struct GuiButton *gbtn);
void achievements_scroll_up(struct GuiButton *gbtn);
void achievements_scroll_down(struct GuiButton *gbtn);
void achievements_scroll(struct GuiButton *gbtn);
void frontend_achievements_scroll_up_maintain(struct GuiButton *gbtn);
void frontend_achievements_scroll_down_maintain(struct GuiButton *gbtn);
void frontend_achievements_scroll_tab_maintain(struct GuiButton *gbtn);
void frontend_draw_achievements_scroll_tab(struct GuiButton *gbtn);
void frontend_achievements_update(void);
unsigned long count_displayable_achievements(void);
/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif
