/******************************************************************************/
// KeeperFX - ImGui Debug Overlay Integration
/******************************************************************************/
/** @file bflib_imgui.cpp
 *     ImGui debug overlay implementation.
 * @par Purpose:
 *     Provides developer debug tools and overlays using ImGui.
 * @author   Community
 * @date     18 Jan 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "bflib_imgui.h"

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "bflib_video.h"
#include "bflib_sndlib.h"
#include "bflib_mouse.h"
#include "config_settings.h"
#include "globals.h"
#include "input_manager.hpp"
#include "imgui_integration.h"

#include <SDL2/SDL.h>

#include "post_inc.h"

/******************************************************************************/
// External variables
extern SDL_Window *lbWindow;

/******************************************************************************/
// Local variables
namespace {    SDL_Window* g_imgui_window = nullptr;    SDL_Renderer* g_imgui_renderer = nullptr;
    bool g_imgui_initialized = false;
    bool g_debug_overlay_visible = false;
    TbBool g_show_audio_device_window = false;
    TbBool g_show_developer_menu = true;
    SDL_bool g_previous_relative_mouse_mode = SDL_FALSE;
}

/******************************************************************************/

extern "C" TbBool ImGui_Initialize(void) {
    if (g_imgui_initialized) {
        return true;
    }
    
    if (!lbWindow) {
        ERRORMSG("Cannot initialize ImGui: SDL window not created");
        return false;
    }
    
    // Check if game is in fullscreen mode
    Uint32 window_flags = SDL_GetWindowFlags(lbWindow);
    bool is_fullscreen = (window_flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) != 0;
    
    if (is_fullscreen) {
        WARNMSG("Game is in fullscreen mode - ImGui overlay may not work properly");
        WARNMSG("Consider switching to windowed mode for ImGui overlay to work");
    }
    
    // Create a separate window for ImGui overlay (always-on-top)
    int main_x, main_y, main_w, main_h;
    SDL_GetWindowPosition(lbWindow, &main_x, &main_y);
    SDL_GetWindowSize(lbWindow, &main_w, &main_h);
    
    // Create overlay window that can receive input
    Uint32 imgui_flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP;
    if (is_fullscreen) {
        // In fullscreen, ensure it's on top
        imgui_flags |= SDL_WINDOW_INPUT_FOCUS;
    }
    
    g_imgui_window = SDL_CreateWindow(
        "KeeperFX Debug Overlay",
        main_x, main_y,
        main_w, main_h,
        imgui_flags
    );
    
    if (!g_imgui_window) {
        ERRORMSG("Cannot create ImGui window: %s", SDL_GetError());
        return false;
    }
    
    // Make the ImGui window transparent
    SDL_SetWindowOpacity(g_imgui_window, 0.95f);
    
    // Create renderer for the ImGui window
    g_imgui_renderer = SDL_CreateRenderer(g_imgui_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_imgui_renderer) {
        WARNMSG("Failed to create accelerated renderer, trying software");
        g_imgui_renderer = SDL_CreateRenderer(g_imgui_window, -1, SDL_RENDERER_SOFTWARE);
    }
    
    if (!g_imgui_renderer) {
        ERRORMSG("Cannot create ImGui renderer: %s", SDL_GetError());
        SDL_DestroyWindow(g_imgui_window);
        g_imgui_window = nullptr;
        return false;
    }
    
    // Enable blend mode for transparency
    SDL_SetRenderDrawBlendMode(g_imgui_renderer, SDL_BLENDMODE_BLEND);
    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Enable ImGui's software cursor - will get normal motion events now
    io.MouseDrawCursor = true;
    
    // Scale up the font for better visibility
    io.FontGlobalScale = 1.5f;
    
    // Setup Dear ImGui style - dark theme
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(g_imgui_window, g_imgui_renderer);
    ImGui_ImplSDLRenderer2_Init(g_imgui_renderer);
    
    // Hide the window initially
    SDL_HideWindow(g_imgui_window);
    
    g_imgui_initialized = true;
    JUSTMSG("ImGui debug overlay initialized (separate transparent window)");
    JUSTMSG("ImGui window created: %p, renderer: %p", g_imgui_window, g_imgui_renderer);
    
    return true;
}

extern "C" void ImGui_Shutdown(void) {
    if (!g_imgui_initialized) {
        return;
    }
    
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    
    if (g_imgui_renderer) {
        SDL_DestroyRenderer(g_imgui_renderer);
        g_imgui_renderer = nullptr;
    }
    
    if (g_imgui_window) {
        SDL_DestroyWindow(g_imgui_window);
        g_imgui_window = nullptr;
    }
    
    g_imgui_initialized = false;
}

