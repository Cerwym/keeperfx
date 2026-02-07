# KeeperFX Data-Driven UI System - Architecture Document

## Executive Summary

This document describes the architecture for modernizing KeeperFX's UI system from hardcoded C arrays to data-driven JSON definitions with visual editing tools. The system enables non-programmers to design menus, supports rapid iteration, and improves localization workflows.

**Goals:**
- Decouple UI layout from game code
- Enable visual menu design via VSCode extension
- Support Lua scripting for UI logic
- Maintain backward compatibility
- Improve modder accessibility

---

## System Overview

### Architecture Layers

```
┌─────────────────────────────────────────────────────────┐
│                    VSCode Extension                      │
│  ┌───────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │ Visual Editor │  │ Sprite Viewer│  │ JSON Manager │ │
│  └───────────────┘  └──────────────┘  └──────────────┘ │
└─────────────────────────────────────────────────────────┘
                            │
                            ▼ (JSON Files)
               ┌─────────────────────────┐
               │   data/menus/*.json     │
               │   data/menus/*.lua      │
               └─────────────────────────┘
                            │
                            ▼ (Load at runtime)
┌─────────────────────────────────────────────────────────┐
│                   KeeperFX Game Engine                   │
│  ┌───────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │ JSON Parser   │→ │ UI Builder   │→ │ Lua VM       │ │
│  │ (cJSON)       │  │ (Dynamic)    │  │ (Callbacks)  │ │
│  └───────────────┘  └──────────────┘  └──────────────┘ │
│                            │                             │
│                            ▼                             │
│  ┌─────────────────────────────────────────────────┐   │
│  │        Existing GUI System (bflib_guibtns)      │   │
│  └─────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

---

## File Format Specification

### Menu Definition (JSON)

**Location**: `data/menus/{menu_name}.json`

```json
{
  "schema_version": "1.0",
  "menu_id": "GMnu_CAMPAIGNS",
  "description": "Campaign selection and management menu",
  "position": {
    "x": "center",
    "y": "center"
  },
  "size": {
    "width": 640,
    "height": 480
  },
  "background": null,
  "buttons": [
    {
      "id": "btn_title",
      "type": "NormalBtn",
      "position": {
        "x": "center",
        "y": 30,
        "anchor": "top-center"
      },
      "size": {
        "width": 495,
        "height": 46
      },
      "visual": {
        "sprite_index": 85,
        "draw_callback": "frontend_draw_vlarge_menu_button"
      },
      "text": {
        "string_id": "GUIStr_Campaigns",
        "alignment": "center"
      },
      "state": {
        "enabled": true,
        "visible": true,
        "clickable": false
      },
      "callbacks": {
        "on_click": null,
        "on_hover": null,
        "on_maintain": null
      }
    },
    {
      "id": "btn_campaign_001",
      "type": "NormalBtn",
      "position": {
        "x": 120,
        "y": 90
      },
      "size": {
        "width": 400,
        "height": 32
      },
      "visual": {
        "draw_callback": "draw_campaign_button"
      },
      "text": {
        "dynamic": true,
        "lua_getter": "get_campaign_name(0)"
      },
      "state": {
        "enabled": true,
        "visible": true
      },
      "callbacks": {
        "on_click": {
          "type": "lua",
          "script": "on_campaign_click",
          "params": { "campaign_index": 0 }
        },
        "on_maintain": {
          "type": "lua",
          "script": "maintain_campaign_button"
        }
      },
      "data": {
        "campaign_index": 0
      }
    }
  ],
  "lua_scripts": [
    "menus/campaigns/main.lua",
    "menus/campaigns/buttons.lua"
  ]
}
```

### JSON Schema Fields

#### Root Object
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `schema_version` | string | Yes | Format version (e.g., "1.0") |
| `menu_id` | string | Yes | Internal menu identifier (maps to C enum) |
| `description` | string | No | Human-readable description |
| `position` | object | Yes | Menu position on screen |
| `size` | object | Yes | Menu dimensions |
| `background` | string/null | No | Background sprite or null |
| `buttons` | array | Yes | Array of button definitions |
| `lua_scripts` | array | No | Additional Lua files to load |

#### Position Object
```json
{
  "x": "center" | number,
  "y": "center" | number,
  "anchor": "top-left" | "center" | "bottom-right" (optional)
}
```

Special values:
- `"center"` = POS_SCRCTR (-997)
- Numbers are absolute pixel positions

#### Button Object
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | Yes | Unique button identifier |
| `type` | enum | Yes | Button type: NormalBtn, HoldableBtn, ToggleBtn, RadioBtn, HorizSlider, EditBox, Hotspot |
| `position` | object | Yes | Button position |
| `size` | object | Yes | Button dimensions |
| `visual` | object | No | Visual appearance settings |
| `text` | object | No | Text display configuration |
| `state` | object | Yes | Button state flags |
| `callbacks` | object | No | Event handlers |
| `data` | object | No | Custom data for button logic |

#### Visual Object
```json
{
  "sprite_index": number,
  "draw_callback": string,
  "hover_sprite": number (optional)
}
```

#### Callbacks Object
```json
{
  "on_click": {
    "type": "c" | "lua" | "state_change",
    "function": "function_name",  // for type "c"
    "script": "lua_code",         // for type "lua"
    "state": "FeSt_STATE_NAME",   // for type "state_change"
    "params": object (optional)
  },
  "on_hover": { /* same structure */ },
  "on_maintain": { /* same structure */ }
}
```

---

## Runtime Architecture (C/C++)

### Core Components

#### 1. JSON Parser Integration

**File**: `src/ui_definitions.c`

Use **cJSON** library (MIT license, single-file, already C-compatible):

```c
#include "cJSON.h"

