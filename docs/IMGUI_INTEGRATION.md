# Integration Instructions for Possession Spell Config with ImGui Debug Overlay

This guide explains how to integrate the possession spell display configuration window into the existing ImGui framework from the `feature/input-manager-refactor` branch.

## Files Added

1. **imgui_integration.h** - C interface for custom ImGui windows
2. **imgui_integration.cpp** - Implementation that manages custom window visibility and rendering
3. **imgui_possession_config.h** - Header for possession spell config UI
4. **imgui_possession_config.cpp** - Implementation of possession spell config UI

## Required Changes to Existing Files

### 1. Modify `src/bflib_imgui.cpp`

**Add include near the top (after other includes):**
```cpp
#include "imgui_integration.h"
```

**In the `ImGui_Render()` function, find where windows are rendered:**
```cpp
// Around line 350-380, in the section:
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
    
    // ADD THIS:
    // Render custom integration windows
    ImGui_RenderCustomWindows();
}
```

**In `ImGui_ShowDeveloperMenu()` function, add menu item:**

Find the section where menu items are added (likely in a "Windows" or "Tools" submenu):
```cpp
// In the developer menu, add:
if (ImGui::BeginMenu("Game Config")) {
    // ADD THIS:
    ImGui_RenderCustomMenuItems();
    
    // ... existing menu items ...
    ImGui::EndMenu();
}
```

Or if there's a "Windows" menu:
```cpp
if (ImGui::BeginMenu("Windows")) {
    // ... existing items ...
    
    ImGui::Separator();
    ImGui_RenderCustomMenuItems();
    
    ImGui::EndMenu();
}
```

### 2. Update Makefile

Add the new source files to the build:

```makefile
# In the KEEPERFX_OBJS section, add:
obj/imgui_integration.o \
obj/imgui_possession_config.o \
```

### 3. Merge the branches

To fully integrate, you'll need to merge your current branch with `feature/input-manager-refactor`:

```bash
# From your current branch (copilot/add-spell-count-indicator)
git fetch origin
git merge origin/feature/input-manager-refactor

# Resolve any conflicts
# The new files (imgui_integration.cpp, imgui_integration.h) shouldn't conflict
```

## Testing the Integration

1. Build with ImGui enabled (ensure `ENABLE_IMGUI` is defined in your build)
2. Run the game
3. Press **F3** to toggle the ImGui debug overlay
4. Open the Developer Menu
5. Navigate to the "Game Config" or "Windows" menu
6. Click "Possession Spell Display" menu item
7. The configuration window should appear

## Keyboard Shortcut (Optional)

You can add a keyboard shortcut in the input manager or key handler:

In `src/input_manager.cpp` or wherever key handling is done:
```cpp
// Example: Ctrl+P to toggle possession config
if (is_key_pressed(KC_P, KMod_CONTROL)) {
    ImGui_TogglePossessionSpellConfig();
}
```

## Configuration Flow

1. User presses F3 → ImGui overlay becomes visible
2. User opens Developer Menu
3. User clicks "Possession Spell Display" menu item
4. Configuration window opens showing mode options
5. User selects a display mode (Off, Icons Only, Text Above, Text Below, Progress Bar)
6. Changes take effect immediately in-game
7. Settings can be persisted to keeperfx.cfg

## File Structure

```
src/
├── bflib_imgui.cpp          (existing - needs modification)
├── bflib_imgui.h            (existing - no changes needed)
├── input_manager.cpp        (existing - optional shortcut)
├── imgui_integration.h      (NEW)
├── imgui_integration.cpp    (NEW)
├── imgui_possession_config.h (EXISTING in your branch)
└── imgui_possession_config.cpp (EXISTING in your branch)
```

## Benefits of This Architecture

- **Separation of Concerns**: Core ImGui framework separate from game-specific config windows
- **Easy Extension**: Adding new config windows only requires:
  1. Create `imgui_myfeature_config.cpp/h`
  2. Add window visibility bool in `imgui_integration.cpp`
  3. Add render call in `ImGui_RenderCustomWindows()`
  4. Add menu item in `ImGui_RenderCustomMenuItems()`
- **No Core Changes**: The main ImGui framework (`bflib_imgui.cpp`) only needs two small additions
- **Type Safety**: C++ ImGui code isolated from C game code via C interface

## Notes

- The `ENABLE_IMGUI` preprocessor flag must be defined to compile this code
- The imgui submodule must be initialized (`git submodule update --init deps/imgui`)
- This integrates with the transparent overlay system from the input-manager branch
- Input handling is automatically managed by the existing input manager context system
