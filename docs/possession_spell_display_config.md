# Possession Spell Display Configuration

## Overview
The spell display in possession mode can be customized through the `keeperfx.cfg` configuration file.

## Configuration Option

Add this line to your `keeperfx.cfg` file:

```
POSSESSION_SPELL_DISPLAY = <mode>
```

## Available Modes

| Mode | Description |
|------|-------------|
| `OFF` | Completely disable spell indicators in possession mode |
| `ICONS_ONLY` | Show only spell icons without any duration information (original behavior) |
| `TEXT_ABOVE` | Show spell duration in seconds above each icon (**default**) |
| `TEXT_BELOW` | Show spell duration in seconds below each icon |
| `PROGRESS_BAR` | Show a visual progress bar underneath each icon that depletes as the spell expires |

## Examples

### Default Configuration (Text Above)
```
POSSESSION_SPELL_DISPLAY = TEXT_ABOVE
```

### Icons Only (Original Behavior)
```
POSSESSION_SPELL_DISPLAY = ICONS_ONLY
```

### Progress Bar Mode
```
POSSESSION_SPELL_DISPLAY = PROGRESS_BAR
```

### Disable Spell Display
```
POSSESSION_SPELL_DISPLAY = OFF
```

## Progress Bar Details

When using `PROGRESS_BAR` mode:
- A green progress bar is drawn underneath each spell icon
- The bar starts full and depletes as the spell duration decreases
- Provides visual feedback without numeric countdown
- Bar width matches the icon width
- 3 pixels tall for visibility without being intrusive

## Integration Notes

For developers with ImGui integration:
- The `possession_spell_display_mode` variable is globally accessible
- Can be modified at runtime and takes effect immediately
- Type: `unsigned char`
- Values: 0-4 (corresponding to the modes above)
- Defined in: `src/config_keeperfx.h`

Example ImGui integration:
```cpp
const char* modes[] = { "Off", "Icons Only", "Text Above", "Text Below", "Progress Bar" };
ImGui::Combo("Possession Spell Display", (int*)&possession_spell_display_mode, modes, 5);
```

## Default Value

If not specified in `keeperfx.cfg`, the default mode is `TEXT_ABOVE` (mode 2).

## Changelog

- v1.0: Initial implementation with 5 display modes
- Added progress bar visualization for spell duration
- Full configuration support via keeperfx.cfg
