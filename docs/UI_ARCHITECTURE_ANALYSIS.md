# KeeperFX UI Architecture Analysis

**Created:** January 20, 2026  
**Purpose:** Comprehensive analysis of current UI systems and roadmap for unified architecture

---

## Executive Summary

KeeperFX has **two distinct UI systems** that evolved separately:
1. **Frontend Menu System** - Main menu, campaign selection, load/save screens (pre-game)
2. **In-Game UI System** - Status panel, room/spell/creature tabs, query menus (during gameplay)

These systems share some base components (button/menu structures from `bflib_guibtns`) but have different rendering pipelines, font systems, state management, and resource handling. This document analyzes both systems and proposes a unified architecture.

---

## 1. Current Architecture Overview

### System Comparison Matrix

| Aspect | Frontend Menu System | In-Game UI System |
|--------|---------------------|-------------------|
| **Primary Files** | `frontend.cpp/h`, `frontmenu_*.c/h` | `gui_*.c/h`, `frontmenu_ingame_*.c/h` |
| **State Management** | `FrontendMenuState` enum (35+ states) | Game operation flags + menu stack |
| **Rendering Context** | Frontend loop (main menu) | Gameplay rendering loop |
| **Font System** | `frontend_font[4]` (dedicated fonts) | In-game text rendering via bflib |
| **Sprite System** | `frontend_sprite` (frontbit.dat/tab) | `gui_panel_sprites` (guiicons.dat/tab) |
| **Background** | Raw bitmap (`front.raw`, 640×480) | 3D rendered game world |
| **Button Count** | ~86 shared `active_buttons[]` | Same shared array |
| **Menu Stack** | 8 active menus via `menu_stack[]` | Same stack, different menus |

### Key Architectural Decision Points

**✓ What Works:**
- Shared `GuiButton` and `GuiMenu` structures (defined in `bflib_guibtns.h`)
- Common button lifecycle (create_button, maintain callbacks, draw callbacks)
- Single `active_buttons[86]` and `active_menus[8]` arrays
- Menu ID enumeration system (`GMnu_*` constants)

**✗ Major Divergences:**
- **Rendering Pipeline:** Frontend uses `frontend_draw()` → `draw_gui()`, in-game uses `redraw_isometric_view()` → `draw_gui()`
- **Input Handling:** `frontend_input()` vs. in-game packet system + `get_player_gui_clicks()`
- **Resource Loading:** `frontend_load_data()` vs. game startup sprite loading
- **Font Rendering:** Frontend uses custom `frontend_font_*` functions, in-game uses standard `LbTextSetFont()`

---

## 2. Frontend Menu System Analysis

### File Structure

**Core Implementation:**
- `frontend.cpp/h` (3926 lines) - Main frontend state machine and button handlers
- `frontend_font[4]` font array, sprite sheet management
- State machine with 35+ `FrontendMenuState` values

**Menu-Specific Modules:**
- `frontmenu_options.c/h` - Graphics, audio, gameplay options
- `frontmenu_options_data.cpp` - Button initialization data
- `frontmenu_select.c/h` - Level/campaign selection
- `frontmenu_select_data.cpp` - Selection screen button layouts
- `frontmenu_net.c/h` - Multiplayer lobby and session management
- `frontmenu_net_data.cpp` - Network menu configurations
- `frontmenu_saves.c/h` - Save/load game menus
- `frontmenu_saves_data.cpp` - Save slot button definitions
- `frontmenu_specials.c/h` - Credits, high scores, statistics

### State Machine Architecture

```cpp
// From frontend.h:28-89
enum FrontendMenuStates {
    FeSt_INITIAL = 0,
    FeSt_MAIN_MENU,
    FeSt_FELOAD_GAME,
    FeSt_LAND_VIEW,           // Campaign map selection
    FeSt_NET_SERVICE,         // Network type selection
    FeSt_NET_SESSION,         // Session browser
    FeSt_NET_START,           // Multiplayer lobby
    FeSt_START_KPRLEVEL,
    FeSt_START_MPLEVEL,
    FeSt_QUIT_GAME,
    FeSt_LOAD_GAME,           // 10
    FeSt_INTRO,               // Video playback
    FeSt_STORY_POEM,
    FeSt_CREDITS,
    FeSt_DEMO,
    FeSt_LEVEL_STATS,
    FeSt_HIGH_SCORES,
    FeSt_TORTURE,
    FeSt_OUTRO,
    FeSt_NETLAND_VIEW,
    FeSt_PACKET_DEMO,         // 25
    FeSt_FEDEFINE_KEYS,
    FeSt_FEOPTIONS,
    FeSt_LEVEL_SELECT,        // 30
    FeSt_CAMPAIGN_SELECT,
    FeSt_DRAG,
    FeSt_CAMPAIGN_INTRO,
    FeSt_MAPPACK_SELECT,
    // 9+ more states...
};
```

**State Transitions:** [frontend.cpp:2951-2966]
```cpp
FrontendMenuState frontend_set_state(FrontendMenuState nstate)
{
    frontend_shutdown_state(frontend_menu_state);  // Cleanup old state
    if (frontend_menu_state)
        fade_out();
    fade_palette_in = 1;
    frontend_menu_state = frontend_setup_state(nstate);  // Initialize new state
    return frontend_menu_state;
}
```

### Button System

**Button Initialization Pattern:**
All frontend menus use `struct GuiButtonInit` arrays defined in `*_data.cpp` files:

```cpp
// Example from frontend.cpp:110-119
struct GuiButtonInit frontend_main_menu_buttons[] = {
    // gbtype, id_num, unused, flags, click_event, rclick_event, ptover_event,
    // btype_value, scr_pos_x, scr_pos_y, pos_x, pos_y, width, height,
    // draw_call, sprite_idx, tooltip_stridx, parent_menu, content, maxval, maintain_call
    { LbBtnT_NormalBtn, BID_MENU_TITLE, 0, 0, NULL, NULL, NULL,
      0, 999, 26, 999, 26, 371, 46, frontend_draw_large_menu_button,
      0, GUIStr_Empty, 0, {1}, 0, NULL },
    { LbBtnT_NormalBtn, BID_DEFAULT, 0, 0, frontend_start_new_game, NULL, frontend_over_button,
      3, 999, 92, 999, 92, 371, 46, frontend_draw_large_menu_button,
      0, GUIStr_Empty, 0, {2}, 0, NULL },
    // ... more buttons
    {-1, BID_DEFAULT, 0, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL, 0, 0, 0, {0}, 0, NULL}
};
```

**Special Position Constants:** [gui_draw.h:32-38]
- `POS_AUTO = -9999` - Automatically calculated
- `POS_MOUSMID = -999` - Follow mouse
- `POS_SCRCTR = -997` - Screen center
- `POS_GAMECTR = 999` - Game/menu center
- `POS_SCRBTM = -996` - Screen bottom

### Font System

**Frontend-Specific Fonts:** [frontend.cpp:422, frontend.h:282]
```cpp
struct TbSpriteSheet * frontend_font[FRONTEND_FONTS_COUNT] = {};  // 4 fonts
```

**Font Operations:**
- `frontend_font_char_width(int fnt_idx, char c)` - [frontend.cpp:462-468]
- `frontend_font_string_width(int fnt_idx, const char *str)` - [frontend.cpp:470-475]
- Font rendering via `LbTextSetFont(frontend_font[fnt_idx])` then standard text draw

**Font Usage by Index:**
- Font 0: Standard menu text
- Font 1: Active/highlighted buttons
- Font 2-3: Special purposes (credits, stats)

### Rendering Pipeline

