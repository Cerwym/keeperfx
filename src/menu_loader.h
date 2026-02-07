/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file menu_loader.h
 *     Header file for menu_loader.c.
 * @par Purpose:
 *     JSON menu loader for data-driven UI system.
 * @par Comment:
 *     Loads menu definitions from JSON files and converts them to GuiButtonInit arrays.
 * @author   KeeperFX Team
 * @date     07 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef MENU_LOADER_H
#define MENU_LOADER_H

#include "bflib_basics.h"
#include "bflib_guibtns.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
#pragma pack(1)

/** Maximum number of buttons that can be loaded from a single JSON menu */
#define MAX_JSON_MENU_BUTTONS 50

/** Structure to hold a loaded JSON menu */
struct JsonMenu {
    char menu_id[64];
    int pos_x;
    int pos_y;
    int width;
    int height;
    struct GuiButtonInit *buttons;
    int button_count;
};

#pragma pack()
/******************************************************************************/

/**
 * Load a menu from a JSON file.
 * @param filepath Path to the JSON file.
 * @param menu Pointer to JsonMenu structure to fill.
 * @return True on success, false on failure.
 */
TbBool load_menu_from_json(const char *filepath, struct JsonMenu *menu);

/**
 * Free resources allocated for a JSON menu.
 * @param menu Pointer to JsonMenu structure to free.
 */
void free_json_menu(struct JsonMenu *menu);

/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif // MENU_LOADER_H
