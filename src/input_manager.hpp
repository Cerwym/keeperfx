/******************************************************************************/
// KeeperFX - Input Manager
/******************************************************************************/
/** @file input_manager.hpp
 *     Modern C++ input abstraction layer for context-aware input handling.
 * @par Purpose:
 *     Provides a clean abstraction layer over raw input (keyboard/mouse/gamepad)
 *     with support for multiple input contexts (game, UI, debug overlay).
 * @par Comment:
 *     Allows input to be routed to different systems based on context,
 *     solving issues like ImGui overlay blocking game input.
 * @author   Community
 * @date     18 Jan 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef INPUT_MANAGER_HPP
#define INPUT_MANAGER_HPP

#include "bflib_basics.h"
#include "bflib_keybrd.h"
#include "globals.h"

#ifdef __cplusplus
#include <array>
#include <stack>
#include <cstring>

/******************************************************************************/
/** Input Manager - Modern C++ singleton for context-aware input routing */
class InputManager {
public:
    /** Input context enumeration */
    enum class Context {
        None = 0,       /**< No input processing */
        Game,           /**< Normal gameplay input */
        UI,             /**< Frontend/menu input */
        Debug,          /**< Debug overlay (ImGui) input */
        Blocked,        /**< Input blocked (e.g., during cinematics) */
    };
    
    /** Input state snapshot for a particular frame/context */
    struct State {
        // Keyboard state
        std::array<uint8_t, KC_LIST_END> keys{};  /**< Key press states */
        TbKeyCode lastKey = KC_UNASSIGNED;         /**< Last key pressed */
        
        // Mouse state
        long mouseX = 0;                           /**< Mouse X position */
        long mouseY = 0;                           /**< Mouse Y position */
        bool leftButton = false;                   /**< Left mouse button pressed */
        bool rightButton = false;                  /**< Right mouse button pressed */
        bool middleButton = false;                 /**< Middle mouse button pressed */
        bool wheelUp = false;                      /**< Mouse wheel scrolled up */
        bool wheelDown = false;                    /**< Mouse wheel scrolled down */
        
        // Mouse button transitions (for detecting clicks)
        bool leftButtonClicked = false;            /**< Left button just pressed */
        bool rightButtonClicked = false;           /**< Right button just pressed */
        bool leftButtonReleased = false;           /**< Left button just released */
        bool rightButtonReleased = false;          /**< Right button just released */
        
        /** Clear all state */
        void clear() {
            keys.fill(0);
            lastKey = KC_UNASSIGNED;
            mouseX = mouseY = 0;
            leftButton = rightButton = middleButton = false;
            wheelUp = wheelDown = false;
            leftButtonClicked = rightButtonClicked = false;
            leftButtonReleased = rightButtonReleased = false;
        }
        
        /** Clear per-frame state (clicks, wheel events) */
        void clearPerFrameState() {
            wheelUp = wheelDown = false;
            leftButtonClicked = rightButtonClicked = false;
            leftButtonReleased = rightButtonReleased = false;
        }
    };
    
private:
    // Singleton - private constructor
    InputManager();
    ~InputManager() = default;
    
    // Disable copy/move
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;
    InputManager(InputManager&&) = delete;
    InputManager& operator=(InputManager&&) = delete;
    
    // Context management
    Context activeContext_ = Context::Game;
    std::stack<Context> contextStack_;
    
    // Input state for each context
    State gameInput_;
    State uiInput_;
    State debugInput_;
    State rawInput_;  /**< Raw unfiltered input */
    
    bool initialized_ = false;
    
    /** Get state for a specific context */
    State* getContextState(Context context);
    const State* getContextState(Context context) const;
    
public:
    /** Get singleton instance */
    static InputManager& instance() {
        static InputManager instance;
        return instance;
    }
    
    /**************************************************************************/
    // Initialization and management
    
    /** Initialize the input manager */
    void initialize();
    
    /** Shutdown the input manager */
    void shutdown();
    
    /** Update for new frame (clears per-frame state) */
    void newFrame();
    