struct MenuDefinition {
    char menu_id[64];
    char description[256];
    TbBool position_centered;
    int pos_x, pos_y;
    int width, height;
    struct ButtonDefinition *buttons;
    int button_count;
    char **lua_scripts;
    int lua_script_count;
};

struct ButtonDefinition {
    char id[64];
    TbButtonType type;
    int pos_x, pos_y;
    int width, height;
    unsigned int sprite_index;
    char draw_callback[128];
    TextStringId text_string_id;
    TbBool text_dynamic;
    char text_lua_getter[256];
    TbBool enabled, visible, clickable;
    struct CallbackDefinition on_click;
    struct CallbackDefinition on_hover;
    struct CallbackDefinition on_maintain;
    cJSON *custom_data;
};

struct CallbackDefinition {
    enum { CB_NONE, CB_C_FUNCTION, CB_LUA_SCRIPT, CB_STATE_CHANGE } type;
    union {
        char c_function_name[128];
        char lua_script[1024];
        int target_state;
    };
    cJSON *params;
};

TbBool load_menu_definition(const char *json_path, struct MenuDefinition *def);
void free_menu_definition(struct MenuDefinition *def);
```

#### 2. UI Builder (Dynamic Menu Construction)

```c
struct GuiMenu* build_menu_from_definition(struct MenuDefinition *def);
struct GuiButtonInit* build_button_array(struct ButtonDefinition *buttons, int count);
void register_dynamic_menu(const char *menu_id, struct GuiMenu *menu);
```

**Workflow**:
1. Parse JSON → `MenuDefinition` struct
2. Convert to `GuiButtonInit[]` array
3. Allocate memory for dynamic arrays
4. Register callbacks via function registry
5. Create `GuiMenu` struct
6. Register with existing menu system

#### 3. Callback Registry

Map string names to function pointers:

```c
typedef struct {
    char name[128];
    Gf_Btn_Callback callback;
} CallbackRegistration;

static CallbackRegistration callback_registry[256];
static int callback_count = 0;