extern "C" void ImGui_NewFrame(void) {
    if (!g_imgui_initialized) {
        return;
    }
    
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    
    // Override mouse position with correct global coordinates after backend processing
    if (g_debug_overlay_visible && g_imgui_window) {
        ImGuiIO& io = ImGui::GetIO();
        
        // Get global mouse position
        int mouse_x, mouse_y;
        SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
        
        // Convert global coordinates to window-relative coordinates
        int win_x, win_y;
        SDL_GetWindowPosition(g_imgui_window, &win_x, &win_y);
        
        // Directly set the corrected mouse position
        io.MousePos = ImVec2((float)(mouse_x - win_x), (float)(mouse_y - win_y));
    }
    
    ImGui::NewFrame();
}

extern "C" void ImGui_Render(void) {
    if (!g_imgui_initialized) {
        return;
    }
    
    // Only show windows if debug overlay is visible
    if (g_debug_overlay_visible) {
        // Show developer menu
        if (g_show_developer_menu) {
            TbBool open = g_show_developer_menu;
            ImGui_ShowDeveloperMenu(&open);
            g_show_developer_menu = open;
        }
        
        // Show audio device window if requested
        if (g_show_audio_device_window) {
            TbBool open = g_show_audio_device_window;
            ImGui_ShowAudioDeviceWindow(&open);
            g_show_audio_device_window = open;
        }
        
        // Render custom integration windows
        ImGui_RenderCustomWindows();
    }
    
    // Always call Render to finish the frame
    ImGui::Render();
    
    // Only actually draw if overlay is visible
    if (g_debug_overlay_visible && g_imgui_window) {
        // Clear with transparency
        SDL_SetRenderDrawColor(g_imgui_renderer, 0, 0, 0, 0);
        SDL_RenderClear(g_imgui_renderer);
        
        // Render ImGui draw data
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), g_imgui_renderer);
        
        // Present
        SDL_RenderPresent(g_imgui_renderer);
    }
}

extern "C" void ImGui_ProcessEvent(void* event) {
    if (!g_imgui_initialized || !g_debug_overlay_visible) {
        return;
    }
    
    SDL_Event* sdl_event = static_cast<SDL_Event*>(event);
    
    // Pass all events to ImGui - backend needs them even though coordinates will be wrong
    ImGui_ImplSDL2_ProcessEvent(sdl_event);
}

extern "C" void ImGui_ToggleDebugOverlay(void) {
    // Try to initialize if not already done
    if (!g_imgui_initialized) {
        WARNMSG("ImGui not initialized yet, attempting to initialize now");
        if (!ImGui_Initialize()) {
            ERRORMSG("Failed to initialize ImGui on toggle");
            return;
        }
    }
    
    g_debug_overlay_visible = !g_debug_overlay_visible;
    
    if (g_debug_overlay_visible) {
        JUSTMSG("ImGui debug overlay ENABLED - Press F3 to hide");
        
        // Push debug input context - blocks game input
        InputManager::instance().pushContext(InputManager::Context::Debug);
        
        g_show_developer_menu = true;  // Auto-show developer menu when overlay is enabled
        
        // Disable game's custom cursor rendering by setting it to NULL
        LbMouseChangeSpriteAndHotspot(NULL, 0, 0);
        JUSTMSG("Disabled game cursor");
        
        // Release mouse capture so user can interact with ImGui
        g_previous_relative_mouse_mode = SDL_GetRelativeMouseMode();
        if (g_previous_relative_mouse_mode) {
            SDL_SetRelativeMouseMode(SDL_FALSE);
            // Hide OS cursor - ImGui will render its own
            SDL_ShowCursor(SDL_DISABLE);
            JUSTMSG("Released mouse capture for ImGui interaction");
        }
        
        // Show the ImGui window
        if (g_imgui_window) {
            JUSTMSG("Showing ImGui window");
            SDL_ShowWindow(g_imgui_window);
            // Raise window and give it input focus so it can receive mouse events
            SDL_RaiseWindow(g_imgui_window);
            SDL_SetWindowInputFocus(g_imgui_window);
            
            // Update window position to match game window
            int x, y, w, h;
            SDL_GetWindowPosition(lbWindow, &x, &y);
            SDL_GetWindowSize(lbWindow, &w, &h);
            SDL_SetWindowPosition(g_imgui_window, x, y);
            SDL_SetWindowSize(g_imgui_window, w, h);
            JUSTMSG("ImGui window positioned at %d,%d size %dx%d", x, y, w, h);
        } else {
            ERRORMSG("ImGui window is NULL!");
        }
    } else {
        JUSTMSG("ImGui debug overlay DISABLED");
        
        // Pop debug context - restore game input
        InputManager::instance().popContext();
        
        // Note: Game cursor will be restored automatically by the game's cursor update logic
        
        // Restore mouse capture if it was previously enabled
        if (g_previous_relative_mouse_mode) {
            SDL_SetRelativeMouseMode(SDL_TRUE);
            SDL_ShowCursor(SDL_DISABLE);
            JUSTMSG("Restored mouse capture");
        }
        
        // Hide the ImGui window
        if (g_imgui_window) {
            SDL_HideWindow(g_imgui_window);
        }
    }
}

