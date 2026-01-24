/******************************************************************************/
// KeeperFX - Input Manager
/******************************************************************************/
/** @file input_manager.cpp
 *     Modern C++ input abstraction layer implementation.
 * @par Purpose:
 *     Implements context-aware input routing and state management.
 * @author   Community
 * @date     18 Jan 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "input_manager.hpp"

#include "bflib_keybrd.h"
#include "bflib_video.h"

#include "post_inc.h"

/******************************************************************************/
// Private methods

InputManager::InputManager() {
    // Constructor intentionally minimal - use initialize() for setup
}

InputManager::State* InputManager::getContextState(Context context) {
    switch (context) {
    case Context::Game:
        return &gameInput_;
    case Context::UI:
        return &uiInput_;
    case Context::Debug:
        return &debugInput_;
    case Context::Blocked:
    case Context::None:
    default:
        return nullptr;
    }
}

const InputManager::State* InputManager::getContextState(Context context) const {
    return const_cast<InputManager*>(this)->getContextState(context);
}

/******************************************************************************/
// Initialization and management

void InputManager::initialize() {
    if (initialized_) {
        return;
    }
    
    activeContext_ = Context::Game;
    while (!contextStack_.empty()) {
        contextStack_.pop();
    }
    
    gameInput_.clear();
    uiInput_.clear();
    debugInput_.clear();
    rawInput_.clear();
    
    initialized_ = true;
    
    JUSTMSG("Input Manager initialized (C++ version)");
}

void InputManager::shutdown() {
    if (!initialized_) {
        return;
    }
    
    initialized_ = false;
    JUSTMSG("Input Manager shutdown");
}

void InputManager::newFrame() {
    if (!initialized_) {
        return;
    }
    
    // Clear per-frame state (clicks, wheel events)
    gameInput_.clearPerFrameState();
    uiInput_.clearPerFrameState();
    debugInput_.clearPerFrameState();
    rawInput_.clearPerFrameState();
}

/******************************************************************************/
// Context management

void InputManager::pushContext(Context context) {
    if (!initialized_) {
        return;
    }
    
    Context prev = activeContext_;
    (void)prev; // Used in SYNCDBG
    contextStack_.push(activeContext_);
    activeContext_ = context;
    
    SYNCDBG(8, "Input context pushed: %d -> %d (stack depth: %u)", 
            static_cast<int>(prev), 
            static_cast<int>(context),
            static_cast<unsigned int>(contextStack_.size()));
}

void InputManager::popContext() {
    if (!initialized_) {
        return;
    }
    
    if (!contextStack_.empty()) {
        activeContext_ = contextStack_.top();
        contextStack_.pop();
        
        SYNCDBG(8, "Input context popped to %d (stack depth: %u)", 
                static_cast<int>(activeContext_),
                static_cast<unsigned int>(contextStack_.size()));
    } else {
        WARNMSG("Attempted to pop input context with empty stack");
    }
}

void InputManager::setContext(Context context) {
    if (!initialized_) {
        return;
    }
    
    activeContext_ = context;
    
    // Clear stack
    while (!contextStack_.empty()) {
        contextStack_.pop();
    }
    
    SYNCDBG(8, "Input context set directly to: %d", 
            static_cast<int>(context));
}

/******************************************************************************/
// Input state accessors

bool InputManager::isKeyPressed(TbKeyCode key) const {
    if (!initialized_ || key >= KC_LIST_END) {
        return false;
    }
    
    const State* state = getContextState(activeContext_);
    if (!state) {
        return false;
    }
    
    return state->keys[key] != 0;
}

TbKeyCode InputManager::getLastKey() const {
    if (!initialized_) {
        return KC_UNASSIGNED;
    }
    
    const State* state = getContextState(activeContext_);
    if (!state) {
        return KC_UNASSIGNED;
    }
    
    return state->lastKey;
}

