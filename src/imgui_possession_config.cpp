/** @file imgui_possession_config.cpp
 *     ImGui integration for possession spell display configuration.
 * @par Purpose:
 *     Provides a debug UI for testing and configuring spell display modes
 *     in possession mode.
 * @par Comment:
 *     This file is optional and only compiled when ImGui is available.
 *     Add this to your ImGui-enabled build for easy configuration testing.
 * @author   GitHub Copilot
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
#include "config_keeperfx.h"
#include "post_inc.h"

extern "C" {
    extern unsigned char possession_spell_display_mode;
}

namespace KeeperFX {
namespace ImGuiIntegration {

/**
 * Renders the Possession Spell Display configuration window.
 * Call this from your main ImGui rendering loop.
 * 
 * @param p_open Pointer to bool controlling window visibility (can be NULL)
 */
void RenderPossessionSpellDisplayConfig(bool* p_open)
{
    if (!ImGui::Begin("Possession: Spell Display", p_open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("Spell Display Configuration");
    ImGui::Separator();
    
    // Mode selection combo box
    const char* mode_names[] = {
        "Off",
        "Icons Only",
        "Text Above",
        "Text Below",
        "Progress Bar"
    };
    
    int current_mode = (int)possession_spell_display_mode;
    if (ImGui::Combo("Display Mode", &current_mode, mode_names, IM_ARRAYSIZE(mode_names)))
    {
        possession_spell_display_mode = (unsigned char)current_mode;
    }
    
    // Help text for each mode
    ImGui::Spacing();
    ImGui::TextWrapped("Mode Description:");
    ImGui::Indent();
    
    switch (possession_spell_display_mode)
    {
        case 0: // OFF
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No spell indicators shown");
            ImGui::TextWrapped("Completely disables spell display in possession mode.");
            break;
            
        case 1: // ICONS_ONLY
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "Icons only (original behavior)");
            ImGui::TextWrapped("Shows spell icons without any duration information. "
                             "This is how the game worked before the duration feature.");
            break;
            
        case 2: // TEXT_ABOVE
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Text above icons (default)");
            ImGui::TextWrapped("Shows spell duration in seconds above each icon. "
                             "This is the default mode providing clear visibility.");
            break;
            
        case 3: // TEXT_BELOW
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Text below icons");
            ImGui::TextWrapped("Shows spell duration in seconds below each icon. "
                             "Alternative placement if you prefer text at the bottom.");
            break;
            
        case 4: // PROGRESS_BAR
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "Visual progress bar");
            ImGui::TextWrapped("Shows a 3-pixel tall green progress bar underneath each icon "
                             "that depletes as the spell expires. Provides visual feedback "
                             "without numeric countdown.");
            break;
            
        default:
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Unknown mode!");
            break;
    }
    
    ImGui::Unindent();
    ImGui::Spacing();
    ImGui::Separator();
    
    // Configuration info
    ImGui::Text("Configuration:");
    ImGui::BulletText("Changes take effect immediately");
    ImGui::BulletText("No game restart required");
    ImGui::BulletText("Can also be set in keeperfx.cfg");
    
    ImGui::Spacing();
    
    // Example keeperfx.cfg entry
    if (ImGui::TreeNode("keeperfx.cfg Example"))
    {
        ImGui::Text("Add this line to your keeperfx.cfg:");
        ImGui::Spacing();
        
        const char* cfg_values[] = {
            "POSSESSION_SPELL_DISPLAY = OFF",
            "POSSESSION_SPELL_DISPLAY = ICONS_ONLY",
            "POSSESSION_SPELL_DISPLAY = TEXT_ABOVE",
            "POSSESSION_SPELL_DISPLAY = TEXT_BELOW",
            "POSSESSION_SPELL_DISPLAY = PROGRESS_BAR"
        };
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
        ImGui::TextWrapped("%s", cfg_values[current_mode]);
        ImGui::PopStyleColor();
        
        ImGui::TreePop();
    }
    
    ImGui::Spacing();
    
    // Quick test buttons
    if (ImGui::TreeNode("Quick Tests"))
    {
        ImGui::Text("Click to quickly test each mode:");
        ImGui::Spacing();
        
        if (ImGui::Button("Off", ImVec2(120, 0)))
            possession_spell_display_mode = 0;
        ImGui::SameLine();
        if (ImGui::Button("Icons Only", ImVec2(120, 0)))
            possession_spell_display_mode = 1;
        
        if (ImGui::Button("Text Above", ImVec2(120, 0)))
            possession_spell_display_mode = 2;
        ImGui::SameLine();
        if (ImGui::Button("Text Below", ImVec2(120, 0)))
            possession_spell_display_mode = 3;
        
        if (ImGui::Button("Progress Bar", ImVec2(120, 0)))
            possession_spell_display_mode = 4;
        
        ImGui::TreePop();
    }
    
    ImGui::End();
}

/**
 * Renders a compact inline control for the menu bar or settings panel.
 * Use this for a minimal integration that doesn't require a separate window.
 */
void RenderPossessionSpellDisplayInline()
{
    const char* mode_names[] = {
        "Off",
        "Icons Only", 
        "Text Above",
        "Text Below",
        "Progress Bar"
    };
    
    int current_mode = (int)possession_spell_display_mode;
    ImGui::SetNextItemWidth(150.0f);
    if (ImGui::Combo("Possession Spell Display", &current_mode, mode_names, IM_ARRAYSIZE(mode_names)))
    {
        possession_spell_display_mode = (unsigned char)current_mode;
    }
    
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Configure how spell durations are displayed");
        ImGui::Text("in possession mode (first-person view)");
        ImGui::EndTooltip();
    }
}

} // namespace ImGuiIntegration
} // namespace KeeperFX

#endif // ENABLE_IMGUI
