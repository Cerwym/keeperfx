# KeeperFX Achievement System Design

## Overview
This document describes the design and implementation of a cross-platform achievement system for KeeperFX that supports multiple distribution platforms and allows campaigns/mods to define custom achievements.

## Supported Platforms

### 1. **Steam** (Already Integrated)
- **SDK**: Steamworks SDK
- **Status**: Basic integration exists in `src/steam_api.cpp`
- **Features**: Achievements, stats, leaderboards
- **API**: ISteamUserStats interface
- **Requirements**: steam_api.dll, steam_appid.txt

### 2. **GOG Galaxy**
- **SDK**: GOG Galaxy SDK
- **API**: IStats interface for achievements
- **Requirements**: Galaxy.dll
- **License**: Free for GOG games
- **Features**: Achievements, stats, leaderboards

### 3. **Xbox (PC + Console)**
- **SDK**: GDK (Game Development Kit) / XDK
- **API**: Xbox Services API (XSAPI)
- **Requirements**: Proper Xbox partnership
- **Features**: Achievements (Gamerscore), stats, leaderboards
- **Note**: Requires Microsoft certification

### 4. **PlayStation (PS4/PS5/Vita)**
- **SDK**: PlayStation SDK
- **API**: NpTrophy API
- **Requirements**: PlayStation partnership
- **Features**: Trophies (Bronze/Silver/Gold/Platinum)
- **Note**: Requires Sony certification

### 5. **Epic Games Store**
- **SDK**: Epic Online Services (EOS)
- **API**: EOS Achievements Interface
- **License**: Free
- **Features**: Achievements, stats

### 6. **Local/Offline**
- **Fallback**: Local achievement tracking
- **Storage**: JSON file in user directory
- **Purpose**: Track achievements when no platform SDK is available

## Architecture

### Core Components

```
achievements/
├── achievement_api.h/c         - Main API interface
├── achievement_definitions.h/c - Achievement definition system
├── achievement_tracker.h/c     - Event tracking and unlock logic
├── achievement_storage.h/c     - Local storage fallback
└── platform/
    ├── achievement_steam.cpp   - Steam implementation
    ├── achievement_gog.cpp     - GOG Galaxy implementation
    ├── achievement_xbox.cpp    - Xbox implementation
    ├── achievement_playstation.cpp - PlayStation implementation
    └── achievement_epic.cpp    - Epic Games Store implementation
```

### Achievement Definition Format

Achievements are defined in campaign configuration files using a simple text format:

```ini
[achievements]
; Achievement ID must be unique per campaign
ACHIEVEMENT = imp_army
    NAME_TEXT_ID = 2001              ; String ID for name
    DESC_TEXT_ID = 2002              ; String ID for description
    ICON = achievements/imp_army.png ; Optional icon path
    HIDDEN = 0                       ; 0=visible, 1=hidden until unlock
    POINTS = 10                      ; Point value (platform-specific)
    ; Conditions
    LEVEL = 5                        ; Must be on level 5
    COMPLETE_LEVEL = 1               ; Must complete the level
    ONLY_CREATURE = IMP              ; Only allow imps
END_ACHIEVEMENT

ACHIEVEMENT = no_slap_challenge
    NAME_TEXT_ID = 2003
    DESC_TEXT_ID = 2004
    ICON = achievements/no_slap.png
    POINTS = 15
    LEVEL = 10
    COMPLETE_LEVEL = 1
    MAX_SLAPS = 0                    ; No slaps allowed
END_ACHIEVEMENT

ACHIEVEMENT = avatar_revenge
    NAME_TEXT_ID = 2005
    DESC_TEXT_ID = 2006
    ICON = achievements/avatar.png
    POINTS = 50
    LEVEL = 20
    CREATURE_KILL = AVATAR           ; Must kill an avatar
    CREATURE_USED = CONVERTED_AVATAR ; Must have converted avatar
END_ACHIEVEMENT
```

### Achievement Conditions

The following conditions can be used to define achievement requirements:

#### Level Conditions
- `LEVEL = <number>` - Achievement only available on specific level
- `LEVEL_RANGE = <min> <max>` - Available on level range
- `COMPLETE_LEVEL = 1` - Must complete the level
- `LEVEL_TIME_UNDER = <seconds>` - Complete in time limit

#### Creature Conditions
- `ONLY_CREATURE = <type>` - Only allow specific creature type
- `MIN_CREATURES = <type> <count>` - Minimum creatures of type
- `MAX_CREATURES = <type> <count>` - Maximum creatures of type
- `CREATURE_KILL = <type>` - Must kill specific creature type
- `CREATURE_USED = <type>` - Must have specific creature
- `NO_CREATURE_DEATHS = 1` - No friendly casualties

#### Combat Conditions
- `MAX_SLAPS = <count>` - Maximum slaps allowed
- `MIN_KILLS = <count>` - Minimum enemy kills
- `BATTLES_WON = <count>` - Win battles
- `HEARTS_DESTROYED = <count>` - Destroy enemy hearts

#### Resource Conditions
- `MAX_GOLD_SPENT = <amount>` - Budget limit
- `MIN_GOLD = <amount>` - Maintain gold level
- `TERRITORY_SIZE = <tiles>` - Control territory

#### Dungeon Conditions
- `ROOM_REQUIRED = <type>` - Must have room type
- `ROOM_FORBIDDEN = <type>` - Cannot build room type
- `SPELL_FORBIDDEN = <type>` - Cannot use spell
- `TRAP_USED = <type>` - Must use trap type

