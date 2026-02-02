# Quick Integration Guide - KeeperFX Achievement System

This guide provides step-by-step instructions to complete the achievement system integration.

## Prerequisites

The achievement system framework is complete. This guide covers integrating it into the game.

Estimated time: 6-8 hours

## Step 1: Build System Integration (10 minutes)

### Makefile

Add to your source lists (around line 100-150):

```makefile
# Achievement system sources
ACHIEVEMENT_SRCS = \
  src/achievement_api.c \
  src/achievement_definitions.c \
  src/achievement_tracker.c \
  src/achievement_steam.cpp

# Add to main sources
OBJS += $(ACHIEVEMENT_SRCS:.c=.o)
OBJS := $(OBJS:.cpp=.o)
```

### CMakeLists.txt

Add to your target sources:

```cmake
set(ACHIEVEMENT_SOURCES
    src/achievement_api.c
    src/achievement_definitions.c
    src/achievement_tracker.c
    src/achievement_steam.cpp
)

# Add to main executable
add_executable(keeperfx ${KEEPERFX_SOURCES} ${ACHIEVEMENT_SOURCES})
```

### Test Build

```bash
make clean
make
# or
cmake --build .
```

If compilation fails, check for missing includes or adjust compiler flags.

## Step 2: Initialize Achievement System (30 minutes)

### File: `src/main.cpp` (or wherever main initialization occurs)

Add includes at top:
```cpp
#include "achievement_api.h"
#include "achievement_definitions.h"
#include "achievement_tracker.h"
#include "achievement_steam.hpp"
```

In main initialization (after campaign loading):
```cpp
// Initialize achievement system
SYNCLOG("Initializing achievement system");
if (achievements_init())
{
    // Register Steam backend
#ifdef _WIN32
    if (steam_achievements_register())
    {
        SYNCLOG("Steam achievement backend registered");
    }
#endif
    
    // Load achievements for current campaign
    int ach_count = load_campaign_achievements(&campaign);
    SYNCLOG("Loaded %d achievements for campaign", ach_count);
}
else
{
    WARNLOG("Failed to initialize achievement system");
}
```

In main shutdown (before exit):
```cpp
// Shutdown achievement system
SYNCLOG("Shutting down achievement system");
achievements_shutdown();
```

In main game loop (once per frame):
```cpp
// Update achievements
achievements_update();
```

### Test

1. Compile and run
2. Check keeperfx.log for:
   - "Initializing achievement system"
   - "Loaded X achievements for campaign"
3. Verify no errors

## Step 3: Level Event Integration (1 hour)

### File: `src/lvl_script.c` or `src/game_loop.c`

Add include:
```c
#include "achievement_tracker.h"
```

#### Level Start

Find where a new level is started (look for level loading code):

```c
// After level is loaded and initialized
achievement_tracker_init(game.loaded_level_number);
achievement_tracker_reset();
SYNCLOG("Achievement tracking initialized for level %d", game.loaded_level_number);
```

#### Level Completion

Find the function that processes win conditions (likely `set_player_as_won_level()` or similar):

```c
void set_player_as_won_level(PlayerNumber plyr_idx)
{
    // Existing code...
    
    // Add achievement tracking
    if (plyr_idx == my_player_number) // Only for human player
    {
        achievement_tracker_level_complete();
        SYNCLOG("Level completed, checking achievements");
    }
}
```

#### Tracker Update

In main game update loop:

```c
void process_game_turn(void)
{
    // Existing code...
    
    // Update achievement tracker
    achievement_tracker_update();
}
```

### Test

1. Start a level
2. Complete a level
3. Check log for achievement messages

## Step 4: Creature Event Integration (1 hour)

### File: `src/creature_control.c` or `src/thing_creature.c`

Add include:
```c
#include "achievement_tracker.h"
```

#### Creature Spawning

Find where creatures are created (look for `create_creature()` or similar):

```c
struct Thing* create_creature(...)
{
    // Existing code...
    
    // Track for achievements
    if (thing_is_creature(thing) && !thing_is_invalid(thing))
    {
        achievement_tracker_creature_spawned(thing->model);
    }
    
    return thing;
}
```

#### Creature Death

Find creature death function (likely `kill_creature()` or `creature_died()`):

