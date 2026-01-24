# ImGui Setup for KeeperFX Development

This guide explains how to enable ImGui for local development without affecting other developers.

## Quick Setup

### 1. Download and vendor ImGui

```bash
# Download ImGui (version 1.89 or later recommended)
cd deps
git clone https://github.com/ocornut/imgui.git imgui-temp
cd imgui-temp
git checkout docking  # or a specific tag like v1.89

# Copy only the files we need
cd ..
mkdir -p imgui/backends
cp imgui-temp/*.cpp imgui/
cp imgui-temp/*.h imgui/
cp imgui-temp/backends/imgui_impl_sdl2.* imgui/backends/
cp imgui-temp/backends/imgui_impl_sdlrenderer2.* imgui/backends/

# Clean up
rm -rf imgui-temp
```

Or on Windows PowerShell:

```powershell
# Download ImGui
cd deps
git clone https://github.com/ocornut/imgui.git imgui-temp
cd imgui-temp
git checkout docking

# Copy files
cd ..
New-Item -ItemType Directory -Force -Path imgui/backends
Copy-Item imgui-temp/*.cpp imgui/
Copy-Item imgui-temp/*.h imgui/
Copy-Item imgui-temp/backends/imgui_impl_sdl2.* imgui/backends/
Copy-Item imgui-temp/backends/imgui_impl_sdlrenderer2.* imgui/backends/

# Clean up
Remove-Item -Recurse -Force imgui-temp
```

### 2. Enable ImGui in your build

**Option A: Use local configuration (recommended for permanent setup)**

Create a local config file that's automatically loaded:

```bash
# Copy the template
cp config.local.mk.template config.local.mk

# Edit config.local.mk and set:
# ENABLE_IMGUI = 1
```

Then just build normally:

```bash
make
```

**Option B: Set environment variable**

```bash
make ENABLE_IMGUI=1
```

Or in PowerShell:

```powershell
$env:ENABLE_IMGUI = "1"
make
```

### 3. Build and test

```bash
wsl make clean
wsl make ENABLE_IMGUI=1 -j4
```

## Usage

Once built with ImGui enabled:

1. Run the game
2. Press **F3** to toggle the ImGui debug overlay
3. The "Developer Tools" window will appear
4. Navigate to **Windows** → **Possession Spell Display** to configure spell display modes

## For Other Developers

**ImGui is disabled by default**. Other developers don't need ImGui to build the game. The code gracefully handles the absence of ImGui through preprocessor directives.

To build without ImGui (default behavior):

```bash
make
# or explicitly
make ENABLE_IMGUI=0
```

## Files Required

When vendored, these ImGui files should be in `deps/imgui/`:

```
imgui/
├── imgui.cpp
├── imgui.h
├── imgui_demo.cpp
├── imgui_draw.cpp
├── imgui_internal.h
├── imgui_tables.cpp
├── imgui_widgets.cpp
├── imconfig.h
├── imstb_rectpack.h
├── imstb_textedit.h
├── imstb_truetype.h
└── backends/
    ├── imgui_impl_sdl2.cpp
    ├── imgui_impl_sdl2.h
    ├── imgui_impl_sdlrenderer2.cpp
    └── imgui_impl_sdlrenderer2.h
```

## Troubleshooting

### "Cannot find imgui.h"

You haven't vendored ImGui yet. Follow step 1 above.

### "undefined reference to ImGui::..."

You need to enable ImGui in your build: `make ENABLE_IMGUI=1`

### ImGui overlay doesn't appear

- Make sure you built with `ENABLE_IMGUI=1`
- Press F3 to toggle the overlay
- Check that you're not in fullscreen mode (windowed mode works better)

## Note for Git

**Don't commit the imgui directory!** Add it to `.gitignore`:

```bash
echo "deps/imgui/" >> .gitignore
```

This keeps ImGui as a local development tool without forcing it on other contributors.