**Main Loop:** [main.cpp:3776-3843 wait_at_frontend()]
```cpp
do {
    LbWindowsControl();
    update_mouse();
    frontend_input();         // Process input
    frontend_update(&finish_menu);  // Update state
    if (LbIsActive()) {
        frontend_draw();      // Draw frame
        LbScreenSwap();      // Present to screen
    }
    process_3d_sounds();
    MonitorStreamedSoundTrack();
    LbSleepUntil(fe_last_loop_time + 30);  // 30ms frame time
} while (!finish_menu);
```

**Draw Function:** [frontend.cpp:3405-3470]
```cpp
short frontend_draw(void)
{
    switch (frontend_menu_state) {
        case FeSt_MAIN_MENU:
        case FeSt_FELOAD_GAME:
        case FeSt_NET_SERVICE:
        case FeSt_NET_SESSION:
        case FeSt_NET_START:
        case FeSt_LEVEL_STATS:
        case FeSt_HIGH_SCORES:
            frontend_copy_background();  // Draw front.raw background
            draw_gui();                  // Draw menu buttons
            break;
        case FeSt_LAND_VIEW:
            frontmap_draw();            // Campaign map view
            break;
        case FeSt_INTRO:
            intro();                    // Video playback
            return 0;
        // ... 20+ more cases
    }
    draw_debug_messages();
    return 1;
}
```

**Button Drawing:** [frontend.cpp:3240-3290]
```cpp
void draw_active_menus_buttons(void)
{
    for (k=0; k < no_of_active_menus; k++) {
        menu_num = menu_id_to_number(menu_stack[k]);
        gmnu = &active_menus[menu_num];
        if ((gmnu->visual_state != 0) && (gmnu->is_turned_on)) {
            if (gmnu->draw_cb != NULL)
                gmnu->draw_cb(gmnu);          // Menu-level draw callback
            if (gmnu->visual_state == 2)
                draw_menu_buttons(gmnu);       // Individual button draws
        }
    }
}
```

### Resource Loading

**Frontend Data Loading:** [frontend.cpp:945-980]
```cpp
TbResult frontend_load_data(void)
{
    free_spritesheet(&frontend_sprite);
    
    // Load background (640x480 raw bitmap)
    frontend_background = (unsigned char *)game.map;  // Reuse game.map memory!
    fname = prepare_file_path(FGrp_LoData, "front.raw");
    len = LbFileLoadAt(fname, frontend_background);
    
    // Load frontend sprites
    strcpy(dat_fname, prepare_file_path(FGrp_LoData, "frontbit.dat"));
    strcpy(tab_fname, prepare_file_path(FGrp_LoData, "frontbit.tab"));
    frontend_sprite = load_spritesheet(dat_fname, tab_fname);
    
    return ret;
}
```

**Key Insight:** Frontend reuses `game.map` memory for background bitmap to save RAM!

### Drawing Functions

**Standard Frontend Button Styles:**
- `frontend_draw_large_menu_button()` - Main menu buttons (371×46)
- `frontend_draw_small_menu_button()` - Sub-menu buttons (247×46)
- `frontend_draw_vlarge_menu_button()` - Extra large (495×46)
- `frontend_draw_slider()` - Horizontal sliders
- `frontend_draw_text()` - Text-only buttons
- `frontend_draw_icon()` - Icon buttons

**Sprite Access:** [frontend.cpp:1288]
```cpp
const struct TbSprite *spr = get_frontend_sprite(gbtn->sprite_idx);
```

### Input Handling

**Frontend Input:** [frontend.cpp:3047-3080 frontend_input()]
- Direct mouse/keyboard polling via bflib
- Modal dialog support (error boxes, message boxes)
- Network message handling in multiplayer states
- State-specific input functions (e.g., `frontmap_input()`, `frontstats_input()`)

---

## 3. In-Game UI System Analysis

### File Structure

**Core In-Game UI Files:**
- `gui_draw.c/h` - Core drawing primitives (slabs, bars, sprites)
- `gui_frontmenu.c/h` - Menu stack management, shared with frontend
- `gui_frontbtns.c/h` - Button activation, input handling
- `gui_boxmenu.c/h` - Box menu system (alternative UI element)
- `gui_tooltips.c/h` - Tooltip rendering and management
- `gui_topmsg.c/h` - Top-of-screen messages
- `gui_msgs.c/h` - Game message system
- `gui_parchment.c/h` - Parchment spell/power UI

**In-Game Tab System:**
- `frontmenu_ingame_tabs.c/h` (2880 lines!) - Main status panel, tabs (Room/Spell/Trap/Creature)
- `frontmenu_ingame_tabs_data.cpp` - Button/menu definitions for tabs
- `frontmenu_ingame_opts.c/h` - In-game pause menu, options
- `frontmenu_ingame_opts_data.cpp` - Pause menu button layouts
- `frontmenu_ingame_map.c/h` - Minimap rendering
- `frontmenu_ingame_evnt.c/h` - Event buttons (battle notifications)

### Panel Architecture

**Status Panel Structure:** [frontmenu_ingame_tabs_data.cpp:462-477]
```cpp
struct TiledSprite status_panel = {
    2, 4, {
        { 1, 2},   // Top-left, top-right sprite indices
        { 3, 4},   // 2nd row
        { 5, 6},   // 3rd row
        {21,22},   // Bottom row
    },
};
```

**Tab Menu System:**
```cpp
struct GuiMenu main_menu;           // GMnu_MAIN - Main status display
struct GuiMenu room_menu;           // GMnu_ROOM - Room building panel
struct GuiMenu spell_menu;          // GMnu_SPELL - Spell casting panel
struct GuiMenu trap_menu;           // GMnu_TRAP - Trap placement
struct GuiMenu creature_menu;       // GMnu_CREATURE - Creature management
struct GuiMenu event_menu;          // GMnu_EVENT - Battle notifications
struct GuiMenu query_menu;          // GMnu_QUERY - Thing inspection
struct GuiMenu creature_query_menu1-4;  // Detailed creature stats (4 pages)
```

**Tab Buttons:** [frontmenu_ingame_tabs_data.cpp:141-150]
```cpp
// Radio buttons that switch between tabs
{ LbBtnT_RadioBtn, BID_INFO_TAB,   0, 0, gui_set_menu_mode, NULL, NULL,
  GMnu_MAIN,   0, 154,  0, 154, 28, 34, gui_draw_tab,
  GPS_rpanel_rpanel_tab_infoa, GUIStr_InfoPanelDesc, 0, {.ptr = &info_tag}, 0, menu_tab_maintain },
{ LbBtnT_RadioBtn, BID_ROOM_TAB,   0, 0, gui_set_menu_mode, NULL, NULL,
  GMnu_ROOM,  28, 154, 28, 154, 28, 34, gui_draw_tab,
  GPS_rpanel_rpanel_tab_rooma, GUIStr_RoomPanelDesc, 0, {.ptr = &room_tag}, 0, menu_tab_maintain },
{ LbBtnT_RadioBtn, BID_SPELL_TAB,  0, 0, gui_set_menu_mode, NULL, NULL,
  GMnu_SPELL, 56, 154, 56, 154, 28, 34, gui_draw_tab,
  GPS_rpanel_rpanel_tab_spela, GUIStr_ResearchPanelDesc, 0, {.ptr = &spell_tag}, 0, menu_tab_maintain },
// ... MNFCT_TAB, CREATR_TAB
```

### Dynamic Button Configuration

**Unlike frontend's static button arrays, in-game UI generates buttons dynamically:**