#### Custom Script Conditions
- `SCRIPT_FLAG = <id>` - Custom flag set by level script
- `LUA_CONDITION = <function>` - Lua callback for complex logic

## Implementation Plan

### Phase 1: Core Infrastructure
1. Create achievement API header with platform-agnostic interface
2. Implement achievement definition parser
3. Create achievement tracker that monitors game events
4. Implement local storage fallback

### Phase 2: Platform Integration
1. Extend Steam API to support achievements
2. Add GOG Galaxy SDK integration (optional)
3. Add platform detection and selection logic
4. Create achievement sync system

### Phase 3: Campaign Integration
1. Add achievement definitions to campaign config parser
2. Update campaign file format documentation
3. Create example achievements for original campaign
4. Add achievement UI to game (optional)

### Phase 4: Testing & Documentation
1. Create test achievements for validation
2. Write modder documentation
3. Test with multiple platforms
4. Create achievement icon templates

## Platform Abstraction API

```c
// Platform-agnostic achievement API
typedef struct Achievement {
    char id[64];              // Internal ID
    char name[256];           // Display name
    char description[512];    // Description
    int points;              // Point value
    TbBool hidden;           // Hidden until unlock
    TbBool unlocked;         // Current unlock state
    time_t unlock_time;      // When unlocked (0 if locked)
} Achievement;

// Initialize achievement system
TbBool achievements_init(void);
void achievements_shutdown(void);

// Achievement management
TbBool achievement_unlock(const char* achievement_id);
TbBool achievement_is_unlocked(const char* achievement_id);
float achievement_get_progress(const char* achievement_id);
void achievement_set_progress(const char* achievement_id, float progress);

// Load achievements from campaign
TbBool load_campaign_achievements(struct GameCampaign* campaign);

// Platform callbacks
typedef struct AchievementPlatform {
    const char* name;
    TbBool (*init)(void);
    void (*shutdown)(void);
    TbBool (*unlock)(const char* id);
    TbBool (*is_unlocked)(const char* id);
    void (*set_progress)(const char* id, float progress);
} AchievementPlatform;
```

## Event Integration Points

The achievement tracker will hook into these existing game events:

### From `game_loop.c` and `lvl_script.c`:
- Level start: `process_level_script()`
- Level completion: `set_player_as_won_level()`
- Level failure: `set_player_as_lost_level()`

### From `dungeon_stats.c`:
- Any stat update triggers achievement check
- End-level statistics compilation

### From `creature_control.c`:
- Creature spawning/death
- Creature state changes

### From `power_hand.c`:
- Slapping creatures

### From `thing_creature.c`:
- Combat events
- Creature kills

### From custom level scripts:
- `SET_FLAG` command to mark achievement conditions
- Custom Lua callbacks

## Data Storage

### Local Storage Format (JSON)
```json
{
  "version": 1,
  "achievements": {
    "keeporig.imp_army": {
      "unlocked": true,
      "unlock_time": 1706875200,
      "progress": 1.0
    },
    "keeporig.no_slap_challenge": {
      "unlocked": false,
      "progress": 0.0
    }
  },
  "stats": {
    "total_achievements": 25,
    "unlocked_achievements": 12,
    "completion_percentage": 48.0
  }
}
```

Storage location: `<user_data>/achievements.json`

## Mod Support

Mods can add achievements by including an `achievements.cfg` file in their campaign directory:

```
campgns/
  mycampaign/
    ├── mycampaign.cfg          # Campaign config
    ├── achievements.cfg        # Achievement definitions
    └── achievements/           # Achievement icons
        ├── achievement1.png
        └── achievement2.png
```

Achievement IDs are automatically namespaced by campaign: `<campaign_name>.<achievement_id>`

## Platform-Specific Notes

### Steam
- Achievement IDs must match Steamworks App Admin configuration
- Use `ISteamUserStats::SetAchievement()` to unlock
- Requires `SteamAPI_RunCallbacks()` in main loop

### GOG Galaxy
- Requires Galaxy client running
- Use `IStats::SetAchievement()` to unlock
- Automatically syncs with Galaxy backend

### Xbox
- Requires Xbox Live configuration in Partner Center
- Achievements have Gamerscore values
- Must follow Xbox achievement guidelines

### PlayStation
- Trophies have grade levels (Bronze/Silver/Gold/Platinum)
- Requires trophy configuration in DevNet
- Must pass Sony certification

## Benefits

1. **Cross-Platform**: Single achievement definition works on all platforms
2. **Moddable**: Campaigns can define custom achievements
3. **Offline Support**: Works without platform SDK via local tracking
4. **Backward Compatible**: Existing campaigns work without achievements
5. **Extensible**: Easy to add new platforms or condition types
6. **Platform-Appropriate**: Respects platform-specific requirements

## Future Enhancements

1. Achievement UI in-game (popup notifications, achievement viewer)
2. Steam Workshop integration for achievement mods
3. Leaderboards for achievement completion time
4. Meta-achievements (complete all achievements in a campaign)
5. Achievement statistics and analytics
6. Cloud save sync for local achievements
7. Achievement editor tool for modders

## References

- Steam: https://partner.steamgames.com/doc/features/achievements
- GOG Galaxy: https://docs.gog.com/galaxyapi/
- Xbox: https://docs.microsoft.com/gaming/gdk/
- PlayStation: https://www.playstation.com/develop/
- Epic: https://dev.epicgames.com/docs/services/en-US/GameServices/Achievements/
