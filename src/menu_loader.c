/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file menu_loader.c
 *     JSON menu loader implementation.
 * @par Purpose:
 *     Loads menu definitions from JSON files and converts them to GuiButtonInit arrays.
 * @par Comment:
 *     Uses centijson library for JSON parsing.
 * @author   KeeperFX Team
 * @date     07 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "menu_loader.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <json.h>
#include <json-dom.h>
#include "bflib_fileio.h"
#include "config_strings.h"
#include "frontend.h"
#include "gui_frontbtns.h"
#include "gui_draw.h"
#include "post_inc.h"

/******************************************************************************/

/** Parse position value from JSON (handles "center" keyword and numbers) */
static int parse_position_value(VALUE *val)
{
    if (value_is_int32(val)) {
        return value_int32(val);
    } else if (value_is_string(val)) {
        const char *str = value_string(val);
        if (strcmp(str, "center") == 0) {
            return POS_SCRCTR; // -997
        }
    }
    return 0;
}

/** Parse a button from JSON */
static TbBool parse_button(VALUE *button_obj, struct GuiButtonInit *btn)
{
    memset(btn, 0, sizeof(struct GuiButtonInit));
    
    // Set defaults
    btn->gbtype = LbBtnT_NormalBtn;
    btn->id_num = BID_DEFAULT;
    btn->button_flags = 0;
    btn->click_event = NULL;
    btn->rclick_event = NULL;
    btn->ptover_event = NULL;
    btn->btype_value = 0;
    btn->draw_call = NULL;
    btn->sprite_idx = 0;
    btn->tooltip_stridx = GUIStr_Empty;
    btn->parent_menu = NULL;
    btn->content.lval = 0;
    btn->maxval = 0;
    btn->maintain_call = NULL;
    
    // Parse button type
    VALUE *type_val = value_dict_get(button_obj, "type");
    if (type_val && value_is_string(type_val)) {
        const char *type = value_string(type_val);
        if (strcmp(type, "NormalBtn") == 0) {
            btn->gbtype = LbBtnT_NormalBtn;
        } else if (strcmp(type, "HoldableBtn") == 0) {
            btn->gbtype = LbBtnT_HoldableBtn;
        } else if (strcmp(type, "ToggleBtn") == 0) {
            btn->gbtype = LbBtnT_ToggleBtn;
        } else if (strcmp(type, "RadioBtn") == 0) {
            btn->gbtype = LbBtnT_RadioBtn;
        }
    }
    
    // Parse position
    VALUE *pos_obj = value_dict_get(button_obj, "position");
    if (pos_obj && value_is_dict(pos_obj)) {
        VALUE *x_val = value_dict_get(pos_obj, "x");
        VALUE *y_val = value_dict_get(pos_obj, "y");
        
        if (x_val) {
            btn->scr_pos_x = parse_position_value(x_val);
            btn->pos_x = btn->scr_pos_x;
        }
        if (y_val) {
            btn->scr_pos_y = parse_position_value(y_val);
            btn->pos_y = btn->scr_pos_y;
        }
    }
    
    // Parse size
    VALUE *size_obj = value_dict_get(button_obj, "size");
    if (size_obj && value_is_dict(size_obj)) {
        VALUE *w_val = value_dict_get(size_obj, "width");
        VALUE *h_val = value_dict_get(size_obj, "height");
        
        if (w_val && value_is_int32(w_val)) {
            btn->width = value_int32(w_val);
        }
        if (h_val && value_is_int32(h_val)) {
            btn->height = value_int32(h_val);
        }
    }
    
    // Parse visual properties
    VALUE *visual_obj = value_dict_get(button_obj, "visual");
    if (visual_obj && value_is_dict(visual_obj)) {
        VALUE *sprite_val = value_dict_get(visual_obj, "sprite_index");
        if (sprite_val && value_is_int32(sprite_val)) {
            btn->sprite_idx = value_int32(sprite_val);
        }
        
        VALUE *draw_cb_val = value_dict_get(visual_obj, "draw_callback");
        if (draw_cb_val && value_is_string(draw_cb_val)) {
            const char *draw_cb = value_string(draw_cb_val);
            // Map draw callback names to function pointers
            if (strcmp(draw_cb, "frontend_draw_large_menu_button") == 0) {
                btn->draw_call = frontend_draw_large_menu_button;
            } else if (strcmp(draw_cb, "frontend_draw_vlarge_menu_button") == 0) {
                btn->draw_call = frontend_draw_vlarge_menu_button;
            }
        }
    }
    
    // Parse text
    VALUE *text_obj = value_dict_get(button_obj, "text");
    if (text_obj && value_is_dict(text_obj)) {
        VALUE *string_id_val = value_dict_get(text_obj, "string_id");
        if (string_id_val && value_is_string(string_id_val)) {
            const char *string_id = value_string(string_id_val);
            // TODO: Map string IDs to actual string indices
            // For now, just store a placeholder
            btn->content.lval = 1;
        }
    }
    
    // Parse state and callbacks
    VALUE *state_obj = value_dict_get(button_obj, "state");
    TbBool is_clickable = true;
    if (state_obj && value_is_dict(state_obj)) {
        VALUE *clickable_val = value_dict_get(state_obj, "clickable");
        if (clickable_val && value_is_bool(clickable_val)) {
            is_clickable = value_bool(clickable_val);
        }
    }
    
    // Parse callbacks
    VALUE *callbacks_obj = value_dict_get(button_obj, "callbacks");
    if (callbacks_obj && value_is_dict(callbacks_obj)) {
        // TODO: Map callback names to function pointers
        // For now, we just check if on_click exists
        VALUE *click_val = value_dict_get(callbacks_obj, "on_click");
        if (click_val && !value_is_null(click_val) && is_clickable) {
            // Set a default click handler - actual mapping would need a callback registry
            // btn->click_event = some_mapped_function;
            // For now, just mark it as clickable by setting ptover_event
            btn->ptover_event = frontend_over_button;
        }
    }
    
    return true;
}

