# KeeperFX Binary Mod Format (.kfxmod) - Implementation Guide

## Overview

This document provides a comprehensive guide to the KeeperFX binary mod format implementation. The `.kfxmod` format revolutionizes mod distribution and management for KeeperFX by providing a standardized, efficient, and feature-rich packaging system.

## Key Features

### 1. Binary Package Format
- **Efficient Storage**: Compressed binary format reduces file sizes
- **Fast Loading**: Optimized for quick metadata access
- **Integrity Checking**: Built-in CRC32 checksums

### 2. Rich Metadata System
- **Comprehensive Information**: Author, version, description, tags, screenshots
- **Semantic Versioning**: Full semver 2.0.0 support
- **Changelog Tracking**: Version history embedded in package

### 3. Dependency Management
- **Required Dependencies**: Specify mods that must be installed
- **Optional Dependencies**: Enhance functionality when present
- **Conflict Detection**: Identify incompatible mods
- **Version Constraints**: Flexible version matching (>=, ^, ~, etc.)

### 4. CDN Integration
- **Update Checking**: Automatic check for new versions
- **Remote Distribution**: Download directly from CDN
- **SHA-256 Verification**: Ensure download integrity
- **Deprecation Support**: Mark old mods as obsolete

### 5. Modular Content
- **Separation of Concerns**: Campaigns can depend on creature packs
- **Reusable Components**: Share common resources across mods
- **Reduced Duplication**: Don't bundle what already exists

## Architecture

### File Structure

```
.kfxmod File Layout:
┌─────────────────────────────────────┐
│ Header (64 bytes)                   │
│ - Magic: "KFXMOD\0\0"              │
│ - Version, compression, offsets     │
│ - CRC32 checksum                    │
├─────────────────────────────────────┤
│ Metadata Section (compressed JSON)  │
│ - Mod info, dependencies, etc.      │
├─────────────────────────────────────┤
│ File Table (variable)               │
│ - Entry 1: path, offset, size, CRC │
│ - Entry 2: ...                      │
│ - Entry N: ...                      │
├─────────────────────────────────────┤
│ Content Section                     │
│ - File 1 data (compressed)          │
│ - File 2 data (compressed)          │
│ - File N data (compressed)          │
└─────────────────────────────────────┘
```

### Code Organization

```
src/
├── config_modpack.h      # Header with structure definitions
├── config_modpack.c      # Core implementation
└── config_campaigns.c    # Integration with campaign system

tools/kfxmod/
├── tool_modpack.c        # Command-line tool
└── Makefile              # Build configuration

docs/
├── mod_binary_format.md  # Full specification
└── mod_implementation.md # This file
```

## Usage Examples

### Creating a Mod Pack

1. **Prepare Your Mod Folder**
   ```
   tempkpr/
   ├── metadata.json          # Required metadata file
   ├── README.md              # Optional documentation
   ├── LICENSE.txt            # Optional license
   ├── levels/                # Level files
   │   ├── map00001.txt
   │   ├── map00001.dat
   │   └── ...
   ├── creatures/             # Creature definitions
   │   ├── Angel.cfg
   │   └── ...
   ├── configs/               # Configuration files
   │   ├── creature.cfg
   │   ├── rules.cfg
   │   └── ...
   ├── media/                 # Music and sounds
   │   ├── battle01.mp3
   │   └── ...
   └── speech/                # Voice acting
       ├── good01.wav
       └── ...
   ```

2. **Create metadata.json**
   ```json
   {
     "mod_id": "tempest_keeper",
     "version": "1.0.0",
     "name": "Tempest Keeper Campaign",
     "author": "YourName",
     "mod_type": "campaign",
     "min_keeperfx_version": "1.0.0",
     "load_order": {
       "priority": 100,
       "load_phase": "after_campaign"
     }
   }
   ```

3. **Pack the Mod**
   ```bash
   cd tools/kfxmod
   make
   ./kfxmod pack ../../tempfolder tempest_keeper-1.0.0.kfxmod --compression zlib
   ```

### Installing a Mod

1. **Manual Installation**
   ```bash
   # Copy .kfxmod file to mods directory
   cp tempest_keeper-1.0.0.kfxmod /path/to/keeperfx/mods/
   
   # Game will auto-detect on next launch
   ```

2. **Extracting (Optional)**
   ```bash
   ./kfxmod unpack tempest_keeper-1.0.0.kfxmod extracted/
   ```

### Inspecting a Mod

```bash
# View mod information
./kfxmod info tempest_keeper-1.0.0.kfxmod

# Validate mod integrity
./kfxmod validate tempest_keeper-1.0.0.kfxmod
```

## Integration with KeeperFX

### Current System

The existing KeeperFX mod system (src/config_mods.c) loads mods from folders:

```
mods/
├── mod1/
│   ├── fxdata/
│   ├── campgns/
│   └── ...
└── mod2/
    └── ...
```

### Enhanced System

With `.kfxmod` support, both folder and binary mods coexist:

```
mods/
├── mod1/                  # Folder-based mod (existing)
├── mod2.kfxmod            # Binary mod (new)
└── mod3.kfxmod            # Binary mod (new)
```

### Loading Process

1. **Scan mods directory** for both folders and `.kfxmod` files
2. **Read metadata** from each mod (metadata.json or embedded)
3. **Build dependency graph** using mod_id references
4. **Resolve dependencies** and check version constraints
5. **Sort by load order** (phase, then priority)
6. **Extract files** to temporary cache if needed
7. **Load mods** in calculated order

### Example Integration Code

```c
// In config_mods.c or similar
TbBool load_all_mods(void)
{
    // Scan for folder mods (existing)
    load_folder_mods("mods/");
    
    // Scan for binary mods (new)
    load_binary_mods("mods/");
    
    // Resolve dependencies
    resolve_mod_dependencies();
    
    // Load in order
    apply_mods_in_load_order();
    
    return true;
}

TbBool load_binary_mods(const char *mods_dir)
{
    // Find all .kfxmod files
    DIR *dir = opendir(mods_dir);
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL)
    {
        if (strstr(entry->d_name, ".kfxmod") != NULL)
        {
            char filepath[DISKPATH_SIZE];
            snprintf(filepath, sizeof(filepath), "%s/%s", 
                     mods_dir, entry->d_name);
            
            struct ModPack modpack;
            if (modpack_load(filepath, &modpack))
            {
                register_mod(&modpack);
            }
        }
    }
    
    closedir(dir);
    return true;
}
```

## Tempest Keeper Campaign Analysis

### Campaign Structure

The Tempest Keeper campaign demonstrates a complex, feature-rich mod:

**Content Overview:**
- 15 single-player levels with unique scripts
- 33 creature types including custom "Angel"
- Custom game rules and configurations
- 30 voice acting files (18MB per level)
- 7 custom music tracks (15MB total)
- Land view graphics and animations

**File Organization:**
```
tempkpr/                   (13MB) - Level maps and scripts
tempkpr_cfgs/              (588KB) - Global configurations
tempkpr_crtr/              (132KB) - Creature definitions
tempkpr_eng/               (36MB) - Voice acting (English)
tempkpr_media/             (15MB) - Music files
tempkpr_Ind/               (land view images)
tempkpr.cfg                - Campaign configuration
```

### Mod System Interconnections

```
Campaign Flow:
tempkpr.cfg → References all folders
    ↓
    ├→ CREATURES_LOCATION: tempkpr_crtr/creature.cfg
    │   └→ Lists: WIZARD, BARBARIAN, ..., ANGEL (33 total)
    │       └→ Each creature has .cfg: tempkpr_crtr/Angel.cfg
    │           └→ Defines: stats, sprites, sounds, behavior
    │               └→ References: LAIR_DRK_NGL from objects.cfg
    │
    ├→ CONFIGS_LOCATION: tempkpr_cfgs/
    │   ├→ creature.cfg - Creature list order (critical!)
    │   ├→ rules.cfg - Game rule overrides
    │   ├→ objects.cfg - Object definitions (lairs, etc.)
    │   └→ trapdoor.cfg - Door/trap configs
    │
    ├→ LEVELS_LOCATION: tempkpr/
    │   └→ map00001.txt - Level script
    │       ├→ CREATURE_AVAILABLE(PLAYER1, FLY, 1, 0)
    │       ├→ SET_CREATURE_HEALTH(KNIGHT, 1250)
    │       └→ CREATE_PARTY(THIEF_GUARD)
    │           └→ ADD_TO_PARTY(THIEF_GUARD, THIEF, 2, ...)
    │
    ├→ MEDIA_LOCATION: tempkpr_media/
    │   └→ battle01.mp3, Theme.mp3, etc.
    │
    └→ SPEECH: tempkpr_eng/
        └→ good01.wav, bad01.wav, ... (per-level narration)
```

### How the Angel Creature Works

The Angel demonstrates full creature integration:

1. **Declaration** in `tempkpr_cfgs/creature.cfg`:
   ```
   Creatures = WIZARD ... ANGEL
   ```
   Position 33 in list = creature ID

2. **Definition** in `tempkpr_crtr/Angel.cfg`:
   - Stats: 1000 HP, 110 strength, 130 dexterity
   - Sprites: ANGEL_STAND, ANGEL_WALK, ANGEL_ATTACK, etc.
   - Sounds: Custom IDs (337=hit, 340=die, etc.)
   - Lair: LAIR_DRK_NGL
   - Powers: FREEZE (lvl 3), HAILSTORM (lvl 5), HEAL (lvl 10)