**Room Tab Configuration:** [frontmenu_ingame_tabs.c:2598-2660]
```cpp
void update_room_tab_to_config(void)
{
    // Clear button grid
    for (i=0; i < 15; i++) {
        ibtn = &room_menu.buttons[i];
        ibtn->sprite_idx = 24;
        ibtn->tooltip_stridx = GUIStr_Empty;
        ibtn->content.lval = RoK_NONE;
        ibtn->click_event = NULL;
        ibtn->draw_call = gui_area_new_null_button;
    }
    
    // Populate from config
    for (i=0; i < game.conf.slab_conf.room_types_count; i++) {
        struct RoomConfigStats* roomst = &game.conf.slab_conf.room_cfgstats[i];
        if (roomst->panel_tab_idx < 1) continue;
        
        if (roomst->panel_tab_idx <= 16) {
            ibtn = &room_menu.buttons[roomst->panel_tab_idx - 1];
        } else {
            ibtn = &room_menu2.buttons[roomst->panel_tab_idx - 17];
        }
        
        ibtn->sprite_idx = roomst->medsym_sprite_idx;
        ibtn->tooltip_stridx = roomst->tooltip_stridx;
        ibtn->content.lval = i;
        ibtn->click_event = gui_choose_room;
        ibtn->rclick_event = gui_go_to_next_room;
        ibtn->ptover_event = gui_over_room_button;
        ibtn->draw_call = gui_area_room_button;
    }
}
```

**Same pattern for:**
- `update_trap_tab_to_config()` - [frontmenu_ingame_tabs.c:2702]
- `update_powers_tab_to_config()` - [frontmenu_ingame_tabs.c:2751]

### In-Game Rendering Pipeline

**Main Gameplay Render:** [engine_redraw.c:718-748 redraw_isometric_view()]
```cpp
void redraw_isometric_view(void)
{
    struct PlayerInfo* player = get_my_player();
    struct Dungeon* dungeon = get_players_num_dungeon(my_player_number);
    struct Camera* render_cam = get_local_camera(&player->cameras[CamIV_Isometric]);
    
    make_camera_deviations(player, dungeon);
    update_explored_flags_for_power_sight(player);
    
    engine(player, render_cam);  // 3D world rendering
    
    if (smooth_on) {
        smooth_screen_area(lbDisplay.WScreen, ...);
    }
    
    remove_explored_flags_for_power_sight(player);
    
    if ((game.operation_flags & GOF_ShowGui) != 0) {
        draw_whole_status_panel();  // Panel background
    }
    
    draw_gui();                 // Buttons and menus
    
    if ((game.operation_flags & GOF_ShowGui) != 0) {
        draw_overlay_compass(...);  // Minimap compass
    }
    
    message_draw();             // Game messages
    gui_draw_all_boxes();       // Dialog boxes
    draw_power_hand();          // Keeper hand cursor
    draw_tooltip();             // Hover tooltips
}
```

**Status Panel Drawing:** [frontmenu_ingame_tabs.c:233-305]
```cpp
void draw_whole_status_panel(void)
{
    int base_width = 140;
    int base_height = 400;
    
    // Calculate position (typically bottom-right corner)
    gmnu_x = (lbDisplay.PhysicalScreenWidth - base_width * units_per_pixel) / pixel_size;
    gmnu_y = (lbDisplay.PhysicalScreenHeight - base_height * units_per_pixel) / pixel_size;
    
    // Draw tiled panel background
    for (int y = 0; y < status_panel.rows; y++) {
        for (int x = 0; x < status_panel.columns; x++) {
            int spr_idx = status_panel.sprites[y][x];
            draw_gui_panel_sprite_left(x_pos, y_pos, ps_units_per_px, spr_idx);
        }
    }
    
    // Draw minimap
    if (player->minimap_zoom != 0) {
        panel_map_draw_slabs(...);
        panel_map_draw_overlay_things(...);
    }
}
```

### Sprite System

**Panel Sprites:** [gui_draw.h:49, gui_draw.c:48]
```cpp
struct TbSpriteSheet * gui_panel_sprites;  // Loaded from guiicons-0.dat/tab

// Sprite enumeration in sprites.h:217+
enum GuiPanelSprites {
    GPS_rpanel_rpanel_frame_portrt_empty = 0,
    GPS_rpanel_frame_portrt_light = 1,
    GPS_rpanel_rpanel_tab_infoa = 8,
    GPS_rpanel_rpanel_tab_rooma = 12,
    GPS_rpanel_rpanel_tab_spela = 16,
    GPS_room_treasury_std_s = 440,
    GPS_room_lair_std_s = 441,
    // ... 514 panel sprites total
};
```

**Drawing Functions:**
- `draw_gui_panel_sprite_left(x, y, units_per_px, spridx)` - Left-aligned panel sprite
- `draw_gui_panel_sprite_centered(x, y, units_per_px, spridx)` - Centered sprite
- `draw_gui_panel_sprite_rmleft(x, y, units_per_px, spridx, remap)` - With color remap

### Button Drawing Patterns

**In-Game Button Types:**

1. **Tab Buttons:** `gui_draw_tab()` - [frontmenu_ingame_tabs.c:165]
   - Draws active/inactive tab headers
   - Handles tab switching animations

2. **Room Buttons:** `gui_area_room_button()` - [frontmenu_ingame_tabs.c:1606]
   - Shows room availability (buildable/locked)
   - Displays room icon, applies graying if unavailable
   - Flashing effect if insufficient gold

3. **Spell Buttons:** `gui_area_spell_button()` - [frontmenu_ingame_tabs.c:591]
   - Shows power availability based on research level
   - Displays mana cost indicator
   - Call-to-Arms and Sight of Evil special handling (blinking when active)

4. **Creature Buttons:** `gui_area_creature_button()` - [frontmenu_ingame_tabs.c:1747]
   - Shows creature portrait
   - Displays count of creatures owned
   - Pick-up interaction visual feedback

5. **Query Panel Buttons:** `gui_area_stat_button()` - [frontmenu_ingame_tabs.c:2087]
   - Creature statistics display
   - Health, experience, gold carried, etc.

### Input Handling

**In-Game Input Flow:**
1. `input()` - [main.cpp] - Global input handling
2. `get_player_gui_clicks()` - [frontend.h:417] - GUI-specific clicks
3. `do_button_click_actions()` - [frontend.cpp:1875] - Button callback execution

**Packet System Integration:**
- Many button clicks generate network packets via `set_players_packet_action()`
- Example: Room placement → `PckA_SetPlyrState` with `PSt_BuildRoom`
- Multiplayer-safe by design

---

## 4. Dependency Mapping

### Shared Core Components

**From bflib (Bullfrog Library):**

| Component | File | Purpose |
|-----------|------|---------|
| `GuiButton` | bflib_guibtns.h:141-169 | Button instance structure |
| `GuiButtonInit` | bflib_guibtns.h:117-139 | Button initialization data |
| `GuiMenu` | bflib_guibtns.h:171-185 | Menu container structure |
| `Gf_Btn_Callback` | bflib_guibtns.h:88 | Button callback type |
| `Gf_Mnu_Callback` | bflib_guibtns.h:89 | Menu callback type |
| Sprite functions | bflib_sprite.h/c | Sprite loading and rendering |
| Font functions | bflib_sprfnt.h/c | Sprite font rendering |
| Video functions | bflib_vidraw.h/c | Low-level drawing primitives |

**Shared Global State:**

```cpp
// From gui_frontmenu.c:38-39
unsigned char menu_stack[ACTIVE_MENUS_COUNT];  // 8 menu slots
struct GuiMenu active_menus[ACTIVE_MENUS_COUNT];

// From frontend.h:267
struct GuiButton active_buttons[ACTIVE_BUTTONS_COUNT];  // 86 button slots

// From gui_frontmenu.c:40
char no_of_active_menus;  // Active menu count
```

### Resource Dependencies

**Frontend System:**
```
frontend_load_data()
├── front.raw (640x480 background)
├── frontbit.dat/.tab (sprite sheet ~100 sprites)
└── frontend_font[4] (loaded elsewhere)
    ├── font0-0.dat/.tab
    ├── font1-0.dat/.tab
    ├── font2-0.dat/.tab
    └── font3-0.dat/.tab
```

