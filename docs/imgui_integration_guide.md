# ImGui Integration for Possession Spell Display

This document describes how to integrate the possession spell display configuration UI into your ImGui-enabled KeeperFX build.

## Overview

The ImGui integration provides an easy-to-use debug UI for testing and configuring the spell display modes in possession mode without editing configuration files.

## Files

- `src/imgui_possession_config.h` - Header file with function declarations
- `src/imgui_possession_config.cpp` - Implementation of ImGui UI
- `docs/imgui_integration_guide.md` - This file

## Prerequisites

- ImGui library integrated into your KeeperFX build
- `ENABLE_IMGUI` preprocessor define enabled in your build system

## Build Integration

### CMake Example

```cmake
# Add to your CMakeLists.txt when ImGui is enabled
if(ENABLE_IMGUI)
    target_sources(keeperfx PRIVATE
        src/imgui_possession_config.cpp
    )
    target_compile_definitions(keeperfx PRIVATE ENABLE_IMGUI)
endif()
```

### Makefile Example

```makefile
# Add to your Makefile when ImGui is enabled
ifdef ENABLE_IMGUI
    CXXFLAGS += -DENABLE_IMGUI
    SOURCES += src/imgui_possession_config.cpp
endif
```

## Code Integration

### Option 1: Standalone Window (Recommended for Testing)

Add this to your main ImGui rendering loop:

```cpp
#ifdef ENABLE_IMGUI
#include "imgui_possession_config.h"

// In your ImGui render function:
void RenderImGuiDebugUI()
{
    static bool show_possession_config = true;
    
    // Add menu item to show/hide the window
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Debug"))
        {
            ImGui::MenuItem("Possession Spell Display", NULL, &show_possession_config);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    
    // Render the configuration window
    if (show_possession_config)
    {
        KeeperFX::ImGuiIntegration::RenderPossessionSpellDisplayConfig(&show_possession_config);
    }
}
#endif
```

### Option 2: Inline Control (Minimal Integration)

Add to your existing settings menu:

```cpp
#ifdef ENABLE_IMGUI
#include "imgui_possession_config.h"

// In your settings menu:
if (ImGui::BeginMenu("Game Settings"))
{
    ImGui::Text("Possession Mode:");
    ImGui::Separator();
    
    // This renders just the combo box with tooltip
    KeeperFX::ImGuiIntegration::RenderPossessionSpellDisplayInline();
    
    // ... other settings ...
    
    ImGui::EndMenu();
}
#endif
```

### Option 3: Direct Variable Access

If you want to build a custom UI:

```cpp
#ifdef ENABLE_IMGUI
#include "config_keeperfx.h"

extern "C" {
    extern unsigned char possession_spell_display_mode;
}

// In your custom UI code:
const char* modes[] = { "Off", "Icons Only", "Text Above", "Text Below", "Progress Bar" };
int mode = (int)possession_spell_display_mode;
if (ImGui::Combo("Spell Display", &mode, modes, 5))
{
    possession_spell_display_mode = (unsigned char)mode;
}
```

## Features of the Standalone Window

The full configuration window (`RenderPossessionSpellDisplayConfig`) provides:

1. **Mode Selection**: Dropdown combo box with all 5 display modes
2. **Dynamic Descriptions**: Color-coded description that updates based on selected mode
3. **Configuration Info**: Bullets explaining behavior and config file usage
4. **keeperfx.cfg Example**: Shows the exact line to add to config file for current mode
5. **Quick Test Buttons**: One-click buttons to quickly cycle through modes for testing

## Testing Workflow

1. **Launch game** with ImGui integration enabled
2. **Open Debug menu** → "Possession Spell Display"
3. **Possess a creature** with active spells (e.g., cast Speed, Armor)
4. **Use quick test buttons** to cycle through display modes in real-time
5. **Observe changes** immediately without restarting
6. **Choose preferred mode** and note the keeperfx.cfg line
7. **Add to config** for persistent setting across game sessions

## Display Modes Reference

| Mode | Value | Description |
|------|-------|-------------|
| Off | 0 | No spell indicators shown |
| Icons Only | 1 | Original behavior (pre-feature) |
| Text Above | 2 | Duration in seconds above icons (**default**) |
| Text Below | 3 | Duration in seconds below icons |
| Progress Bar | 4 | Visual depletion bar under icons |

## Screenshot Locations

When testing, you may want to capture screenshots of each mode:

1. **Off**: No spell icons visible
2. **Icons Only**: Spell icons without timers (baseline)
3. **Text Above**: Icons with numbers above (👻 **30**)
4. **Text Below**: Icons with numbers below
5. **Progress Bar**: Icons with green progress bars underneath

## Troubleshooting

### Window Not Appearing

- Ensure `ENABLE_IMGUI` is defined during compilation
- Check that `show_possession_config` bool is true
- Verify ImGui is properly initialized

### Changes Not Taking Effect

- Changes should be immediate - no restart needed
- Ensure you're in possession mode (first-person view)
- Creature must have active spells to see display

### Compilation Errors

If you get undefined reference errors:
- Make sure `imgui_possession_config.cpp` is in your build
- Verify ImGui headers are in your include path
- Check that `config_keeperfx.h` is accessible

### Mode Value Out of Range

The mode value should always be 0-4. If you see unexpected values:
- Check for memory corruption
- Verify config file parsing is working correctly
- Use the ImGui UI to reset to a valid value

## Advanced Usage

### Custom Styling

You can customize the appearance by modifying colors in the `.cpp` file:

```cpp
// Change mode description colors:
ImGui::TextColored(ImVec4(r, g, b, a), "Your text");

// Change button sizes:
if (ImGui::Button("Mode Name", ImVec2(width, height)))
```

### Integration with Save System

To persist the setting when changed via ImGui:

```cpp
if (ImGui::Combo("Display Mode", &current_mode, mode_names, 5))
{
    possession_spell_display_mode = (unsigned char)current_mode;
    
    // Optional: Write to config file immediately
    SaveConfigToFile();
}
```

### Keyboard Shortcuts

Add hotkeys for quick mode cycling:

```cpp
// In your input handling:
if (ImGui::IsKeyPressed(ImGuiKey_F5))
{
    possession_spell_display_mode = (possession_spell_display_mode + 1) % 5;
}
```

## Example Complete Integration

Here's a complete example showing all pieces together:

```cpp
// main_imgui.cpp - Your ImGui integration file

#ifdef ENABLE_IMGUI

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "imgui_possession_config.h"

static bool show_possession_config = false;
static bool show_demo_window = false;

void InitImGui(SDL_Window* window, SDL_GLContext gl_context)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 130");
    
    ImGui::StyleColorsDark();
}

void RenderImGui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    
    // Main menu bar
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Debug"))
        {
            ImGui::MenuItem("Possession Config", NULL, &show_possession_config);
            ImGui::MenuItem("ImGui Demo", NULL, &show_demo_window);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    
    // Render windows
    if (show_possession_config)
    {
        KeeperFX::ImGuiIntegration::RenderPossessionSpellDisplayConfig(&show_possession_config);
    }
    
    if (show_demo_window)
    {
        ImGui::ShowDemoWindow(&show_demo_window);
    }
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ShutdownImGui()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

#endif // ENABLE_IMGUI
```

## Support

For issues or questions:
- Check the main documentation: `docs/possession_spell_display_config.md`
- Review the game architecture: `docs/game_architecture.md`
- The underlying configuration system is in `src/config_keeperfx.c`

## License

This ImGui integration follows the same GPL v2 license as KeeperFX.
