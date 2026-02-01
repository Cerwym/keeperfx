# KeeperFX Mod Studio - UI/UX Design

## Overview

KeeperFX Mod Studio is a desktop application for authoring .kfxmod binary mod packages. It provides a visual interface for converting folder-based mods, editing metadata, and managing mod assets.

## Design Goals

1. **Ease of Use**: Simple workflow for converting existing mods
2. **Visual Feedback**: Clear indication of mod structure and contents
3. **Cross-Platform**: Works on Windows, Linux, and macOS
4. **Professional**: Uses KeeperFX branding (workshop icon)
5. **Extensible**: Easy to add new features (map viewer, asset preview)

## Window Layout

```
┌─────────────────────────────────────────────────────────────────┐
│ File   Tools   Help                                    [_][□][X]│
├─────────────┬───────────────────────────────────────────────────┤
│  Mod        │ Metadata ┊ Files ┊ Preview ┊ Raw JSON           │
│  Contents   │                                                   │
│             │  ┌─────────────────────────────────────────────┐ │
│             │  │ Basic Information                          │ │
│             │  │                                             │ │
│             │  │ Mod ID:      tempest_keeper                │ │
│ ┌─────────┐ │  │ Name:        Tempest Keeper Campaign       │ │
│ │ Convert │ │  │ Version:     1.0.0                         │ │
│ │ Folder  │ │  │ Author:      CommunityMember               │ │
│ └─────────┘ │  │ Type:        [Campaign ▼]                  │ │
│             │  │                                             │ │
│ ┌─────────┐ │  │ Description:                               │ │
│ │  Open   │ │  │ ┌────────────────────────────────────────┐ │ │
│ │ .kfxmod │ │  │ │ An extremely feature-filled campaign   │ │ │
│ └─────────┘ │  │ │ with custom creatures...               │ │ │
│             │  │ └────────────────────────────────────────┘ │ │
│             │  └─────────────────────────────────────────────┘ │
│             │                                                   │
│ No mod      │  ┌─────────────────────────────────────────────┐ │
│ loaded      │  │ Dependencies                                │ │
│             │  │                                             │ │
│             │  │ ☑ angel_creature_pack >= 1.0.0 (required) │ │
│             │  │ ☐ hd_textures >= 2.0.0 (optional)          │ │
│             │  │                                             │ │
│             │  │ [Add Dependency] [Remove]                  │ │
│             │  └─────────────────────────────────────────────┘ │
│             │                                                   │
├─────────────┴───────────────────────────────────────────────────┤
│ Ready                    Type: Campaign    Compression: ZLib    │
└─────────────────────────────────────────────────────────────────┘
```

## Color Scheme

Following AvaloniaUI Fluent dark theme:

- **Background**: #1e1e1e (dark gray)
- **Surface**: #252525 (slightly lighter)
- **Borders**: #404040 (medium gray)
- **Text**: #ffffff (white) / #808080 (muted)
- **Accent**: #0078d4 (blue)
- **Success**: #107c10 (green)
- **Warning**: #ff8c00 (orange)
- **Error**: #e81123 (red)

## Icon

Workshop icon from FXGraphics repository:
- Source: `https://github.com/dkfans/FXGraphics/blob/main/menufx/gui2-256/room_256/workshop_std.png`
- Size: 256x256 PNG
- Style: Dungeon Keeper workshop room sprite
- Purpose: Application icon and branding

## Workflows

### 1. Converting a Folder Mod

```
┌────────────────────────────────────────────┐
│  Convert Folder to .kfxmod                 │
├────────────────────────────────────────────┤
│                                            │
│  Source Folder:                            │
│  ┌──────────────────────────────────────┐ │
│  │ /home/user/mods/tempkpr           [▣] │ │
│  └──────────────────────────────────────┘ │
│                                            │
│  Output File:                              │
│  ┌──────────────────────────────────────┐ │
│  │ tempest_keeper-1.0.0.kfxmod       [▣] │ │
│  └──────────────────────────────────────┘ │
│                                            │
│  Metadata (auto-detected):                 │
│  ┌──────────────────────────────────────┐ │
│  │ Mod ID:    tempest_keeper            │ │
│  │ Name:      Tempest Keeper            │ │
│  │ Version:   1.0.0                     │ │
│  │ Author:    Unknown                   │ │
│  └──────────────────────────────────────┘ │
│                                            │
│  Compression: [ZLib ▼]                     │
│                                            │
│  ┌──────────────────────────────────────┐ │
│  │ Progress: [=========>         ] 60%  │ │
│  │ Compressing metadata...              │ │
│  └──────────────────────────────────────┘ │
│                                            │
│          [Cancel]        [Convert]         │
└────────────────────────────────────────────┘
```

### 2. Editing Mod Metadata

User clicks on any field in the Metadata tab:
- Fields become editable when "Edit Mode" is enabled
- Changes are tracked
- Save button activates when changes detected
- Validation occurs on blur (e.g., version format)