void register_ui_callback(const char *name, Gf_Btn_Callback func) {
    strncpy(callback_registry[callback_count].name, name, 127);
    callback_registry[callback_count].callback = func;
    callback_count++;
}

Gf_Btn_Callback get_ui_callback(const char *name) {
    for (int i = 0; i < callback_count; i++) {
        if (strcmp(callback_registry[i].name, name) == 0) {
            return callback_registry[i].callback;
        }
    }
    return NULL;
}

// During game initialization:
void register_all_ui_callbacks(void) {
    register_ui_callback("frontend_change_state", &frontend_change_state);
    register_ui_callback("frontend_load_continue_game", &frontend_load_continue_game);
    register_ui_callback("frontend_draw_large_menu_button", (Gf_Btn_Callback)&frontend_draw_large_menu_button);
    // ... register all existing callbacks
}
```

#### 4. String ID Resolver

Map string names to `TextStringId` enums:

```c
TextStringId resolve_string_id(const char *name) {
    if (strcmp(name, "GUIStr_MnuContinueGame") == 0) return GUIStr_MnuContinueGame;
    if (strcmp(name, "GUIStr_MnuLoadGame") == 0) return GUIStr_MnuLoadGame;
    // ... auto-generate from config_strings.h
    return GUIStr_Empty;
}
```

**Build Process**: Generate `string_id_map.c` from `config_strings.h` via script.

---

## Lua Scripting Layer

### Lua VM Integration

KeeperFX already has Lua integration. Extend it for UI callbacks:

**File**: `src/lua_ui.c`

```c
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

// Execute Lua button callback
void execute_button_lua_callback(struct GuiButton *gbtn, const char *script, cJSON *params) {
    lua_State *L = get_game_lua_state();
    
    // Push button userdata
    lua_pushlightuserdata(L, gbtn);
    lua_setglobal(L, "current_button");
    
    // Push parameters
    if (params) {
        push_cjson_to_lua(L, params);
        lua_setglobal(L, "params");
    }
    
    // Execute script
    if (luaL_dostring(L, script) != LUA_OK) {
        ERRORLOG("Lua UI callback error: %s", lua_tostring(L, -1));
    }
}
```

### Lua API for UI

Expose UI functions to Lua:

```lua
-- State management
frontend_set_state(state_name)
get_current_state()

-- Button manipulation
button_set_enabled(button_id, enabled)
button_set_visible(button_id, visible)
button_set_text(button_id, text)

-- Campaign data access
get_available_campaigns()
get_campaign_progress(campaign_fname)
set_active_campaign(campaign_idx)
get_campaign_save_count(campaign_fname)

-- Save game data
get_save_game_catalogue()
load_save_game(slot_num)

-- Drawing helpers
draw_text(x, y, text, alignment)
draw_sprite(x, y, sprite_idx)
get_button_bounds(button_id)

-- Player input
is_mouse_over_button(button_id)
get_mouse_position()
```

### Example Lua Scripts

**File**: `data/menus/campaigns/main.lua`

```lua
-- Campaign selection logic
function on_campaign_click(campaign_index)
    set_active_campaign(campaign_index)
    frontend_set_state("FeSt_CAMPAIGN_OPTIONS")
end

-- Dynamic campaign button drawing
function draw_campaign_button(button)
    local campaign_idx = button.data.campaign_index
    local campaigns = get_available_campaigns()
    local campaign = campaigns[campaign_idx + 1]  -- Lua 1-indexed
    
    if not campaign then
        button_set_visible(button.id, false)
        return
    end
    
    -- Draw campaign name
    draw_text(button.x + 10, button.y + 8, campaign.display_name, "left")
    
    -- Show progress indicator
    local progress = get_campaign_progress(campaign.fname)
    if progress > 0 then
        local progress_text = string.format("Level %d", progress)
        draw_text(button.x + button.width - 80, button.y + 8, progress_text, "right")
        
        -- Draw save count
        local save_count = get_campaign_save_count(campaign.fname)
        if save_count > 0 then
            local save_text = string.format("%d saves", save_count)
            draw_text(button.x + button.width - 80, button.y + 20, save_text, "right")
        end
    else
        draw_text(button.x + button.width - 80, button.y + 8, "New", "right")
    end