/**
 * Load a menu from a JSON file.
 */
TbBool load_menu_from_json(const char *filepath, struct JsonMenu *menu)
{
    if (menu == NULL) {
        ERRORLOG("Menu pointer is NULL");
        return false;
    }
    
    memset(menu, 0, sizeof(struct JsonMenu));
    
    // Read file into buffer
    long fsize = LbFileLengthRnc(filepath);
    if (fsize <= 0) {
        ERRORLOG("Cannot get file size: %s", filepath);
        return false;
    }
    
    char *file_buffer = (char *)malloc(fsize + 1);
    if (file_buffer == NULL) {
        ERRORLOG("Cannot allocate memory for JSON file");
        return false;
    }
    
    TbFileHandle fhandle = LbFileOpen(filepath, Lb_FILE_MODE_READ_ONLY);
    if (fhandle == NULL) {
        ERRORLOG("Cannot open file: %s", filepath);
        free(file_buffer);
        return false;
    }
    
    long bytes_read = LbFileRead(fhandle, file_buffer, fsize);
    LbFileClose(fhandle);
    
    if (bytes_read != fsize) {
        ERRORLOG("Failed to read complete file: %s", filepath);
        free(file_buffer);
        return false;
    }
    
    file_buffer[fsize] = '\0';
    
    // Parse JSON
    VALUE root;
    JSON_INPUT_POS json_pos;
    int ret = json_dom_parse(file_buffer, fsize, NULL, 0, &root, &json_pos);
    
    if (ret != JSON_ERR_SUCCESS) {
        ERRORLOG("JSON parse error in %s at line %d, col %d", 
                 filepath, json_pos.line_number, json_pos.column_number);
        free(file_buffer);
        return false;
    }
    
    if (!value_is_dict(&root)) {
        ERRORLOG("JSON root is not an object: %s", filepath);
        value_fini(&root);
        free(file_buffer);
        return false;
    }
    
    // Parse menu_id
    VALUE *menu_id_val = value_dict_get(&root, "menu_id");
    if (menu_id_val && value_is_string(menu_id_val)) {
        strncpy(menu->menu_id, value_string(menu_id_val), sizeof(menu->menu_id) - 1);
        menu->menu_id[sizeof(menu->menu_id) - 1] = '\0';
    }
    
    // Parse position
    VALUE *pos_obj = value_dict_get(&root, "position");
    if (pos_obj && value_is_dict(pos_obj)) {
        VALUE *x_val = value_dict_get(pos_obj, "x");
        VALUE *y_val = value_dict_get(pos_obj, "y");
        
        menu->pos_x = x_val ? parse_position_value(x_val) : POS_SCRCTR;
        menu->pos_y = y_val ? parse_position_value(y_val) : POS_SCRCTR;
    } else {
        menu->pos_x = POS_SCRCTR;
        menu->pos_y = POS_SCRCTR;
    }
    
    // Parse size
    VALUE *size_obj = value_dict_get(&root, "size");
    if (size_obj && value_is_dict(size_obj)) {
        VALUE *w_val = value_dict_get(size_obj, "width");
        VALUE *h_val = value_dict_get(size_obj, "height");
        
        menu->width = (w_val && value_is_int32(w_val)) ? value_int32(w_val) : 640;
        menu->height = (h_val && value_is_int32(h_val)) ? value_int32(h_val) : 480;
    } else {
        menu->width = 640;
        menu->height = 480;
    }
    
    // Parse buttons array
    VALUE *buttons_arr = value_dict_get(&root, "buttons");
    if (buttons_arr && value_is_array(buttons_arr)) {
        size_t button_count = value_array_size(buttons_arr);
        
        if (button_count > MAX_JSON_MENU_BUTTONS) {
            WARNLOG("Menu has %zu buttons, limiting to %d", button_count, MAX_JSON_MENU_BUTTONS);
            button_count = MAX_JSON_MENU_BUTTONS;
        }
        
        // Allocate button array (+ 1 for terminator)
        menu->buttons = (struct GuiButtonInit *)malloc(sizeof(struct GuiButtonInit) * (button_count + 1));
        if (menu->buttons == NULL) {
            ERRORLOG("Cannot allocate memory for buttons");
            value_fini(&root);
            free(file_buffer);
            return false;
        }
        
        menu->button_count = 0;
        for (size_t i = 0; i < button_count; i++) {
            VALUE *button_obj = value_array_get(buttons_arr, i);
            if (button_obj && value_is_dict(button_obj)) {
                if (parse_button(button_obj, &menu->buttons[menu->button_count])) {
                    menu->button_count++;
                }
            }
        }
        
        // Add terminator button
        memset(&menu->buttons[menu->button_count], 0, sizeof(struct GuiButtonInit));
        menu->buttons[menu->button_count].gbtype = -1;
        menu->buttons[menu->button_count].id_num = BID_DEFAULT;
    }
    
    value_fini(&root);
    free(file_buffer);
    
    SYNCLOG("Loaded JSON menu '%s' with %d buttons", menu->menu_id, menu->button_count);
    return true;
}

/**
 * Free resources allocated for a JSON menu.
 */
void free_json_menu(struct JsonMenu *menu)
{
    if (menu == NULL) {
        return;
    }
    
    if (menu->buttons != NULL) {
        free(menu->buttons);
        menu->buttons = NULL;
    }
    
    menu->button_count = 0;
}

/******************************************************************************/
