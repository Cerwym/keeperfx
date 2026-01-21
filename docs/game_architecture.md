# KeeperFX Game Architecture Documentation

This document provides a comprehensive overview of the KeeperFX game architecture, focusing on key systems and their implementation details.

---

## Table of Contents

1. [Overview](#overview)
2. [Possession Mode](#possession-mode)
   - 2.1 [Camera Modes](#camera-modes)
   - 2.2 [First-Person View](#first-person-view)
   - 2.3 [Key Files](#possession-key-files)
3. [Creature System](#creature-system)
   - 3.1 [Thing Structure](#thing-structure)
   - 3.2 [Creature Control](#creature-control)
   - 3.3 [Creature Instances](#creature-instances)
4. [Spell System](#spell-system)
   - 4.1 [Spell Types](#spell-types)
   - 4.2 [Spell Configuration](#spell-configuration)
   - 4.3 [Casted Spell Data](#casted-spell-data)
   - 4.4 [Spell Processing](#spell-processing)
5. [UI Rendering](#ui-rendering)
   - 5.1 [Spell Icon Rendering](#spell-icon-rendering)
   - 5.2 [In-Game Tabs and Menus](#in-game-tabs-and-menus)
   - 5.3 [Map Overlays](#map-overlays)
6. [Engine Systems](#engine-systems)
   - 6.1 [Rendering Pipeline](#rendering-pipeline)
   - 6.2 [Camera System](#camera-system)
7. [Code Locations Reference](#code-locations-reference)

---

## Overview

KeeperFX is structured around a central "Thing" system where almost everything in the game world is represented as a `Thing` object. The game uses a component-based approach where creatures have additional control structures attached to them, and different systems interact through well-defined interfaces.

**Key Concepts:**
- **Things**: Base objects for all game entities (creatures, objects, effects, etc.)
- **Creature Control**: Extended data structure for creature-specific state
- **View Modes**: Different camera perspectives (isometric, possession, etc.)
- **Spells/Powers**: Active effects that can be cast on creatures or tiles

---

## Possession Mode

Possession mode allows the player to directly control a creature from a first-person perspective, providing an immersive gameplay experience.

### Camera Modes

The game supports multiple camera/view modes defined in `player_data.h`:

```c
enum TbCameraMode {
    PVM_IsoWibbleView,      // Standard isometric view
    PVM_IsoStraightView,    // Isometric without wobble
    PVM_FrontView,          // Front facing view
    PVM_ParchmentView,      // Map/parchment view
    PVM_CreatureView,       // First-person possession view
    PVM_ParchFadeView,      // Transitional view
};
```

**Primary Mode**: `PVM_CreatureView` is the possession mode where players see from the creature's perspective.

### First-Person View

When in `PVM_CreatureView`:
- Camera is attached to the controlled creature
- UI shows creature-specific spells and abilities
- Active spell effects are displayed with icons at the bottom of the screen
- Player controls movement and actions directly

**Key Functions:**
- `set_engine_view(player, PVM_CreatureView)` - Activates possession mode
- Camera follows creature position and rotation
- Special rendering considerations for first-person visibility

### Possession Key Files

| File | Purpose |
|------|---------|
| `engine_camera.c` | Camera mode switching and positioning |
| `player_data.c` | Player view state management |
| `engine_redraw.c` | First-person UI rendering (spell icons, etc.) |
| `local_camera.c` | Camera interpolation and movement |
| `creature_states.c` | Creature behavior when possessed |

---

## Creature System

### Thing Structure

Every entity in the game is a `Thing` (defined in `thing_data.h`):

```c
struct Thing {
    ThingClass class_id;      // Type of thing (creature, object, etc.)
    ThingModel model;         // Specific model within class
    PlayerNumber owner;       // Owning player
    struct Coord3d mappos;    // Position in world
    // ... many other fields
};
```

**Thing Classes:**
- `TCls_Creature` - Living creatures
- `TCls_Object` - Items, spell books, gold
- `TCls_Effect` - Visual effects
- `TCls_Shot` - Projectiles
- `TCls_Trap` - Traps
- `TCls_Door` - Doors

### Creature Control

Creatures have additional state managed through `CreatureControl` (defined in `creature_control.h`):

```c
struct CreatureControl {
    CctrlIndex index;
    unsigned short creature_control_flags;
    unsigned char creature_state_flags;
    
    // Spell tracking
    struct CastedSpellData casted_spells[CREATURE_MAX_SPELLS_CASTED_AT];
    uint32_t spell_flags;           // Bitmask of active spell effects
    EffectOrEffElModel spell_aura;  // Visual effect ID
    
    // Combat
    struct Thing* combat_opponent;
    short distance_to_destination;
    
    // State
    CrtrStateId active_state;
    CrInstance active_instance_id;  // Current active skill/ability
    
    // Navigation
    struct Navigation navi;
    struct Ariadne arid;  // Pathfinding data
    
    // Other creature-specific data...
};
```

**Key Limits:**
- `CREATURE_MAX_SPELLS_CASTED_AT = 5` - Maximum simultaneous spells per creature

### Creature Instances

Creature abilities/skills are called "instances" (defined in `creature_instances.h`). These include:
- Combat attacks
- Special abilities
- Spell casting
- Movement abilities

---

## Spell System

### Spell Types

Spells in KeeperFX are categorized by their effects:

**Duration-Based Spells:**
- `FREEZE` - Immobilizes creature
- `SPEED` - Increases movement speed
- `SLOW` - Decreases movement speed
- `ARMOUR` - Increases defense
- `INVISIBILITY` - Makes creature invisible
- `DRAIN` - Drains health over time
- `FEAR` - Causes creature to flee

**Active Spells:**
- `HEAL` - Restores health
- `LIGHTNING` - Deals damage
- `REBOUND` - Reflects damage
- `TELEPORT` - Moves creature

**Ranged Spells:**
- `RANGED_HEAL` - Heal at distance
- `RANGED_SPEED` - Speed buff at distance
- `RANGED_ARMOUR` - Armor buff at distance
- `RANGED_REBOUND` - Rebound at distance

### Spell Configuration

Each spell has a configuration defining its properties (`magic_powers.h`):

```c
struct SpellConfig {
    SpellKind spkind;              // Spell type identifier
    long medsym_sprite_idx;        // Medium symbol sprite for UI
    uint32_t spell_flags;          // Effect flags (CSAfF_*)
    GameTurnDelta duration;        // Default duration
    // ... other configuration
};
```

**Spell Flags** (`CSAfF_*` prefix):
- `CSAfF_Invisibility` - Grants invisibility
- `CSAfF_Freeze` - Freezes target
- `CSAfF_Timebomb` - Has countdown timer
- `CSAfF_Speed` - Affects speed
- And many more...

### Casted Spell Data

Active spells on a creature are tracked in the `casted_spells` array:

```c
struct CastedSpellData {
    SpellKind spkind;           // Type of spell
    GameTurnDelta duration;     // Remaining duration in game turns
    CrtrExpLevel caster_level;  // Power level of spell
    PlayerNumber caster_owner;  // Who cast the spell
};
```

**Storage:**
- Located in `CreatureControl.casted_spells[5]`
- Empty slots have `spkind = 0`
- Duration counts down each game turn
- When duration reaches 0, spell is removed

### Spell Processing

**Key Functions in `thing_creature.c`:**

| Function | Purpose |
|----------|---------|
| `creature_add_spell_effect()` | Adds a new spell to a creature |
| `creature_remove_spell_effect()` | Removes a spell effect |
| `creature_affected_by_spell()` | Checks if spell is active |
| `process_thing_spell_effects()` | Updates all active spell effects |
| `creature_under_spell_effect()` | Checks for specific spell flags |

**Processing Flow:**
1. Spell is cast → `creature_add_spell_effect()`
2. Finds empty slot in `casted_spells[5]`
3. Fills in spell kind, duration, caster info
4. Each game turn → `process_thing_spell_effects()`
5. Decrements duration for each active spell
6. When duration = 0 → spell is removed
7. Spell effects are applied through flags and direct stat modifications

---

## UI Rendering

### Spell Icon Rendering

**Primary Function**: `draw_creature_view_icons()` in `engine_redraw.c`

This function is responsible for rendering spell icons in possession mode (first-person view).

**Rendering Process:**
```c
static void draw_creature_view_icons(struct Thing* creatng)
{
    // 1. Get creature control data
    struct CreatureControl *cctrl = creature_control_get_from_thing(creatng);
    
    // 2. Position calculation (bottom-left of screen)
    ScreenCoord x = menu_width + horizontal_offset;
    ScreenCoord y = MyScreenHeight - (sprite_height * 2);
    
    // 3. Iterate through active spells
    for each spell in casted_spells[5]:
        // Skip empty slots (spkind == 0)
        // Get spell configuration
        // Determine sprite to display
        
        // 4. Render count indicator (if multiple)
        if (spell_count > 1):
            render_text("x{count}")
        
        // 5. Render timebomb countdown (if applicable)
        else if (has_timebomb_flag):
            render_text(countdown_seconds)
        
        // 6. Draw spell icon
        draw_gui_panel_sprite_left(x, y, sprite_idx)
        
        // 7. Advance position for next icon
        x += sprite_width
}
```

**Visual Layout:**
```
Bottom of screen:
┌────────────────────────────────────────┐
│                                        │
│                                        │
│           [Game View]                  │
│                                        │
│                                        │
└────────────────────────────────────────┘
  [Menu]  ⚡x3  🛡️  💣5  [Other UI]
           ↑    ↑   ↑
        Speed  Armor Timebomb
         (3x)  (1x)  (5 sec)
```

**Text Rendering:**
- Position: Centered above spell icon
- Format: "x{N}" for counts, "{seconds}" for timebombs
- Style: Centered text with shadow for readability
- Font scaling: Adjusts based on screen resolution

### In-Game Tabs and Menus

**File**: `frontmenu_ingame_tabs.c`

Handles the spell selection UI and power management:

| Function | Purpose |
|----------|---------|
| `gui_choose_spell()` | Player selects a spell to cast |
| `maintain_spell()` | Updates spell button states |
| `maintain_big_spell()` | Updates main spell panel |
| `spell_lost_first_person()` | Handles spell loss in possession |

**Spell Menu Structure:**
- Two spell menus: `spell_menu` and `spell_menu2`
- Button arrays for different spell types
- Panel tabs for organization (16 main, 16 secondary)
- Click events trigger spell selection

### Map Overlays

**Files**: `frontmenu_ingame_map.c`, `gui_parchment.c`

These handle spell visualization on the minimap and parchment view:

**Functions:**
- `draw_overlay_spells_and_boxes()` - 3D view overlays (special boxes, spell books)
- `draw_overhead_spells()` - Parchment view spell markers
- Spell items blink at specific intervals for visibility
- Uses color coding for different spell/item types

---

## Engine Systems

### Rendering Pipeline

**Main File**: `engine_redraw.c`

The rendering system handles different view modes and UI overlays:

1. **Setup Phase**
   - `setup_engine_window()` - Establishes viewport
   - Camera positioning based on current view mode

2. **World Rendering**
   - 3D geometry (when applicable)
   - Things (creatures, objects, effects)
   - Map elements

3. **UI Overlay**
   - Spell icons (possession mode)
   - Health bars
   - Instance cooldowns
   - Dragged items
   - Mouse cursor

4. **Special Rendering**
   - `draw_spell_cursor()` - Shows spell casting cursor
   - Spell cost display
   - Visual effects and particles

### Camera System

**Files**: `engine_camera.c`, `local_camera.c`

The camera system manages view modes and smooth transitions:

**Camera Structure:**
```c
struct Camera {
    TbCameraMode view_mode;      // Current view mode
    struct Coord3d mappos;       // Camera position
    unsigned short orient_a;     // Rotation angle
    unsigned short rotation_angle_x;  // Additional rotation
    long zoom;                   // Zoom level
    // ... other fields
};
```

**Key Features:**
- Smooth interpolation between positions
- View mode transitions
- Collision detection in first-person
- Rotation and zoom controls
- Position clamping to map bounds

---

## Code Locations Reference

### Core Systems

| System | Primary Files | Key Structures |
|--------|---------------|----------------|
| **Things** | `thing_data.h`, `thing_creature.c` | `Thing`, `ThingClass` |
| **Creatures** | `creature_control.h`, `thing_creature.c` | `CreatureControl`, `CastedSpellData` |
| **Spells** | `magic_powers.h/c`, `thing_creature.c` | `SpellConfig`, `CastedSpellData` |
| **Camera** | `engine_camera.c`, `local_camera.c` | `Camera`, `TbCameraMode` |
| **Rendering** | `engine_redraw.c`, `engine_render.c` | Various rendering functions |

### Possession Mode

| Aspect | Files | Functions |
|--------|-------|-----------|
| **Mode Switching** | `player_data.c`, `engine_camera.c` | `set_engine_view()` |
| **UI Rendering** | `engine_redraw.c` | `draw_creature_view_icons()` |
| **Controls** | `front_input.c` | Input handling for possessed creatures |
| **State Management** | `creature_states.c` | Creature behavior when controlled |

### Spell System

| Component | Files | Key Elements |
|-----------|-------|--------------|
| **Configuration** | `config/magic.cfg`, `magic_powers.c` | Spell definitions |
| **Processing** | `thing_creature.c` | `process_thing_spell_effects()` |
| **Casting** | `magic_powers.c`, `player_instances.c` | Power activation |
| **UI** | `frontmenu_ingame_tabs.c`, `engine_redraw.c` | Spell buttons and icons |

### Constants and Limits

```c
// From creature_control.h
#define CREATURE_MAX_SPELLS_CASTED_AT 5  // Max active spells per creature

// From player_data.h
enum TbCameraMode {
    PVM_CreatureView = 4  // Possession mode value
};

// Spell flags (CSAfF_* prefix in config files)
// Used as bitmasks for creature spell effects
```

---

## Development Notes

### Adding New Spells

1. Define spell in `config/magic.cfg`
2. Add spell kind to enum in appropriate header
3. Implement spell effect in `thing_creature.c`
4. Add UI sprite and icon
5. Configure spell flags and behavior
6. Test in various scenarios (possessed, enemy cast, etc.)

### Modifying UI Rendering

- Spell icon rendering: `engine_redraw.c::draw_creature_view_icons()`
- Screen scaling: Use `scale_ui_value_lofi()` and similar functions
- Text rendering: `LbTextDrawResized()` with proper font setup
- Always test at different resolutions

### Performance Considerations

- Spell processing runs every game turn (typically 20 FPS)
- UI rendering runs at display refresh rate (higher)
- Minimize nested loops in spell processing
- Cache frequently accessed configurations
- Profile rendering in complex scenes (many creatures with spells)

---

## Version History

- **2026-01-21**: Initial documentation created
  - Covered possession mode, creature control, spell system, and UI rendering
  - Based on analysis for spell count indicator feature

---

## See Also

- `data_structure.md` - Overview of map and thing structures
- `build_instructions.txt` - Compilation and build process
- `coding_style_for_eclipse.xml` - Code style guidelines
- Source code comments and doxygen annotations

---

*This documentation is maintained as part of the KeeperFX project. For questions or updates, consult the development team.*