void InputManager::clearLastKey() {
    if (!initialized_) {
        return;
    }
    
    State* state = getContextState(activeContext_);
    if (state) {
        state->lastKey = KC_UNASSIGNED;
    }
}

long InputManager::getMouseX() const {
    if (!initialized_) {
        return 0;
    }
    
    const State* state = getContextState(activeContext_);
    return state ? state->mouseX : 0;
}

long InputManager::getMouseY() const {
    if (!initialized_) {
        return 0;
    }
    
    const State* state = getContextState(activeContext_);
    return state ? state->mouseY : 0;
}

bool InputManager::isLeftButtonPressed() const {
    if (!initialized_) {
        return false;
    }
    
    const State* state = getContextState(activeContext_);
    return state ? state->leftButton : false;
}

bool InputManager::isRightButtonPressed() const {
    if (!initialized_) {
        return false;
    }
    
    const State* state = getContextState(activeContext_);
    return state ? state->rightButton : false;
}

bool InputManager::isMiddleButtonPressed() const {
    if (!initialized_) {
        return false;
    }
    
    const State* state = getContextState(activeContext_);
    return state ? state->middleButton : false;
}

bool InputManager::isWheelUp() const {
    if (!initialized_) {
        return false;
    }
    
    const State* state = getContextState(activeContext_);
    return state ? state->wheelUp : false;
}

bool InputManager::isWheelDown() const {
    if (!initialized_) {
        return false;
    }
    
    const State* state = getContextState(activeContext_);
    return state ? state->wheelDown : false;
}

bool InputManager::isLeftButtonClicked() const {
    if (!initialized_) {
        return false;
    }
    
    const State* state = getContextState(activeContext_);
    return state ? state->leftButtonClicked : false;
}

bool InputManager::isRightButtonClicked() const {
    if (!initialized_) {
        return false;
    }
    
    const State* state = getContextState(activeContext_);
    return state ? state->rightButtonClicked : false;
}

/******************************************************************************/
// Raw input updates

void InputManager::updateKey(TbKeyCode key, bool pressed) {
    if (!initialized_ || key >= KC_LIST_END) {
        return;
    }
    
    // Update raw input
    rawInput_.keys[key] = pressed ? 1 : 0;
    if (pressed) {
        rawInput_.lastKey = key;
    }
    
    // Route to active context
    State* state = getContextState(activeContext_);
    if (state) {
        state->keys[key] = pressed ? 1 : 0;
        if (pressed) {
            state->lastKey = key;
        }
    }
}

void InputManager::updateMousePosition(long x, long y) {
    if (!initialized_) {
        return;
    }
    
    // Update raw input
    rawInput_.mouseX = x;
    rawInput_.mouseY = y;
    
    // Route to active context
    State* state = getContextState(activeContext_);
    if (state) {
        state->mouseX = x;
        state->mouseY = y;
    }
}

void InputManager::updateMouseButton(int button, bool pressed) {
    if (!initialized_) {
        return;
    }
    
    auto updateButton = [](State* state, int btn, bool press) {
        if (!state) return;
        
        bool* buttonState = nullptr;
        bool* clicked = nullptr;
        bool* released = nullptr;
        
        switch (btn) {
        case 0: // Left
            buttonState = &state->leftButton;
            clicked = &state->leftButtonClicked;
            released = &state->leftButtonReleased;
            break;
        case 1: // Right
            buttonState = &state->rightButton;
            clicked = &state->rightButtonClicked;
            released = &state->rightButtonReleased;
            break;
        case 2: // Middle
            buttonState = &state->middleButton;
            break;
        default:
            return;
        }
        
        // Detect transitions
        if (press && !(*buttonState)) {
            if (clicked) *clicked = true;
        } else if (!press && (*buttonState)) {
            if (released) *released = true;
        }
        *buttonState = press;
    };
    
    // Update raw input
    updateButton(&rawInput_, button, pressed);
    
    // Route to active context
    State* state = getContextState(activeContext_);
    updateButton(state, button, pressed);
}