```c
void kill_creature(struct Thing* thing, ...)
{
    // Existing code...
    
    // Track for achievements
    TbBool is_friendly = (thing->owner == my_player_number);
    achievement_tracker_creature_died(thing->model, is_friendly);
    
    // Track kills
    if (killer_thing != NULL && thing_is_creature(killer_thing))
    {
        TbBool killer_friendly = (killer_thing->owner == my_player_number);
        if (killer_friendly && !is_friendly)
        {
            achievement_tracker_creature_killed(killer_thing->model, thing->model);
        }
    }
}
```

### Test

1. Start level
2. Create creatures
3. Kill enemies
4. Check log for tracking messages

## Step 5: Player Action Integration (1 hour)

### File: `src/power_hand.c`

Add include:
```c
#include "achievement_tracker.h"
```

#### Slapping

Find slap function (search for "slap" in power_hand.c):

```c
void thing_slap(struct Thing* thing)
{
    // Existing code...
    
    // Track for achievements
    achievement_tracker_slap_used();
    SYNCLOG("Slap tracked for achievements");
}
```

### File: `src/dungeon_data.c` or wherever gold is spent

#### Gold Spending

```c
void player_spend_gold(long amount)
{
    // Existing code...
    
    // Track for achievements
    achievement_tracker_gold_spent(amount);
}
```

### File: Room/building code

#### Room Construction

```c
void place_room(int room_kind, ...)
{
    // Existing code...
    
    // Track for achievements
    achievement_tracker_room_built(room_kind);
}
```

### Test

1. Slap creatures
2. Spend gold
3. Build rooms
4. Check log

## Step 6: Combat Event Integration (1 hour)

### File: Combat/battle system

#### Battle Results

```c
void process_battle_result(TbBool player_won)
{
    // Existing code...
    
    // Track for achievements
    achievement_tracker_battle(player_won);
}
```

#### Heart Destruction

Find where dungeon hearts are destroyed:

```c
void destroy_dungeon_heart(struct Thing* heartng)
{
    // Existing code...
    
    // Track for achievements
    if (heartng->owner != my_player_number)
    {
        achievement_tracker_heart_destroyed();
    }
}
```

### Test

1. Fight battles
2. Destroy hearts
3. Check tracking

## Step 7: Implement Condition Checking (2 hours)

### File: `src/achievement_definitions.c`

Complete the `check_achievement_conditions()` function:

```c
TbBool check_achievement_conditions(struct AchievementDefinition* achievement_def)
{
    if (achievement_def == NULL)
        return false;
    
    // Check all conditions
    for (int i = 0; i < achievement_def->condition_count; i++)
    {
        struct AchievementCondition* cond = &achievement_def->conditions[i];
        
        switch (cond->type)
        {
            case AchCond_Level:
                if (achievement_tracker.current_level != cond->data.level.level_num)
                    return false;
                break;
                
            case AchCond_CompleteLevel:
                if (!achievement_tracker.level_completed)
                    return false;
                break;
                
            case AchCond_MaxSlaps:
                if (achievement_tracker.slaps_used > cond->data.slaps.max_count)
                    return false;
                break;
                
            case AchCond_NoCreatureDeaths:
                if (achievement_tracker.creature_deaths > 0)
                    return false;
                break;
                
            case AchCond_HeartsDestroyed:
                if (achievement_tracker.hearts_destroyed < cond->data.hearts.count)
                    return false;
                break;
                
            // TODO: Implement other condition types
            
            default:
                WARNLOG("Unimplemented condition type: %d", cond->type);
                break;
        }
    }
    
    return true; // All conditions met
}
```

### File: `src/achievement_tracker.c`

Complete the `achievement_tracker_update()` function:

```c
void achievement_tracker_update(void)
{
    // Check active achievements
    for (int i = 0; i < achievement_tracker.active_achievement_count; i++)
    {
        struct AchievementDefinition* ach_def = achievement_tracker.active_achievements[i];
        
        if (ach_def == NULL)
            continue;
        
        // Check if conditions are met
        if (check_achievement_conditions(ach_def))
        {
            // Build full achievement ID
            char achievement_id[ACHIEVEMENT_ID_LEN];
            snprintf(achievement_id, sizeof(achievement_id), "%s.%s", 
                     campaign.name, ach_def->id);
            
            // Unlock achievement
            if (achievement_unlock(achievement_id))
            {
                SYNCLOG("Achievement unlocked: %s", achievement_id);
                
                // Remove from active list
                achievement_tracker.active_achievements[i] = NULL;
            }
        }
    }
}
```

