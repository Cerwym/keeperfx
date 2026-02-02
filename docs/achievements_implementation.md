# KeeperFX Achievement System

## Overview

The KeeperFX achievement system provides a comprehensive, cross-platform framework for tracking and unlocking achievements in the game. It supports multiple distribution platforms (Steam, GOG, Xbox, PlayStation, Epic) and allows campaign creators to define custom achievements.

## Architecture

### Core Components

```
src/
├── achievement_api.h/c              # Platform-agnostic API
├── achievement_definitions.h/c     # Achievement definition parser
├── achievement_tracker.h/c         # Game event tracking
├── achievement_steam.cpp/hpp       # Steam backend
└── (future: achievement_gog.cpp, achievement_xbox.cpp, etc.)
```

### Key Features

- **Cross-Platform**: Single achievement definition works on all platforms
- **Moddable**: Campaigns can define custom achievements via config files
- **Offline Support**: Works without platform SDK via local storage
- **Backward Compatible**: Existing campaigns work without achievements
- **Extensible**: Easy to add new platforms or condition types

## Platform Support

### Currently Implemented
- **Steam** (Steamworks SDK) - `src/achievement_steam.cpp`
- **Local Storage** (JSON fallback) - Built into `achievement_api.c`

### Planned
- GOG Galaxy SDK
- Xbox GDK (PC and Console)
- PlayStation SDK (PS4/PS5/Vita)
- Epic Online Services (EOS)

## For Campaign Creators

See `docs/achievement_system.txt` for complete guide on creating achievements.

### Quick Example

Create `campgns/mycampaign/achievements.cfg`:

```ini
ACHIEVEMENT first_win
    NAME = "First Victory"
    DESCRIPTION = "Complete the first level"
    POINTS = 10
    LEVEL = 1
    COMPLETE_LEVEL = 1
END_ACHIEVEMENT

ACHIEVEMENT imp_army
    NAME = "Imp Army"
    DESCRIPTION = "Complete level 5 using only Imps"
    POINTS = 25
    LEVEL = 5
    COMPLETE_LEVEL = 1
    ONLY_CREATURE = IMP
END_ACHIEVEMENT
```

## For Developers

### Integrating Achievement Tracking

Achievement tracking hooks into existing game systems. Add tracker calls at appropriate points:

#### Level Events (`lvl_script.c`, `game_loop.c`)

```c
// Level start
achievement_tracker_init(level_number);

// Level completion
if (player_won) {
    achievement_tracker_level_complete();
}
```

#### Creature Events (`creature_control.c`, `thing_creature.c`)

```c
// Creature spawned
achievement_tracker_creature_spawned(creature_model);

// Creature died
achievement_tracker_creature_died(creature_model, is_friendly);

// Creature killed enemy
achievement_tracker_creature_killed(killer_model, victim_model);
```

#### Player Actions (`power_hand.c`)

```c
// Player slapped creature
achievement_tracker_slap_used();
```

#### Combat Events

```c
// Battle won/lost
achievement_tracker_battle(player_won);

// Heart destroyed
achievement_tracker_heart_destroyed();
```

#### Resource Tracking

```c
// Gold spent
achievement_tracker_gold_spent(amount);
```

#### Construction

```c
// Room built
achievement_tracker_room_built(room_kind);

// Trap/door placed
achievement_tracker_trap_used(trap_kind);
```

### Initialization

In main game initialization (`main.cpp` or similar):

```c
// After loading campaign
achievements_init();
load_campaign_achievements(&campaign);

// Register platform backends
#ifdef STEAM_ENABLED
steam_achievements_register();
#endif
```

In main game loop:

```c
// Each frame/turn
achievements_update();
```

In shutdown:

```c
achievements_shutdown();
```

### Adding New Platform Backend

1. Create `src/achievement_<platform>.cpp`
2. Implement `AchievementBackend` interface:

```cpp
static struct AchievementBackend my_backend = {
    "MyPlatform",                    // name
    AchPlat_MyPlatform,             // platform_type
    my_platform_init,               // init
    my_platform_shutdown,           // shutdown
    my_platform_unlock,             // unlock
    my_platform_is_unlocked,        // is_unlocked
    my_platform_set_progress,       // set_progress
    my_platform_get_progress,       // get_progress
    my_platform_sync,               // sync (optional)
};
```

3. Register backend: `achievements_register_backend(&my_backend);`

### Achievement Condition Implementation

To implement achievement condition checking, extend `achievement_tracker.c`:

```c
void achievement_tracker_update(void)
{
    // Check all active achievements
    for (int i = 0; i < achievement_tracker.active_achievement_count; i++)
    {
        struct AchievementDefinition* ach = achievement_tracker.active_achievements[i];
        
        if (check_achievement_conditions(ach))
        {
            // Build namespaced ID
            char achievement_id[ACHIEVEMENT_ID_LEN];
            snprintf(achievement_id, sizeof(achievement_id), "%s.%s", 
                     campaign.name, ach->id);
            
            // Unlock
            if (achievement_unlock(achievement_id))
            {
                // Remove from active list
                // ...
            }
        }
    }
}
```

### Testing Achievements

1. Create test achievements with simple conditions
2. Add logging to track condition states
3. Use script flags for manual testing: `achievement_tracker_script_flag(999);`
4. Check `keeperfx.log` for achievement messages

### Adding New Condition Types

