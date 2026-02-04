/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file front_achievements.c
 *     Achievements screen displaying routines.
 * @par Purpose:
 *     Functions to show and maintain the achievements screen.
 * @par Comment:
 *     None.
 * @author   KeeperFX Team
 * @date     04 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "front_achievements.h"
#include "globals.h"
#include "bflib_basics.h"

#include "bflib_sprite.h"
#include "bflib_guibtns.h"
#include "bflib_vidraw.h"
#include "bflib_sprfnt.h"
#include "config_campaigns.h"
#include "config_strings.h"
#include "frontend.h"
#include "gui_draw.h"
#include "sprites.h"
#include "gui_frontbtns.h"
#include "achievement_api.h"
#include "achievement_definitions.h"
#include "post_inc.h"

/******************************************************************************/
int achievements_scroll_offset = 0;
int frontend_achievements_items_visible = 10;
static unsigned long achievements_count;

// Sorted indices for displaying achievements
static int sorted_achievement_indices[MAX_ACHIEVEMENTS];
static int sorted_achievements_count = 0;
/******************************************************************************/

/**
 * Sort achievements for display: unlocked first, then in-progress, then hidden
 */
static void sort_achievements_for_display(void)
{
    sorted_achievements_count = 0;
    
    // First pass: unlocked achievements
    for (int i = 0; i < achievement_count; i++)
    {
        if (achievement_is_unlocked(i))
        {
            sorted_achievement_indices[sorted_achievements_count++] = i;
        }
    }
    
    // Second pass: visible achievements in progress (not hidden, not unlocked)
    for (int i = 0; i < achievement_count; i++)
    {
        AchievementDefinition *def = &achievement_definitions[i];
        if (!achievement_is_unlocked(i) && !def->hidden)
        {
            sorted_achievement_indices[sorted_achievements_count++] = i;
        }
    }
    
    // Third pass: hidden achievements (not yet unlocked)
    for (int i = 0; i < achievement_count; i++)
    {
        AchievementDefinition *def = &achievement_definitions[i];
        if (!achievement_is_unlocked(i) && def->hidden)
        {
            sorted_achievement_indices[sorted_achievements_count++] = i;
        }
    }
    
    achievements_count = sorted_achievements_count;
}

unsigned long count_displayable_achievements(void)
{
    sort_achievements_for_display();
    return achievements_count;
}

void draw_achievement_entry(int display_idx, long pos_x, long pos_y, int width, int units_per_px)
{
    if (display_idx >= sorted_achievements_count)
    {
        return;
    }
    
    int ach_idx = sorted_achievement_indices[display_idx];
    AchievementDefinition *def = &achievement_definitions[ach_idx];
    TbBool unlocked = achievement_is_unlocked(ach_idx);
    
    // Draw achievement icon (trophy or custom sprite)
    int icon_size = 24 * units_per_px / 16;
    // TODO: Draw sprite using def->icon_sprite or default trophy
    
    // Draw achievement name
    lbDisplay.DrawFlags = Lb_TEXT_HALIGN_LEFT;
    int text_x = pos_x + icon_size + (4 * units_per_px / 16);
    
    const char *name;
    const char *desc;
    
    if (def->hidden && !unlocked)
    {
        name = "Hidden Achievement";
        desc = "???";
    }
    else
    {
        name = def->name[0] ? def->name : "Unknown Achievement";
        desc = def->description[0] ? def->description : "";
    }
    
    // Draw name
    if (unlocked)
    {
        lbDisplay.DrawColour = colours[18][0][0]; // Gold/yellow color for unlocked
    }
    else
    {
        lbDisplay.DrawColour = colours[2][0][0]; // Grey for locked
    }
    
    LbTextStringDraw(text_x, pos_y, units_per_px, name, Fnt_LeftJustify);
    
    // Draw description on next line
    if (!def->hidden || unlocked)
    {
        lbDisplay.DrawColour = colours[2][0][0]; // Grey for description
        LbTextStringDraw(text_x, pos_y + (12 * units_per_px / 16), units_per_px * 3 / 4, desc, Fnt_LeftJustify);
    }
    
    // Draw progress if available and not unlocked
    if (!unlocked && !def->hidden)
    {
        float progress = achievement_get_progress(ach_idx);
        if (progress > 0.0f && progress < 1.0f)
        {
            char progress_text[32];
            snprintf(progress_text, sizeof(progress_text), "%d%%", (int)(progress * 100));
            int progress_x = pos_x + width - (60 * units_per_px / 16);
            lbDisplay.DrawColour = colours[5][0][0]; // Blue for progress
            LbTextStringDraw(progress_x, pos_y, units_per_px, progress_text, Fnt_RightJustify);
        }
    }
    
    // Draw unlocked checkmark
    if (unlocked)
    {
        int check_x = pos_x + width - (30 * units_per_px / 16);
        LbTextStringDraw(check_x, pos_y, units_per_px, "\x96", Fnt_RightJustify); // Checkmark symbol
    }
}