## Step 8: Local Storage Implementation (1 hour)

### Create: `src/achievement_storage.c`

```c
#include "achievement_api.h"
#include "bflib_fileio.h"
#include <stdio.h>
#include <time.h>

TbBool achievement_save_local(const char* filename)
{
    FILE* fp = fopen(filename, "w");
    if (!fp)
        return false;
    
    fprintf(fp, "{\n");
    fprintf(fp, "  \"version\": 1,\n");
    fprintf(fp, "  \"achievements\": {\n");
    
    for (int i = 0; i < achievements_count; i++)
    {
        struct Achievement* ach = &achievements[i];
        fprintf(fp, "    \"%s\": {\n", ach->id);
        fprintf(fp, "      \"unlocked\": %s,\n", ach->unlocked ? "true" : "false");
        fprintf(fp, "      \"unlock_time\": %ld,\n", (long)ach->unlock_time);
        fprintf(fp, "      \"progress\": %.2f\n", ach->progress);
        fprintf(fp, "    }%s\n", (i < achievements_count - 1) ? "," : "");
    }
    
    fprintf(fp, "  }\n");
    fprintf(fp, "}\n");
    
    fclose(fp);
    return true;
}

// TODO: Implement achievement_load_local()
```

## Step 9: Testing (2-3 hours)

### Test Plan

1. **Basic Achievement Test**
   - Start level 1
   - Complete level 1
   - Verify "first_win" achievement unlocks
   - Check Steam (if available)
   - Check local storage file

2. **Slap Test**
   - Start level 3
   - Complete without slapping
   - Verify "no_slap" achievement unlocks

3. **Creature Test**
   - Start level 5
   - Only use imps
   - Complete level
   - Verify "imp_army" achievement unlocks

4. **Steam Integration Test**
   - Run with Steam client
   - Unlock achievement
   - Verify in Steam overlay
   - Check Steam achievement page

5. **Performance Test**
   - Play through multiple levels
   - Check CPU usage
   - Monitor memory
   - Verify no lag

### Debug Logging

Add temporary debug logging:

```c
// In achievement_tracker_update()
if (game.play_gameturn % 100 == 0) // Every 100 turns
{
    SYNCLOG("Tracker state: slaps=%ld, deaths=%ld, kills=%ld", 
            achievement_tracker.slaps_used,
            achievement_tracker.creature_deaths,
            achievement_tracker.enemy_kills);
}
```

## Step 10: Polish (1 hour)

1. Remove debug logging
2. Add user notification (optional)
3. Test edge cases
4. Update documentation with any changes
5. Create PR

## Common Issues

### Achievement Not Unlocking

- Check condition evaluation logic
- Verify tracker events are firing
- Check log for condition states
- Verify achievement ID matches

### Compilation Errors

- Check includes
- Verify function signatures
- Check for typos in function names

### Steam Not Working

- Verify steam_api.dll present
- Check Steam client is running
- Verify app ID in steam_appid.txt
- Check log for Steam errors

### Performance Issues

- Limit tracker_update() frequency
- Cache condition checks
- Profile with game profiler

## Verification Checklist

- [ ] Code compiles without errors
- [ ] Game starts and loads achievements
- [ ] Level completion tracked
- [ ] Creature events tracked
- [ ] Player actions tracked
- [ ] Achievement unlocks work
- [ ] Steam integration works (if available)
- [ ] Local storage works
- [ ] No memory leaks
- [ ] No performance impact
- [ ] Log messages appropriate
- [ ] Documentation updated

## Done!

Once complete, achievements will be fully functional. Campaign creators can immediately start adding custom achievements to their campaigns.

## Support

- Review `docs/achievements_implementation.md` for API details
- Check `docs/ACHIEVEMENT_SUMMARY.md` for overview
- See `campgns/keeporig/achievements.cfg` for examples
- Join Discord for help: https://discord.gg/hE4p7vy2Hb
