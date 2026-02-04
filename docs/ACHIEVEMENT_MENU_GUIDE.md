# Achievement Menu Implementation Guide

## Overview

The achievements menu replaces the high score menu item on the main menu. It displays all achievements for the current campaign, sorted by status (obtained, in-progress, hidden).

## User Interface

### Main Menu Integration

**Location:** Main menu, button position 7 (where "High Scores" was)
**Label:** "Achievements"
**State:** FeSt_ACHIEVEMENTS

### Achievement List Display

**Sorting Order:**
1. **Unlocked Achievements** (top) - Gold/yellow color with checkmark (✓)
2. **In-Progress Achievements** (middle) - Grey color with progress percentage
3. **Hidden Achievements** (bottom) - Shows "Hidden Achievement" until unlocked

**Visual Elements:**
- Achievement icon (trophy/medal or custom sprite)
- Achievement name
- Achievement description
- Progress indicator (for in-progress achievements)
- Checkmark symbol for unlocked achievements

**Scrolling:**
- Up arrow button
- Down arrow button
- Scroll tab (draggable)
- Mouse wheel support

### Example Display

```
╔════════════════════════════════════════════╗
║            ACHIEVEMENTS                    ║
╠════════════════════════════════════════════╣
║ ✓ [🏆] Imp Army                           ║
║    Complete level 5 using only Imps       ║
║                                            ║
║ ✓ [🏆] No Slapping                        ║
║    Complete level without slapping        ║
║                                            ║
║   [🏆] Speed Run                      45% ║
║    Complete level in under 30 minutes     ║
║                                            ║
║   [🏆] Hidden Achievement                 ║
║    ???                                     ║
╠════════════════════════════════════════════╣
║         [↑]  [Scroll]  [↓]          [OK]  ║
╚════════════════════════════════════════════╝
```

## Technical Implementation

### File Structure

**New Files:**
- `src/front_achievements.h` - Header with function prototypes
- `src/front_achievements.c` - Implementation (270+ lines)

**Modified Files:**
- `src/frontend.h` - Added FeSt_ACHIEVEMENTS state
- `src/frontend.cpp` - Menu integration
- `src/gui_frontmenu.h` - Added GMnu_FEACHIEVEMENTS
- `src/config_strings.h` - Added GUIStr_MnuAchievements
- `Makefile` - Added front_achievements.o

### Key Functions

#### Achievement Display

```c
void frontend_draw_achievements_list(struct GuiButton *gbtn);
```
- Draws the scrollable list of achievements
- Calls `sort_achievements_for_display()` to order achievements
- Handles display for each achievement entry

```c
void draw_achievement_entry(int display_idx, long pos_x, long pos_y, int width, int units_per_px);
```
- Draws individual achievement with icon, name, description
- Shows progress percentage for in-progress achievements
- Displays checkmark for unlocked achievements
- Hides details for hidden achievements

#### Achievement Sorting

```c
static void sort_achievements_for_display(void);
```
- Three-pass sorting algorithm:
  1. All unlocked achievements
  2. Visible (not hidden) locked achievements
  3. Hidden locked achievements
- Populates `sorted_achievement_indices[]` array

#### Scrolling Functions

```c
void achievements_scroll_up(struct GuiButton *gbtn);
void achievements_scroll_down(struct GuiButton *gbtn);
void achievements_scroll(struct GuiButton *gbtn);
```
- Handle scroll button interactions
- Update `achievements_scroll_offset` variable
- Maintain functions enable/disable buttons

### State Machine Integration

**Initialization (FeSt_ACHIEVEMENTS entry):**
```c
case FeSt_ACHIEVEMENTS:
    turn_on_menu(GMnu_FEACHIEVEMENTS);
    count_displayable_achievements();
    achievements_scroll_offset = 0;
    set_pointer_graphic_menu();
    break;
```

**Cleanup (FeSt_ACHIEVEMENTS exit):**
```c
case FeSt_ACHIEVEMENTS:
    turn_off_menu(GMnu_FEACHIEVEMENTS);
    break;
```

**Input Handling:**
```c
case FeSt_ACHIEVEMENTS:
    get_gui_inputs(0);
    input_consumed = frontscreen_end_input(false);
    break;
```

