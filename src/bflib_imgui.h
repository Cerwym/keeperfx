/******************************************************************************/
// KeeperFX - ImGui Debug Overlay Integration
/******************************************************************************/
/** @file bflib_imgui.h
 *     Header file for ImGui debug overlay integration.
 * @par Purpose:
 *     Provides developer debug tools and overlays using ImGui.
 * @par Comment:
 *     This is only active in debug builds or when explicitly enabled.
 * @author   Community
 * @date     18 Jan 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef BFLIB_IMGUI_H
#define BFLIB_IMGUI_H

#include "bflib_basics.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
// Initialization and cleanup
TbBool ImGui_Initialize(void);
void ImGui_Shutdown(void);

// Per-frame functions
void ImGui_NewFrame(void);
void ImGui_Render(void);
void ImGui_ProcessEvent(void* event);

// Toggle debug overlay
void ImGui_ToggleDebugOverlay(void);
TbBool ImGui_IsDebugOverlayVisible(void);

// Input capture detection
TbBool ImGui_WantCaptureMouse(void);
TbBool ImGui_WantCaptureKeyboard(void);

// Debug windows
void ImGui_ShowAudioDeviceWindow(TbBool* p_open);
void ImGui_ShowDeveloperMenu(TbBool* p_open);

/******************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // BFLIB_IMGUI_H