**In-Game System:**
```
Game startup sprite loading
├── guiicons-0.dat/.tab (514 panel sprites)
├── gui_panel_sprites (main panel graphics)
└── Standard game fonts (loaded with game data)
```

### Library Dependencies Graph

```
┌─────────────────────────────────────────────────────┐
│                   Application Layer                  │
│  frontend.cpp           frontmenu_ingame_tabs.c     │
│  frontmenu_*.c          gui_*.c                     │
└─────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────┐
│                  Shared UI Layer                     │
│  gui_frontmenu.c (menu stack)                       │
│  gui_frontbtns.c (button input)                     │
│  gui_draw.c (drawing primitives)                    │
└─────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────┐
│                Base Library (bflib)                  │
│  bflib_guibtns.h (structures)                       │
│  bflib_sprite.c (sprite loading)                    │
│  bflib_sprfnt.c (font rendering)                    │
│  bflib_vidraw.c (low-level drawing)                 │
└─────────────────────────────────────────────────────┘
```

### Data Flow

**Button Creation Flow:**
```
1. Static GuiButtonInit array defined in *_data.cpp
2. create_menu(gmnu) called [frontend.cpp:2292]
   ├── Allocates menu slot in active_menus[]
   ├── Iterates GuiButtonInit array
   └── For each button:
       └── create_button(amnu, &btninit[i], units_per_px) [frontend.cpp:2070]
           ├── guibutton_get_unused_slot() - Find free slot
           ├── Copy data from GuiButtonInit to GuiButton
           ├── Setup callbacks, positions, sprites
           └── Store in active_buttons[]
```

**Button Input Flow:**
```
1. Mouse click detected
2. get_button_from_input() [gui_frontbtns.c:218]
   └── Iterate active_buttons[], check bounds
3. do_button_click_actions() [frontend.cpp:1881]
   └── Execute gbtn->click_event callback
4. Callback function (e.g., gui_choose_room())
   └── Update game state or send packet
```

**Button Rendering Flow:**
```
1. draw_gui() [frontend.cpp:3352]
2. draw_active_menus_buttons() [frontend.cpp:3257]
   └── For each menu in menu_stack:
       ├── gmnu->draw_cb(gmnu) - Menu-level drawing
       └── draw_menu_buttons(gmnu) [frontend.cpp:3206]
           └── For each button in menu:
               └── gbtn->draw_call(gbtn) - Button-specific drawing
                   └── e.g., gui_area_room_button() [frontmenu_ingame_tabs.c:1606]
                       ├── draw_gui_panel_sprite_left() - Background frame
                       ├── draw_gui_panel_sprite_left() - Room icon
                       └── draw_string64k() - Text overlay (if any)
```

---

## 5. Pain Points & Technical Debt

### Critical Issues

#### 1. **Duplicate Font Systems**
**Impact: High | Complexity: Medium**

```cpp
// Frontend system
frontend_font[4]
frontend_font_char_width()
frontend_font_string_width()

// In-game system  
LbTextSetFont() with game fonts
Direct bflib text rendering
```

**Problems:**
- Two separate font loading paths
- Different API for measuring text width
- Cannot easily share fonts between systems
- Font switching hacks when in-game menus need "frontend look"

**Example of confusion:** [bflib_sprfnt.c:1369]
```cpp
if (lbFontPtr == frontend_font[0]) {
    // Special case handling because frontend fonts are different!
}
```

#### 2. **Sprite Sheet Separation**
**Impact: High | Complexity: Low**

```cpp
frontend_sprite;      // Frontend buttons, sliders, icons
gui_panel_sprites;    // In-game panel UI
```

**Problems:**
- Cannot use in-game panel sprites in frontend menus (e.g., room icons in campaign selection)
- Duplicate assets (some sprites exist in both sheets)
- 514 panel sprites vs ~100 frontend sprites - huge disparity
- Loading/unloading complexity

**Memory waste example:**
- Treasury room icon exists in both `frontbit.dat` and `guiicons-0.dat`

#### 3. **Rendering Context Switching**
**Impact: High | Complexity: High**

**Frontend:**
```cpp
frontend_draw()
├── frontend_copy_background()  // Blit front.raw
└── draw_gui()
```

**In-Game:**
```cpp
redraw_isometric_view()
├── engine(player, render_cam)  // 3D world
├── draw_whole_status_panel()   // Panel tiles
└── draw_gui()
```

**Problems:**
- `draw_gui()` behaves differently depending on global state
- No clear interface for "where am I rendering?"
- Tight coupling between rendering and game state
- Cannot easily preview in-game UI in frontend context (e.g., tutorial screenshots)

#### 4. **Global State Pollution**
**Impact: Medium | Complexity: High**

**Active Arrays:**
```cpp
active_buttons[86]    // Shared pool - easy to exhaust
active_menus[8]       // Only 8 simultaneous menus allowed
menu_stack[8]         // Stack-based menu management
```

**Problems:**
- Fixed array sizes cause silent failures
- No ownership tracking (who created which button?)
- Difficult to debug ("which menu is button 47 part of?")
- Menu stack can overflow with nested dialogs

**Real bug example:** [frontend.cpp:2146]
```cpp
if (gidx == -1) {
    // No free buttons - SILENTLY FAIL!
    return -1;
}
```

#### 5. **Position Calculation Inconsistency**
**Impact: Medium | Complexity: Medium**

```cpp
// Frontend uses magic constants
POS_SCRCTR  = -997;  // Screen center
POS_GAMECTR =  999;  // Game center (?)

// In-game calculates from pixel_size
gmnu_x = (lbDisplay.PhysicalScreenWidth - base_width * units_per_pixel) / pixel_size;
```

**Problems:**
- `compute_menu_position_x/y()` has complex, hard-to-follow logic
- Different coordinate systems (screen pixels, UI units, game coordinates)
- Difficult to support multiple resolutions
- Tooltip positioning is especially fragile

#### 6. **Button Lifecycle Management**
**Impact: Medium | Complexity: Medium**

**Creation:**
```cpp
create_menu() → create_button() → slot allocated in active_buttons[]
```

**Destruction:**
```cpp
kill_menu() → kill_button() → flags &= ~LbBtnF_Active
```

**Problems:**
- No RAII-style cleanup
- Leaked button slots if menu killed improperly
- Maintain callbacks can be orphaned
- Circular dependencies (button → menu → button)

#### 7. **Dynamic Config Integration Hack**
**Impact: Low | Complexity: High**

In-game tabs dynamically update from config:
```cpp
update_room_tab_to_config()
update_trap_tab_to_config()
update_powers_tab_to_config()
```