void frontend_draw_achievements_list(struct GuiButton *gbtn)
{
    count_displayable_achievements();
    gui_draw_scroll_box(gbtn, 12, true);
    
    int fs_units_per_px;
    const struct TbSprite *spr;
    {
        int orig_size = 0;
        spr = get_frontend_sprite(GFS_hugearea_thn_cor_ml);
        for (int i=0; i < 6; i++)
        {
            orig_size += spr->SHeight;
            spr++;
        }
        fs_units_per_px = (gbtn->height * 16 + orig_size/2) / orig_size;
    }
    
    int row_height = 30 * fs_units_per_px / 16;
    frontend_achievements_items_visible = gbtn->height / row_height;
    if (frontend_achievements_items_visible > (int)achievements_count)
        frontend_achievements_items_visible = achievements_count;
    
    int pos_y = gbtn->scr_pos_y;
    for (int i = 0; i < frontend_achievements_items_visible; i++)
    {
        int display_idx = achievements_scroll_offset + i;
        if (display_idx >= (int)achievements_count)
            break;
            
        draw_achievement_entry(display_idx, gbtn->scr_pos_x, pos_y, gbtn->width, fs_units_per_px);
        pos_y += row_height;
    }
}

void frontend_quit_achievements_screen(struct GuiButton *gbtn)
{
    frontend_set_state(FeSt_MAIN_MENU);
}

void frontend_maintain_achievements_ok_button(struct GuiButton *gbtn)
{
    if (gbtn == NULL)
        return;
    gbtn->flags |= LbBtnF_Enabled;
}

void achievements_scroll_up(struct GuiButton *gbtn)
{
    if (achievements_scroll_offset > 0)
        achievements_scroll_offset--;
}

void achievements_scroll_down(struct GuiButton *gbtn)
{
    int max_offset = (int)achievements_count - frontend_achievements_items_visible;
    if (max_offset < 0)
        max_offset = 0;
    if (achievements_scroll_offset < max_offset)
        achievements_scroll_offset++;
}

void achievements_scroll(struct GuiButton *gbtn)
{
    achievements_scroll_offset = frontend_scroll_tab_to_offset(gbtn, GetMouseY(), 
        frontend_achievements_items_visible, achievements_count);
}

void frontend_achievements_scroll_up_maintain(struct GuiButton *gbtn)
{
    if (gbtn == NULL)
        return;
    if (achievements_scroll_offset > 0)
        gbtn->flags |= LbBtnF_Enabled;
    else
        gbtn->flags &= ~LbBtnF_Enabled;
}

void frontend_achievements_scroll_down_maintain(struct GuiButton *gbtn)
{
    if (gbtn == NULL)
        return;
    int max_offset = (int)achievements_count - frontend_achievements_items_visible;
    if (max_offset < 0)
        max_offset = 0;
    if (achievements_scroll_offset < max_offset)
        gbtn->flags |= LbBtnF_Enabled;
    else
        gbtn->flags &= ~LbBtnF_Enabled;
}

void frontend_achievements_scroll_tab_maintain(struct GuiButton *gbtn)
{
    if (gbtn == NULL)
        return;
    if (achievements_count > (unsigned long)frontend_achievements_items_visible)
        gbtn->flags |= LbBtnF_Enabled;
    else
        gbtn->flags &= ~LbBtnF_Enabled;
}

void frontend_draw_achievements_scroll_tab(struct GuiButton *gbtn)
{
    frontend_draw_scroll_tab(gbtn, achievements_scroll_offset, 
        frontend_achievements_items_visible, achievements_count);
}

void frontend_achievements_update(void)
{
    // Update scroll bounds if needed
    int max_offset = (int)achievements_count - frontend_achievements_items_visible;
    if (max_offset < 0)
        max_offset = 0;
    if (achievements_scroll_offset > max_offset)
        achievements_scroll_offset = max_offset;
}
/******************************************************************************/
