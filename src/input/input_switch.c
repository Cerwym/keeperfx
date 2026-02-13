/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file input_switch.c
 *     Nintendo Switch input implementation.
 * @par Purpose:
 *     Provides input interface implementation for Nintendo Switch controls.
 * @par Comment:
 *     Maps Joy-Con buttons, analog sticks, and touchscreen to game controls.
 * @author   KeeperFX Team
 * @date     12 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "../pre_inc.h"
#include "input_interface.h"

#ifdef PLATFORM_SWITCH

#include <switch.h>
#include <string.h>

#include "../bflib_keybrd.h"
#include "../post_inc.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/

// Button mapping from Switch to KeeperFX keycodes
static const struct {
    u64 btn_switch;
    int keycode;
} s_buttonMap[] = {
    { HidNpadButton_A,      KC_RETURN },      // A button = Enter
    { HidNpadButton_B,      KC_ESCAPE },      // B button = Escape
    { HidNpadButton_X,      KC_SPACE },       // X button = Space
    { HidNpadButton_Y,      KC_TAB },         // Y button = Tab
    { HidNpadButton_L,      KC_LSHIFT },      // L shoulder = Left Shift
    { HidNpadButton_R,      KC_LCONTROL },    // R shoulder = Left Control
    { HidNpadButton_ZL,     KC_Q },           // ZL = Q
    { HidNpadButton_ZR,     KC_E },           // ZR = E
    { HidNpadButton_Plus,   KC_ESCAPE },      // Plus (Start) = Escape
    { HidNpadButton_Minus,  KC_M },           // Minus (Select) = M (map)
    { HidNpadButton_Up,     KC_UP },          // D-pad Up
    { HidNpadButton_Down,   KC_DOWN },        // D-pad Down
    { HidNpadButton_Left,   KC_LEFT },        // D-pad Left
    { HidNpadButton_Right,  KC_RIGHT },       // D-pad Right
};

#define BUTTON_MAP_SIZE (sizeof(s_buttonMap) / sizeof(s_buttonMap[0]))

/******************************************************************************/
// Global state
static PadState s_pad;
static HidTouchScreenState s_touchState;
static bool s_touchActive = false;
static int s_mouseX = 640;  // Center of 1280x720 screen
static int s_mouseY = 360;
static int s_mouseButtons = 0;

// Virtual keyboard state
static unsigned char s_keyState[KC_LIST_END];

/******************************************************************************/
// Helper functions

static void update_key_states(u64 keysHeld)
{
    // Clear all keys
    memset(s_keyState, 0, KC_LIST_END);

    // Map Switch buttons to keyboard keys
    for (int i = 0; i < BUTTON_MAP_SIZE; i++) {
        if (keysHeld & s_buttonMap[i].btn_switch) {
            s_keyState[s_buttonMap[i].keycode] = 1;
        }
    }
}

static void update_mouse_from_touch(void)
{
    // Read touch screen state
    if (hidGetTouchScreenStates(&s_touchState, 1)) {
        if (s_touchState.count > 0) {
            s_touchActive = true;
            
            // Get first touch point
            s_mouseX = s_touchState.touches[0].x;
            s_mouseY = s_touchState.touches[0].y;
            
            s_mouseButtons = INPUT_MOUSE_BUTTON_LEFT;
        } else {
            if (s_touchActive) {
                // Touch released
                s_mouseButtons = 0;
                s_touchActive = false;
            }
        }
    }
}

static void update_mouse_from_analog(void)
{
    // Get left stick position
    HidAnalogStickState stick_l = padGetStickPos(&s_pad, 0);  // Left stick
    
    // Apply deadzone
    const int deadzone = 5000;
    int lx = stick_l.x;
    int ly = stick_l.y;
    
    if (abs(lx) < deadzone) lx = 0;
    if (abs(ly) < deadzone) ly = 0;

    // Move cursor based on analog stick
    if (lx != 0 || ly != 0) {
        s_mouseX += lx / 2000;
        s_mouseY -= ly / 2000;  // Invert Y

        // Clamp to screen bounds (1280x720 in handheld, 1920x1080 in docked)
        // Using 1280x720 as default
        if (s_mouseX < 0) s_mouseX = 0;
        if (s_mouseX >= 1280) s_mouseX = 1279;
        if (s_mouseY < 0) s_mouseY = 0;
        if (s_mouseY >= 720) s_mouseY = 719;
    }

    // Get button state
    u64 keysHeld = padGetButtons(&s_pad);

    // A button acts as left click
    if (keysHeld & HidNpadButton_A) {
        s_mouseButtons |= INPUT_MOUSE_BUTTON_LEFT;
    }

    // B button acts as right click
    if (keysHeld & HidNpadButton_B) {
        s_mouseButtons |= INPUT_MOUSE_BUTTON_RIGHT;
    }
}

/******************************************************************************/
// Input interface implementation

static void input_switch_poll_events(void)
{
    // Update pad state
    padUpdate(&s_pad);
    u64 keysHeld = padGetButtons(&s_pad);

    // Update virtual keyboard state
    update_key_states(keysHeld);

    // Update mouse state from touch or analog stick
    // Priority: touch > analog stick
    s_mouseButtons = 0;
    update_mouse_from_touch();
    if (!s_touchActive) {
        update_mouse_from_analog();
    }
}

static TbBool input_switch_is_key_down(int keycode)
{
    if (keycode < 0 || keycode >= KC_LIST_END)
        return false;
    return s_keyState[keycode];
}

static void input_switch_get_mouse(int* x, int* y, int* buttons)
{
    if (x != NULL)
        *x = s_mouseX;
    if (y != NULL)
        *y = s_mouseY;
    if (buttons != NULL)
        *buttons = s_mouseButtons;
}

static TbBool input_switch_get_gamepad_axis(int axis, int* value)
{
    if (value == NULL)
        return false;

    HidAnalogStickState stick_l = padGetStickPos(&s_pad, 0);  // Left stick
    HidAnalogStickState stick_r = padGetStickPos(&s_pad, 1);  // Right stick

    int raw_value = 0;
    bool valid = false;

    switch (axis) {
        case 0:  // Left X axis
            raw_value = stick_l.x;
            valid = true;
            break;
        case 1:  // Left Y axis
            raw_value = -stick_l.y;  // Invert Y
            valid = true;
            break;
        case 2:  // Right X axis
            raw_value = stick_r.x;
            valid = true;
            break;
        case 3:  // Right Y axis
            raw_value = -stick_r.y;  // Invert Y
            valid = true;
            break;
        default:
            *value = 0;
            return false;
    }

    // Apply deadzone
    const int deadzone = 5000;
    if (abs(raw_value) < deadzone)
        raw_value = 0;

    // Stick values are -32768 to 32767, which matches expected range
    *value = raw_value;
    return valid;
}

static InputInterface input_switch_impl = {
    input_switch_poll_events,
    input_switch_is_key_down,
    input_switch_get_mouse,
    input_switch_get_gamepad_axis,
};

void input_switch_initialize(void)
{
    // Configure input
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    
    // Initialize pad state
    padInitializeDefault(&s_pad);

    // Initialize touch screen
    hidInitializeTouchScreen();

    // Initialize input state
    memset(s_keyState, 0, KC_LIST_END);
    s_mouseX = 640;  // Center of screen
    s_mouseY = 360;
    s_mouseButtons = 0;

    g_input = &input_switch_impl;
}

/******************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // PLATFORM_SWITCH
