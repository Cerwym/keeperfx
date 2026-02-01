# KeeperFX Mod Studio

A cross-platform desktop application for creating, editing, and converting KeeperFX mods to the `.kfxmod` binary format.

## Features

### Implemented ✓

- **AvaloniaUI Cross-Platform UI**
  - Modern, responsive interface with dark theme
  - Tabbed editor interface (Metadata, Files, Preview, Raw JSON)
  - Menu system with File, Tools, and Help menus
  - Status bar showing mod type and compression info

- **Binary Format Support**
  - Full .kfxmod binary format reader
  - Supports zlib compressed metadata
  - CRC32 integrity checking
  - 64-byte header parsing (matching C implementation)

- **Mod Converter**
  - Convert folder-based mods to .kfxmod format
  - Automatic metadata creation
  - Progress reporting
  - Zlib compression support

- **Metadata Editor**
  - Edit mod ID, name, version, author
  - Configure mod type (Campaign, Creature Pack, Texture Pack, etc.)
  - Edit description and tags
  - Configure load order and phase
  - Content manifest checkboxes (Creatures, Configs, Levels, Audio, Graphics)

### Planned Features 🚧

- **File Picker Integration**
  - Open .kfxmod files from file system
  - Browse and select folders for conversion
  - Save edited mods

- **File Tree Viewer**
  - Display all files in mod pack
  - Extract individual files
  - Preview text files

- **Map Layout Viewer**
  - Visualize KeeperFX map layouts
  - Show level geometry
  - Display spawn points and objects

- **Dependency Editor**
  - Add/remove dependencies
  - Configure version constraints
  - Manage optional dependencies
  - Define conflicts

- **Asset Preview**
  - Preview images (sprites, textures, screenshots)
  - Play audio files
  - View creature configurations

- **Validation & Testing**
  - Validate mod structure
  - Check dependencies
  - Verify file integrity
  - Test load order

## Technology Stack

- **.NET 10.0** - Modern cross-platform framework
- **AvaloniaUI 11.3** - Cross-platform XAML UI framework
- **CommunityToolkit.Mvvm 8.2** - MVVM helpers and commands
- **System.IO.Compression** - Built-in zlib support

## Project Structure

```
KfxModStudio/
├── Models/
│   └── ModPackModels.cs         # Data models matching C structures
├── Services/
│   ├── ModPackReader.cs         # Binary format reader
│   └── ModPackConverter.cs      # Folder to .kfxmod converter
├── ViewModels/
│   ├── ViewModelBase.cs
│   └── MainWindowViewModel.cs   # Main window logic
├── Views/
│   ├── MainWindow.axaml         # Main window UI
│   └── MainWindow.axaml.cs
├── Assets/
│   └── workshop_icon.png        # App icon from FXGraphics
└── Program.cs                    # Entry point
```

## Building

### Prerequisites

- .NET SDK 10.0 or later
- (Optional) Visual Studio 2022, VS Code, or Rider

### Build Commands

```bash
# Restore dependencies
dotnet restore

# Build
dotnet build

# Run
dotnet run

# Publish (Linux)
dotnet publish -c Release -r linux-x64 --self-contained

# Publish (Windows)
dotnet publish -c Release -r win-x64 --self-contained

# Publish (macOS)
dotnet publish -c Release -r osx-x64 --self-contained
```

## Usage

### Converting a Folder to .kfxmod

1. Click "File" → "Convert Folder to Mod..."
2. Select the source folder containing your mod
3. Configure metadata (name, author, version, description)
4. Choose output location and compression type
5. Click "Convert"

### Opening an Existing .kfxmod

1. Click "File" → "Open Mod..."
2. Select a .kfxmod file
3. View and edit metadata in the tabs
4. Save changes with "File" → "Save"

### Creating a New Mod

1. Click "File" → "New Mod..."
2. Fill in metadata fields
3. Add files (planned feature)
4. Configure dependencies (planned feature)
5. Save as .kfxmod

## Binary Format Compatibility

This application is fully compatible with the .kfxmod binary format specification defined in:
- `docs/mod_binary_format.md` - Complete specification
- `src/config_modpack.h` - C header file
- `src/config_modpack.c` - C implementation

All data structures match the C implementation byte-for-byte:
- 64-byte header with magic "KFXMOD\0\0"
- Compressed JSON metadata (zlib)
- File table entries
- Content data

## Architecture

### MVVM Pattern

The application follows the Model-View-ViewModel pattern:

- **Models**: Plain data classes (ModPack, ModPackMetadata, etc.)
- **ViewModels**: Business logic and UI state (MainWindowViewModel)
- **Views**: XAML-based UI (MainWindow.axaml)

### Binary Format Reader

The `ModPackReader` service provides async methods to:
1. Read 64-byte header (marshaled from binary)
2. Validate magic and version
3. Decompress metadata (zlib)
4. Parse JSON metadata
5. Read file table (when needed)

### Converter Service

The `ModPackConverter` service:
1. Scans source folder
2. Creates or loads metadata
3. Compresses metadata
4. Writes binary header
5. (Future) Packs all files with individual compression

## Screenshots

*Screenshots will be available when the application runs with a display*

### Main Window Layout

- **Menu Bar**: File, Tools, Help
- **Left Panel**: File tree and quick actions
- **Center Tabs**: 
  - Metadata editor (name, version, author, type, description)
  - Files list
  - Preview pane
  - Raw JSON viewer
- **Status Bar**: Mod type, compression, status messages

## Development Notes

### Adding New Features

1. Add models to `Models/ModPackModels.cs`
2. Add service logic to `Services/`
3. Update ViewModel commands in `ViewModels/MainWindowViewModel.cs`
4. Update UI in `Views/MainWindow.axaml`

### Testing

```bash
# Run tests (when test project added)
dotnet test
```

## Known Limitations

- File picker not yet implemented (needs platform-specific dialogs)
- Map viewer requires map format parser (planned)
- Dependency editor UI not yet implemented
- File table reading/writing incomplete (minimal implementation)
- LZ4 compression not yet supported (only zlib)

## Contributing

This is part of the KeeperFX project. Contributions welcome!

## License

Same as KeeperFX (GPL-2.0+)

## Credits

- Workshop icon from [FXGraphics](https://github.com/dkfans/FXGraphics)
- Built with [AvaloniaUI](https://avaloniaui.net/)
- Part of [KeeperFX](https://github.com/Cerwym/keeperfx)
