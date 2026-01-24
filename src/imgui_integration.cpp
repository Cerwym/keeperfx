/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file imgui_integration.cpp
 *     Integration point for custom ImGui windows with the debug overlay.
 * @par Purpose:
 *     Bridges custom ImGui configuration windows with the main ImGui framework.
 *     This file should be updated when adding new ImGui debug windows.
 * @par Comment:
 *     Add calls to your RenderXXX functions in ImGui_RenderCustomWindows().
 * @author   GitHub Copilot / Community
 * @date     24 Jan 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/

#ifdef ENABLE_IMGUI

#include "pre_inc.h"
#include "imgui.h"
#include "imgui_possession_config.h"
#include "post_inc.h"

/******************************************************************************/
// Window visibility state
namespace {
    bool g_show_possession_spell_config = false;
}

/******************************************************************************/
// C interface for toggling windows

extern "C" void ImGui_TogglePossessionSpellConfig(void) {
    g_show_possession_spell_config = !g_show_possession_spell_config;
}

extern "C" TbBool ImGui_IsPossessionSpellConfigVisible(void) {
    return g_show_possession_spell_config ? 1 : 0;
}

/******************************************************************************/
// Render all custom windows - called from main ImGui render loop

extern "C" void ImGui_RenderCustomWindows(void) {
    // Possession Spell Display Configuration
    if (g_show_possession_spell_config) {
        KeeperFX::ImGuiIntegration::RenderPossessionSpellDisplayConfig(&g_show_possession_spell_config);
    }

    // Add more custom windows here as needed:
    // if (g_show_creature_stats) {
    //     RenderCreatureStatsWindow(&g_show_creature_stats);
    // }
}

/******************************************************************************/
// Render menu items for opening custom windows

extern "C" void ImGui_RenderCustomMenuItems(void) {
    // Possession menu item
    if (ImGui::MenuItem("Possession Spell Display", "Ctrl+P", g_show_possession_spell_config)) {
        g_show_possession_spell_config = !g_show_possession_spell_config;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Configure how spell durations are displayed");
        ImGui::Text("in first-person possession mode");
        ImGui::EndTooltip();
    }

    // Add more menu items here:
    // ImGui::Separator();
    // if (ImGui::MenuItem("Creature Stats", NULL, g_show_creature_stats)) {
    //     g_show_creature_stats = !g_show_creature_stats;
    // }
}

#endif // ENABLE_IMGUI