### 3. Viewing Files

Files tab shows:
- Tree view of all files in mod pack
- File sizes and types
- Right-click context menu:
  - Extract File
  - View in Hex Editor
  - Delete from Pack (if edit mode)
  - Add to Pack (if edit mode)

### 4. Map Preview

Preview tab shows (when implemented):
```
┌─────────────────────────────────────────────┐
│  Map: map00001 - Ambergarde                 │
├─────────────────────────────────────────────┤
│                                             │
│         ┌─────────────────────┐             │
│         │                     │             │
│         │   [2D Map View]     │             │
│         │                     │             │
│         │   Shows tiles,      │             │
│         │   spawn points,     │             │
│         │   objects           │             │
│         │                     │             │
│         └─────────────────────┘             │
│                                             │
│  Dimensions: 85x85 tiles                    │
│  Players: 2                                 │
│  Heart Position: (42, 42)                   │
│                                             │
└─────────────────────────────────────────────┘
```

## Keyboard Shortcuts

- **Ctrl+N**: New Mod
- **Ctrl+O**: Open Mod
- **Ctrl+S**: Save
- **Ctrl+Shift+S**: Save As
- **Ctrl+W**: Close Mod
- **F5**: Validate Mod
- **Ctrl+E**: Toggle Edit Mode
- **F1**: Help

## Context Menus

### File Tree Context Menu
```
┌─────────────────────────┐
│ Open                   │
│ Extract...             │
│ ──────────────────────│
│ Copy Path             │
│ Properties            │
│ ──────────────────────│
│ Delete from Pack      │
└────────────────────────┘
```

### Dependency Item Context Menu
```
┌─────────────────────────┐
│ Edit Version...        │
│ Toggle Required        │
│ ──────────────────────│
│ Remove Dependency     │
└────────────────────────┘
```

## Status Messages

Examples of status bar messages:
- "Ready"
- "Loaded tempest_keeper-1.0.0.kfxmod"
- "Converting folder... 45% complete"
- "Validation passed: All checksums valid"
- "Error: Invalid mod pack header"
- "Saved changes to tempest_keeper-1.0.0.kfxmod"

## Future Enhancements

### Dependency Graph Visualizer
```
┌─────────────────────────────────────────────┐
│  Dependency Graph                           │
├─────────────────────────────────────────────┤
│                                             │
│     [Tempest Keeper]                        │
│           │                                 │
│           └──requires──> [Angel Pack]       │
│           │                    │            │
│           │                    └─> [Base]   │
│           │                                 │
│           └──optional──> [HD Textures]      │
│                                             │
│  Legend: ─requires─  ─optional─  ─conflicts─│
└─────────────────────────────────────────────┘
```

### Asset Preview Panel
```
┌─────────────────────────────────────────────┐
│  Selected: angel.zip                        │
├─────────────────────────────────────────────┤
│                                             │
│  ┌─────────────────┐  Angel Creature       │
│  │                 │                        │
│  │  [Animation]    │  Type: Creature       │
│  │  Frame 1/8      │  Size: 578 KB         │
│  │                 │  Format: PNG sprites  │
│  └─────────────────┘                        │
│                                             │
│  ◄ | ► Play Animation                       │
│                                             │
└─────────────────────────────────────────────┘
```

### Validation Results
```
┌─────────────────────────────────────────────┐
│  Validation Results                         │
├─────────────────────────────────────────────┤
│                                             │
│  ✓ Header valid (magic: KFXMOD)             │
│  ✓ Metadata JSON valid                      │
│  ✓ All checksums match                      │
│  ⚠ Large file: battle01.mp3 (6.3 MB)       │
│  ✓ Dependencies resolved                    │
│  ✗ Missing creature: DRUID                  │
│                                             │
│  Overall: Valid with warnings               │
│                                             │
│         [Details]         [Close]           │
└─────────────────────────────────────────────┘
```

## Accessibility

- Full keyboard navigation
- Screen reader support (via Avalonia accessibility)
- High contrast mode support
- Tooltips on all interactive elements
- Clear error messages

## Localization

Future support for:
- English (default)
- German
- French
- Polish
- Russian

Following KeeperFX language conventions.

## Performance Considerations

- Lazy loading of file contents (don't load all files into memory)
- Async operations for file I/O
- Progress reporting for long operations
- Cancellation support for background tasks
- Efficient JSON parsing (System.Text.Json)

## Testing Strategy

1. **Unit Tests**: Service layer (ModPackReader, ModPackConverter)
2. **Integration Tests**: Full conversion workflow
3. **UI Tests**: Avalonia Headless testing
4. **Manual Testing**: Real mod files from community

## Build Outputs

- **Windows**: KfxModStudio.exe (self-contained)
- **Linux**: KfxModStudio (AppImage or self-contained)
- **macOS**: KfxModStudio.app (bundle)

Each ~50-100 MB due to .NET runtime bundling.