end

-- Button state maintenance
function maintain_campaign_button(button)
    local campaign_idx = button.data.campaign_index
    local campaigns = get_available_campaigns()
    
    -- Hide if no campaign at this index
    if campaign_idx >= #campaigns then
        button_set_enabled(button.id, false)
        button_set_visible(button.id, false)
    else
        button_set_enabled(button.id, true)
        button_set_visible(button.id, true)
    end
end
```

---

## VSCode Extension Design

### Extension Structure

```
keeperfx-devtools/
├── package.json
├── src/
│   ├── extension.ts           // Extension entry point
│   ├── ui-editor/
│   │   ├── UIEditorProvider.ts       // Custom editor provider
│   │   ├── UIEditorPanel.ts          // Webview panel
│   │   ├── webview/                  // Webview UI (React)
│   │   │   ├── index.tsx
│   │   │   ├── components/
│   │   │   │   ├── Canvas.tsx        // Main editing canvas
│   │   │   │   ├── ButtonInspector.tsx  // Properties panel
│   │   │   │   ├── MenuTree.tsx      // Menu structure tree
│   │   │   │   ├── SpriteViewer.tsx  // Sprite picker
│   │   │   │   └── LuaEditor.tsx     // Code editor for Lua
│   │   │   ├── hooks/
│   │   │   │   ├── useMenuDefinition.ts
│   │   │   │   └── useSpriteLoader.ts
│   │   │   └── utils/
│   │   │       ├── spriteLoader.ts   // Load .dat files
│   │   │       └── validation.ts     // JSON schema validation
│   │   └── models/
│   │       ├── MenuDefinition.ts     // TypeScript interfaces
│   │       └── ButtonDefinition.ts
│   ├── sprite-viewer/
│   │   ├── SpriteTreeProvider.ts     // Tree view of sprites
│   │   └── SpriteParser.ts           // Parse .dat/.tab files
│   └── commands/
│       ├── createMenu.ts
│       ├── exportMenu.ts
│       └── validateMenu.ts
├── media/
│   ├── styles.css
│   └── icons/
└── schemas/
    └── menu-definition.schema.json   // JSON Schema
```

### Key Components

#### 1. Custom Editor Provider

```typescript
export class UIEditorProvider implements vscode.CustomTextEditorProvider {
    public static register(context: vscode.ExtensionContext): vscode.Disposable {
        const provider = new UIEditorProvider(context);
        return vscode.window.registerCustomEditorProvider(
            'keeperfx.uiEditor',
            provider,
            { webviewOptions: { retainContextWhenHidden: true } }
        );
    }

    async resolveCustomTextEditor(
        document: vscode.TextDocument,
        webviewPanel: vscode.WebviewPanel,
        token: vscode.CancellationToken
    ): Promise<void> {
        // Setup webview
        webviewPanel.webview.options = { enableScripts: true };
        webviewPanel.webview.html = this.getHtmlForWebview(webviewPanel.webview);
        
        // Parse menu JSON
        const menuDef = JSON.parse(document.getText());
        
        // Send to webview
        webviewPanel.webview.postMessage({ 
            type: 'loadMenu', 
            menu: menuDef 
        });
        
        // Handle updates from webview
        webviewPanel.webview.onDidReceiveMessage(async (message) => {
            switch (message.type) {
                case 'updateMenu':
                    await this.updateDocument(document, message.menu);
                    break;
                case 'validateMenu':
                    const errors = await this.validateMenu(message.menu);
                    webviewPanel.webview.postMessage({ 
                        type: 'validationResult', 
                        errors 
                    });
                    break;
            }
        });
    }
}
```

#### 2. Canvas Editor (React)

```typescript
interface CanvasProps {
    menu: MenuDefinition;
    selectedButtonId?: string;
    onButtonSelect: (buttonId: string) => void;
    onButtonMove: (buttonId: string, x: number, y: number) => void;
    onButtonResize: (buttonId: string, width: number, height: number) => void;
}