**Problems:**
- Frontend menus are 100% static (compiled button arrays)
- Config changes require restarting to take effect in frontend
- Inconsistent behavior (in-game respects config, frontend doesn't)
- Modders can't easily extend frontend menus

#### 8. **State Machine Complexity**
**Impact: High | Complexity: Very High**

35+ `FrontendMenuState` values with complex transition logic:
```cpp
get_menu_state_when_back_from_substate()
get_menu_state_based_on_last_level()
get_startup_menu_state()
```

**Problems:**
- No visual state diagram
- State transitions scattered across multiple files
- Easy to create unreachable states
- Difficult to add new menu flows
- Network states (FeSt_NET_*) are especially complex

**Example of complexity:** [frontend.cpp:3791-3866]
```cpp
// 75 lines just to determine "where do we go when we cancel this menu?"
FrontendMenuState get_menu_state_when_back_from_substate(FrontendMenuState substate)
{
    switch (substate) {
        case FeSt_FEDEFINE_KEYS:
            if (game.game_kind == GKind_Unknown)
                return FeSt_FEOPTIONS;
            else
                return FeSt_OPTIONS;
        case FeSt_FEOPTIONS:
            if (game.game_kind == GKind_LocalGame)
                return FeSt_OPTIONS;
            // ... 20 more cases
    }
}
```

### Minor Issues

#### 9. **Text Buffer Overflow Risk**
```cpp
char gui_textbuf[TEXT_BUFFER_LENGTH];  // Global buffer, TEXT_BUFFER_LENGTH = 128
```
Multiple systems write to this. No bounds checking in some draw paths.

#### 10. **Callback Function Signature Inconsistency**
Some callbacks take `struct GuiButton*`, others also need menu context but have no way to get it except through `gbtn->gmenu_idx`.

#### 11. **Tooltip System Separate from Button System**
Tooltips are managed separately in `gui_tooltips.c`, leading to sync issues between button state and tooltip state.

#### 12. **No Animation System**
Button state changes are instant (no fade in/out, no smooth transitions). Menu fading is the only animation, and it's hacky.

#### 13. **Hard-Coded Button IDs**
```cpp
enum IngameButtonDesignationIDs {
    BID_INFO_TAB = BID_DEFAULT+1,
    BID_ROOM_TAB,
    BID_SPELL_TAB,
    // ... 60+ hard-coded IDs
}
```
Adding new buttons requires editing this enum and hoping for no conflicts.

---

## 6. Unified System Design Proposal

### Design Goals

1. **Single Responsibility:** Each component has one job
2. **Resource Sharing:** One font system, one sprite system
3. **Rendering Independence:** UI can render to any context (frontend, in-game, mini-preview)
4. **Config-Driven:** All UI layouts from config files (like in-game tabs)
5. **Type Safety:** Replace magic constants with enums/structs
6. **Ownership:** Clear ownership of buttons, menus, resources
7. **Scalability:** Support 4K resolutions, dynamic UI scaling

### Proposed Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                         │
│  ┌──────────────────────┐  ┌───────────────────────────┐   │
│  │   Frontend Menus     │  │   In-Game Panels          │   │
│  │   - Main menu        │  │   - Status panel          │   │
│  │   - Options          │  │   - Query menus           │   │
│  │   - Level select     │  │   - Pause menu            │   │
│  └──────────────────────┘  └───────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   Unified UI Framework                       │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  UIContext (rendering target, input state)           │   │
│  │  - DrawContext (where to render)                     │   │
│  │  - InputManager (event distribution)                 │   │
│  │  - LayoutEngine (coordinate translation)             │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  UIWidgetSystem                                       │   │
│  │  - Button, Label, Slider, Icon, List                 │   │
│  │  - Layouts: HBox, VBox, Grid                         │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  UIMenuSystem                                         │   │
│  │  - Menu containers, navigation, stacking             │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  UIResourceManager                                    │   │
│  │  - Unified sprite system                             │   │
│  │  - Unified font system                               │   │
│  │  - Resource sharing and caching                      │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                  Abstraction Layer                           │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Renderer (abstracts bflib drawing)                  │   │
│  │  - DrawSprite, DrawText, DrawRect, DrawLine         │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  InputAbstraction                                     │   │
│  │  - Mouse, Keyboard, Gamepad (future)                 │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              Base Library (bflib - unchanged)                │
│  bflib_sprite, bflib_vidraw, bflib_sprfnt                   │
└─────────────────────────────────────────────────────────────┘
```

### Key Components

#### 1. UIContext - Rendering Target Abstraction

```cpp
// New file: ui_context.h

enum UIRenderTarget {
    UIRT_Frontend,      // Rendering to frontend background
    UIRT_GameWorld,     // Rendering over 3D world
    UIRT_Overlay,       // Fullscreen overlay (parchment, etc.)
    UIRT_OffScreen,     // For preview/screenshots
};

struct UIDrawContext {
    UIRenderTarget target;
    int viewport_x, viewport_y;
    int viewport_width, viewport_height;
    float ui_scale;           // For resolution independence
    TbPixel *framebuffer;     // Where to draw
    struct UIClipRect *clip;  // Clipping region
};

struct UIContext {
    UIDrawContext draw_ctx;
    UIInputManager *input;
    UIResourceManager *resources;
    UILayoutEngine *layout;
};
```

**Usage:**
```cpp
// Frontend
UIContext frontend_ctx;
ui_context_init(&frontend_ctx, UIRT_Frontend);
ui_context_set_background(&frontend_ctx, frontend_background);

// In-game
UIContext game_ctx;
ui_context_init(&game_ctx, UIRT_GameWorld);
ui_context_set_world_camera(&game_ctx, player->acamera);
```

#### 2. UIResourceManager - Unified Resource Access

```cpp
// New file: ui_resources.h

struct UIResourceManager {
    // Unified sprite system
    struct SpriteBank *banks[UI_SPRITEBANK_COUNT];
    
    // Unified font system
    struct FontFamily *fonts[UI_FONT_COUNT];
    
    // Sprite cache
    struct SpriteCache *cache;
};

// Public API
struct TbSprite* ui_get_sprite(UIResourceManager *mgr, const char *sprite_name);
struct FontFamily* ui_get_font(UIResourceManager *mgr, UIFontID font_id);
void ui_load_sprite_bank(UIResourceManager *mgr, const char *bank_name);
```

**Migration strategy:**
```cpp
// Instead of:
const struct TbSprite *spr = get_frontend_sprite(GFS_slider_indicator_std);
const struct TbSprite *spr = get_panel_sprite(GPS_rpanel_frame_portrt_empty);

// Use:
const struct TbSprite *spr = ui_get_sprite(ctx->resources, "slider_indicator_std");
const struct TbSprite *spr = ui_get_sprite(ctx->resources, "panel_frame_portrt_empty");
```

**Benefits:**
- One sprite loading path
- Easy to merge `frontend_sprite` and `gui_panel_sprites`
- Can query "is this sprite loaded?" without checking two variables
- Resource hot-reloading for development

#### 3. UIWidget - Base Widget Class

```cpp
// New file: ui_widget.h

typedef enum {
    UIW_Button,
    UIW_Label,
    UIW_Icon,
    UIW_Slider,
    UIW_List,
    UIW_Panel,
    UIW_Custom,
} UIWidgetType;

struct UIWidget {
    UIWidgetType type;
    int id;
    
    // Transform
    UIRect bounds;           // Position and size
    UIRect padding;
    TbBool visible;
    TbBool enabled;
    
    // Hierarchy
    struct UIWidget *parent;
    struct UIWidget *children;
    int child_count;
    
    // Callbacks
    UIEventCallback on_click;
    UIEventCallback on_hover;
    UIEventCallback on_draw;
    void *userdata;
    
    // Rendering
    UIDrawInfo draw_info;    // Colors, sprites, text
};

// Widget lifecycle
struct UIWidget* ui_widget_create(UIWidgetType type);
void ui_widget_destroy(struct UIWidget *widget);
void ui_widget_add_child(struct UIWidget *parent, struct UIWidget *child);
void ui_widget_remove_child(struct UIWidget *parent, struct UIWidget *child);

// Widget operations
void ui_widget_set_position(struct UIWidget *widget, int x, int y);
void ui_widget_set_size(struct UIWidget *widget, int width, int height);
void ui_widget_set_visible(struct UIWidget *widget, TbBool visible);
void ui_widget_set_enabled(struct UIWidget *widget, TbBool enabled);

// Rendering
void ui_widget_draw(struct UIWidget *widget, UIContext *ctx);
void ui_widget_update(struct UIWidget *widget, float delta_time);

// Input
TbBool ui_widget_handle_input(struct UIWidget *widget, UIInputEvent *event);
```

#### 4. UIMenu - Modern Menu System

```cpp
// New file: ui_menu.h

struct UIMenu {
    MenuID id;
    char *name;
    
    struct UIWidget *root_widget;  // Root of widget tree
    struct UIMenu *parent_menu;    // For navigation
    
    UIMenuState state;    // Opening, Active, Closing, Closed
    float transition_time;
    
    TbBool modal;         // Blocks input to other menus
    TbBool closes_on_click_outside;
};

// Menu management
struct UIMenu* ui_menu_create(MenuID id, const char *name);
void ui_menu_destroy(struct UIMenu *menu);
void ui_menu_open(struct UIMenu *menu, UIContext *ctx);
void ui_menu_close(struct UIMenu *menu);
void ui_menu_add_widget(struct UIMenu *menu, struct UIWidget *widget);

// Menu stack (replaces global menu_stack[])
struct UIMenuStack {
    struct UIMenu *menus[UI_MAX_MENU_STACK];
    int count;
};
void ui_menu_stack_push(UIMenuStack *stack, struct UIMenu *menu);
void ui_menu_stack_pop(UIMenuStack *stack);
struct UIMenu* ui_menu_stack_top(UIMenuStack *stack);
```

#### 5. UILayoutEngine - Automatic Layouts

```cpp
// New file: ui_layout.h

typedef enum {
    UIL_Absolute,    // Fixed positions (legacy mode)
    UIL_HBox,        // Horizontal box
    UIL_VBox,        // Vertical box
    UIL_Grid,        // Grid layout
    UIL_Flow,        // Flowing layout (wrap)
} UILayoutType;

struct UILayout {
    UILayoutType type;
    int spacing;
    UIAlignment alignment;
    UIJustify justify;
};

void ui_layout_compute(struct UIWidget *container, UIContext *ctx);
```

**Example usage:**
```cpp
// Create a vertical menu
UIWidget *menu_root = ui_widget_create(UIW_Panel);
ui_widget_set_layout(menu_root, UIL_VBox, 10);  // 10px spacing

// Add buttons - they'll auto-arrange!
for (int i = 0; i < 5; i++) {
    UIWidget *button = ui_button_create("Button Text", button_callback);
    ui_widget_add_child(menu_root, button);
}

ui_layout_compute(menu_root, ctx);  // Auto-positions all children
```

### Migration Strategy

#### Phase 1: Foundation (2-3 weeks)
**Goal:** Build new systems alongside old, no breaking changes

1. ✅ Create new UI files:
   - `ui_context.h/c` - Context management
   - `ui_resources.h/c` - Resource manager
   - `ui_widget.h/c` - Base widget system
   - `ui_menu.h/c` - Modern menu system
   - `ui_layout.h/c` - Layout engine

2. ✅ Implement UIResourceManager:
   - Load `frontend_sprite` and `gui_panel_sprites` into unified manager
   - Implement `ui_get_sprite()` wrapper
   - Create sprite name → index mapping

3. ✅ Create adapter layer:
   - `GuiButton → UIWidget` converter
   - `GuiMenu → UIMenu` converter
   - Keep both systems working in parallel

**Test:** All existing menus work unchanged. New sprite API works alongside old.

#### Phase 2: Frontend Migration (3-4 weeks)
**Goal:** Migrate frontend menus to new system

1. ✅ Convert frontend main menu:
   - Rewrite `frontend_main_menu_buttons[]` as UIWidget tree
   - Use `ui_menu_create()` instead of `create_menu()`
   - Keep old code in `frontend_legacy.cpp` for comparison

2. ✅ Migrate frontend drawing:
   - `frontend_draw()` uses UIContext
   - Rendering backends (frontend background, 3D world) as `UIRenderTarget` implementations

3. ✅ Convert remaining frontend menus:
   - Options menu
   - Level select
   - High scores
   - Network menus (most complex!)

4. ✅ Remove `frontend_font[]` in favor of unified font system

**Test:** All frontend menus work with new system. Old system still accessible for in-game.

#### Phase 3: In-Game Migration (4-5 weeks)
**Goal:** Migrate in-game panel and tabs

1. ✅ Convert status panel:
   - Panel background as UIWidget with custom draw
   - Minimap as UIWidget
   - Tab buttons as UIButton widgets

2. ✅ Convert tab contents:
   - Room menu (16 room buttons + big preview)
   - Spell menu (16 spell buttons + big preview)
   - Trap menu
   - Creature menu

3. ✅ Convert query menus:
   - Creature query panels (4 pages)
   - Thing inspection panels

4. ✅ Convert pause menu:
   - Options menu (duplicate of frontend options!)
   - Save/load (duplicate of frontend save/load!)
   - Quit confirmation

**Test:** In-game UI works with new system. FPS is not impacted.

#### Phase 4: Cleanup (2 weeks)
**Goal:** Remove legacy systems

1. ✅ Delete old code:
   - Remove `frontend_font[]`
   - Remove separate `frontend_sprite` / `gui_panel_sprites`
   - Remove `active_buttons[]` and `active_menus[]` globals
   - Remove `menu_stack[]`

2. ✅ Unify duplicated menus:
   - Frontend options = in-game options (one implementation)
   - Frontend save/load = in-game save/load (one implementation)

3. ✅ Documentation:
   - UI programmer's guide
   - Widget creation tutorial
   - Migration guide for modders

**Test:** Full regression test. All menus work. Code size reduced by ~30%.

#### Phase 5: Config-Driven UI (3-4 weeks)
**Goal:** Move all UI layouts to config files

1. ✅ Create UI definition format:
   ```json
   {
     "menu_id": "main_menu",
     "layout": "vbox",
     "widgets": [
       {
         "type": "button",
         "text": "@GUI_START_NEW_GAME",
         "onclick": "start_new_game",
         "sprite": "large_button"
       },
       {
         "type": "button",
         "text": "@GUI_CONTINUE_GAME",
         "onclick": "continue_game",
         "sprite": "large_button",
         "visible_if": "save_game_exists"
       }
     ]
   }
   ```

2. ✅ Implement UI parser:
   - Parse JSON/CFG files
   - Construct widget trees from config
   - Handle callbacks via string → function pointer map

3. ✅ Convert all menus to config files:
   - `data/ui/frontend_main.json`
   - `data/ui/ingame_status.json`
   - `data/ui/options.json`
   - Etc.

**Test:** Can modify menu layout without recompiling. Mods can add custom menus.

---

## 7. Concrete Next Steps

### Immediate Actions (This Week)

#### ⚡ Step 1: Create Prototype Branch
```bash
git checkout -b feature/unified-ui-prototype
```

#### ⚡ Step 2: Implement UIContext Foundation
**File:** `src/ui/ui_context.h` and `src/ui/ui_context.c`

**Tasks:**
- [ ] Define `UIRenderTarget` enum
- [ ] Define `UIDrawContext` struct
- [ ] Implement `ui_context_init()`
- [ ] Implement `ui_context_set_target()`

**Goal:** Working context that can wrap either frontend or in-game rendering.

**Test:**
```cpp
// Test code (in test file)
UIContext ctx;
ui_context_init(&ctx, UIRT_Frontend);
assert(ctx.draw_ctx.target == UIRT_Frontend);

ui_context_set_target(&ctx, UIRT_GameWorld);
assert(ctx.draw_ctx.target == UIRT_GameWorld);

ui_context_destroy(&ctx);
```

**Time estimate:** 4 hours

#### ⚡ Step 3: Implement UIResourceManager Skeleton
**File:** `src/ui/ui_resources.h` and `src/ui/ui_resources.c`

**Tasks:**
- [ ] Define `UIResourceManager` struct
- [ ] Implement `ui_resources_init()`
- [ ] Implement `ui_get_sprite()` (wraps existing functions for now)
- [ ] Implement `ui_get_font()` (wraps existing functions for now)

**Goal:** Unified API for accessing sprites and fonts, even if implementation still calls old code.

**Test:**
```cpp
UIResourceManager mgr;
ui_resources_init(&mgr);

// Load both sprite systems
ui_resources_load_frontend_sprites(&mgr);
ui_resources_load_panel_sprites(&mgr);

// Test unified access
const TbSprite *spr1 = ui_get_sprite(&mgr, "frontend:slider_indicator_std");
const TbSprite *spr2 = ui_get_sprite(&mgr, "panel:rpanel_frame_portrt_empty");

assert(spr1 != NULL);
assert(spr2 != NULL);

ui_resources_destroy(&mgr);
```

**Time estimate:** 6 hours

#### ⚡ Step 4: Create UIWidget Base Class
**File:** `src/ui/ui_widget.h` and `src/ui/ui_widget.c`

**Tasks:**
- [ ] Define `UIWidget` struct
- [ ] Implement `ui_widget_create()`
- [ ] Implement `ui_widget_destroy()`
- [ ] Implement `ui_widget_set_position()`, `ui_widget_set_size()`
- [ ] Implement `ui_widget_draw()` (just draws a colored rectangle for now)

**Goal:** Working widget that can be positioned and drawn.

**Test:**
```cpp
UIWidget *widget = ui_widget_create(UIW_Button);
ui_widget_set_position(widget, 100, 100);
ui_widget_set_size(widget, 200, 50);

UIContext ctx;
ui_context_init(&ctx, UIRT_Frontend);
ui_widget_draw(widget, &ctx);  // Should draw a rectangle

ui_widget_destroy(widget);
```

**Time estimate:** 8 hours

### Short-Term Goals (Next 2 Weeks)

#### 🎯 Goal 1: Prototype One Frontend Menu
**Target:** Main menu (simplest menu)

**Tasks:**
- [ ] Extend UIWidget to support buttons with text
- [ ] Implement button click handling
- [ ] Create `ui_button_create(text, callback)`
- [ ] Rewrite `frontend_main_menu_buttons[]` as UIWidget tree
- [ ] Create `frontend_main_menu_new()` that uses new system
- [ ] Add compile flag to switch between old/new menu

**Test:** Main menu renders and responds to clicks using new system.

**Success Metric:** Can toggle between old and new main menu, both work identically.

**Time estimate:** 20 hours (2-3 days)

#### 🎯 Goal 2: Implement Layout System
**Target:** Auto-layout for vertical button lists

**Tasks:**
- [ ] Implement `UIL_VBox` layout type
- [ ] Implement `ui_layout_compute()` for VBox
- [ ] Test with main menu buttons (should auto-position)
- [ ] Add spacing and alignment parameters

**Test:** 5 buttons in VBox auto-arrange with correct spacing.

**Time estimate:** 12 hours (1.5 days)

#### 🎯 Goal 3: Performance Profiling
**Target:** Ensure new system is not slower than old

**Tasks:**
- [ ] Add profiling hooks to `ui_widget_draw()`
- [ ] Compare frame time: old frontend vs new frontend
- [ ] Optimize hotspots if needed
- [ ] Document performance characteristics

**Success Metric:** New system draws in ≤ 105% time of old system.

**Time estimate:** 8 hours (1 day)

### Medium-Term Goals (Next 4-6 Weeks)

#### 📋 Week 3-4: Convert All Frontend Menus
- [ ] Options menu (most complex frontend menu)
- [ ] Level select menu
- [ ] Save/load menus
- [ ] Network menus
- [ ] High scores

#### 📋 Week 5-6: Begin In-Game Migration
- [ ] Status panel background
- [ ] Tab buttons
- [ ] Room menu (first tab)
- [ ] Basic query menu

### Long-Term Goals (Next 3 Months)

#### 🗓️ Month 2: Complete In-Game Migration
- [ ] All tabs (Room, Spell, Trap, Creature)
- [ ] Query menus (4 pages)
- [ ] Pause menu
- [ ] Event menu
- [ ] Message system integration

#### 🗓️ Month 3: Unification & Cleanup
- [ ] Remove legacy systems
- [ ] Merge duplicated menus
- [ ] Performance optimization pass
- [ ] Memory leak testing
- [ ] Modding API documentation

#### 🗓️ Month 4: Config-Driven UI (Stretch Goal)
- [ ] UI definition file format
- [ ] Parser implementation
- [ ] Convert menus to config files
- [ ] Modding tutorial

---

## 8. Risk Assessment

### High Risks

#### ⚠️ Risk 1: Performance Degradation
**Impact: Critical | Probability: Medium**

**Description:** New abstraction layers could slow down rendering, causing FPS drops.

**Mitigation:**
- Profile early and often
- Implement zero-cost abstractions where possible
- Use C, not C++, for hot paths
- Keep widget trees shallow (max 3-4 levels deep)
- Cache computed layouts

**Contingency:** If perf target missed by >10%, revert specific components to direct rendering.

#### ⚠️ Risk 2: Regression Bugs
**Impact: High | Probability: High**

**Description:** Breaking existing menus during migration.

**Mitigation:**
- Parallel implementation (keep old code working)
- Compile-time flag to switch between old/new
- Extensive regression testing before each commit
- Manual testing of all menus each week
- Screenshot comparison tool

**Contingency:** Feature flag system allows rolling back to old UI per-menu if issues found.

### Medium Risks

#### ⚠️ Risk 3: Scope Creep
**Impact: Medium | Probability: High**

**Description:** "While we're here, let's also fix..." syndrome.

**Mitigation:**
- Stick to migration plan
- Document "nice to haves" separately
- Code reviews focus on migration goals only
- Defer new features to post-migration

#### ⚠️ Risk 4: Network Multiplayer Issues
**Impact: Medium | Probability: Medium**

**Description:** Network menus are complex (FeSt_NET_*). Changes could break multiplayer.

**Mitigation:**
- Migrate network menus last
- Extensive multiplayer testing
- Packet recording/replay for reproducibility
- Community beta testing

### Low Risks

#### ⚠️ Risk 5: Config Format Adoption
**Impact: Low | Probability: Low**

**Description:** Modders might not like new config format.

**Mitigation:**
- JSON is widely known
- Provide migration script for existing mods
- Backward compatibility for hardcoded menus
- Good documentation

---

## Appendices

### Appendix A: File Inventory

#### Frontend System Files (18 files, ~8000 LOC)
```
src/frontend.cpp (3926 lines) ⭐ Core state machine
src/frontend.h (449 lines)
src/frontmenu_options.c
src/frontmenu_options.h
src/frontmenu_options_data.cpp
src/frontmenu_select.c
src/frontmenu_select.h
src/frontmenu_select_data.cpp
src/frontmenu_net.c
src/frontmenu_net.h
src/frontmenu_net_data.cpp
src/frontmenu_saves.c
src/frontmenu_saves.h
src/frontmenu_saves_data.cpp
src/frontmenu_specials.c
src/frontmenu_specials.h
```

#### In-Game UI Files (24 files, ~9000 LOC)
```
src/frontmenu_ingame_tabs.c (2880 lines) ⭐ Main status panel
src/frontmenu_ingame_tabs.h
src/frontmenu_ingame_tabs_data.cpp
src/frontmenu_ingame_opts.c
src/frontmenu_ingame_opts.h
src/frontmenu_ingame_opts_data.cpp
src/frontmenu_ingame_map.c
src/frontmenu_ingame_map.h
src/frontmenu_ingame_evnt.c
src/frontmenu_ingame_evnt.h
src/frontmenu_ingame_evnt_data.cpp
src/gui_draw.c (834 lines)
src/gui_draw.h
src/gui_frontmenu.c
src/gui_frontmenu.h
src/gui_frontbtns.c
src/gui_frontbtns.h
src/gui_boxmenu.c
src/gui_boxmenu.h
src/gui_tooltips.c
src/gui_tooltips.h
src/gui_topmsg.c
src/gui_topmsg.h
src/gui_msgs.c
src/gui_msgs.h
src/gui_parchment.c
src/gui_parchment.h
```

#### Shared/Base Files (5 files, ~1200 LOC)
```
src/bflib_guibtns.h (239 lines) ⭐ Core structures
src/bflib_sprite.h
src/bflib_sprite.c
src/bflib_sprfnt.h
src/bflib_sprfnt.c
```

**Total:** 47 files, ~18,200 lines of UI code

### Appendix B: Button Type Reference

```cpp
enum TbButtonType {
    LbBtnT_NormalBtn = 0,      // Standard clickable button
    LbBtnT_HoldableBtn,        // Continuous effect while held (scrolling)
    LbBtnT_ToggleBtn,          // On/off switch
    LbBtnT_RadioBtn,           // Exclusive selection (tabs)
    LbBtnT_HorizSlider,        // Horizontal slider widget
    LbBtnT_EditBox,            // Text input field
    LbBtnT_Hotspot,            // Invisible clickable area
};
```

### Appendix C: Menu ID Reference

```cpp
enum GUI_Menus {
    GMnu_MAIN = 1,              // In-game main status panel
    GMnu_ROOM = 2,              // Room building panel
    GMnu_SPELL = 3,             // Spell casting panel
    GMnu_TRAP = 4,              // Trap placement panel
    GMnu_CREATURE = 5,          // Creature selection panel
    GMnu_EVENT = 6,             // Battle notifications
    GMnu_QUERY = 7,             // Thing inspection
    GMnu_OPTIONS = 8,           // In-game pause menu
    GMnu_INSTANCE = 9,          // Creature instance menu
    GMnu_QUIT = 10,             // Quit confirmation
    GMnu_LOAD = 11,             // In-game load menu
    GMnu_SAVE = 12,             // In-game save menu
    GMnu_VIDEO = 13,            // Video options
    GMnu_SOUND = 14,            // Sound options
    GMnu_ERROR_BOX = 15,        // Error dialog
    GMnu_TEXT_INFO = 16,        // Text display
    GMnu_HOLD_AUDIENCE = 17,    // Hold Audience spell menu
    GMnu_FEMAIN = 18,           // Frontend main menu
    GMnu_FELOAD = 19,           // Frontend load game
    GMnu_FENET_SERVICE = 20,    // Network service select
    GMnu_FENET_SESSION = 21,    // Network session browser
    GMnu_FENET_START = 22,      // Network lobby
    GMnu_FESTATISTICS = 25,     // Statistics screen
    GMnu_FEHIGH_SCORE_TABLE = 26, // High scores
    GMnu_CREATURE_QUERY1-4 = 31-35, // Creature query pages
    GMnu_FEDEFINE_KEYS = 36,    // Key binding menu
    GMnu_AUTOPILOT = 37,        // Autopilot menu
    GMnu_SPELL_LOST = 38,       // Lost first person spell menu
    GMnu_FEOPTION = 39,         // Frontend options
    GMnu_FELEVEL_SELECT = 40,   // Level selection
    GMnu_FECAMPAIGN_SELECT = 41,// Campaign selection
    GMnu_FEERROR_BOX = 42,      // Frontend error box
    GMnu_FEADD_SESSION = 43,    // Add network session
    GMnu_MAPPACK_SELECT = 44,   // Mappack selection
    GMnu_MSG_BOX = 45,          // Message box
    GMnu_SPELL2 = 46,           // Spell panel page 2
    GMnu_ROOM2 = 47,            // Room panel page 2
    GMnu_TRAP2 = 48,            // Trap panel page 2
};
```

### Appendix D: Rendering Function Call Chains

**Frontend Rendering:**
```
main.cpp: wait_at_frontend()
└─ frontend.cpp: frontend_draw()
   ├─ frontend.cpp: frontend_copy_background()  [Blit front.raw]
   └─ frontend.cpp: draw_gui()
      └─ frontend.cpp: draw_active_menus_buttons()
         ├─ [Menu callback] gmnu->draw_cb(gmnu)
         └─ frontend.cpp: draw_menu_buttons(gmnu)
            └─ [Button callback] gbtn->draw_call(gbtn)
               ├─ frontend.cpp: frontend_draw_large_menu_button()
               │  └─ gui_draw.c: draw_bar64k()
               │     └─ bflib_sprite.c: LbSpriteDrawResized()
               └─ frontend.cpp: frontend_draw_text()
                  └─ gui_draw.c: draw_string64k()
                     └─ bflib_sprfnt.c: LbTextDraw()
```

**In-Game Rendering:**
```
main.cpp: gameplay_loop_draw()
└─ main.cpp: keeper_screen_redraw()
   └─ engine_redraw.c: redraw_isometric_view()
      ├─ engine_render.c: engine()  [3D world rendering]
      ├─ frontmenu_ingame_tabs.c: draw_whole_status_panel()
      │  ├─ frontmenu_ingame_map.c: panel_map_draw_slabs()
      │  └─ gui_draw.c: draw_gui_panel_sprite_left()
      │     └─ bflib_sprite.c: LbSpriteDrawResized()
      ├─ frontend.cpp: draw_gui()
      │  └─ [Same as frontend chain above]
      ├─ gui_msgs.c: message_draw()
      ├─ power_hand.c: draw_power_hand()
      └─ gui_tooltips.c: draw_tooltip()
```

### Appendix E: Key Global Variables

```cpp
// Shared UI state (all in global scope)
struct GuiButton active_buttons[ACTIVE_BUTTONS_COUNT];  // 86 slots
struct GuiMenu active_menus[ACTIVE_MENUS_COUNT];        // 8 slots
unsigned char menu_stack[ACTIVE_MENUS_COUNT];           // Menu ordering
char no_of_active_menus;                                // Active count

// Frontend-specific
FrontendMenuState frontend_menu_state;
struct TbSpriteSheet *frontend_font[FRONTEND_FONTS_COUNT];
struct TbSpriteSheet *frontend_sprite;
unsigned char *frontend_background;  // Points to game.map!
long frontend_mouse_over_button;

// In-game specific
struct TbSpriteSheet *gui_panel_sprites;
int gui_blink_rate;
int neutral_flash_rate;
char gui_textbuf[TEXT_BUFFER_LENGTH];
```

---

## Conclusion

KeeperFX's UI architecture reflects its 20+ year evolution: a frontend menu system designed for simplicity (single-player, main menu flow) and an in-game UI system designed for real-time gameplay (status panel, multiplayer-safe packets). These systems share base components but diverge in critical areas (rendering, fonts, resources, state management).

**The path forward:**
1. ✅ **Phase 1 (Foundation):** Build unified framework alongside existing code (3 weeks)
2. ✅ **Phase 2 (Frontend Migration):** Port frontend menus to new system (4 weeks)
3. ✅ **Phase 3 (In-Game Migration):** Port gameplay UI to new system (5 weeks)
4. ✅ **Phase 4 (Cleanup):** Remove legacy code, unify duplicates (2 weeks)
5. ✅ **Phase 5 (Config-Driven):** Move layouts to config files (4 weeks)

**Total estimated time:** 18 weeks (4.5 months) for complete migration.

**Key Benefits:**
- 🎯 **Single codebase** for all UI (reduce ~30% of UI code)
- 🎨 **Consistent look & feel** across all menus
- ⚡ **Better performance** through resource sharing
- 🔧 **Easier maintenance** (one system to debug)
- 📦 **Modding support** via config-driven layouts
- 🖥️ **Resolution independence** for 4K+ displays

The unified architecture positions KeeperFX for modern UI expectations while preserving the classic Dungeon Keeper aesthetic. The migration is feasible with a phased approach that maintains backward compatibility throughout.

**Document Status:** ✅ Complete  
**Next Action:** Implement Step 1 (UIContext prototype)  
**Owner:** Development team  
**Review Date:** End of Phase 1 (3 weeks)

---

*End of Document*