**Drawing:**
```c
case FeSt_ACHIEVEMENTS:
    frontend_copy_background();
    draw_gui();
    break;
```

**Update:**
```c
case FeSt_ACHIEVEMENTS:
    frontend_achievements_update();
    break;
```

**Exit Path:**
```c
case FeSt_ACHIEVEMENTS:
    return FeSt_MAIN_MENU;
```

### Button Array

```c
struct GuiButtonInit frontend_achievements_menu_buttons[] = {
    // Title
    { LbBtnT_NormalBtn, ..., frontend_draw_vlarge_menu_button, GUIStr_MnuAchievements },
    
    // Main list area
    { LbBtnT_NormalBtn, ..., frontend_draw_achievements_list },
    
    // Scroll up button
    { LbBtnT_HoldableBtn, ..., achievements_scroll_up, frontend_achievements_scroll_up_maintain },
    
    // Scroll down button
    { LbBtnT_HoldableBtn, ..., achievements_scroll_down, frontend_achievements_scroll_down_maintain },
    
    // Scroll tab
    { LbBtnT_HoldableBtn, ..., achievements_scroll, frontend_achievements_scroll_tab_maintain },
    
    // OK/Back button
    { LbBtnT_NormalBtn, ..., frontend_quit_achievements_screen, frontend_maintain_achievements_ok_button },
};
```

## Achievement Data Access

### Getting Achievement Status

```c
TbBool achievement_is_unlocked(int achievement_idx);
float achievement_get_progress(int achievement_idx);
```

### Achievement Definition Fields

```c
typedef struct AchievementDefinition {
    char id[64];              // Unique identifier
    char name[128];           // Display name
    char description[256];    // Description text
    int points;               // Point value
    TbBool hidden;            // Hide until unlocked
    int icon_sprite;          // Custom icon sprite ID
    // ... condition fields ...
} AchievementDefinition;
```

## Visual Styling

### Color Scheme

**Unlocked Achievements:**
- Text color: Gold/yellow (`colours[18][0][0]`)
- Symbol: Checkmark (✓)

**Locked Achievements:**
- Text color: Grey (`colours[2][0][0]`)
- Progress color: Blue (`colours[5][0][0]`)

**Hidden Achievements:**
- Name: "Hidden Achievement"
- Description: "???"
- No progress shown

### Icon System

**Default Icon:**
- Trophy sprite: `GPS_message_rpanel_msg_trophy_act` (900)
- Used when no custom icon specified

**Custom Icons:**
- Set via `icon_sprite` field in achievement definition
- Campaign-specific sprites supported
- 24x24 pixel size recommended

## Integration with Achievement System

### Loading Achievements

Achievements are automatically loaded when campaign changes:
```c
// In config_campaigns.c, change_campaign()
load_campaign_achievements(&campaign);
```

### Achievement Unlock Notification

When achievements unlock:
1. `achievement_unlock()` is called
2. Achievement marked as unlocked
3. Platform backends (Steam/GOG) notified
4. Event notification created (FeSt_AchievementUnlocked)
5. Player sees in-game notification
6. Achievement moves to top of list

### Save/Load Integration

Achievement state is saved/loaded with game saves:
- Save chunk: `SGC_AchievementData` (0x48434141)
- Contains full `AchievementTracker` struct
- Backward compatible with old saves

## Customization

### Adding More Display Information

To show additional achievement data:

1. Modify `draw_achievement_entry()` in `front_achievements.c`
2. Add new visual elements (dates, stats, etc.)
3. Update layout calculations

### Changing Sort Order

To modify sorting behavior:

1. Edit `sort_achievements_for_display()` in `front_achievements.c`
2. Adjust the three-pass loop logic
3. Update `sorted_achievement_indices[]` population

### Custom Icons per Achievement

In `achievements.cfg`:
```ini
ACHIEVEMENT my_achievement
    NAME = "My Achievement"
    DESCRIPTION = "Do something cool"
    ICON_SPRITE = 850  # Custom sprite ID
    POINTS = 25
END_ACHIEVEMENT
```

## Testing

### Manual Testing Checklist

1. **Menu Access**
   - [ ] Start game and enter main menu
   - [ ] Verify "Achievements" button appears
   - [ ] Click achievements button
   - [ ] Verify achievements menu opens