export const Canvas: React.FC<CanvasProps> = ({ 
    menu, 
    selectedButtonId, 
    onButtonSelect,
    onButtonMove,
    onButtonResize 
}) => {
    const canvasRef = useRef<HTMLCanvasElement>(null);
    const [dragState, setDragState] = useState<DragState | null>(null);
    
    useEffect(() => {
        const canvas = canvasRef.current;
        if (!canvas) return;
        
        const ctx = canvas.getContext('2d');
        renderMenu(ctx, menu, selectedButtonId);
    }, [menu, selectedButtonId]);
    
    const handleMouseDown = (e: React.MouseEvent) => {
        const rect = canvasRef.current?.getBoundingClientRect();
        if (!rect) return;
        
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;
        
        // Check which button was clicked
        const clickedButton = findButtonAtPosition(menu.buttons, x, y);
        if (clickedButton) {
            onButtonSelect(clickedButton.id);
            setDragState({ 
                buttonId: clickedButton.id, 
                startX: x, 
                startY: y 
            });
        }
    };
    
    const handleMouseMove = (e: React.MouseEvent) => {
        if (!dragState) return;
        
        const rect = canvasRef.current?.getBoundingClientRect();
        if (!rect) return;
        
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;
        
        const deltaX = x - dragState.startX;
        const deltaY = y - dragState.startY;
        
        onButtonMove(dragState.buttonId, deltaX, deltaY);
    };
    
    return (
        <div className="canvas-container">
            <canvas
                ref={canvasRef}
                width={menu.size.width}
                height={menu.size.height}
                onMouseDown={handleMouseDown}
                onMouseMove={handleMouseMove}
                onMouseUp={() => setDragState(null)}
                className="ui-canvas"
            />
            <ResizeHandles 
                button={getSelectedButton(menu, selectedButtonId)}
                onResize={onButtonResize}
            />
        </div>
    );
};
```

#### 3. Button Inspector (Properties Panel)

```typescript
export const ButtonInspector: React.FC<{ 
    button: ButtonDefinition;
    onChange: (updated: ButtonDefinition) => void;
}> = ({ button, onChange }) => {
    const [sprites, setSprites] = useState<SpriteInfo[]>([]);
    const [callbacks, setCallbacks] = useState<string[]>([]);
    
    useEffect(() => {
        // Load available sprites from game data
        loadSprites().then(setSprites);
        // Load registered callbacks
        loadCallbacks().then(setCallbacks);
    }, []);
    
    return (
        <div className="inspector-panel">
            <h3>Button Properties</h3>
            
            <PropertyGroup label="Identification">
                <TextInput 
                    label="ID" 
                    value={button.id}
                    onChange={(id) => onChange({ ...button, id })}
                />
                <Select
                    label="Type"
                    value={button.type}
                    options={['NormalBtn', 'HoldableBtn', 'ToggleBtn', 'RadioBtn']}
                    onChange={(type) => onChange({ ...button, type })}
                />
            </PropertyGroup>
            
            <PropertyGroup label="Position">
                <NumberInput
                    label="X"
                    value={button.position.x}
                    onChange={(x) => onChange({
                        ...button,
                        position: { ...button.position, x }
                    })}
                />
                <NumberInput
                    label="Y"
                    value={button.position.y}
                    onChange={(y) => onChange({
                        ...button,
                        position: { ...button.position, y }
                    })}
                />
            </PropertyGroup>
            
            <PropertyGroup label="Size">
                <NumberInput
                    label="Width"
                    value={button.size.width}
                    onChange={(width) => onChange({
                        ...button,
                        size: { ...button.size, width }
                    })}
                />
                <NumberInput
                    label="Height"
                    value={button.size.height}
                    onChange={(height) => onChange({
                        ...button,
                        size: { ...button.size, height }
                    })}
                />
            </PropertyGroup>
            
            <PropertyGroup label="Visual">
                <SpriteSelector
                    sprites={sprites}
                    selected={button.visual?.sprite_index}
                    onChange={(sprite_index) => onChange({
                        ...button,
                        visual: { ...button.visual, sprite_index }
                    })}
                />
                <Select
                    label="Draw Callback"
                    value={button.visual?.draw_callback}
                    options={callbacks.filter(c => c.startsWith('frontend_draw_'))}
                    onChange={(draw_callback) => onChange({
                        ...button,
                        visual: { ...button.visual, draw_callback }
                    })}
                />
            </PropertyGroup>
            
            <PropertyGroup label="Text">
                <StringIdSelector
                    value={button.text?.string_id}
                    onChange={(string_id) => onChange({
                        ...button,
                        text: { ...button.text, string_id }
                    })}
                />
            </PropertyGroup>
            
            <PropertyGroup label="Callbacks">
                <CallbackEditor
                    label="On Click"
                    callback={button.callbacks?.on_click}
                    availableCallbacks={callbacks}
                    onChange={(on_click) => onChange({
                        ...button,
                        callbacks: { ...button.callbacks, on_click }
                    })}
                />
            </PropertyGroup>
            
            <PropertyGroup label="State">
                <Checkbox
                    label="Enabled"
                    checked={button.state?.enabled}
                    onChange={(enabled) => onChange({
                        ...button,
                        state: { ...button.state, enabled }
                    })}
                />
                <Checkbox
                    label="Visible"
                    checked={button.state?.visible}
                    onChange={(visible) => onChange({
                        ...button,
                        state: { ...button.state, visible }
                    })}
                />
            </PropertyGroup>
        </div>
    );
};
```

#### 4. Sprite Loader

```typescript
export class SpriteLoader {
    async loadSpriteSheet(datPath: string, tabPath: string): Promise<SpriteSheet> {
        // Read .dat file (bitmap data)
        const datBuffer = await vscode.workspace.fs.readFile(vscode.Uri.file(datPath));
        
        // Read .tab file (sprite metadata)
        const tabBuffer = await vscode.workspace.fs.readFile(vscode.Uri.file(tabPath));
        const tabView = new DataView(tabBuffer.buffer);
        
        const sprites: Sprite[] = [];
        let offset = 0;
        
        while (offset < tabBuffer.length) {
            const sprite = {
                width: tabView.getUint16(offset, true),
                height: tabView.getUint16(offset + 2, true),
                dataOffset: tabView.getUint32(offset + 4, true),
                dataLength: tabView.getUint32(offset + 8, true)
            };
            sprites.push(sprite);
            offset += 12;  // Size of sprite metadata entry
        }
        
        return { datBuffer, sprites };
    }
    
