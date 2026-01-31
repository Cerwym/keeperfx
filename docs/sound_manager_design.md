# KeeperFX Sound Manager Design & Lua Integration

## Overview

This document outlines the design and implementation of a unified `SoundManager` class for KeeperFX that:
1. Encapsulates all sound/music playback into a single, easy-to-use abstraction
2. Provides runtime loading capabilities for custom sounds
3. Exposes sound functionality to Lua scripting for modders
4. Replaces scattered sound function calls across the codebase with a clean API

## Table of Contents

1. [Current State Analysis](#1-current-state-analysis)
2. [Language Choice: C++ vs C](#2-language-choice-c-vs-c)
3. [SoundManager Architecture](#3-soundmanager-architecture)
4. [Implementation Plan](#4-implementation-plan)
5. [Lua API Integration](#5-lua-api-integration)
6. [Migration Strategy](#6-migration-strategy)
7. [Runtime Loading System](#7-runtime-loading-system)
8. [Example Use Cases](#8-example-use-cases)

---

## 1. Current State Analysis

### 1.1 Current Sound System Issues

**Scattered Function Calls:**
```c
// Combat (creature_states_combt.c)
play_creature_sound(fighter, CrSnd_Fight, 3, 0);

// Magic Powers (magic_powers.c)
play_creature_sound(thing, CrSnd_Slap, 3, 0);
thing_play_sample(creatng, smpl_idx, pitch, repeats, ctype, flags, priority, volume);

// Music (multiple files)
play_music(filename);
play_music_track(track_number);
stop_music();

// Speech (gui_soundmsgs.c)
output_message(SMsg_Slap, duration);
play_speech_sample(smpl_idx);

// Ambient (sounds.c)
play_atmos_sound(smpl_idx);
play_non_3d_sample(smpl_idx);
```

**Problems:**
- No unified interface
- Difficult to mock/test
- Hard to add features (e.g., runtime loading, dynamic mixing)
- No abstraction for Lua scripting
- Tight coupling between game logic and sound implementation

### 1.2 Current Lua Sound Functions

**Existing Lua API (limited):**
```lua
SetMusic(track_number)           -- Play music track
SetMusic("filename.ogg")         -- Play music file
PlayMessage(player, type, id)    -- Play speech message
```

**Missing Capabilities:**
- Play creature sounds from Lua
- Play sound effects at positions
- Load custom sounds at runtime
- Control sound parameters (volume, pitch, pan)
- Query sound state (is playing, duration, etc.)

---

## 2. Language Choice: C++ vs C

### 2.1 Current Codebase Analysis

**KeeperFX Already Uses Modern C++:**

The codebase currently uses C++11/14/17 features extensively, particularly in performance-critical and complex subsystems:

**Sound System (`src/bflib_sndlib.cpp`):**
```cpp
// C++11: Smart pointers for RAII
using ALCdevice_ptr = std::unique_ptr<ALCdevice, device_deleter>;
using ALCcontext_ptr = std::unique_ptr<ALCcontext, context_deleter>;

// C++11: STL containers
std::vector<sound_sample> g_banks[2];
std::set<uint32_t> g_tick_samples;
std::atomic<Mix_Music *> g_mix_music;

// C++11: Classes with RAII
class openal_buffer {
public:
    ALuint id = 0;
    openal_buffer() { /* allocate */ }
    ~openal_buffer() noexcept { alDeleteBuffers(1, &id); }
    
    // Move semantics
    openal_buffer(openal_buffer && other) : id(std::exchange(other.id, 0)) {}
    openal_buffer & operator=(openal_buffer && other);
    
    // Deleted copy constructor (C++11)
    openal_buffer(const openal_buffer &) = delete;
};

// C++11: Exception classes
class openal_error : public std::runtime_error {
    openal_error(const char * description, ALenum errcode = alGetError())
    : runtime_error(std::string("OpenAL error: ") + description + ": " + alErrorStr(errcode))
    {}
};
```

**Other Subsystems:**
- **Timing** (`bflib_datetm.cpp`): Uses `std::chrono` extensively
- **Video** (`bflib_fmvids.cpp`): Uses C++ classes, exceptions, and STL
- **Networking** (`bflib_network.cpp`): Uses STL containers and smart pointers
- **Rendering** (`spritesheet.cpp`, `engine_render_data.cpp`): Modern C++ patterns

### 2.2 C++ vs C Comparison for SoundManager

| Aspect | C Implementation | C++ Implementation | Winner |
|--------|------------------|-------------------|--------|
| **Type Safety** | Manual type checking, void* casting | Compile-time type safety, templates | **C++** |
| **Memory Safety** | Manual malloc/free, error-prone | RAII, smart pointers, automatic cleanup | **C++** |
| **Encapsulation** | Global state, function prefixes | Classes, namespaces, visibility modifiers | **C++** |
| **Error Handling** | Return codes, easy to ignore | Exceptions, RAII ensures cleanup | **C++** |
| **Code Clarity** | Verbose, boilerplate-heavy | Expressive, self-documenting | **C++** |
| **Standard Library** | Limited (no containers) | Rich STL (vector, map, string, etc.) | **C++** |
| **Compatibility** | Works with C code directly | Requires extern "C" wrappers | **C** |
| **Performance** | Equal to C++ | Equal to C (zero-cost abstractions) | **Tie** |
| **Debugging** | Good debugger support | Excellent debugger support, better type info | **C++** |
| **Refactoring** | Difficult, risky | Easy with proper interfaces | **C++** |

### 2.3 Why C++ is Better for SoundManager

#### A. Memory Management

**C Approach (Error-Prone):**
```c
struct SoundManager {
    CustomSound* custom_sounds;
    size_t custom_sounds_count;
    size_t custom_sounds_capacity;
};

void sound_manager_add_sound(struct SoundManager* mgr, const char* name) {
    // Manual memory management - easy to leak or double-free
    if (mgr->custom_sounds_count >= mgr->custom_sounds_capacity) {
        size_t new_cap = mgr->custom_sounds_capacity * 2;
        CustomSound* new_arr = realloc(mgr->custom_sounds, new_cap * sizeof(CustomSound));
        if (!new_arr) {
            // Memory leak if we don't handle this properly!
            return;
        }
        mgr->custom_sounds = new_arr;
        mgr->custom_sounds_capacity = new_cap;
    }
    // Must manually copy string, manage lifetime
    mgr->custom_sounds[mgr->custom_sounds_count].name = strdup(name);
    // ... more manual memory management
}

void sound_manager_destroy(struct SoundManager* mgr) {
    // Must remember to free everything
    for (size_t i = 0; i < mgr->custom_sounds_count; i++) {
        free(mgr->custom_sounds[i].name);
        // What about other resources? Easy to miss!
    }
    free(mgr->custom_sounds);
}
```

**C++ Approach (Safe, Automatic):**
```cpp
class SoundManager {
private:
    std::unordered_map<std::string, CustomSound> custom_sounds_;
    
public:
    void addSound(const std::string& name) {
        // Automatic memory management, no leaks
        custom_sounds_[name] = CustomSound(name);
        // String, map all cleaned up automatically in destructor
    }
    
    ~SoundManager() {
        // All resources cleaned up automatically via RAII
        // No manual cleanup needed!
    }
};
```

#### B. Type Safety and API Clarity

**C Approach (Weak Types):**
```c
// Easy to pass wrong type, no compile-time checks
SoundEmitterID sound_manager_play_effect(
    void* sound_data,           // What type is this?
    int sample_or_priority,     // Ambiguous parameter
    int volume_or_flags         // Which is which?
);

// Easy to misuse
sound_manager_play_effect(thing, 3, 256);  // Is 3 the sample or priority?
```

**C++ Approach (Strong Types):**
```cpp
// Clear, self-documenting API
class SoundManager {
public:
    SoundEmitterID playEffect(const SoundEffect& effect);
    SoundEmitterID playEffect(SoundSmplTblID sample_id, 
                              long priority = 3, 
                              SoundVolume volume = FULL_LOUDNESS);
    SoundEmitterID playEffectOn(SoundSmplTblID sample_id, 
                                struct Thing* thing,
                                long priority = 3, 
                                SoundVolume volume = FULL_LOUDNESS);
};

// Clear usage
manager.playEffect(sample_id, 3, 256);           // Obvious what each param is
manager.playEffectOn(sample_id, thing, 3, 256);  // Type-safe, clear intent
```

#### C. Encapsulation and Visibility

**C Approach (No Privacy):**
```c
// Header file - everything is visible
struct SoundManager {
    CustomSound* custom_sounds;        // Anyone can modify this!
    size_t next_sample_id;             // No protection
    TbBool initialized;                // Public implementation details
    // No way to hide internal state
};

// Anyone can do this:
manager.next_sample_id = 0;  // Oops, broke the manager!
manager.custom_sounds = NULL; // Double oops!
```

**C++ Approach (Proper Encapsulation):**
```cpp
class SoundManager {
public:
    // Only public API exposed
    SoundEmitterID playEffect(SoundSmplTblID sample_id);
    bool loadCustomSound(const std::string& name, const std::string& filepath);
    
private:
    // Implementation hidden, cannot be accessed externally
    std::unordered_map<std::string, CustomSound> custom_sounds_;
    SoundSmplTblID next_custom_sample_id_;
    bool initialized_;
    
    // Private helper methods
    SoundEmitterID playEffectInternal(const SoundEffect& effect);
    bool loadWavFile(const std::string& filepath, SoundSmplTblID sample_id);
};

// Compile error - cannot access private members!
// manager.next_custom_sample_id_ = 0;  // ERROR!
```

#### D. Error Handling and Resource Cleanup

**C Approach (Manual, Error-Prone):**
```c
int sound_manager_load_custom_sound(const char* name, const char* filepath) {
    FILE* fp = fopen(filepath, "rb");
    if (!fp) return -1;
    
    char* buffer = malloc(1024);
    if (!buffer) {
        fclose(fp);  // Must remember to close!
        return -1;
    }
    
    ALuint al_buffer;
    alGenBuffers(1, &al_buffer);
    if (alGetError() != AL_NO_ERROR) {
        free(buffer);    // Must remember to free!
        fclose(fp);      // Must remember to close!
        return -1;
    }
    
    // ... more code with more cleanup paths
    
    // Must remember to clean up in ALL error paths!
    free(buffer);
    fclose(fp);
    return al_buffer;
}
```

**C++ Approach (Automatic, Safe):**
```cpp
SoundSmplTblID SoundManager::loadCustomSound(const std::string& name, 
                                              const std::string& filepath) {
    // RAII - automatic cleanup on any exit path
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        return 0;  // file automatically closed
    }
    
    std::vector<char> buffer(1024);  // Automatic memory management
    
    openal_buffer al_buf;  // RAII wrapper, automatic cleanup
    
    // If exception thrown, ALL resources cleaned up automatically
    // If early return, ALL resources cleaned up automatically
    // No manual cleanup needed!
    
    return next_custom_sample_id_++;
}
```

#### E. Runtime Loading with Complex State

**C Approach (Complex State Management):**
```c
struct CustomSound {
    char* name;              // Manual string management
    char* filepath;          // Manual string management
    SoundSmplTblID sample_id;
    TbBool loaded;
};

struct CustomSoundList {
    CustomSound* sounds;
    size_t count;
    size_t capacity;
};

// Need manual searching, resizing, cleanup
int find_custom_sound(struct CustomSoundList* list, const char* name) {
    for (size_t i = 0; i < list->count; i++) {
        if (strcmp(list->sounds[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}
```

**C++ Approach (Simple, Efficient):**
```cpp
class SoundManager {
private:
    std::unordered_map<std::string, CustomSound> custom_sounds_;
    
public:
    SoundSmplTblID getCustomSoundId(const std::string& name) const {
        auto it = custom_sounds_.find(name);  // O(1) lookup
        return (it != custom_sounds_.end()) ? it->second.sample_id : 0;
    }
    
    bool isCustomSoundLoaded(const std::string& name) const {
        return custom_sounds_.find(name) != custom_sounds_.end();
    }
};
```

### 2.4 C++20 Specific Benefits

**Recommended: C++20** (KeeperFX should target C++20 for new code)

**C++20 Features Useful for SoundManager:**

```cpp
// 1. Designated initializers - cleaner configuration
SoundEffect effect {
    .sample_id = 100,
    .pitch = NORMAL_PITCH,
    .priority = 3,
    .volume = FULL_LOUDNESS
};

// 2. std::span - safe array views without copying
void processSamples(std::span<const SoundSmplTblID> samples) {
    // No copy, safe bounds checking
    for (auto sample : samples) {
        playEffect(sample);
    }
}

// 3. Concepts - better template constraints
template<typename T>
concept SoundSource = requires(T t) {
    { t.getPosition() } -> std::same_as<Coord3d>;
    { t.getSoundEmitterId() } -> std::same_as<SoundEmitterID>;
};

void playSoundAt(SoundSource auto& source, SoundSmplTblID sample_id);

// 4. std::format - type-safe string formatting (or use fmt library)
#include <format>
SYNCDBG(6, std::format("Loaded custom sound '{}' as sample {}", name, sample_id));

// 5. Ranges - cleaner iteration
void stopAllSounds() {
    for (auto& [name, sound] : custom_sounds_ | std::views::filter([](auto& p) { 
        return p.second.loaded; 
    })) {
        unloadCustomSound(name);
    }
}

// 6. Three-way comparison operator
struct SoundEffect {
    auto operator<=>(const SoundEffect&) const = default;
};
```

### 2.5 Compatibility Strategy

**Best of Both Worlds: C++ Implementation with C API**

```cpp
// sound_manager.h - C++ header
#ifdef __cplusplus
namespace KeeperFX {
    class SoundManager {
        // Modern C++ implementation
    };
}

extern "C" {
#endif

// C API for existing code
SoundEmitterID sound_manager_play_effect(SoundSmplTblID sample_id, 
                                         long priority, 
                                         SoundVolume volume);

#ifdef __cplusplus
}
#endif
```

**This approach provides:**
- ✅ Clean C++ implementation internally
- ✅ Simple C API for existing code
- ✅ Gradual migration path
- ✅ No breaking changes to existing code
- ✅ Type safety where possible
- ✅ Performance benefits of modern C++

### 2.6 Recommendation: Use C++20

**Reasons:**

1. **Consistency**: Sound system already uses modern C++ (`bflib_sndlib.cpp`)
2. **Safety**: RAII prevents resource leaks automatically
3. **Clarity**: Self-documenting code with visibility modifiers
4. **Maintainability**: Easier to refactor and extend
5. **Performance**: Zero-cost abstractions, same performance as C
6. **Future-proof**: Access to modern language features
7. **Developer Experience**: Better IDE support, clearer errors
8. **Less Code**: STL containers eliminate boilerplate

**Quote from Bjarne Stroustrup (C++ creator):**
> "C makes it easy to shoot yourself in the foot; C++ makes it harder, but when you do, it blows your whole leg off."

**But with modern C++ (11/14/17/20):**
> "C++20 makes it very hard to shoot yourself in the foot, and when you try, the compiler stops you."

**Conclusion**: Implement SoundManager in **C++20** with **C wrapper functions** for backward compatibility. This matches the existing codebase patterns and provides the best developer experience while maintaining compatibility.

---

## 3. SoundManager Architecture

### 2.1 Class Design

**Header:** `src/sound_manager.h` (new file)

```cpp
#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include "bflib_basics.h"
#include "bflib_sound.h"
#include "globals.h"

#ifdef __cplusplus

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace KeeperFX {

/**
 * @brief Sound effect descriptor for playback requests
 */
struct SoundEffect {
    SoundSmplTblID sample_id;      // Sample ID in sound bank
    SoundPitch pitch;               // Pitch (100 = normal)
    char repeats;                   // Loop count (-1 = infinite)
    unsigned char channel_type;     // Channel type (3 = normal)
    unsigned char flags;            // Special flags
    long priority;                  // Priority (1-6)
    SoundVolume volume;             // Volume (0-256)
    
    // Optional 3D positioning
    bool is_3d;
    struct Coord3d position;
    struct Thing* attached_thing;
    
    // Constructor with defaults
    SoundEffect(SoundSmplTblID id) 
        : sample_id(id)
        , pitch(NORMAL_PITCH)
        , repeats(0)
        , channel_type(3)
        , flags(0)
        , priority(3)
        , volume(FULL_LOUDNESS)
        , is_3d(false)
        , attached_thing(nullptr)
    {}
};

/**
 * @brief Music track descriptor
 */
struct MusicTrack {
    std::string filename;
    int track_number;
    bool is_streaming;
    
    MusicTrack(const std::string& file) 
        : filename(file), track_number(-1), is_streaming(true) {}
    MusicTrack(int track) 
        : filename(""), track_number(track), is_streaming(false) {}
};

/**
 * @brief Custom sound descriptor for runtime loading
 */
struct CustomSound {
    std::string name;              // Unique identifier
    std::string filepath;          // Path to WAV file
    SoundSmplTblID sample_id;      // Assigned sample ID (0 = not loaded)
    bool loaded;                   // Load status
    
    CustomSound(const std::string& n, const std::string& p)
        : name(n), filepath(p), sample_id(0), loaded(false) {}
};

/**
 * @brief Unified sound manager for KeeperFX
 * 
 * Provides a single point of control for all audio:
 * - Sound effects (2D and 3D)
 * - Music playback
 * - Speech/messages
 * - Runtime sound loading
 * - Lua scripting interface
 */
class SoundManager {
public:
    static SoundManager& getInstance();
    
    // === Sound Effects ===
    
    /**
     * @brief Play a sound effect with full control
     * @param effect Sound effect descriptor with all parameters
     * @return Sound emitter ID, or 0 if failed
     */
    SoundEmitterID playEffect(const SoundEffect& effect);
    
    /**
     * @brief Play a simple 2D sound effect
     * @param sample_id Sound sample ID
     * @param priority Priority (1-6, default 3)
     * @param volume Volume (0-256, default 256)
     * @return Sound emitter ID, or 0 if failed
     */
    SoundEmitterID playEffect(SoundSmplTblID sample_id, long priority = 3, 
                              SoundVolume volume = FULL_LOUDNESS);
    
    /**
     * @brief Play a 3D sound effect at a position
     * @param sample_id Sound sample ID
     * @param position 3D world position
     * @param priority Priority (1-6, default 3)
     * @param volume Volume (0-256, default 256)
     * @return Sound emitter ID, or 0 if failed
     */
    SoundEmitterID playEffectAt(SoundSmplTblID sample_id, const struct Coord3d& position,
                                long priority = 3, SoundVolume volume = FULL_LOUDNESS);
    
    /**
     * @brief Play a sound effect attached to a thing
     * @param sample_id Sound sample ID
     * @param thing Thing to attach sound to
     * @param priority Priority (1-6, default 3)
     * @param volume Volume (0-256, default 256)
     * @return Sound emitter ID, or 0 if failed
     */
    SoundEmitterID playEffectOn(SoundSmplTblID sample_id, struct Thing* thing,
                                long priority = 3, SoundVolume volume = FULL_LOUDNESS);
    
    /**
     * @brief Stop a playing sound
     * @param emitter_id Emitter ID returned by playEffect*()
     */
    void stopEffect(SoundEmitterID emitter_id);
    
    /**
     * @brief Check if a sound is playing
     * @param emitter_id Emitter ID
     * @return true if sound is still playing
     */
    bool isEffectPlaying(SoundEmitterID emitter_id) const;
    
    // === Creature Sounds ===
    
    /**
     * @brief Play a creature sound (uses creature config)
     * @param thing Creature thing
     * @param sound_type Sound type (CrSnd_Hurt, CrSnd_Slap, etc.)
     * @param priority Priority (1-6, default 3)
     */
    void playCreatureSound(struct Thing* thing, long sound_type, long priority = 3);
    
    /**
     * @brief Stop a creature sound
     * @param thing Creature thing
     * @param sound_type Sound type to stop
     */
    void stopCreatureSound(struct Thing* thing, long sound_type);
    
    /**
     * @brief Check if creature is playing a sound
     * @param thing Creature thing
     * @param sound_type Sound type to check
     * @return true if sound is playing
     */
    bool isCreatureSoundPlaying(struct Thing* thing, long sound_type) const;
    
    // === Music ===
    
    /**
     * @brief Play music track
     * @param track Music track descriptor
     * @return true if successful
     */
    bool playMusic(const MusicTrack& track);
    
    /**
     * @brief Play music by track number
     * @param track_number Track number (1-N)
     * @return true if successful
     */
    bool playMusic(int track_number);
    
    /**
     * @brief Play music by filename
     * @param filename Music filename (OGG/MP3)
     * @return true if successful
     */
    bool playMusic(const std::string& filename);
    
    /**
     * @brief Stop music playback
     */
    void stopMusic();
    
    /**
     * @brief Pause music playback
     */
    void pauseMusic();
    
    /**
     * @brief Resume music playback
     */
    void resumeMusic();
    
    /**
     * @brief Check if music is playing
     * @return true if music is playing
     */
    bool isMusicPlaying() const;
    
    /**
     * @brief Set music volume
     * @param volume Volume (0-127)
     */
    void setMusicVolume(SoundVolume volume);
    
    // === Speech/Messages ===
    
    /**
     * @brief Play speech message
     * @param message_id Message ID (SMsg_*)
     * @param duration Duration in game turns (0 = default)
     * @return true if successful
     */
    bool playMessage(SoundSmplTblID message_id, long duration = 0);
    
    /**
     * @brief Play custom speech file
     * @param filename WAV filename
     * @param duration Duration in game turns (0 = default)
     * @return true if successful
     */
    bool playCustomMessage(const std::string& filename, long duration = 0);
    
    /**
     * @brief Stop all messages
     */
    void stopMessages();
    
    // === Runtime Loading ===
    
    /**
     * @brief Load a custom sound from file
     * @param name Unique identifier for the sound
     * @param filepath Path to WAV file
     * @return Sample ID if successful, 0 if failed
     */
    SoundSmplTblID loadCustomSound(const std::string& name, const std::string& filepath);
    
    /**
     * @brief Unload a custom sound
     * @param name Sound identifier
     * @return true if successful
     */
    bool unloadCustomSound(const std::string& name);
    
    /**
     * @brief Get sample ID for a custom sound
     * @param name Sound identifier
     * @return Sample ID, or 0 if not found
     */
    SoundSmplTblID getCustomSoundId(const std::string& name) const;
    
    /**
     * @brief Check if custom sound is loaded
     * @param name Sound identifier
     * @return true if loaded
     */
    bool isCustomSoundLoaded(const std::string& name) const;
    
    // === Volume Control ===
    
    /**
     * @brief Set master volume
     * @param volume Volume (0-127)
     */
    void setMasterVolume(SoundVolume volume);
    
    /**
     * @brief Set sound effects volume
     * @param volume Volume (0-127)
     */
    void setSoundVolume(SoundVolume volume);
    
    /**
     * @brief Get current master volume
     * @return Volume (0-127)
     */
    SoundVolume getMasterVolume() const;
    
    /**
     * @brief Get current sound effects volume
     * @return Volume (0-127)
     */
    SoundVolume getSoundVolume() const;
    
    // === System ===
    
    /**
     * @brief Initialize sound manager
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Shutdown sound manager
     */
    void shutdown();
    
    /**
     * @brief Update sound manager (call each game turn)
     */
    void update();
    
    /**
     * @brief Mute/unmute all audio
     * @param mute true to mute, false to unmute
     */
    void setMuted(bool mute);
    
    /**
     * @brief Check if audio is muted
     * @return true if muted
     */
    bool isMuted() const;

private:
    SoundManager();
    ~SoundManager();
    
    // Disable copy/move
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;
    
    // Internal state
    std::unordered_map<std::string, CustomSound> custom_sounds_;
    SoundSmplTblID next_custom_sample_id_;
    bool initialized_;
    bool muted_;
    
    // Helper methods
    SoundEmitterID playEffectInternal(const SoundEffect& effect);
    bool loadWavFile(const std::string& filepath, SoundSmplTblID sample_id);
};

} // namespace KeeperFX

extern "C" {
#endif // __cplusplus

// C API for existing code (thin wrappers)
TbBool sound_manager_init(void);
void sound_manager_shutdown(void);
void sound_manager_update(void);

// Simplified C functions
SoundEmitterID sound_manager_play_effect(SoundSmplTblID sample_id, long priority, SoundVolume volume);
SoundEmitterID sound_manager_play_effect_at(SoundSmplTblID sample_id, struct Coord3d* position, long priority, SoundVolume volume);
SoundEmitterID sound_manager_play_effect_on(SoundSmplTblID sample_id, struct Thing* thing, long priority, SoundVolume volume);
void sound_manager_stop_effect(SoundEmitterID emitter_id);

void sound_manager_play_creature_sound(struct Thing* thing, long sound_type, long priority);
void sound_manager_stop_creature_sound(struct Thing* thing, long sound_type);

TbBool sound_manager_play_music_track(int track_number);
TbBool sound_manager_play_music_file(const char* filename);
void sound_manager_stop_music(void);

TbBool sound_manager_play_message(SoundSmplTblID message_id, long duration);
TbBool sound_manager_play_custom_message(const char* filename, long duration);

SoundSmplTblID sound_manager_load_custom_sound(const char* name, const char* filepath);
TbBool sound_manager_unload_custom_sound(const char* name);
SoundSmplTblID sound_manager_get_custom_sound_id(const char* name);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SOUND_MANAGER_H
```

### 2.2 Benefits of This Design

**Encapsulation:**
- All sound logic in one place
- Easy to modify/extend
- Cleaner game code

**Testability:**
- Can mock SoundManager for tests
- Can verify sound calls without actual audio
- Can unit test sound logic separately

**Flexibility:**
- Easy to add new features (spatial audio enhancements, DSP effects)
- Easy to swap audio backends (OpenAL → SDL2_mixer, etc.)
- Easy to add profiling/debugging

**Modularity:**
- C++ implementation with C wrapper for legacy code
- Gradual migration possible
- Lua binding built on clean API

---

## 4. Implementation Plan

### 3.1 Phase 1: Core SoundManager (C++)

**Files to Create:**
- `src/sound_manager.h` - Header (shown above)
- `src/sound_manager.cpp` - Implementation

**Implementation Priorities:**
1. Basic sound effect playback (wraps existing `thing_play_sample`)
2. Music playback (wraps existing music functions)
3. Creature sound playback (wraps `play_creature_sound`)
4. Volume control

**Example Implementation:**

```cpp
// src/sound_manager.cpp

#include "sound_manager.h"
#include "sounds.h"
#include "bflib_sndlib.h"
#include "creature_control.h"

namespace KeeperFX {

SoundManager& SoundManager::getInstance() {
    static SoundManager instance;
    return instance;
}

SoundManager::SoundManager() 
    : next_custom_sample_id_(10000)  // Start custom sounds at 10000
    , initialized_(false)
    , muted_(false)
{}

bool SoundManager::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Call existing init functions
    if (!init_sound()) {
        return false;
    }
    
    initialized_ = true;
    return true;
}

SoundEmitterID SoundManager::playEffect(const SoundEffect& effect) {
    if (!initialized_ || muted_ || SoundDisabled) {
        return 0;
    }
    
    return playEffectInternal(effect);
}

SoundEmitterID SoundManager::playEffect(SoundSmplTblID sample_id, long priority, SoundVolume volume) {
    SoundEffect effect(sample_id);
    effect.priority = priority;
    effect.volume = volume;
    return playEffect(effect);
}

SoundEmitterID SoundManager::playEffectAt(SoundSmplTblID sample_id, const struct Coord3d& position,
                                          long priority, SoundVolume volume) {
    SoundEffect effect(sample_id);
    effect.priority = priority;
    effect.volume = volume;
    effect.is_3d = true;
    effect.position = position;
    return playEffect(effect);
}

SoundEmitterID SoundManager::playEffectOn(SoundSmplTblID sample_id, struct Thing* thing,
                                          long priority, SoundVolume volume) {
    if (thing_is_invalid(thing)) {
        return 0;
    }
    
    SoundEffect effect(sample_id);
    effect.priority = priority;
    effect.volume = volume;
    effect.is_3d = true;
    effect.attached_thing = thing;
    return playEffect(effect);
}

SoundEmitterID SoundManager::playEffectInternal(const SoundEffect& effect) {
    if (effect.attached_thing != nullptr) {
        // Use existing thing_play_sample
        thing_play_sample(effect.attached_thing, effect.sample_id, effect.pitch,
                         effect.repeats, effect.channel_type, effect.flags,
                         effect.priority, effect.volume);
        return effect.attached_thing->snd_emitter_id;
    } else if (effect.is_3d) {
        // Create 3D emitter at position
        return S3DCreateSoundEmitterPri(
            effect.position.x.val, effect.position.y.val, effect.position.z.val,
            effect.sample_id, 0, effect.pitch, effect.volume,
            effect.repeats, effect.flags | 0x01, effect.priority
        );
    } else {
        // Play 2D sound
        play_non_3d_sample(effect.sample_id);
        return Non3DEmitter;
    }
}

void SoundManager::playCreatureSound(struct Thing* thing, long sound_type, long priority) {
    if (!initialized_ || muted_ || thing_is_invalid(thing)) {
        return;
    }
    
    // Use existing function
    play_creature_sound(thing, sound_type, priority, 0);
}

bool SoundManager::playMusic(int track_number) {
    if (!initialized_ || muted_) {
        return false;
    }
    
    if (track_number == 0) {
        stopMusic();
        return true;
    }
    
    return play_music_track(track_number);
}

bool SoundManager::playMusic(const std::string& filename) {
    if (!initialized_ || muted_) {
        return false;
    }
    
    return play_music(filename.c_str());
}

// ... more implementations

} // namespace KeeperFX

// C API wrappers
extern "C" {

TbBool sound_manager_init(void) {
    return KeeperFX::SoundManager::getInstance().initialize();
}

SoundEmitterID sound_manager_play_effect(SoundSmplTblID sample_id, long priority, SoundVolume volume) {
    return KeeperFX::SoundManager::getInstance().playEffect(sample_id, priority, volume);
}

void sound_manager_play_creature_sound(struct Thing* thing, long sound_type, long priority) {
    KeeperFX::SoundManager::getInstance().playCreatureSound(thing, sound_type, priority);
}

TbBool sound_manager_play_music_track(int track_number) {
    return KeeperFX::SoundManager::getInstance().playMusic(track_number);
}

// ... more C wrappers

} // extern "C"
```

### 3.2 Phase 2: Runtime Loading

**Key Implementation:**

```cpp
SoundSmplTblID SoundManager::loadCustomSound(const std::string& name, const std::string& filepath) {
    // Check if already loaded
    auto it = custom_sounds_.find(name);
    if (it != custom_sounds_.end() && it->second.loaded) {
        return it->second.sample_id;
    }
    
    // Allocate new sample ID
    SoundSmplTblID sample_id = next_custom_sample_id_++;
    
    // Load WAV file into OpenAL buffer
    if (!loadWavFile(filepath, sample_id)) {
        ERRORLOG("Failed to load custom sound '%s' from '%s'", name.c_str(), filepath.c_str());
        return 0;
    }
    
    // Register in map
    CustomSound custom_sound(name, filepath);
    custom_sound.sample_id = sample_id;
    custom_sound.loaded = true;
    custom_sounds_[name] = custom_sound;
    
    SYNCDBG(6, "Loaded custom sound '%s' as sample %d", name.c_str(), sample_id);
    return sample_id;
}

bool SoundManager::loadWavFile(const std::string& filepath, SoundSmplTblID sample_id) {
    // Open WAV file
    FILE* fp = fopen(filepath.c_str(), "rb");
    if (!fp) {
        return false;
    }
    
    // Parse WAVE format (reuse existing wave_file class from bflib_sndlib.cpp)
    // Allocate OpenAL buffer
    // Upload PCM data to buffer
    // Associate buffer with sample_id
    
    fclose(fp);
    return true;
}
```

### 3.3 Phase 3: Lua Integration

See [Section 4: Lua API Integration](#4-lua-api-integration) for details.

---

## 5. Lua API Integration

### 4.1 Lua Sound Module Registration

**File:** `src/lua_api_sound.c` (new file)

**Registration Pattern:**

```c
#include "lua_api_sound.h"
#include "sound_manager.h"
#include "lua_params.h"

// Global sound functions
static const struct luaL_Reg sound_global_methods[] = {
    // Effects
    {"PlaySound", lua_PlaySound},
    {"PlaySoundAt", lua_PlaySoundAt},
    {"StopSound", lua_StopSound},
    {"IsSoundPlaying", lua_IsSoundPlaying},
    
    // Music
    {"PlayMusic", lua_PlayMusic},
    {"StopMusic", lua_StopMusic},
    {"PauseMusic", lua_PauseMusic},
    {"ResumeMusic", lua_ResumeMusic},
    {"IsMusicPlaying", lua_IsMusicPlaying},
    {"SetMusicVolume", lua_SetMusicVolume},
    
    // Messages
    {"PlayMessage", lua_PlayMessage},
    {"PlayCustomMessage", lua_PlayCustomMessage},
    {"StopMessages", lua_StopMessages},
    
    // Runtime loading
    {"LoadCustomSound", lua_LoadCustomSound},
    {"UnloadCustomSound", lua_UnloadCustomSound},
    {"GetCustomSoundId", lua_GetCustomSoundId},
    
    // Volume
    {"SetMasterVolume", lua_SetMasterVolume},
    {"SetSoundVolume", lua_SetSoundVolume},
    {"GetMasterVolume", lua_GetMasterVolume},
    {"GetSoundVolume", lua_GetSoundVolume},
    
    // Mute
    {"MuteAudio", lua_MuteAudio},
    {"IsAudioMuted", lua_IsAudioMuted},
    
    {NULL, NULL}
};

// Thing/Creature methods (extends Thing class)
static const struct luaL_Reg thing_sound_methods[] = {
    {"playSound", lua_Thing_PlaySound},
    {"stopSound", lua_Thing_StopSound},
    {"isSoundPlaying", lua_Thing_IsSoundPlaying},
    {NULL, NULL}
};

void sound_register(lua_State *L) {
    // Register global sound functions
    for (size_t i = 0; sound_global_methods[i].name != NULL; i++) {
        lua_register(L, sound_global_methods[i].name, sound_global_methods[i].func);
    }
    
    // Extend Thing class with sound methods
    luaL_getmetatable(L, "Thing");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "__methods");
        if (lua_istable(L, -1)) {
            for (size_t i = 0; thing_sound_methods[i].name != NULL; i++) {
                lua_pushcfunction(L, thing_sound_methods[i].func);
                lua_setfield(L, -2, thing_sound_methods[i].name);
            }
        }
        lua_pop(L, 1); // __methods
    }
    lua_pop(L, 1); // Thing metatable
}
```

### 4.2 Lua Function Implementations

#### Global Sound Functions

```c
// Play a sound effect
// Usage: PlaySound(sample_id, priority, volume)
static int lua_PlaySound(lua_State *L) {
    SoundSmplTblID sample_id = luaL_checkinteger(L, 1);
    long priority = luaL_optinteger(L, 2, 3);  // Default priority 3
    SoundVolume volume = luaL_optinteger(L, 3, FULL_LOUDNESS);
    
    SoundEmitterID emitter_id = sound_manager_play_effect(sample_id, priority, volume);
    
    lua_pushinteger(L, emitter_id);
    return 1;  // Return emitter ID
}

// Play a sound effect at a 3D position
// Usage: PlaySoundAt(sample_id, x, y, z, priority, volume)
static int lua_PlaySoundAt(lua_State *L) {
    SoundSmplTblID sample_id = luaL_checkinteger(L, 1);
    
    struct Coord3d position;
    if (lua_isPos3d(L, 2)) {
        position = luaL_checkPos3d(L, 2);
    } else {
        position.x.val = luaL_checkinteger(L, 2);
        position.y.val = luaL_checkinteger(L, 3);
        position.z.val = luaL_checkinteger(L, 4);
    }
    
    long priority = luaL_optinteger(L, lua_isPos3d(L, 2) ? 3 : 5, 3);
    SoundVolume volume = luaL_optinteger(L, lua_isPos3d(L, 2) ? 4 : 6, FULL_LOUDNESS);
    
    SoundEmitterID emitter_id = sound_manager_play_effect_at(sample_id, &position, priority, volume);
    
    lua_pushinteger(L, emitter_id);
    return 1;
}

// Stop a sound
// Usage: StopSound(emitter_id)
static int lua_StopSound(lua_State *L) {
    SoundEmitterID emitter_id = luaL_checkinteger(L, 1);
    sound_manager_stop_effect(emitter_id);
    return 0;
}

// Check if sound is playing
// Usage: is_playing = IsSoundPlaying(emitter_id)
static int lua_IsSoundPlaying(lua_State *L) {
    SoundEmitterID emitter_id = luaL_checkinteger(L, 1);
    bool is_playing = S3DEmitterIsPlayingAnySample(emitter_id);
    lua_pushboolean(L, is_playing);
    return 1;
}

// Play music
// Usage: PlayMusic(track_number) or PlayMusic("filename.ogg")
static int lua_PlayMusic(lua_State *L) {
    TbBool success;
    
    if (lua_isnumber(L, 1)) {
        int track_number = luaL_checkinteger(L, 1);
        success = sound_manager_play_music_track(track_number);
    } else {
        const char* filename = luaL_checkstring(L, 1);
        success = sound_manager_play_music_file(filename);
    }
    
    lua_pushboolean(L, success);
    return 1;
}

// Stop music
// Usage: StopMusic()
static int lua_StopMusic(lua_State *L) {
    sound_manager_stop_music();
    return 0;
}

// Load custom sound
// Usage: sample_id = LoadCustomSound("my_sound", "path/to/sound.wav")
static int lua_LoadCustomSound(lua_State *L) {
    const char* name = luaL_checkstring(L, 1);
    const char* filepath = luaL_checkstring(L, 2);
    
    SoundSmplTblID sample_id = sound_manager_load_custom_sound(name, filepath);
    
    if (sample_id == 0) {
        return luaL_error(L, "Failed to load custom sound '%s' from '%s'", name, filepath);
    }
    
    lua_pushinteger(L, sample_id);
    return 1;
}

// Unload custom sound
// Usage: UnloadCustomSound("my_sound")
static int lua_UnloadCustomSound(lua_State *L) {
    const char* name = luaL_checkstring(L, 1);
    TbBool success = sound_manager_unload_custom_sound(name);
    lua_pushboolean(L, success);
    return 1;
}

// Get custom sound ID
// Usage: sample_id = GetCustomSoundId("my_sound")
static int lua_GetCustomSoundId(lua_State *L) {
    const char* name = luaL_checkstring(L, 1);
    SoundSmplTblID sample_id = sound_manager_get_custom_sound_id(name);
    
    if (sample_id == 0) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, sample_id);
    }
    return 1;
}

// Set master volume
// Usage: SetMasterVolume(100)  -- 0-127
static int lua_SetMasterVolume(lua_State *L) {
    SoundVolume volume = luaL_checkinteger(L, 1);
    if (volume < 0) volume = 0;
    if (volume > 127) volume = 127;
    SetSoundMasterVolume(volume);
    return 0;
}

// Get master volume
// Usage: volume = GetMasterVolume()
static int lua_GetMasterVolume(lua_State *L) {
    SoundVolume volume = GetCurrentSoundMasterVolume();
    lua_pushinteger(L, volume);
    return 1;
}
```

#### Thing/Creature Sound Methods

```c
// Play creature sound
// Usage: creature:playSound("hurt") or creature:playSound(CrSnd_Hurt)
static int lua_Thing_PlaySound(lua_State *L) {
    struct Thing* thing = luaL_checkThing(L, 1);
    
    if (thing_is_invalid(thing)) {
        return luaL_error(L, "Invalid thing");
    }
    
    long sound_type;
    if (lua_isnumber(L, 2)) {
        sound_type = luaL_checkinteger(L, 2);
    } else {
        const char* sound_name = luaL_checkstring(L, 2);
        sound_type = get_creature_sound_type_from_name(sound_name);
        if (sound_type < 0) {
            return luaL_error(L, "Unknown sound type '%s'", sound_name);
        }
    }
    
    long priority = luaL_optinteger(L, 3, 3);
    
    sound_manager_play_creature_sound(thing, sound_type, priority);
    return 0;
}

// Stop creature sound
// Usage: creature:stopSound("hurt") or creature:stopSound(CrSnd_Hurt)
static int lua_Thing_StopSound(lua_State *L) {
    struct Thing* thing = luaL_checkThing(L, 1);
    
    if (thing_is_invalid(thing)) {
        return luaL_error(L, "Invalid thing");
    }
    
    long sound_type;
    if (lua_isnumber(L, 2)) {
        sound_type = luaL_checkinteger(L, 2);
    } else {
        const char* sound_name = luaL_checkstring(L, 2);
        sound_type = get_creature_sound_type_from_name(sound_name);
        if (sound_type < 0) {
            return luaL_error(L, "Unknown sound type '%s'", sound_name);
        }
    }
    
    sound_manager_stop_creature_sound(thing, sound_type);
    return 0;
}

// Check if creature sound is playing
// Usage: is_playing = creature:isSoundPlaying("hurt")
static int lua_Thing_IsSoundPlaying(lua_State *L) {
    struct Thing* thing = luaL_checkThing(L, 1);
    
    if (thing_is_invalid(thing)) {
        lua_pushboolean(L, false);
        return 1;
    }
    
    long sound_type;
    if (lua_isnumber(L, 2)) {
        sound_type = luaL_checkinteger(L, 2);
    } else {
        const char* sound_name = luaL_checkstring(L, 2);
        sound_type = get_creature_sound_type_from_name(sound_name);
        if (sound_type < 0) {
            lua_pushboolean(L, false);
            return 1;
        }
    }
    
    bool is_playing = playing_creature_sound(thing, sound_type);
    lua_pushboolean(L, is_playing);
    return 1;
}
```

### 4.3 Lua API Constants

**File:** `config/fxdata/lua/constants/Sound.lua` (new file)

```lua
-- Sound type constants for creature sounds
---@enum CreatureSoundType
CreatureSoundType = {
    None = 0,
    Hurt = 1,
    Hit = 2,
    Happy = 3,
    Sad = 4,
    Hang = 5,
    Drop = 6,
    Torture = 7,
    Slap = 8,
    Die = 9,
    Foot = 10,
    Fight = 11,
    Piss = 12,
}

-- Sound priority levels
---@enum SoundPriority
SoundPriority = {
    Low = 1,
    MediumLow = 2,
    Medium = 3,
    High = 4,
    VeryHigh = 5,
    Critical = 6,
}

-- Common sound volumes
---@enum SoundVolume
SoundVolume = {
    Silent = 0,
    VeryQuiet = 64,
    Quiet = 128,
    Normal = 192,
    Loud = 224,
    VeryLoud = 256,
}

-- Common pitch values
---@enum SoundPitch
SoundPitch = {
    VeryLow = 50,
    Low = 75,
    Normal = 100,
    High = 125,
    VeryHigh = 150,
}
```

### 4.4 Lua Type Definitions

**File:** `config/fxdata/lua/types/Sound.lua` (new file)

```lua
---@meta

---Play a sound effect
---@param sample_id integer Sound sample ID
---@param priority? integer Priority (1-6, default 3)
---@param volume? integer Volume (0-256, default 256)
---@return integer emitter_id Emitter ID for controlling the sound
function PlaySound(sample_id, priority, volume) end

---Play a sound effect at a 3D position
---@param sample_id integer Sound sample ID
---@param x number|Pos3d X coordinate or Pos3d object
---@param y? number Y coordinate (if x is number)
---@param z? number Z coordinate (if x is number)
---@param priority? integer Priority (1-6, default 3)
---@param volume? integer Volume (0-256, default 256)
---@return integer emitter_id Emitter ID for controlling the sound
function PlaySoundAt(sample_id, x, y, z, priority, volume) end

---Stop a playing sound
---@param emitter_id integer Emitter ID from PlaySound/PlaySoundAt
function StopSound(emitter_id) end

---Check if a sound is playing
---@param emitter_id integer Emitter ID
---@return boolean is_playing True if sound is still playing
function IsSoundPlaying(emitter_id) end

---Play music track
---@param track integer|string Track number or filename
---@return boolean success True if successful
function PlayMusic(track) end

---Stop music playback
function StopMusic() end

---Pause music playback
function PauseMusic() end

---Resume music playback
function ResumeMusic() end

---Check if music is playing
---@return boolean is_playing True if music is playing
function IsMusicPlaying() end

---Set music volume
---@param volume integer Volume (0-127)
function SetMusicVolume(volume) end

---Play a speech message
---@param message_id integer Message ID (SMsg_*)
---@param duration? integer Duration in game turns (0 = default)
---@return boolean success True if successful
function PlayMessage(message_id, duration) end

---Play custom speech file
---@param filename string WAV filename
---@param duration? integer Duration in game turns (0 = default)
---@return boolean success True if successful
function PlayCustomMessage(filename, duration) end

---Stop all messages
function StopMessages() end

---Load a custom sound from file (runtime loading)
---@param name string Unique identifier for the sound
---@param filepath string Path to WAV file
---@return integer sample_id Sample ID for use with PlaySound
function LoadCustomSound(name, filepath) end

---Unload a custom sound
---@param name string Sound identifier
---@return boolean success True if successful
function UnloadCustomSound(name) end

---Get sample ID for a custom sound
---@param name string Sound identifier
---@return integer|nil sample_id Sample ID or nil if not found
function GetCustomSoundId(name) end

---Set master volume
---@param volume integer Volume (0-127)
function SetMasterVolume(volume) end

---Set sound effects volume
---@param volume integer Volume (0-127)
function SetSoundVolume(volume) end

---Get master volume
---@return integer volume Volume (0-127)
function GetMasterVolume() end

---Get sound effects volume
---@return integer volume Volume (0-127)
function GetSoundVolume() end

---Mute/unmute all audio
---@param mute boolean True to mute, false to unmute
function MuteAudio(mute) end

---Check if audio is muted
---@return boolean is_muted True if muted
function IsAudioMuted() end

---@class Thing
Thing = {}

---Play a creature sound
---@param sound_type integer|string Sound type (CreatureSoundType or "hurt", "slap", etc.)
---@param priority? integer Priority (1-6, default 3)
function Thing:playSound(sound_type, priority) end

---Stop a creature sound
---@param sound_type integer|string Sound type
function Thing:stopSound(sound_type) end

---Check if creature is playing a sound
---@param sound_type integer|string Sound type
---@return boolean is_playing True if sound is playing
function Thing:isSoundPlaying(sound_type) end
```

---

## 6. Migration Strategy

### 5.1 Gradual Migration Approach

**Phase 1: Add SoundManager alongside existing code**
- Create `SoundManager` class
- Implement C wrappers
- Keep existing functions working
- No breaking changes

**Phase 2: Migrate high-priority areas**
```c
// Before (scattered across multiple files)
play_creature_sound(thing, CrSnd_Slap, 3, 0);
thing_play_sample(thing, 100, NORMAL_PITCH, 0, 3, 0, 3, FULL_LOUDNESS);
play_music_track(5);

// After (unified)
sound_manager_play_creature_sound(thing, CrSnd_Slap, 3);
sound_manager_play_effect_on(100, thing, 3, FULL_LOUDNESS);
sound_manager_play_music_track(5);
```

**Phase 3: Deprecate old functions**
- Mark old functions as deprecated
- Redirect to SoundManager
- Add compiler warnings

**Phase 4: Remove old functions**
- Remove deprecated functions
- Update all call sites
- Final cleanup

### 5.2 Compatibility Macros

For easier migration:

```c
// src/sound_compat.h
#define PLAY_CREATURE_SOUND(thing, type, priority) \
    sound_manager_play_creature_sound(thing, type, priority)

#define PLAY_SOUND_AT(sample, pos, priority, volume) \
    sound_manager_play_effect_at(sample, pos, priority, volume)

#define PLAY_MUSIC(track) \
    sound_manager_play_music_track(track)
```

---

## 7. Runtime Loading System

### 6.1 Use Cases

**Campaign-Specific Sounds:**
```lua
-- In campaign init script
LoadCustomSound("dragon_roar", "campgns/mycampaign/sounds/dragon_roar.wav")
LoadCustomSound("epic_music", "campgns/mycampaign/music/boss_battle.ogg")

-- Use in game
local roar_id = GetCustomSoundId("dragon_roar")
PlaySound(roar_id, SoundPriority.High, SoundVolume.Loud)
```

**Level-Specific Sounds:**
```lua
-- In level script
function OnInit()
    LoadCustomSound("secret_door", "levels/secret/door_open.wav")
    LoadCustomSound("treasure_found", "levels/secret/treasure.wav")
end

function OnSecretFound(player)
    local treasure_id = GetCustomSoundId("treasure_found")
    PlaySound(treasure_id, SoundPriority.High)
end
```

**Creature-Specific Sounds:**
```lua
-- Load custom sounds for modded creature
function LoadPhoenixSounds()
    LoadCustomSound("phoenix_hurt_1", "creatures/phoenix/hurt1.wav")
    LoadCustomSound("phoenix_hurt_2", "creatures/phoenix/hurt2.wav")
    LoadCustomSound("phoenix_die", "creatures/phoenix/die.wav")
    LoadCustomSound("phoenix_resurrect", "creatures/phoenix/resurrect.wav")
end

-- Play custom sound on creature
function OnPhoenixHurt(phoenix_creature)
    local hurt_id = GetCustomSoundId("phoenix_hurt_1")
    if math.random() > 0.5 then
        hurt_id = GetCustomSoundId("phoenix_hurt_2")
    end
    PlaySoundAt(hurt_id, phoenix_creature.pos, SoundPriority.Medium)
end
```

### 6.2 Memory Management

**Automatic Cleanup:**
```cpp
class SoundManager {
    // Unload sounds when level ends
    void onLevelEnd() {
        for (auto& pair : custom_sounds_) {
            if (pair.second.loaded) {
                unloadCustomSound(pair.first);
            }
        }
        custom_sounds_.clear();
    }
};
```

**Manual Control:**
```lua
-- Preload sounds at level start
function OnInit()
    for i = 1, 10 do
        LoadCustomSound("ambient_" .. i, "sounds/ambient" .. i .. ".wav")
    end
end

-- Cleanup when done
function OnCleanup()
    for i = 1, 10 do
        UnloadCustomSound("ambient_" .. i)
    end
end
```

---

## 8. Example Use Cases

### 7.1 Basic Sound Effects

**Lua Script:**
```lua
-- Play door opening sound
local door_open_id = 250  -- Sample ID from sound bank
PlaySound(door_open_id, SoundPriority.Medium, SoundVolume.Normal)

-- Play explosion at specific location
local explosion_id = 300
local explosion_pos = Pos3d.new(5120, 5120, 256)
PlaySoundAt(explosion_id, explosion_pos, SoundPriority.High, SoundVolume.VeryLoud)
```

### 7.2 Creature Sounds

**Lua Script:**
```lua
-- Play hurt sound on creature
function OnCreatureTakesDamage(creature, damage)
    creature:playSound(CreatureSoundType.Hurt, SoundPriority.Medium)
    
    -- Play death sound if health too low
    if creature.health <= 0 then
        creature:playSound(CreatureSoundType.Die, SoundPriority.High)
    end
end

-- Check if creature is already making noise
function OnCreatureSlapped(creature)
    if not creature:isSoundPlaying(CreatureSoundType.Slap) then
        creature:playSound(CreatureSoundType.Slap, SoundPriority.High)
    end
end
```

### 7.3 Dynamic Music

**Lua Script:**
```lua
local current_music = nil
local battle_intensity = 0

function UpdateMusic()
    local creature_count = GetPlayerCreatureCount(PLAYER0)
    local enemy_count = GetPlayerCreatureCount(PLAYER1)
    
    if creature_count > 0 and enemy_count > 0 then
        -- Battle music
        battle_intensity = battle_intensity + 1
        if battle_intensity > 100 then
            if current_music ~= "battle_intense" then
                PlayMusic("music/battle_intense.ogg")
                current_music = "battle_intense"
            end
        elseif battle_intensity > 50 then
            if current_music ~= "battle_medium" then
                PlayMusic("music/battle_medium.ogg")
                current_music = "battle_medium"
            end
        end
    else
        -- Peace music
        battle_intensity = math.max(0, battle_intensity - 1)
        if battle_intensity == 0 and current_music ~= "ambient" then
            PlayMusic("music/ambient.ogg")
            current_music = "ambient"
        end
    end
end
```

### 7.4 Custom Campaign Sounds

**Campaign Structure:**
```
campgns/
  my_campaign/
    sounds/
      narrator_intro.wav
      narrator_victory.wav
      narrator_defeat.wav
      custom_spell.wav
    music/
      theme.ogg
      boss_battle.ogg
```

**Campaign Script:**
```lua
-- Load campaign-specific sounds
function CampaignInit()
    LoadCustomSound("narrator_intro", "campgns/my_campaign/sounds/narrator_intro.wav")
    LoadCustomSound("narrator_victory", "campgns/my_campaign/sounds/narrator_victory.wav")
    LoadCustomSound("narrator_defeat", "campgns/my_campaign/sounds/narrator_defeat.wav")
    LoadCustomSound("custom_spell", "campgns/my_campaign/sounds/custom_spell.wav")
    
    -- Play intro
    local intro_id = GetCustomSoundId("narrator_intro")
    PlayCustomMessage("campgns/my_campaign/sounds/narrator_intro.wav", 500)
    
    -- Play theme music
    PlayMusic("campgns/my_campaign/music/theme.ogg")
end

function OnLevelWon(player)
    if player == PLAYER0 then
        PlayCustomMessage("campgns/my_campaign/sounds/narrator_victory.wav", 300)
    end
end

function OnCustomSpellCast(creature)
    local spell_id = GetCustomSoundId("custom_spell")
    creature:playSound(spell_id, SoundPriority.High)
end
```

### 7.5 Interactive Sound System

**Lua Script:**
```lua
-- Sound zones that change ambient based on location
local sound_zones = {
    {pos = Pos3d.new(10, 10, 0), radius = 5, sound = "zone_water"},
    {pos = Pos3d.new(20, 20, 0), radius = 5, sound = "zone_fire"},
    {pos = Pos3d.new(30, 30, 0), radius = 5, sound = "zone_ice"},
}

function OnGameTick()
    local player_creatures = GetPlayerCreatures(PLAYER0)
    
    for _, creature in ipairs(player_creatures) do
        for _, zone in ipairs(sound_zones) do
            local distance = GetDistance(creature.pos, zone.pos)
            if distance < zone.radius then
                -- Play zone ambient if not already playing
                local zone_id = GetCustomSoundId(zone.sound)
                if zone_id and not creature:isSoundPlaying(zone_id) then
                    -- Play at zone location
                    PlaySoundAt(zone_id, zone.pos, SoundPriority.Low, SoundVolume.Quiet)
                end
            end
        end
    end
end
```

### 7.6 Advanced: Procedural Audio

**Lua Script:**
```lua
-- Dynamic pitch/volume based on game state
function PlayScaledSound(sample_id, scale_factor)
    local base_pitch = SoundPitch.Normal
    local base_volume = SoundVolume.Normal
    
    -- Scale pitch and volume
    local scaled_pitch = math.floor(base_pitch * scale_factor)
    local scaled_volume = math.floor(base_volume * scale_factor)
    
    -- Clamp values
    scaled_pitch = math.max(50, math.min(150, scaled_pitch))
    scaled_volume = math.max(0, math.min(256, scaled_volume))
    
    -- Note: This would require extended API to support pitch
    -- For now, just volume scaling
    PlaySound(sample_id, SoundPriority.Medium, scaled_volume)
end

-- Use in gameplay
function OnCreatureGrowsLevel(creature)
    local level = creature.level
    local scale = 0.8 + (level * 0.05)  -- Bigger creatures = deeper sounds
    PlayScaledSound(levelup_sound_id, scale)
end
```

---

## 9. Implementation Checklist

### Phase 1: Foundation (Week 1-2)
- [ ] Create `src/sound_manager.h` with complete class definition
- [ ] Implement `src/sound_manager.cpp` with basic functionality
- [ ] Add C wrapper functions for compatibility
- [ ] Test basic sound playback
- [ ] Test creature sound playback
- [ ] Test music playback

### Phase 2: Runtime Loading (Week 3-4)
- [ ] Implement `loadCustomSound()` function
- [ ] Implement `unloadCustomSound()` function
- [ ] Add WAV file parsing/loading
- [ ] Test custom sound loading and playback
- [ ] Add memory management and cleanup
- [ ] Document runtime loading system

### Phase 3: Lua Integration (Week 5-6)
- [ ] Create `src/lua_api_sound.c`
- [ ] Register global sound functions
- [ ] Extend Thing class with sound methods
- [ ] Add Lua constants file
- [ ] Add Lua type definitions
- [ ] Test Lua sound API
- [ ] Write Lua examples

### Phase 4: Documentation (Week 7)
- [ ] Complete API documentation
- [ ] Write migration guide
- [ ] Create example scripts
- [ ] Add debugging/troubleshooting section
- [ ] Update existing documentation

### Phase 5: Migration (Week 8+)
- [ ] Identify high-priority areas for migration
- [ ] Migrate magic_powers.c
- [ ] Migrate creature_states_combt.c
- [ ] Migrate other sound call sites
- [ ] Remove deprecated functions
- [ ] Final testing and cleanup

---

## 10. Benefits Summary

### For Developers
- **Single point of control** - All sound logic in one place
- **Easier testing** - Mock SoundManager for unit tests
- **Better debugging** - Centralized logging/profiling
- **Cleaner code** - No scattered sound calls

### For Modders
- **Lua scripting** - Full sound control from Lua
- **Runtime loading** - Load custom sounds dynamically
- **Easy integration** - Simple API for custom campaigns
- **Rich functionality** - 3D audio, volume control, etc.

### For Players
- **Better performance** - Optimized sound management
- **More variety** - Easy for modders to add custom sounds
- **Enhanced gameplay** - Dynamic audio based on game state
- **Compatibility** - Works with existing saves/campaigns

---

## Conclusion

The proposed `SoundManager` provides:
1. **Unified API** for all sound/music operations
2. **Runtime loading** for custom sounds without rebuilding
3. **Lua integration** for modder-friendly scripting
4. **Gradual migration** path from existing code
5. **Future-proof** architecture for enhancements

This design allows for incremental implementation while maintaining compatibility with existing code. The Lua API enables powerful modding capabilities without requiring C++ knowledge, making custom sound implementation accessible to a wider audience.

---

**Next Steps:**
1. Review and approve this design
2. Create implementation tasks/issues
3. Begin Phase 1 implementation
4. Iterate based on feedback

---

**Document Version:** 1.0  
**Date:** 2026-01-31  
**Author:** KeeperFX Team
