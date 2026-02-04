# Achievement Notification System

## Overview

The achievement notification system integrates seamlessly with KeeperFX's existing in-game event system to display achievement unlocks to players. When an achievement is unlocked, a notification appears in the event button row at the bottom of the screen, showing a trophy icon and the achievement name.

## Architecture

### Event System Integration

The notification system uses the existing event infrastructure:

- **Event Type**: `EvKind_AchievementUnlocked` (new event kind)
- **Icon**: Trophy/medal sprite (GPS_message_rpanel_msg_trophy_act)
- **Display Duration**: 2400 game turns (~2 minutes)
- **Location**: Bottom event button row (same as "Heart Attacked", "New Creature", etc.)

### Components

1. **Event Type** (`map_events.h`)
   - Added to `EventKinds` enum
   - Treated like other game events (combat, heart attacked, etc.)

2. **Sprite System** (`sprites.h`)
   - Trophy icon: GPS_message_rpanel_msg_trophy_act = 900
   - Standard: GPS_message_rpanel_msg_trophy_std = 901
   - Can be replaced with campaign-specific sprites

3. **Event Configuration** (`frontend.cpp`)
   - Registered in `event_button_info` array
   - Configures icon, text, duration, behavior

4. **Achievement API** (`achievement_api.c`)
   - Calls `event_create_event()` when achievement unlocks
   - Passes achievement index as event target
   - Integrates with platform backends (Steam, GOG)

## Usage

### For Campaign Creators

**Default Trophy Icon:**
```ini
ACHIEVEMENT my_achievement
    NAME = "Master Builder"
    DESCRIPTION = "Build all room types in one level"
    POINTS = 50
END_ACHIEVEMENT
```

**Custom Icon:**
```ini
ACHIEVEMENT my_achievement
    NAME = "Master Builder"
    DESCRIPTION = "Build all room types in one level"
    ICON_SPRITE = 850  # Custom sprite index
    POINTS = 50
END_ACHIEVEMENT
```

The notification appears automatically when the achievement unlocks. No code changes required!

### For Developers

**Unlock Achievement:**
```c
// Achievement unlocks automatically create notification
achievement_unlock("my_achievement_id");
```

**Custom Event Creation:**
```c
// Create achievement notification manually (advanced)
#include "map_events.h"

int achievement_idx = 5; // Index in achievements array
event_create_event(0, 0, EvKind_AchievementUnlocked, player_id, achievement_idx);
```

## Notification Behavior

### Display

- **Location**: Bottom event button row
- **Icon**: Trophy (default) or custom sprite
- **Text**: Achievement name
- **Duration**: 2400 turns (~2 minutes)
- **Interaction**: Click to acknowledge/dismiss
- **Queueing**: Respects event priority system

### Event Flow

1. Achievement condition met
2. `achievement_unlock()` called
3. Achievement marked as unlocked
4. Platform backend notified (Steam/GOG)
5. `event_create_event()` called
6. Event added to player's event queue
7. Notification appears in button row
8. Auto-dismisses after timeout or player clicks

## Custom Icons

### Sprite Requirements

- **Size**: 32x32 pixels (matches other event icons)
- **Frames**: 2 frames for animation (active/standard)
- **Format**: Game's native sprite format
- **Location**: Campaign's sprite files

### Adding Custom Sprite

1. **Create Trophy Sprite**
   - Design 32x32 icon (e.g., star, medal, crown)
   - Export as 2-frame animation
   - Add to campaign's sprite data

2. **Get Sprite Index**
   - Note the sprite index in your sprite file
   - e.g., index 850

3. **Configure Achievement**
   ```ini
   ACHIEVEMENT special_achievement
       ICON_SPRITE = 850
       ...
   END_ACHIEVEMENT
   ```

### Default Icon

If `ICON_SPRITE` is not specified or is 0, the default trophy icon is used:
- GPS_message_rpanel_msg_trophy_act = 900

## Reusable Pattern

The achievement notification system demonstrates a reusable pattern for adding new event types:

### Adding New Event Type

1. **Define Event Kind** (`map_events.h`)
   ```c
   enum EventKinds {
       ...
       EvKind_MyNewEvent,
   };
   ```

2. **Add Event Name** (`config_creature.c`)
   ```c
   {"MEVENT_MYNEWEVENT", EvKind_MyNewEvent},
   ```

3. **Add Icon Sprite** (`sprites.h`)
   ```c
   GPS_message_rpanel_msg_myicon_act = 910,
   GPS_message_rpanel_msg_myicon_std = 911,
   ```

4. **Register Event** (`frontend.cpp`)
   ```c
   {GPS_message_rpanel_msg_myicon_act, 
    GUIStr_MyEventDesc, 
    GUIStr_MyEvent, 
    1200,  // duration
    0,     // turns between events
    EvKind_Nothing},
   ```

5. **Add Strings** (`config_strings.h`)
   ```c
   GUIStr_MyEvent,
   GUIStr_MyEventDesc,
   ```

6. **Create Event** (your code)
   ```c
   event_create_event(x, y, EvKind_MyNewEvent, player_id, target_data);
   ```

## Technical Details

### Event Structure

