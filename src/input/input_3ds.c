/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file input_3ds.c
 *     Nintendo 3DS input implementation.
 * @par Purpose:
 *     Provides input interface implementation for Nintendo 3DS controls.
 * @par Comment:
 *     Maps 3DS buttons and touchscreen to game controls.
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

#ifdef PLATFORM_3DS

#include <3ds.h>
#include "../bflib_keybrd.h"
#include "../post_inc.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/

// Button mapping from 3DS to KeeperFX keycodes
static const struct {
    u32 btn_3ds;
    int keycode;
} s_buttonMap[] = {
    { KEY_A,      KC_RETURN },     // A button = Enter
    { KEY_B,      KC_ESCAPE },     // B button = Escape
    { KEY_X,      KC_SPACE },      // X button = Space
    { KEY_Y,      KC_TAB },        // Y button = Tab
    { KEY_L,      KC_LSHIFT },     // L shoulder = Left Shift
    { KEY_R,      KC_LCONTROL },   // R shoulder = Left Control
    { KEY_START,  KC_ESCAPE },     // Start = Escape
    { KEY_SELECT, KC_M },          // Select = M (map)
    { KEY_DUP,    KC_UP },         // D-pad Up
    { KEY_DDOWN,  KC_DOWN },       // D-pad Down
    { KEY_DLEFT,  KC_LEFT },       // D-pad Left
    { KEY_DRIGHT, KC_RIGHT },      // D-pad Right
};

#define BUTTON_MAP_SIZE (sizeof(s_buttonMap) / sizeof(s_buttonMap[0]))

/******************************************************************************/
// Global state
static u32 s_keysHeld = 0;
static u32 s_keysDown = 0;
static u32 s_keysUp = 0;
static touchPosition s_touch;
static bool s_touchActive = false;
static int s_mouseX = 0;
static int s_mouseY = 0;
static int s_mouseButtons = 0;
static circlePosition s_circlePos;

// Virtual keyboard state
static unsigned char s_keyState[KC_LIST_END];

/******************************************************************************/
// Helper functions

static void update_key_states(void)
{
    // Clear all keys
    memset(s_keyState, 0, KC_LIST_END);

    // Map 3DS buttons to keyboard keys
    for (int i = 0; i < BUTTON_MAP_SIZE; i++) {
        if (s_keysHeld & s_buttonMap[i].btn_3ds) {
            s_keyState[s_buttonMap[i].keycode] = 1;
        }
    }
}

static void update_mouse_state(void)
{
    // Use touchscreen as mouse input
    if (s_keysHeld & KEY_TOUCH) {
        hidTouchRead(&s_touch);
        s_touchActive = true;
        
        // Map touch coordinates to screen space (400x240 top screen)
        // Touch screen is 320x240, so scale X coordinate
        s_mouseX = (s_touch.px * 400) / 320;
        s_mouseY = s_touch.py;
        
        s_mouseButtons = INPUT_MOUSE_BUTTON_LEFT;
    } else {
        if (s_touchActive) {
            // Touch released
            s_mouseButtons = 0;
            s_touchActive = false;
        }
    }

    // Alternative: Use circle pad to move cursor
    hidCircleRead(&s_circlePos);
    if (abs(s_circlePos.dx) > 20 || abs(s_circlePos.dy) > 20) {
        s_mouseX += s_circlePos.dx / 30;
        s_mouseY -= s_circlePos.dy / 30;
        
        // Clamp to screen bounds
        if (s_mouseX < 0) s_mouseX = 0;
        if (s_mouseX >= 400) s_mouseX = 399;
        if (s_mouseY < 0) s_mouseY = 0;
        if (s_mouseY >= 240) s_mouseY = 239;
    }

    // A button acts as left click when not touching
    if (!s_touchActive && (s_keysHeld & KEY_A)) {
        s_mouseButtons |= INPUT_MOUSE_BUTTON_LEFT;
    }

    // R shoulder acts as right click
    if (s_keysHeld & KEY_R) {
        s_mouseButtons |= INPUT_MOUSE_BUTTON_RIGHT;
    }
}

/******************************************************************************/
// Input interface implementation

static void input_3ds_poll_events(void)
{
    // Scan for input
    hidScanInput();
    
    // Get button states
    s_keysHeld = hidKeysHeld();
    s_keysDown = hidKeysDown();
    s_keysUp = hidKeysUp();

    // Update virtual keyboard state
    update_key_states();

    // Update mouse state from touch/circle pad
    update_mouse_state();
}

static TbBool input_3ds_is_key_down(int keycode)
{
    if (keycode < 0 || keycode >= KC_LIST_END)
        return false;
    return s_keyState[keycode];
}

static void input_3ds_get_mouse(int* x, int* y, int* buttons)
{
    if (x != NULL)
        *x = s_mouseX;
    if (y != NULL)
        *y = s_mouseY;
    if (buttons != NULL)
        *buttons = s_mouseButtons;
}

static TbBool input_3ds_get_gamepad_axis(int axis, int* value)
{
    if (value == NULL)
        return false;

    // Map circle pad to gamepad axes
    switch (axis) {
        case 0:  // Left X axis
            *value = s_circlePos.dx * 256;
            return true;
        case 1:  // Left Y axis
            *value = -s_circlePos.dy * 256;  // Invert Y
            return true;
        default:
            *value = 0;
            return false;
    }
}

static InputInterface input_3ds_impl = {
    input_3ds_poll_events,
    input_3ds_is_key_down,
    input_3ds_get_mouse,
    input_3ds_get_gamepad_axis,
};

void input_3ds_initialize(void)
{
    // Initialize input state
    memset(s_keyState, 0, KC_LIST_END);
    s_mouseX = 200;  // Center of screen
    s_mouseY = 120;
    s_mouseButtons = 0;

    g_input = &input_3ds_impl;
}

/******************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // PLATFORM_3DS