    async renderSprite(
        sheet: SpriteSheet, 
        spriteIndex: number,
        canvas: HTMLCanvasElement
    ): Promise<void> {
        const sprite = sheet.sprites[spriteIndex];
        const ctx = canvas.getContext('2d');
        if (!ctx) return;
        
        canvas.width = sprite.width;
        canvas.height = sprite.height;
        
        // Extract sprite data and render
        const spriteData = new Uint8Array(
            sheet.datBuffer.buffer,
            sprite.dataOffset,
            sprite.dataLength
        );
        
        const imageData = ctx.createImageData(sprite.width, sprite.height);
        // Convert paletted data to RGBA
        for (let i = 0; i < spriteData.length; i++) {
            const paletteIndex = spriteData[i];
            imageData.data[i * 4 + 0] = this.palette[paletteIndex * 3 + 0];  // R
            imageData.data[i * 4 + 1] = this.palette[paletteIndex * 3 + 1];  // G
            imageData.data[i * 4 + 2] = this.palette[paletteIndex * 3 + 2];  // B
            imageData.data[i * 4 + 3] = 255;  // A
        }
        
        ctx.putImageData(imageData, 0, 0);
    }
    
    private palette =  [/* 256 RGB triplets from game palette */];
}
```

---

## Migration Strategy

### Phase 1: Foundation (Week 1-2)
- [ ] Add cJSON library to KeeperFX
- [ ] Implement JSON parser for menu definitions
- [ ] Create callback registry system
- [ ] Build dynamic menu construction
- [ ] Test with simple single-button menu

### Phase 2: VSCode Extension (Week 2-3)
- [ ] Setup extension project structure
- [ ] Build basic Canvas editor
- [ ] Implement Button Inspector
- [ ] Create sprite loader for .dat files
- [ ] Add JSON export/import

### Phase 3: Lua Integration (Week 3-4)
- [ ] Expose UI functions to Lua
- [ ] Implement Lua callback execution
- [ ] Add Lua editor to VSCode extension
- [ ] Create example Lua scripts

### Phase 4: Pilot Implementation (Week 4-5)
- [ ] Convert Campaigns menu to JSON
- [ ] Implement campaign list logic in Lua
- [ ] Test visual editor workflow
- [ ] Document process for modders

### Phase 5: Validation & Polish (Week 5-6)
- [ ] Add JSON schema validation
- [ ] Performance testing
- [ ] Create tutorial documentation
- [ ] Community feedback iteration

---

## Performance Considerations

### Memory Management
- **JSON parsing**: One-time cost at menu initialization (~1-5ms per menu)
- **Dynamic arrays**: Allocate once, reuse throughout session
- **Lua scripts**: Pre-compile to bytecode, cache in memory
- **Expected overhead**: <1% compared to hardcoded arrays

### Optimization Strategies
1. **Lazy loading**: Load menu definitions only when needed
2. **Caching**: Keep parsed menus in memory after first load
3. **Precompilation**: Option to compile JSON to C arrays for release builds
4. **Fallback**: Keep critical menus (main menu) in C as backup

### Hot Reload (Development Only)
```c
#ifdef DEBUG_MODE
void reload_menu_definition(const char *menu_id) {
    struct MenuDefinition def;
    char path[256];
    sprintf(path, "data/menus/%s.json", menu_id);
    
    if (load_menu_definition(path, &def)) {
        // Rebuild menu dynamically
        struct GuiMenu *menu = build_menu_from_definition(&def);
        register_dynamic_menu(menu_id, menu);
        SYNCLOG("Reloaded menu: %s", menu_id);
    }
}