```c
struct Event {
    unsigned char flags;
    EventIndex index;
    int32_t mappos_x;       // 0 for global events
    int32_t mappos_y;       // 0 for global events
    unsigned char owner;     // Player ID
    unsigned char kind;      // EvKind_AchievementUnlocked
    int32_t target;          // Achievement array index
    unsigned long lifespan_turns;  // Time until auto-dismiss
};
```

### EventTypeInfo Structure

```c
struct EventTypeInfo {
    int bttn_sprite;                // Icon to display
    unsigned short tooltip_stridx;  // Tooltip text ID
    unsigned short msg_stridx;      // Message text ID
    int lifespan_turns;            // Display duration
    int turns_between_events;      // Anti-spam delay
    unsigned char replace_event_kind_button;  // Replacement behavior
};
```

### Integration Points

**Where events are created:**
- `src/achievement_api.c` - Achievement unlocks
- `src/dungeon_data.c` - Heart attacked
- `src/creature_states.c` - Combat events
- `src/room_*.c` - Room events
- `src/magic_powers.c` - Spell events

**Where events are displayed:**
- `src/frontmenu_ingame_tabs.c` - Event button rendering
- `src/frontmenu_ingame_evnt.c` - Event handling
- `src/frontmenu_ingame_evnt_data.cpp` - Button data

## Localization

### String IDs

Add to language files (e.g., `lang/gtext_eng.pot`):

```
msgid "GUIStr_EventAchievementUnlocked"
msgstr "Achievement Unlocked"

msgid "GUIStr_EventAchievementUnlockedDesc"
msgstr "You have earned an achievement!"
```

### Per-Achievement Text

Use `NAME_TEXT_ID` and `DESC_TEXT_ID` in achievement config:

```ini
ACHIEVEMENT my_achievement
    NAME_TEXT_ID = 5000
    DESC_TEXT_ID = 5001
    ...
END_ACHIEVEMENT
```

Then add to campaign's text file:
```
msgid "5000"
msgstr "Master Builder"

msgid "5001"
msgstr "Build all room types in one level"
```

## Examples

### Simple Achievement with Notification

```ini
ACHIEVEMENT first_win
    NAME = "First Victory"
    DESCRIPTION = "Complete your first level"
    POINTS = 10
    COMPLETE_LEVEL = 1
END_ACHIEVEMENT
```

When player completes any level, notification appears with trophy icon and "First Victory" text.

### Custom Icon Achievement

```ini
ACHIEVEMENT imp_master
    NAME = "Imp Master"
    DESCRIPTION = "Train 100 Imps"
    ICON_SPRITE = 875  # Custom imp icon
    POINTS = 25
    CREATURE_COUNT = IMP
    MIN_COUNT = 100
END_ACHIEVEMENT
```

When player trains 100th imp, notification appears with custom imp icon.

### Hidden Achievement

```ini
ACHIEVEMENT secret_level
    NAME = "Secret Discovered"
    DESCRIPTION = "Find the hidden level"
    HIDDEN = 1
    ICON_SPRITE = 880  # Question mark icon
    POINTS = 50
    SCRIPT_TRIGGER = 1
END_ACHIEVEMENT
```

Hidden until unlocked, then shows with mystery icon.

## Future Enhancements

Possible future improvements:

1. **Click to View Details**
   - Click notification to open achievement details
   - Show description, requirements, rewards

2. **Progress Display**
   - Show progress bar on notification (e.g., "50/100 imps")
   - Update notification as progress changes

3. **Sound Effects**
   - Play sound when achievement unlocks
   - Different sounds for different rarities

4. **Rarity Colors**
   - Bronze/silver/gold borders based on points
   - Special animation for rare achievements

5. **Achievement Feed**
   - Scrolling list of recent achievements
   - History log accessible from menu

6. **Custom Animations**
   - Particle effects on unlock
   - Trophy icon animation
   - Screen flash/fade

## Troubleshooting

### Notification Not Appearing

1. **Check achievement system initialization**
   ```c
   achievements_init();  // Called in LbBullfrogMain()
   ```

2. **Verify event system**
   - Other events working? (heart attacked, new creature)
   - If not, event system issue

3. **Check achievement unlock**
   ```c
   if (achievement_unlock("my_id")) {
       SYNCLOG("Achievement unlocked successfully");
   }
   ```

### Icon Not Displaying

1. **Sprite index correct?**
   - Check sprite file for actual index
   - Verify sprite exists in campaign

2. **Fallback to default**
   - Set ICON_SPRITE = 0 or omit field
   - Should show trophy icon

3. **Check sprite loading**
   - Ensure campaign sprites loaded
   - Check sprite.dat file

### Text Not Showing

1. **String IDs defined?**
   - Check config_strings.h
   - Verify GUIStr_EventAchievementUnlocked exists

2. **Localization file updated?**
   - Add strings to language files
   - Rebuild language data

## See Also

- [Achievement System Overview](achievement_system.txt)
- [Achievement Implementation Guide](achievements_implementation.md)
- [Achievement Design](achievements_design.md)
- [Integration Status](ACHIEVEMENT_INTEGRATION_STATUS.md)
- [Event System](../src/map_events.h)