extern "C" TbBool ImGui_IsDebugOverlayVisible(void) {
    return g_debug_overlay_visible;
}
extern "C" TbBool ImGui_WantCaptureMouse(void) {
    if (!g_imgui_initialized || !g_debug_overlay_visible) {
        return false;
    }
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

extern "C" TbBool ImGui_WantCaptureKeyboard(void) {
    if (!g_imgui_initialized || !g_debug_overlay_visible) {
        return false;
    }
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureKeyboard;
}
extern "C" void ImGui_ShowAudioDeviceWindow(TbBool* p_open) {
    bool open = (*p_open != 0);
    if (!ImGui::Begin("Audio Device Selector", &open)) {
        ImGui::End();
        *p_open = open ? 1 : 0;
        return;
    }
    *p_open = open ? 1 : 0;
    
    ImGui::Text("Audio Device Configuration");
    ImGui::Separator();
    
    // Get list of available devices
    int device_count = 0;
    char** device_list = get_audio_device_list(&device_count);
    
    if (device_count > 0 && device_list) {
        ImGui::Text("Available Devices: %d", device_count);
        ImGui::Spacing();
        
        // Show current device
        const char* current_device = get_current_audio_device();
        if (current_device) {
            ImGui::Text("Current: %s", current_device);
        } else {
            ImGui::Text("Current: No device opened");
        }
        
        ImGui::Separator();
        
        // List all devices with selection
        static int selected_device = -1;
        for (int i = 0; i < device_count; i++) {
            bool is_selected = (selected_device == i);
            bool is_current = (current_device && strcmp(device_list[i], current_device) == 0);
            
            // Mark current device with asterisk
            char label[256];
            snprintf(label, sizeof(label), "%s%s", 
                     is_current ? "* " : "  ", 
                     device_list[i]);
            
            if (ImGui::Selectable(label, is_selected)) {
                selected_device = i;
            }
        }
        
        ImGui::Spacing();
        
        // Apply button
        if (selected_device >= 0 && ImGui::Button("Apply Selected Device")) {
            // Copy selected device name to settings
            strncpy(settings.audio_device_name, device_list[selected_device], 
                    sizeof(settings.audio_device_name) - 1);
            settings.audio_device_name[sizeof(settings.audio_device_name) - 1] = '\0';
            
            // Save settings
            save_settings();
            
            ImGui::Text("Device set! Restart audio to apply.");
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Restart Audio")) {
            // Reinitialize audio with new device
            FreeAudio();
            SoundSettings snd_settings;
            snd_settings.max_number_of_samples = 128;  // Changed from 256 to fit in unsigned char
            InitAudio(&snd_settings);
        }
        
        free_audio_device_list(device_list, device_count);
    } else {
        ImGui::Text("No audio devices found or enumeration failed.");
    }
    
    ImGui::End();
}

extern "C" void ImGui_ShowDeveloperMenu(TbBool* p_open) {
    bool open = (*p_open != 0);
    if (!ImGui::Begin("Developer Tools", &open, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        *p_open = open ? 1 : 0;
        return;
    }
    *p_open = open ? 1 : 0;
    
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Windows")) {
            bool audio_window_open = (g_show_audio_device_window != 0);
            if (ImGui::MenuItem("Audio Device Selector", nullptr, &audio_window_open)) {
                g_show_audio_device_window = audio_window_open ? 1 : 0;
            }
            
            ImGui::Separator();
            
            // Custom game configuration windows
            ImGui_RenderCustomMenuItems();
            
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
    
    ImGui::Text("KeeperFX Developer Overlay");
    ImGui::Separator();
    
    ImGui::Text("Press F3 to toggle this overlay");
    ImGui::Text("Press ESC to close this window");
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Quick stats
    if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
    }
    
    if (ImGui::CollapsingHeader("Audio System")) {
        const char* current_device = get_current_audio_device();
        if (current_device) {
            ImGui::Text("Device: %s", current_device);
        } else {
            ImGui::Text("Device: Not initialized");
        }
        
        if (ImGui::Button("Open Audio Device Selector")) {
            g_show_audio_device_window = true;
        }
    }
    
    ImGui::End();
}
