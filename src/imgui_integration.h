/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file imgui_integration.h
 *     Header for ImGui integration functions.
 * @par Purpose:
 *     Provides C interface for custom ImGui windows integration.
 * @author   GitHub Copilot / Community
 * @date     24 Jan 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef IMGUI_INTEGRATION_H
#define IMGUI_INTEGRATION_H

#include "bflib_basics.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
// Custom window toggle functions

void ImGui_TogglePossessionSpellConfig(void);
TbBool ImGui_IsPossessionSpellConfigVisible(void);

/******************************************************************************/
// Integration functions (called from bflib_imgui.cpp)

/** Render all registered custom ImGui windows */
void ImGui_RenderCustomWindows(void);

/** Render menu items for opening custom windows */
void ImGui_RenderCustomMenuItems(void);

/******************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // IMGUI_INTEGRATION_H
