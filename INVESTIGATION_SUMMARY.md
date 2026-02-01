# Save System Investigation Summary

## Question
"How does the game perform saving? What details are stored in the save file. I want you to look at what is crucial to be saved and what is being saved as a side effect. I want to know what things are being stored that should ideally be loaded on demand so that I can make cross version updates less susceptible to breaking saves, or having a player have to start a level from the beginning because mod updates (like sound one) don't get picked up upon saving."

## Answer

### Good News: The System is More Resilient Than Expected!

The save system already has excellent resilience to mod updates for **sprites, sounds, and configurations**. Here's what happens:

### What Gets Reloaded From Files (Not From Save)

When loading a save, the game **immediately reloads from files**:

1. ✅ **Custom Sprites** - `init_custom_sprites()` reinitializes all sprites
   - Sprite mods ARE picked up on load
   - New/changed sprites work with old saves

2. ✅ **Custom Sounds** - `sound_manager_clear_custom_sounds()` clears and rebuilds
   - Sound mods ARE picked up on load
   - New/changed sounds work with old saves

3. ✅ **All Configuration Data** - `load_stats_files()` reloads:
   - Creature stats and behaviors
   - Trap/door configurations
   - Magic spell settings
   - Object properties
   - Game rules
   - Balance changes ARE picked up on load

4. ✅ **Campaign Data** - Reloaded from campaign files

### What Is Saved

The save file contains 4 chunks:

1. **InfoBlock (CatalogueEntry)** - ~256 bytes
   - Campaign name/filename
   - Level number
   - Player name
   - Game version

2. **GameOrig (struct Game)** - ~194KB
   - **Crucial:** Game state (turns, seeds, player/creature/room data)
   - **Crucial:** Map state (slabs, tiles, navigation)
   - **Crucial:** Active entities (things, effects, traps)
   - **Side-effect:** Config data (gets overwritten by files on load)
   - **Side-effect:** Light system (could be rebuilt)

3. **IntralevelData** - ~12KB
   - **Crucial:** Bonus levels found
   - **Crucial:** Transferred creatures between levels
   - **Crucial:** Campaign flags

4. **LuaData** - Variable size
   - **Crucial:** Lua script state

### Compatibility Matrix

| Mod Type | Works with Old Saves? | Why |
|----------|----------------------|-----|
| Sound mods | ✅ Yes | Cleared and rebuilt from files |
| Sprite mods | ✅ Yes | Reinitialized from files |
| Balance changes | ✅ Yes | Stats reloaded from files |
| Config changes | ✅ Yes | Configs reloaded from files |
| New creatures (within limits) | ✅ Yes | If under CREATURE_TYPES_MAX |
| Removing creatures | ❌ No | Saved instances become invalid |
| Map size changes | ❌ No | Arrays are compile-time sized |
| Lua script changes | ⚠️ Maybe | Depends on compatibility |

### Recommendations

#### You're Already In Good Shape!

The current system handles your main concern well:
- ✅ Sound updates WILL be picked up when loading old saves
- ✅ Sprite updates WILL be picked up when loading old saves
- ✅ Balance/config updates WILL be picked up when loading old saves

#### Future Improvements (Optional)

1. **Version Tagging** - Tag config data in saves
   - Skip loading if version mismatch detected
   - Always prefer file versions

2. **Symbolic References** - Use names instead of indices
   - Replace creature type indices with names
   - Resolve names to indices on load
   - More resilient to additions/removals

3. **Exclude Config Data** - Don't save game.conf
   - Saves ~several KB per file
   - Always rebuild from files
   - Better version compatibility

### What Was Changed

This PR adds:
- Comprehensive inline documentation explaining save/load behavior
- Comments identifying crucial vs side-effect data
- Architecture document (docs/save_system_architecture.md)
- Compatibility notes for modders

**No functional code changes** - just documentation to help future development.

### Conclusion

**Your concern about mod updates breaking saves is largely unfounded!** The system already reloads sprites, sounds, and configs from files on every load. This design makes saves quite resilient to the types of updates you mentioned.

The main limitations are:
- Structural changes (adding/removing creature types)
- Map size changes
- Incompatible Lua script changes

For normal modding (new sounds, new sprites, balance tweaks), old saves will work fine.
