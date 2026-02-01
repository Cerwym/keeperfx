# KeeperFX Save System Architecture

## Overview

The KeeperFX save system stores game state in a chunked binary format. This document explains what data is saved, what is loaded on demand, and how to make saves more resilient to mod and version updates.

## Save File Format

Save files contain 4 main chunks:

### 1. InfoBlock (CatalogueEntry)
**Purpose:** Metadata about the save
**Size:** ~256 bytes
**Critical:** Yes

Contains:
- Campaign filename and name
- Level number
- Player name
- Game version (major, minor, release, build)

### 2. GameOrig (struct Game)
**Purpose:** Complete game state
**Size:** ~194KB
**Critical:** Partially

This massive struct contains:
- **CRUCIAL DATA:**
  - Game turn counters (`play_gameturn`, `pckt_gameturn`)
  - Random seeds (`action_random_seed`, `ai_random_seed`, etc.)
  - Player states (all 6 players)
  - Creature data (all creature instances)
  - Room and dungeon data
  - Map state (slabs, tiles, navigation maps)
  - Active entities (things, effects, traps, doors)
  - GUI state, messages, timers
  - Campaign progress

- **SIDE-EFFECT DATA (reloaded from files):**
  - `game.conf` (Configs struct) - All configuration data
  - `game.lightst` (LightSystemState) - Light system state
  - Sound settings (indices rebuilt on load)

### 3. IntralevelData
**Purpose:** Campaign-persistent data
**Size:** ~12KB
**Critical:** Yes

Contains:
- Bonus levels found/completed
- Creatures transferred between campaign levels
- Campaign-specific flags
- Next level progression state

This data cannot be regenerated and must persist across campaign levels.

### 4. LuaData
**Purpose:** Serialized Lua script state
**Size:** Variable (depends on script)
**Critical:** Yes (for scripted levels)

Contains the serialized state of custom Lua scripts, enabling complex level logic to persist across saves.

## Load Process and Mod Resilience

### What Gets Reloaded From Files

During `load_game_chunks()`, the following are **reloaded from files** (not from save):

1. **Campaign system** - Validates campaign exists
2. **Map strings** - Level-specific text data
3. **Custom sprites** (`init_custom_sprites()`)
   - ✅ Sprite mods are picked up
   - ✅ New sprites work with old saves
4. **Custom sounds** (`sound_manager_clear_custom_sounds()`)
   - ✅ Sound mods are picked up
   - ✅ New sounds work with old saves
5. **Stats files** (`load_stats_files()`)
   - ✅ Balance changes are picked up
   - ✅ New creatures/objects/spells work (within limits)
   - ✅ Rule changes are applied
6. **Creature scores** - Recalculated from reloaded data

### Mod Update Compatibility

| Mod Type | Compatibility | Notes |
|----------|---------------|-------|
| Sound mods | ✅ Excellent | Sounds cleared and rebuilt on load |
| Sprite mods | ✅ Excellent | Sprites reinitialized from files |
| Balance mods | ✅ Good | Stats reloaded, but creature instances may have old stats |
| Config changes | ✅ Good | Configs reloaded from files |
| New creature types | ⚠️ Limited | Works if within CREATURE_TYPES_MAX |
| Removing creatures | ❌ Breaks | Saved creature instances become invalid |
| Map size changes | ❌ Breaks | Navigation maps are compile-time sized |
| Lua script changes | ⚠️ Depends | State may be incompatible with modified scripts |

## Optimization Opportunities

### Current Redundancies

The following data is saved but immediately overwritten on load:

1. **game.conf (Configs struct)** - ~Several KB
   - Saved in GameOrig chunk
   - Overwritten by `load_stats_files()`
   - Could be excluded from saves

2. **Light system state** - ~Size varies
   - Exported before save
   - Could be rebuilt from map geometry

3. **Sound indices** - Already cleared and rebuilt
   - Could use symbolic names instead of indices

### Recommended Improvements

To make saves more resilient to version/mod updates:

#### Short-term (Documentation)
- ✅ Add inline comments explaining crucial vs side-effect data
- ✅ Document which fields are reloaded from files
- ✅ Create this architecture document

#### Medium-term (Version Tags)
- Add version tags to Configs in save files
- Skip loading Configs if version mismatch detected
- Always prefer file versions over saved versions

#### Long-term (Symbolic References)
- Replace direct indices with symbolic names for:
  - Creature types
  - Sound effects
  - Sprite IDs
- Resolve names to indices on load based on current configs
- Makes saves resilient to content additions/removals

## Developer Guidelines

### When Adding New Save Data

Ask yourself:
1. **Can this be regenerated from files?**
   - If yes, consider making it load-on-demand
2. **Will this break compatibility if changed?**
   - If yes, add version tagging
3. **Is this crucial for game state?**
   - If no, consider excluding from saves

### When Modifying Existing Data

Be aware:
- Changing struct sizes breaks all existing saves
- Adding fields at the end is safer than inserting
- Use versioning for chunks that may evolve
- Document compatibility impacts

### Testing Save Compatibility

When making changes:
1. Create a save with old version
2. Make your changes
3. Load the old save
4. Verify game state is correct
5. Test that mods get picked up (sprites, sounds, configs)

## Technical Details

### File Structure
```
[FileChunkHeader] ID=SGC_InfoBlock, len=sizeof(CatalogueEntry)
[CatalogueEntry data]

[FileChunkHeader] ID=SGC_GameOrig, len=sizeof(struct Game)
[struct Game data - ~194KB]

[FileChunkHeader] ID=SGC_IntralevelData, len=sizeof(struct IntralevelData)
[struct IntralevelData data]

[FileChunkHeader] ID=SGC_LuaData, len=variable
[Lua serialized data - variable size]
```

### Key Functions

- `save_game_chunks()` - Writes all chunks to file
- `load_game_chunks()` - Reads chunks and reloads file-based data
- `load_stats_files()` - Reloads all config data from files
- `init_custom_sprites()` - Reinitializes sprite data
- `sound_manager_clear_custom_sounds()` - Clears and rebuilds sound data

## Conclusion

The KeeperFX save system is reasonably resilient to mod updates for sprites, sounds, and configuration data. However, it has limitations around structural changes (creature types, map sizes).

The key insight is that **much of what's saved is immediately reloaded from files**, making the system more compatible than it initially appears. This behavior should be preserved and potentially enhanced to further improve mod compatibility.

For questions or improvements, refer to:
- `src/game_saves.c` - Core save/load implementation
- `src/game_legacy.h` - struct Game definition
- `src/game_merge.h` - struct IntralevelData definition
