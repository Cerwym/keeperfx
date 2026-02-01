# KeeperFX Binary Mod Format (.kfxmod) Specification

## Version 1.0

## Overview

The `.kfxmod` format is a binary package format designed to improve the distribution, discovery, and management of KeeperFX mods and campaigns. It supports metadata, versioning, dependency resolution, and CDN-based update checking.

## Design Goals

1. **Modular Dependencies**: Campaigns can depend on mods rather than bundling them
2. **Version Management**: Track mod versions and check for updates
3. **CDN Integration**: Support for remote update checking and distribution
4. **Backward Compatibility**: Works alongside existing file-based mods
5. **Metadata Rich**: Include author, description, screenshots, dependencies
6. **Efficient Storage**: Compressed format with selective extraction

## Inspiration from Other Games

- **Skyrim (ESM/ESP)**: Master/plugin dependency system, load order management
- **Minecraft (JAR mods)**: Simple archive format with metadata (mod.json)
- **Source Engine (VPK)**: Chunked, versioned, efficient binary format
- **Factorio (ZIP mods)**: ZIP-based with info.json metadata and dependency resolution
- **Stellaris**: Descriptor files with dependencies and version tracking

## File Format Structure

### Header (64 bytes)

```
Offset | Size | Type     | Description
-------|------|----------|--------------------------------------------------
0x00   | 8    | char[8]  | Magic: "KFXMOD\0\0" (with null terminators)
0x08   | 2    | uint16   | Format version (1)
0x0A   | 2    | uint16   | Compression type (0=none, 1=zlib, 2=lz4)
0x0C   | 4    | uint32   | Metadata offset (from start of file)
0x10   | 4    | uint32   | Metadata size (compressed)
0x14   | 4    | uint32   | Metadata size (uncompressed)
0x18   | 4    | uint32   | File table offset
0x1C   | 4    | uint32   | File table entry count
0x20   | 4    | uint32   | Content offset (start of file data)
0x24   | 4    | uint32   | Total file size
0x28   | 4    | uint32   | CRC32 checksum (of entire file)
0x2C   | 4    | uint32   | Flags (reserved for future use)
0x30   | 16   | byte[16] | Reserved (zeros)
```

### Metadata Section (JSON, compressed)

The metadata is stored as JSON and compressed. Structure:

```json
{
  "mod_id": "tempest_keeper",
  "version": "1.2.0",
  "format_version": 1,
  "name": "Tempest Keeper Campaign",
  "display_name": "Tempest Keeper",
  "author": "CommunityMember",
  "description": "An extremely feature-filled campaign with custom creatures, voice acting, and music",
  "mod_type": "campaign",
  "created_date": "2024-01-15T00:00:00Z",
  "updated_date": "2024-03-20T00:00:00Z",
  "homepage_url": "https://example.com/tempest-keeper",
  "update_url": "https://cdn.example.com/mods/tempest_keeper/version.json",
  "min_keeperfx_version": "1.0.0",
  "max_keeperfx_version": null,
  "tags": ["campaign", "single-player", "custom-creatures", "voice-acting"],
  "dependencies": [
    {
      "mod_id": "angel_creature_pack",
      "version": ">=1.0.0",
      "required": true,
      "update_url": "https://cdn.example.com/mods/angel_creature_pack/version.json"
    }
  ],
  "optional_dependencies": [
    {
      "mod_id": "hd_textures",
      "version": ">=2.0.0",
      "required": false
    }
  ],
  "conflicts": [
    {
      "mod_id": "vanilla_creatures_only",
      "reason": "Replaces creature definitions"
    }
  ],
  "load_order": {
    "priority": 100,
    "load_phase": "after_campaign"
  },
  "changelog": [
    {
      "version": "1.2.0",
      "date": "2024-03-20",
      "changes": [
        "Fixed Angel creature balance",
        "Added new voice lines for level 15"
      ]
    },
    {
      "version": "1.1.0",
      "date": "2024-02-10",
      "changes": [
        "Added custom music tracks",
        "Improved land view graphics"
      ]
    }
  ],
  "screenshots": [
    "screenshots/level01.png",
    "screenshots/angel_creature.png"
  ],
  "readme": "README.md",
  "license": "CC-BY-NC-SA-4.0",
  "campaign_config": {
    "levels_count": 15,
    "has_multiplayer": false,
    "difficulty": "medium",
    "estimated_playtime": "8-10 hours"
  },
  "content_manifest": {
    "has_creatures": true,
    "has_configs": true,
    "has_levels": true,
    "has_audio": true,
    "has_graphics": true,
    "creatures_list": ["ANGEL", "DRUID", "TIME_MAGE"],
    "new_objects": ["LAIR_DRK_NGL"],
    "modified_rules": ["ImpWorkExperience"]
  }
}
```