void InputManager::updateMouseWheel(bool up) {
    if (!initialized_) {
        return;
    }
    
    // Update raw input
    if (up) {
        rawInput_.wheelUp = true;
    } else {
        rawInput_.wheelDown = true;
    }
    
    // Route to active context
    State* state = getContextState(activeContext_);
    if (state) {
        if (up) {
            state->wheelUp = true;
        } else {
            state->wheelDown = true;
        }
    }
}

/******************************************************************************/
// Direct state access

InputManager::State* InputManager::getActiveState() {
    if (!initialized_) {
        return nullptr;
    }
    return getContextState(activeContext_);
}

const InputManager::State* InputManager::getActiveState() const {
    if (!initialized_) {
        return nullptr;
    }
    return getContextState(activeContext_);
}

/******************************************************************************/
// C-style wrapper API for gradual migration

extern "C" {

void InputManager_Initialize() {
    InputManager::instance().initialize();
}

void InputManager_Shutdown() {
    InputManager::instance().shutdown();
}

void InputManager_NewFrame() {
    InputManager::instance().newFrame();
}

void InputManager_PushContext(int context) {
    InputManager::instance().pushContext(static_cast<InputManager::Context>(context));
}

void InputManager_PopContext() {
    InputManager::instance().popContext();
}

void InputManager_SetContext(int context) {
    InputManager::instance().setContext(static_cast<InputManager::Context>(context));
}

int InputManager_GetActiveContext() {
    return static_cast<int>(InputManager::instance().getActiveContext());
}

TbBool InputManager_IsKeyPressed(TbKeyCode key) {
    return InputManager::instance().isKeyPressed(key) ? 1 : 0;
}

TbKeyCode InputManager_GetLastKey() {
    return InputManager::instance().getLastKey();
}

void InputManager_ClearLastKey() {
    InputManager::instance().clearLastKey();
}

long InputManager_GetMouseX() {
    return InputManager::instance().getMouseX();
}

long InputManager_GetMouseY() {
    return InputManager::instance().getMouseY();
}

TbBool InputManager_IsLeftButtonPressed() {
    return InputManager::instance().isLeftButtonPressed() ? 1 : 0;
}

TbBool InputManager_IsRightButtonPressed() {
    return InputManager::instance().isRightButtonPressed() ? 1 : 0;
}

TbBool InputManager_IsMiddleButtonPressed() {
    return InputManager::instance().isMiddleButtonPressed() ? 1 : 0;
}

TbBool InputManager_IsWheelUp() {
    return InputManager::instance().isWheelUp() ? 1 : 0;
}

TbBool InputManager_IsWheelDown() {
    return InputManager::instance().isWheelDown() ? 1 : 0;
}

TbBool InputManager_IsLeftButtonClicked() {
    return InputManager::instance().isLeftButtonClicked() ? 1 : 0;
}

TbBool InputManager_IsRightButtonClicked() {
    return InputManager::instance().isRightButtonClicked() ? 1 : 0;
}

void InputManager_UpdateKey(TbKeyCode key, TbBool pressed) {
    InputManager::instance().updateKey(key, pressed != 0);
}

void InputManager_UpdateMousePosition(long x, long y) {
    InputManager::instance().updateMousePosition(x, y);
}

void InputManager_UpdateMouseButton(int button, TbBool pressed) {
    InputManager::instance().updateMouseButton(button, pressed != 0);
}

void InputManager_UpdateMouseWheel(TbBool up) {
    InputManager::instance().updateMouseWheel(up != 0);
}

// Convenience wrappers for easier migration
TbBool input_key_pressed(TbKeyCode key) {
    return InputManager_IsKeyPressed(key);
}

TbKeyCode input_last_key(void) {
    return InputManager_GetLastKey();
}

void input_clear_last_key(void) {
    InputManager_ClearLastKey();
}

} // extern "C"