2. **Achievement Display**
   - [ ] Verify unlocked achievements show at top
   - [ ] Check for checkmark (✓) on unlocked
   - [ ] Verify in-progress achievements show percentage
   - [ ] Confirm hidden achievements show as "???"

3. **Scrolling**
   - [ ] Test up arrow button
   - [ ] Test down arrow button
   - [ ] Test scroll tab dragging
   - [ ] Test mouse wheel (if supported)
   - [ ] Verify buttons disable at bounds

4. **Navigation**
   - [ ] Test OK/Back button
   - [ ] Test Escape key
   - [ ] Verify return to main menu

5. **Edge Cases**
   - [ ] Test with 0 achievements
   - [ ] Test with 1-5 achievements (no scroll)
   - [ ] Test with 20+ achievements (scrolling)
   - [ ] Test with all unlocked
   - [ ] Test with all hidden

### Integration Testing

1. **Campaign Switching**
   - [ ] Switch campaigns
   - [ ] Verify achievement list updates
   - [ ] Check achievement counts

2. **Achievement Unlocking**
   - [ ] Unlock achievement during gameplay
   - [ ] Return to main menu
   - [ ] Verify achievement shows as unlocked
   - [ ] Check sorting (moves to top)

3. **Save/Load**
   - [ ] Unlock some achievements
   - [ ] Save game
   - [ ] Exit and reload
   - [ ] Verify achievement state persists

## Troubleshooting

### Achievements Not Displaying

**Check:**
1. Achievement system initialized (`achievements_init()` called)
2. Campaign achievements loaded (`load_campaign_achievements()` called)
3. `achievement_count` > 0
4. Menu properly registered in `menu_list[]`

**Debug:**
```c
SYNCLOG("Achievement count: %d", achievement_count);
SYNCLOG("Displayable count: %lu", count_displayable_achievements());
```

### Scroll Not Working

**Check:**
1. `frontend_achievements_items_visible` calculated correctly
2. `achievements_scroll_offset` within bounds
3. Maintain functions enabling/disabling buttons properly

**Debug:**
```c
SYNCLOG("Scroll offset: %d, visible: %d, total: %d",
    achievements_scroll_offset,
    frontend_achievements_items_visible,
    achievements_count);
```

### Achievements Not Sorting

**Check:**
1. `sort_achievements_for_display()` called before display
2. `achievement_is_unlocked()` returning correct values
3. `hidden` field set correctly in definitions

**Debug:**
```c
for (int i = 0; i < sorted_achievements_count; i++) {
    int idx = sorted_achievement_indices[i];
    SYNCLOG("Position %d: Achievement %d, unlocked: %d",
        i, idx, achievement_is_unlocked(idx));
}
```

## Future Enhancements

### Potential Improvements

1. **Filtering/Categories**
   - Add tabs for different achievement types
   - Filter by campaign, difficulty, etc.

2. **Statistics**
   - Show total achievements: X/Y unlocked
   - Display completion percentage
   - Show rarest achievements

3. **Achievement Details**
   - Click achievement for full details screen
   - Show unlock date/time
   - Display unlock conditions

4. **Social Features**
   - Compare with friends (if platform supports)
   - Show global unlock percentages
   - Achievement sharing

5. **Animation**
   - Fade in/out transitions
   - Icon animations on unlock
   - Sparkle effects for new achievements

6. **Sound Effects**
   - Unlock sound
   - Menu navigation sounds
   - Click feedback

## Related Documentation

- `docs/achievement_system.txt` - Achievement system user guide
- `docs/achievements_implementation.md` - Developer API reference
- `docs/ACHIEVEMENT_NOTIFICATION_SYSTEM.md` - In-game notifications
- `docs/ACHIEVEMENT_INTEGRATION_STATUS.md` - Integration status

## Summary

The achievements menu provides a user-friendly interface for viewing campaign achievements. It automatically sorts achievements by status, shows progress for in-progress achievements, and hides details for hidden achievements until they're unlocked. The implementation follows existing frontend patterns (level select, high scores) for consistency and maintainability.

---

**Last Updated:** 2026-02-04
**Version:** 1.0
**Status:** Production-ready