    /** Check if initialized */
    bool isInitialized() const { return initialized_; }
    
    /**************************************************************************/
    // Context management
    
    /** Push a new input context onto the stack */
    void pushContext(Context context);
    
    /** Pop the current input context from the stack */
    void popContext();
    
    /** Set the active input context directly (clears stack) */
    void setContext(Context context);
    
    /** Get the currently active input context */
    Context getActiveContext() const { return activeContext_; }
    
    /**************************************************************************/
    // Input state accessors (read from active context)
    
    /** Check if a key is currently pressed */
    bool isKeyPressed(TbKeyCode key) const;
    
    /** Get the last key that was pressed */
    TbKeyCode getLastKey() const;
    
    /** Clear the last key press */
    void clearLastKey();
    
    /** Get mouse X position */
    long getMouseX() const;
    
    /** Get mouse Y position */
    long getMouseY() const;
    
    /** Check if left mouse button is pressed */
    bool isLeftButtonPressed() const;
    
    /** Check if right mouse button is pressed */
    bool isRightButtonPressed() const;
    
    /** Check if middle mouse button is pressed */
    bool isMiddleButtonPressed() const;
    
    /** Check if mouse wheel was scrolled up this frame */
    bool isWheelUp() const;
    
    /** Check if mouse wheel was scrolled down this frame */
    bool isWheelDown() const;
    
    /** Check if left button was just clicked this frame */
    bool isLeftButtonClicked() const;
    
    /** Check if right button was just clicked this frame */
    bool isRightButtonClicked() const;
    
    /**************************************************************************/
    // Raw input updates (called by input system, not by game code)
    
    /** Update keyboard state from raw input */
    void updateKey(TbKeyCode key, bool pressed);
    
    /** Update mouse position from raw input */
    void updateMousePosition(long x, long y);
    
    /** Update mouse button state from raw input */
    void updateMouseButton(int button, bool pressed);
    
    /** Update mouse wheel state from raw input */
    void updateMouseWheel(bool up);
    
    /**************************************************************************/
    // Direct state access (for migration/debugging)
    
    /** Get pointer to active input state */
    State* getActiveState();
    const State* getActiveState() const;
    
    /** Get pointer to raw input state (unfiltered) */
    State* getRawState() { return &rawInput_; }
    const State* getRawState() const { return &rawInput_; }
};

#endif // __cplusplus

/******************************************************************************/
// C-style wrapper API for gradual migration from existing code
#ifdef __cplusplus
extern "C" {
#endif
    void InputManager_Initialize();
    void InputManager_Shutdown();
    void InputManager_NewFrame();
    
    void InputManager_PushContext(int context);
    void InputManager_PopContext();
    void InputManager_SetContext(int context);
    int InputManager_GetActiveContext();
    
    TbBool InputManager_IsKeyPressed(TbKeyCode key);
    TbKeyCode InputManager_GetLastKey();
    void InputManager_ClearLastKey();
    long InputManager_GetMouseX();
    long InputManager_GetMouseY();
    TbBool InputManager_IsLeftButtonPressed();
    TbBool InputManager_IsRightButtonPressed();
    TbBool InputManager_IsMiddleButtonPressed();
    TbBool InputManager_IsWheelUp();
    TbBool InputManager_IsWheelDown();
    TbBool InputManager_IsLeftButtonClicked();
    TbBool InputManager_IsRightButtonClicked();
    
    void InputManager_UpdateKey(TbKeyCode key, TbBool pressed);
    void InputManager_UpdateMousePosition(long x, long y);
    void InputManager_UpdateMouseButton(int button, TbBool pressed);
    void InputManager_UpdateMouseWheel(TbBool up);
    
    // Convenience wrappers for easier migration (prefixed to avoid conflicts)
    TbBool input_key_pressed(TbKeyCode key);
    TbKeyCode input_last_key(void);
    void input_clear_last_key(void);

#ifdef __cplusplus
}
#endif

// Legacy compatibility macros - these can gradually be phased out
#define input_is_key_down(key) InputManager_IsKeyPressed(key)

#endif // INPUT_MANAGER_HPP
