/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file imgui_possession_config.h
 *     Header for ImGui possession spell display configuration.
 * @par Purpose:
 *     Provides function declarations for ImGui debug UI integration.
 * @par Comment:
 *     Include this in your ImGui integration code.
 * @author   GitHub Copilot
 * @date     24 Jan 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/

#ifndef IMGUI_POSSESSION_CONFIG_H
#define IMGUI_POSSESSION_CONFIG_H

#ifdef ENABLE_IMGUI

#ifdef __cplusplus
namespace KeeperFX {
namespace ImGuiIntegration {
#endif

/**
 * Renders the Possession Spell Display configuration window.
 * 
 * This creates a standalone window with full configuration options including:
 * - Mode selection combo box
 * - Description of current mode
 * - Quick test buttons for each mode
 * - Example keeperfx.cfg configuration
 * - Help text and usage information
 * 
 * Usage:
 *   static bool show_possession_config = true;
 *   if (show_possession_config) {
 *       KeeperFX::ImGuiIntegration::RenderPossessionSpellDisplayConfig(&show_possession_config);
 *   }
 * 
 * @param p_open Pointer to bool controlling window visibility.
 *               Pass NULL if you don't need close button functionality.
 */
void RenderPossessionSpellDisplayConfig(bool* p_open);

/**
 * Renders a compact inline control for the configuration.
 * 
 * This is a minimal integration option that just shows a combo box
 * with a tooltip. Suitable for embedding in existing settings menus
 * or menu bars without creating a separate window.
 * 
 * Usage:
 *   if (ImGui::BeginMenu("Settings")) {
 *       KeeperFX::ImGuiIntegration::RenderPossessionSpellDisplayInline();
 *       ImGui::EndMenu();
 *   }
 */
void RenderPossessionSpellDisplayInline();

#ifdef __cplusplus
} // namespace ImGuiIntegration
} // namespace KeeperFX
#endif

#endif // ENABLE_IMGUI

#endif // IMGUI_POSSESSION_CONFIG_H