### File Table Entry (variable size)

Each file entry describes a file contained in the mod:

```
Offset | Size     | Type     | Description
-------|----------|----------|------------------------------------------
0x00   | 2        | uint16   | Path length (N)
0x02   | N        | char[N]  | Relative path (UTF-8, no null terminator)
0x02+N | 8        | uint64   | File offset (from content offset)
       | 8        | uint64   | Compressed size
       | 8        | uint64   | Uncompressed size
       | 4        | uint32   | CRC32 checksum
       | 4        | uint32   | File flags (0x01=directory, 0x02=executable, etc.)
       | 4        | uint32   | Timestamp (Unix epoch)
```

### Content Section

Raw file data, compressed individually or in chunks. Files are stored sequentially as referenced by the file table.

## File Organization Within Mod

The internal structure follows KeeperFX conventions:

```
/
├── metadata.json (extracted to root for quick access)
├── README.md
├── LICENSE.txt
├── screenshots/
│   ├── level01.png
│   └── angel_creature.png
├── levels/              (maps to LEVELS_LOCATION)
│   ├── map00001.txt
│   ├── map00001.dat
│   └── ...
├── creatures/           (maps to CREATURES_LOCATION)
│   ├── Angel.cfg
│   ├── druid.cfg
│   └── ...
├── configs/             (maps to CONFIGS_LOCATION)
│   ├── creature.cfg
│   ├── rules.cfg
│   ├── objects.cfg
│   └── trapdoor.cfg
├── media/               (maps to MEDIA_LOCATION)
│   ├── battle01.mp3
│   └── ...
├── speech/              (maps to SPEECH_LOCATION)
│   ├── good01.wav
│   └── ...
├── land/                (maps to LAND_LOCATION)
│   ├── rgmap00.png
│   └── viframe00.png
└── campaign.cfg         (optional: campaign configuration)
```

## Version String Format

Semantic versioning (semver) 2.0.0: `MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]`

Examples:
- `1.0.0` - Initial release
- `1.2.3` - Patch update
- `2.0.0-beta.1` - Beta version
- `1.0.0+20240320` - With build metadata

Version comparison operators:
- `=` or `==` - Exact version match
- `>=` - Greater than or equal
- `>` - Greater than
- `<=` - Less than or equal
- `<` - Less than
- `~` - Compatible patch version (e.g., `~1.2.0` matches `>=1.2.0 <1.3.0`)
- `^` - Compatible minor version (e.g., `^1.2.0` matches `>=1.2.0 <2.0.0`)

## Mod Types

- `campaign` - Full campaign with levels
- `creature_pack` - Collection of creatures
- `texture_pack` - Graphics/textures replacement
- `audio_pack` - Music and sound effects
- `config_mod` - Configuration overrides
- `content_pack` - Mixed content (levels + creatures + configs)
- `total_conversion` - Complete game overhaul

## Load Phases

Mods can be loaded at different phases to control application order:

1. `after_base` - After base game files, before campaign
2. `after_campaign` - After campaign files, before map
3. `after_map` - After map-specific files (highest priority)

## CDN Update System

### version.json Format

CDN hosts a `version.json` file at the mod's update URL:

```json
{
  "mod_id": "tempest_keeper",
  "current_version": "1.3.0",
  "min_keeperfx_version": "1.0.0",
  "download_url": "https://cdn.example.com/mods/tempest_keeper/tempest_keeper-1.3.0.kfxmod",
  "file_size": 52428800,
  "checksum_sha256": "abc123...",
  "release_date": "2024-04-01T00:00:00Z",
  "release_notes": "https://example.com/tempest-keeper/changelog",
  "is_deprecated": false,
  "deprecation_reason": null,
  "alternative_mod_id": null
}
```

### Update Check Flow

1. Game reads installed mod's `metadata.json` to get `update_url`
2. HTTP GET request to `update_url` (with If-Modified-Since header)
3. Parse returned `version.json`
4. Compare `current_version` with installed version
5. If newer version available, notify player
6. Player can download from `download_url`
7. Verify `checksum_sha256` after download
8. Install new version (automatic or manual)