1. Add enum value to `AchievementConditionType` in `achievement_definitions.h`
2. Add union member to `AchievementCondition.data`
3. Add command to `achievement_commands[]` in `achievement_definitions.c`
4. Add parsing logic in `load_achievements_config()`
5. Add condition check in `check_achievement_conditions()`
6. Document in `docs/achievement_system.txt`

## Build Integration

### Makefile Changes

Add new source files to build:

```makefile
ACHIEVEMENT_SRCS = \
  src/achievement_api.c \
  src/achievement_definitions.c \
  src/achievement_tracker.c \
  src/achievement_steam.cpp

OBJS += $(ACHIEVEMENT_SRCS:.c=.o)
OBJS := $(OBJS:.cpp=.o)
```

### CMakeLists.txt Changes

```cmake
set(ACHIEVEMENT_SOURCES
    src/achievement_api.c
    src/achievement_definitions.c
    src/achievement_tracker.c
    src/achievement_steam.cpp
)

add_library(keeperfx ${KEEPERFX_SOURCES} ${ACHIEVEMENT_SOURCES})
```

### Platform-Specific Compilation

Use preprocessor guards for platform-specific backends:

```cpp
#ifdef STEAM_ENABLED
#include "achievement_steam.hpp"
steam_achievements_register();
#endif

#ifdef GOG_ENABLED
#include "achievement_gog.hpp"
gog_achievements_register();
#endif
```

## API Reference

### Core API (`achievement_api.h`)

- `achievements_init()` - Initialize system
- `achievements_shutdown()` - Cleanup
- `achievements_update()` - Update each frame
- `achievement_unlock(id)` - Unlock achievement
- `achievement_is_unlocked(id)` - Check unlock status
- `achievement_set_progress(id, progress)` - Set progress (0.0-1.0)
- `achievement_get_progress(id)` - Get progress
- `achievement_register(achievement)` - Register achievement
- `achievements_clear()` - Clear all achievements

### Tracker API (`achievement_tracker.h`)

- `achievement_tracker_init(level)` - Initialize for level
- `achievement_tracker_update()` - Update tracker
- `achievement_tracker_level_complete()` - Mark level complete
- `achievement_tracker_creature_spawned(model)` - Track creature
- `achievement_tracker_creature_died(model, friendly)` - Track death
- `achievement_tracker_slap_used()` - Track slap
- `achievement_tracker_battle(won)` - Track battle
- `achievement_tracker_heart_destroyed()` - Track heart
- `achievement_tracker_gold_spent(amount)` - Track gold
- `achievement_tracker_room_built(kind)` - Track room
- `achievement_tracker_spell_used(kind)` - Track spell
- `achievement_tracker_trap_used(kind)` - Track trap
- `achievement_tracker_script_flag(id)` - Script trigger

### Definition API (`achievement_definitions.h`)

- `load_campaign_achievements(campaign)` - Load from campaign
- `load_achievements_config(file, campaign)` - Load from file
- `register_achievement_definition(def, campaign)` - Register definition
- `check_achievement_conditions(def)` - Check conditions

## Data Flow

```
Campaign File (achievements.cfg)
  ↓
achievement_definitions.c (parse)
  ↓
achievement_api.c (register)
  ↓
achievement_tracker.c (monitor game events)
  ↓
achievement_api.c (unlock)
  ↓
Platform Backend (Steam/GOG/etc.) OR Local Storage
```

## Logging

Achievement system uses standard KeeperFX logging:

- `SYNCLOG` - Initialization, shutdown, major events
- `JUSTLOG` - Achievement registration, file loading
- `WARNLOG` - Warnings (duplicate IDs, failed unlocks)
- `ERRORLOG` - Errors (file not found, parse errors)

Check `keeperfx.log` for achievement-related messages.

## Future Enhancements

### Planned Features
- In-game achievement notification UI
- Achievement viewer/browser
- Progress tracking UI
- Cloud save sync for local achievements
- Achievement statistics and analytics
- Meta-achievements (complete all in campaign)
- Time-based achievements (daily/weekly challenges)
- Multiplayer achievements
- Leaderboards for achievement completion

### Technical Improvements
- Lua scripting integration for complex conditions
- Achievement editor tool
- Batch unlock API for testing
- Achievement reset/clear functionality
- Save/load achievement state with game saves
- Network sync for multiplayer achievement unlocks

## Known Limitations

- Steam implementation requires Steamworks SDK headers
- Some platforms require partnerships/certifications
- Achievement condition checking runs every turn (performance consideration)
- Limited to 100 achievements per campaign (configurable)
- No built-in achievement UI (notification/viewer)
- Progress tracking not fully implemented for Steam
- Local storage doesn't sync across devices

## Contributing

To contribute to the achievement system:

1. Follow existing code style
2. Add logging for debugging
3. Test with multiple platforms if possible
4. Update documentation
5. Add example achievements for testing
6. Consider backward compatibility

## Resources

- Design Document: `docs/achievements_design.md`
- User Guide: `docs/achievement_system.txt`
- Example Config: `campgns/keeporig/achievements.cfg`
- Steam API: https://partner.steamgames.com/doc/features/achievements
- GOG Galaxy: https://docs.gog.com/galaxyapi/
- Xbox GDK: https://docs.microsoft.com/gaming/gdk/
- PlayStation SDK: https://www.playstation.com/develop/