// Triggered by F5 key or file watcher
void check_for_ui_updates(void) {
    // Check file modification times
    // Reload changed menus
}
#endif
```

---

## Backward Compatibility

### Hybrid System
- **New menus**: Use JSON definitions
- **Existing menus**: Keep C arrays (70+ existing menus)
- **Menu registration**: Check for `.json` file first, fallback to C array

```c
struct GuiMenu* get_menu(const char *menu_id) {
    // Try JSON definition first
    char json_path[256];
    sprintf(json_path, "data/menus/%s.json", menu_id);
    
    if (file_exists(json_path)) {
        struct MenuDefinition def;
        if (load_menu_definition(json_path, &def)) {
            return build_menu_from_definition(&def);
        }
    }
    
    // Fallback to hardcoded C array
    return get_legacy_menu(menu_id);
}
```

### Conversion Tool
VSCode extension provides command to convert C arrays to JSON:

```typescript
// Parse C source code → extract GuiButtonInit array → generate JSON
async function convertCMenuToJson(cSourcePath: string): Promise<MenuDefinition> {
    const source = await readFile(cSourcePath);
    const ast = parseCSource(source);
    const buttonArray = extractButtonArray(ast, 'frontend_main_menu_buttons');
    return generateJsonFromButtons(buttonArray);
}
```

---

## Security & Validation

### JSON Schema Validation
- Validate menu definitions against schema before loading
- Reject malformed JSON to prevent crashes
- Type checking for all fields

### Lua Sandboxing
- Restrict dangerous Lua functions (file I/O, system calls)
- Timeout protection for infinite loops
- Memory limits per script

### Error Handling
```c
TbBool load_menu_definition_safe(const char *json_path, struct MenuDefinition *def) {
    cJSON *json = NULL;
    
    // Read file
    char *file_content = read_file(json_path);
    if (!file_content) {
        ERRORLOG("Failed to read menu file: %s", json_path);
        return false;
    }
    
    // Parse JSON
    json = cJSON_Parse(file_content);
    if (!json) {
        ERRORLOG("JSON parse error in %s: %s", json_path, cJSON_GetErrorPtr());
        free(file_content);
        return false;
    }
    
    // Validate required fields
    if (!cJSON_HasObjectItem(json, "menu_id") || 
        !cJSON_HasObjectItem(json, "buttons")) {
        ERRORLOG("Invalid menu definition: missing required fields");
        cJSON_Delete(json);
        free(file_content);
        return false;
    }
    
    // Parse definition
    TbBool success = parse_menu_definition(json, def);
    
    cJSON_Delete(json);
    free(file_content);
    return success;
}
```

---

## Documentation Requirements

### For Modders
1. **UI Definition Format Guide**: Complete JSON schema reference
2. **Lua Scripting API**: All exposed functions with examples
3. **Visual Editor Tutorial**: Step-by-step walkthrough
4. **Sprite Reference**: List of all available sprites with previews
5. **Example Menus**: Complete working examples for common patterns

### For Developers
1. **Architecture Overview**: This document
2. **API Reference**: C function documentation
3. **Contributing Guide**: How to add new callbacks/features
4. **Performance Profiling**: Benchmarking methodology

---

## Future Enhancements

### Potential Additions
- **Animation support**: Button transitions, hover effects
- **Layout system**: Flexbox-like automatic positioning
- **Theme system**: Color schemes, style presets
- **Component library**: Reusable button templates
- **Live preview**: See changes in running game instantly
- **Multiplayer UI**: Network-aware dynamic menus
- **Accessibility**: Screen reader support, high contrast mode

### Community Contributions
Once stable, open-source the VSCode extension and accept community PRs for:
- New UI components
- Additional Lua APIs
- Sprite packs
- Menu templates
- Localization improvements

---

## Success Metrics

### Technical Goals
- ✅ JSON menu loads in <5ms
- ✅ No memory leaks after 1000 menu transitions
- ✅ Lua scripts execute in <1ms per frame
- ✅ 100% backward compatibility with existing menus

### User Experience Goals
- ✅ Non-programmer can create basic menu in <30 minutes
- ✅ Visual editor workflow faster than C editing
- ✅ Modders adopt system for custom campaigns
- ✅ Community creates UI component library

---

## Conclusion

This data-driven UI system modernizes KeeperFX's menu architecture while maintaining performance and compatibility. The visual editor democratizes UI design, enabling non-programmers to contribute. Lua scripting provides flexibility without C recompilation. The pilot implementation (Campaigns menu) validates the approach before full migration.

**Next Steps**: Prototype VSCode extension to demonstrate feasibility and gather feedback before implementing C runtime.

---

## Appendix A: JSON Schema

See `schemas/menu-definition.schema.json` (will be created during implementation)

## Appendix B: Complete Example Menu

See `examples/campaigns-menu.json` (will be created during implementation)

## Appendix C: Callback Registry

See `docs/callback-reference.md` (to be generated from source)