3. **Lair Object** in `tempkpr_cfgs/objects.cfg`:
   ```
   LAIR_DRK_NGL - Dark angel totem
   ```

4. **Sprite Assets** in `tempkpr_cfgs/angel.zip`:
   - PNG files for all animations
   - attack_tp/rotation01_frame01.png, etc.

5. **Usage in Levels**:
   ```
   CREATURE_AVAILABLE(PLAYER1, ANGEL, 1, 0)
   ADD_CREATURE_TO_POOL(ANGEL, 5)
   ```

### Modular Design Opportunity

**Current:** Campaign bundles everything (65MB total)

**With .kfxmod:**

**angel_creature_pack.kfxmod** (5MB):
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
Contains: Angel.cfg, angel.zip, object definitions

**tempest_keeper.kfxmod** (60MB):
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
Contains: Levels, scripts, audio, but NOT Angel creature

**Benefits:**
- Angel pack can be used by other campaigns
- Updates to Angel don't require campaign redownload
- Smaller campaign download for users who already have Angel
- Clearer attribution (Angel creator gets credit)

## CDN Update System

### Update Check Flow

```
Game Startup:
    ↓
Load installed mods
    ↓
For each mod with update_url:
    ↓
HTTP GET update_url/version.json
    ↓
Parse response:
{
  "mod_id": "tempest_keeper",
  "current_version": "1.2.0",
  "download_url": "https://cdn.../tempest_keeper-1.2.0.kfxmod",
  "checksum_sha256": "abc123...",
  "release_notes": "https://..."
}
    ↓
Compare current_version with installed version
    ↓
If newer:
    ├→ Show notification in UI
    ├→ Player clicks "Update"
    ├→ Download from download_url
    ├→ Verify checksum_sha256
    ├→ Backup old version
    ├→ Install new version
    └→ Prompt restart if needed
```

### Example CDN Structure

```
cdn.example.com/mods/
├── tempest_keeper/
│   ├── version.json                    # Latest version info
│   ├── tempest_keeper-1.0.0.kfxmod     # Old version
│   ├── tempest_keeper-1.1.0.kfxmod     # Old version
│   └── tempest_keeper-1.2.0.kfxmod     # Current version
│
└── angel_creature_pack/
    ├── version.json
    └── angel_creature_pack-1.0.0.kfxmod
```

## Future Enhancements

### Phase 2: Advanced Features
- **Digital signatures**: Cryptographic verification
- **Delta updates**: Download only changed files
- **Mod collections**: Bundle multiple related mods
- **In-game browser**: Browse and install from UI

### Phase 3: Community Features
- **Workshop integration**: Steam Workshop, ModDB
- **Rating system**: User reviews and ratings
- **Multiplayer sync**: Ensure same mods across clients
- **Profile support**: Different mod sets per playthrough

### Phase 4: Developer Tools
- **Hot reloading**: Test changes without restart
- **Debugging support**: Enhanced logging and diagnostics
- **Automated testing**: Validate mods in CI/CD
- **Documentation generator**: Auto-generate from metadata

## Comparison with Other Games

| Feature | .kfxmod | Skyrim | Factorio | Minecraft |
|---------|---------|--------|----------|-----------|
| Binary format | ✓ | ✓ (ESM/ESP) | ✗ (ZIP) | ✗ (JAR) |
| Compression | ✓ | ✗ | ✓ | ✓ |
| JSON metadata | ✓ | ✗ | ✓ (info.json) | ✓ (mod.json) |
| Dependencies | ✓ | ✓ | ✓ | ✓ |
| Semver | ✓ | ✗ | ✓ | ~ |
| CDN updates | ✓ | ✗ | ✓ | ✗ |
| Load phases | ✓ | ✗ | ✗ | ✗ |
| Conflicts | ✓ | ~ | ✗ | ✗ |

## Conclusion

The `.kfxmod` format provides a modern, robust solution for KeeperFX mod management. By combining:
- **Binary efficiency** with metadata richness
- **Backward compatibility** with new features
- **Dependency resolution** with conflict detection
- **CDN integration** with local caching

It creates a system that serves both mod creators and players effectively.

The Tempest Keeper campaign serves as an excellent demonstration of the system's capabilities, showcasing how complex, multi-component mods can be packaged, distributed, and managed efficiently.

## References

- Full specification: `docs/mod_binary_format.md`
- Header file: `src/config_modpack.h`
- Implementation: `src/config_modpack.c`
- Tool source: `tools/kfxmod/tool_modpack.c`
- Example metadata: `tempfolder/metadata.json`