## Dependency Resolution Algorithm

1. Load all installed mods and read metadata
2. Build dependency graph
3. Detect circular dependencies (error condition)
4. Topological sort to determine load order
5. Check version constraints for all dependencies
6. If dependency missing or version mismatch, prompt user
7. Resolve conflicts (show warning, let user choose)
8. Apply load order priorities within each load phase
9. Load mods in calculated order

## Implementation Notes

### Performance

- Lazy loading: Only extract metadata on startup
- File table loaded on-demand when mod is activated
- Individual files extracted to temp cache when needed
- Cache can be persistent across sessions

### Security

- CRC32 checksums for corruption detection
- SHA-256 for download verification
- Sandboxed file extraction (prevent path traversal)
- Digital signatures (future enhancement)

### Backward Compatibility

- File-based mods continue to work
- `.kfxmod` files can be unpacked to traditional folder structure
- Game loads both folder mods and `.kfxmod` files
- Folder mods take precedence over `.kfxmod` with same `mod_id`

## Tools

### Mod Packager

Command: `kfxmod pack <input_dir> <output.kfxmod> [options]`

Options:
- `--compression <type>` - Compression method (zlib, lz4, none)
- `--metadata <file>` - Path to metadata JSON file
- `--validate` - Validate contents before packing

### Mod Unpacker

Command: `kfxmod unpack <input.kfxmod> <output_dir> [options]`

Options:
- `--metadata-only` - Extract only metadata
- `--files <pattern>` - Extract specific files

### Mod Validator

Command: `kfxmod validate <file.kfxmod>`

Checks:
- Format validity
- CRC checksums
- Metadata schema
- File references
- Dependency versions

### Mod Info

Command: `kfxmod info <file.kfxmod>`

Displays mod metadata in human-readable format.

## Example Workflows

### Creating a Mod

1. Organize files in standard folder structure
2. Create `metadata.json` with mod information
3. Run `kfxmod pack modname modname-1.0.0.kfxmod`
4. Upload to distribution platform
5. (Optional) Create CDN `version.json` for updates

### Installing a Mod

1. Download `.kfxmod` file
2. Place in `mods/` directory
3. Game auto-detects and loads on next start
4. (Or) Extract to `mods/modname/` folder

### Updating a Mod

1. Game checks `update_url` periodically
2. Notification shown if update available
3. Player clicks "Update"
4. New `.kfxmod` downloaded to temp
5. Old version backed up
6. New version installed
7. Game prompts restart if needed

### Creating Modular Content

**Base Mod: Angel Creature Pack**
```json
{
  "mod_id": "angel_creature_pack",
  "version": "1.0.0",
  "mod_type": "creature_pack",
  "content_manifest": {
    "creatures_list": ["ANGEL"],
    "new_objects": ["LAIR_DRK_NGL"]
  }
}
```

**Dependent Campaign: Tempest Keeper**
```json
{
  "mod_id": "tempest_keeper",
  "version": "1.0.0",
  "mod_type": "campaign",
  "dependencies": [
    {
      "mod_id": "angel_creature_pack",
      "version": ">=1.0.0",
      "required": true
    }
  ]
}
```

Now the campaign doesn't need to bundle the Angel creature - it just depends on the creature pack!

## Future Enhancements

- **Digital signatures** - Verify mod authenticity
- **Delta updates** - Download only changed files
- **Mod collections** - Bundles of related mods
- **In-game mod browser** - Browse and install from UI
- **Workshop integration** - Steam Workshop, ModDB, etc.
- **Multiplayer sync** - Ensure clients have same mods
- **Profile support** - Different mod sets for different playstyles
- **Rollback capability** - Revert to previous mod versions

## Appendix: Comparison with Other Formats

| Feature | .kfxmod | Skyrim ESM/ESP | Factorio ZIP | Source VPK |
|---------|---------|----------------|--------------|------------|
| Binary format | ✓ | ✓ | ✗ | ✓ |
| Compression | ✓ | ✗ | ✓ | ✓ |
| Metadata | ✓ (JSON) | ✗ (separate) | ✓ (JSON) | ✗ |
| Dependencies | ✓ | ✓ | ✓ | ✗ |
| Versioning | ✓ | ✗ | ✓ | ✓ |
| Update checking | ✓ | ✗ | ✓ | ✗ |
| Load order | ✓ | ✓ | ✗ | ✗ |

## License

This specification is released under CC-BY-4.0. Implementations should follow the GPL license of KeeperFX.
