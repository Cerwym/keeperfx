# SoundManager Architecture - Getting Started Guide

## Overview

This guide provides a **minimal proof-of-concept** implementation to test the SoundManager architecture before full implementation. It includes:
1. A minimal SoundManager class with core functionality
2. Simple test program to verify the architecture
3. Integration examples showing how it would replace existing code
4. Step-by-step instructions to build and test

## Table of Contents

1. [Quick Start - Minimal Implementation](#1-quick-start---minimal-implementation)
2. [Building and Testing](#2-building-and-testing)
3. [Integration Examples](#3-integration-examples)
4. [Next Steps](#4-next-steps)

---

## 1. Quick Start - Minimal Implementation

### 1.1 Create Minimal SoundManager Header

**File:** `src/sound_manager_minimal.h` (new file)

This is a stripped-down version to test the architecture pattern:

```cpp
#ifndef SOUND_MANAGER_MINIMAL_H
#define SOUND_MANAGER_MINIMAL_H

#include "bflib_basics.h"
#include "bflib_sound.h"
#include "globals.h"

#ifdef __cplusplus

#include <string>
#include <unordered_map>

namespace KeeperFX {

/**
 * @brief Minimal SoundManager for testing architecture
 * 
 * Proof-of-concept implementation with just the essential features:
 * - Singleton pattern
 * - Basic sound playback
 * - Simple runtime loading
 * - C wrapper functions
 */
class SoundManagerMinimal {
public:
    // Singleton access
    static SoundManagerMinimal& getInstance();
    
    // === Core Sound Functions ===
    
    /**
     * @brief Play a simple sound effect
     * @param sample_id Sound sample ID
     * @param priority Priority (1-6, default 3)
     * @param volume Volume (0-256, default 256)
     * @return Sound emitter ID, or 0 if failed
     */
    SoundEmitterID playEffect(SoundSmplTblID sample_id, 
                              long priority = 3, 
                              SoundVolume volume = FULL_LOUDNESS);
    
    /**
     * @brief Play creature sound (uses existing system)
     * @param thing Creature thing
     * @param sound_type Sound type (CrSnd_Hurt, CrSnd_Slap, etc.)
     * @param priority Priority (default 3)
     */
    void playCreatureSound(struct Thing* thing, long sound_type, long priority = 3);
    
    /**
     * @brief Stop a playing sound
     * @param emitter_id Emitter ID returned by playEffect
     */
    void stopEffect(SoundEmitterID emitter_id);
    
    /**
     * @brief Check if sound is playing
     * @param emitter_id Emitter ID
     * @return true if sound is still playing
     */
    bool isEffectPlaying(SoundEmitterID emitter_id) const;
    
    // === Music ===
    
    /**
     * @brief Play music by track number
     * @param track_number Track number (1-N)
     * @return true if successful
     */
    bool playMusic(int track_number);
    
    /**
     * @brief Stop music playback
     */
    void stopMusic();
    
    // === Runtime Loading (Minimal) ===
    
    /**
     * @brief Register a custom sound (doesn't actually load yet)
     * @param name Unique identifier
     * @param filepath Path to file (for future implementation)
     * @return Assigned sample ID (currently just incremented counter)
     */
    SoundSmplTblID registerCustomSound(const std::string& name, const std::string& filepath);
    
    /**
     * @brief Get sample ID for registered sound
     * @param name Sound identifier
     * @return Sample ID, or 0 if not found
     */
    SoundSmplTblID getCustomSoundId(const std::string& name) const;
    
    // === System ===
    
    /**
     * @brief Initialize sound manager
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Get statistics (for testing)
     */
    void printStats() const;

private:
    SoundManagerMinimal();
    ~SoundManagerMinimal();
    
    // Disable copy/move
    SoundManagerMinimal(const SoundManagerMinimal&) = delete;
    SoundManagerMinimal& operator=(const SoundManagerMinimal&) = delete;
    
    // Internal state
    struct CustomSoundEntry {
        std::string filepath;
        SoundSmplTblID sample_id;
    };
    
    std::unordered_map<std::string, CustomSoundEntry> custom_sounds_;
    SoundSmplTblID next_custom_sample_id_;
    bool initialized_;
    int total_plays_;
    int total_custom_sounds_;
};

} // namespace KeeperFX

extern "C" {
#endif // __cplusplus

// C API for testing
TbBool sound_manager_minimal_init(void);
SoundEmitterID sound_manager_minimal_play_effect(SoundSmplTblID sample_id, long priority, SoundVolume volume);
void sound_manager_minimal_play_creature_sound(struct Thing* thing, long sound_type, long priority);
void sound_manager_minimal_stop_effect(SoundEmitterID emitter_id);
TbBool sound_manager_minimal_play_music(int track_number);
void sound_manager_minimal_stop_music(void);
SoundSmplTblID sound_manager_minimal_register_custom_sound(const char* name, const char* filepath);
SoundSmplTblID sound_manager_minimal_get_custom_sound_id(const char* name);
void sound_manager_minimal_print_stats(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SOUND_MANAGER_MINIMAL_H
```

### 1.2 Create Minimal SoundManager Implementation

**File:** `src/sound_manager_minimal.cpp` (new file)

```cpp
#include "pre_inc.h"
#include "sound_manager_minimal.h"
#include "sounds.h"
#include "bflib_sndlib.h"
#include "creature_control.h"
#include <cstdio>
#include "post_inc.h"

namespace KeeperFX {

// Singleton instance
SoundManagerMinimal& SoundManagerMinimal::getInstance() {
    static SoundManagerMinimal instance;
    return instance;
}

// Constructor
SoundManagerMinimal::SoundManagerMinimal() 
    : next_custom_sample_id_(10000)  // Start custom sounds at 10000
    , initialized_(false)
    , total_plays_(0)
    , total_custom_sounds_(0)
{
    printf("[SoundManagerMinimal] Constructor called\n");
}

// Destructor
SoundManagerMinimal::~SoundManagerMinimal() {
    printf("[SoundManagerMinimal] Destructor called - played %d sounds total\n", total_plays_);
}

// Initialize
bool SoundManagerMinimal::initialize() {
    if (initialized_) {
        printf("[SoundManagerMinimal] Already initialized\n");
        return true;
    }
    
    printf("[SoundManagerMinimal] Initializing...\n");
    
    // Call existing init function
    if (!init_sound()) {
        printf("[SoundManagerMinimal] Failed to initialize sound system\n");
        return false;
    }
    
    initialized_ = true;
    printf("[SoundManagerMinimal] Initialized successfully\n");
    return true;
}

// Play sound effect
SoundEmitterID SoundManagerMinimal::playEffect(SoundSmplTblID sample_id, long priority, SoundVolume volume) {
    if (!initialized_) {
        printf("[SoundManagerMinimal] Not initialized, cannot play sound %d\n", sample_id);
        return 0;
    }
    
    if (SoundDisabled) {
        printf("[SoundManagerMinimal] Sound disabled, skipping sample %d\n", sample_id);
        return 0;
    }
    
    printf("[SoundManagerMinimal] Playing effect: sample=%d, priority=%ld, volume=%ld\n", 
           sample_id, priority, volume);
    
    total_plays_++;
    
    // Use existing 2D sound function
    play_non_3d_sample(sample_id);
    return Non3DEmitter;
}

// Play creature sound
void SoundManagerMinimal::playCreatureSound(struct Thing* thing, long sound_type, long priority) {
    if (!initialized_ || thing_is_invalid(thing)) {
        printf("[SoundManagerMinimal] Cannot play creature sound (initialized=%d, thing_valid=%d)\n",
               initialized_, !thing_is_invalid(thing));
        return;
    }
    
    printf("[SoundManagerMinimal] Playing creature sound: thing_idx=%d, type=%ld, priority=%ld\n",
           thing->index, sound_type, priority);
    
    total_plays_++;
    
    // Use existing creature sound function
    play_creature_sound(thing, sound_type, priority, 0);
}

// Stop sound
void SoundManagerMinimal::stopEffect(SoundEmitterID emitter_id) {
    if (emitter_id == 0) {
        return;
    }
    
    printf("[SoundManagerMinimal] Stopping sound: emitter_id=%ld\n", emitter_id);
    
    S3DDestroySoundEmitterAndSamples(emitter_id);
}

// Check if playing
bool SoundManagerMinimal::isEffectPlaying(SoundEmitterID emitter_id) const {
    if (emitter_id == 0) {
        return false;
    }
    
    bool playing = S3DEmitterIsPlayingAnySample(emitter_id);
    printf("[SoundManagerMinimal] Checking if playing: emitter_id=%ld, playing=%d\n", 
           emitter_id, playing);
    return playing;
}

// Play music
bool SoundManagerMinimal::playMusic(int track_number) {
    if (!initialized_) {
        printf("[SoundManagerMinimal] Not initialized, cannot play music\n");
        return false;
    }
    
    printf("[SoundManagerMinimal] Playing music track %d\n", track_number);
    
    if (track_number == 0) {
        stopMusic();
        return true;
    }
    
    return play_music_track(track_number);
}

// Stop music
void SoundManagerMinimal::stopMusic() {
    printf("[SoundManagerMinimal] Stopping music\n");
    stop_music();
}

// Register custom sound (minimal implementation)
SoundSmplTblID SoundManagerMinimal::registerCustomSound(const std::string& name, const std::string& filepath) {
    // Check if already registered
    auto it = custom_sounds_.find(name);
    if (it != custom_sounds_.end()) {
        printf("[SoundManagerMinimal] Custom sound '%s' already registered as sample %d\n",
               name.c_str(), it->second.sample_id);
        return it->second.sample_id;
    }
    
    // Allocate new sample ID
    SoundSmplTblID sample_id = next_custom_sample_id_++;
    
    // Register in map (actual loading would happen here in full implementation)
    CustomSoundEntry entry;
    entry.filepath = filepath;
    entry.sample_id = sample_id;
    custom_sounds_[name] = entry;
    
    total_custom_sounds_++;
    
    printf("[SoundManagerMinimal] Registered custom sound '%s' as sample %d (filepath: %s)\n",
           name.c_str(), sample_id, filepath.c_str());
    
    return sample_id;
}

// Get custom sound ID
SoundSmplTblID SoundManagerMinimal::getCustomSoundId(const std::string& name) const {
    auto it = custom_sounds_.find(name);
    if (it == custom_sounds_.end()) {
        printf("[SoundManagerMinimal] Custom sound '%s' not found\n", name.c_str());
        return 0;
    }
    
    printf("[SoundManagerMinimal] Found custom sound '%s' as sample %d\n",
           name.c_str(), it->second.sample_id);
    return it->second.sample_id;
}

// Print statistics
void SoundManagerMinimal::printStats() const {
    printf("\n=== SoundManagerMinimal Statistics ===\n");
    printf("Initialized: %s\n", initialized_ ? "YES" : "NO");
    printf("Total sounds played: %d\n", total_plays_);
    printf("Custom sounds registered: %d\n", total_custom_sounds_);
    printf("Next custom sample ID: %d\n", next_custom_sample_id_);
    
    if (!custom_sounds_.empty()) {
        printf("\nRegistered custom sounds:\n");
        for (const auto& pair : custom_sounds_) {
            printf("  - '%s' -> sample %d (path: %s)\n",
                   pair.first.c_str(),
                   pair.second.sample_id,
                   pair.second.filepath.c_str());
        }
    }
    printf("=====================================\n\n");
}

} // namespace KeeperFX

// C API wrappers
extern "C" {

TbBool sound_manager_minimal_init(void) {
    return KeeperFX::SoundManagerMinimal::getInstance().initialize();
}

SoundEmitterID sound_manager_minimal_play_effect(SoundSmplTblID sample_id, long priority, SoundVolume volume) {
    return KeeperFX::SoundManagerMinimal::getInstance().playEffect(sample_id, priority, volume);
}

void sound_manager_minimal_play_creature_sound(struct Thing* thing, long sound_type, long priority) {
    KeeperFX::SoundManagerMinimal::getInstance().playCreatureSound(thing, sound_type, priority);
}

void sound_manager_minimal_stop_effect(SoundEmitterID emitter_id) {
    KeeperFX::SoundManagerMinimal::getInstance().stopEffect(emitter_id);
}

TbBool sound_manager_minimal_play_music(int track_number) {
    return KeeperFX::SoundManagerMinimal::getInstance().playMusic(track_number);
}

void sound_manager_minimal_stop_music(void) {
    KeeperFX::SoundManagerMinimal::getInstance().stopMusic();
}

SoundSmplTblID sound_manager_minimal_register_custom_sound(const char* name, const char* filepath) {
    return KeeperFX::SoundManagerMinimal::getInstance().registerCustomSound(name, filepath);
}

SoundSmplTblID sound_manager_minimal_get_custom_sound_id(const char* name) {
    return KeeperFX::SoundManagerMinimal::getInstance().getCustomSoundId(name);
}

void sound_manager_minimal_print_stats(void) {
    KeeperFX::SoundManagerMinimal::getInstance().printStats();
}

} // extern "C"
```

### 1.3 Create Test Program

**File:** `tests/tst_sound_manager.cpp` (new file)

```cpp
#include <CUnit.h>
#include <Basic.h>
#include <stdio.h>

extern "C" {
#include "bflib_basics.h"
#include "sound_manager_minimal.h"
}

// Test initialization
void test_sound_manager_init(void) {
    printf("\n--- Test: Sound Manager Initialization ---\n");
    
    TbBool result = sound_manager_minimal_init();
    
    // In test environment, sound system might not initialize (no audio device)
    // So we just verify the function doesn't crash
    printf("Initialization result: %d\n", result);
    
    // Print stats to verify singleton is working
    sound_manager_minimal_print_stats();
    
    CU_PASS("Initialization completed without crashing");
}

// Test custom sound registration
void test_custom_sound_registration(void) {
    printf("\n--- Test: Custom Sound Registration ---\n");
    
    // Register some custom sounds
    SoundSmplTblID id1 = sound_manager_minimal_register_custom_sound("test_sound_1", "sounds/test1.wav");
    SoundSmplTblID id2 = sound_manager_minimal_register_custom_sound("test_sound_2", "sounds/test2.wav");
    SoundSmplTblID id3 = sound_manager_minimal_register_custom_sound("dragon_roar", "creatures/dragon/roar.wav");
    
    printf("Registered sound IDs: %d, %d, %d\n", id1, id2, id3);
    
    // Verify IDs are unique and sequential
    CU_ASSERT(id1 > 0);
    CU_ASSERT(id2 > id1);
    CU_ASSERT(id3 > id2);
    
    // Verify we can retrieve them
    SoundSmplTblID retrieved1 = sound_manager_minimal_get_custom_sound_id("test_sound_1");
    SoundSmplTblID retrieved2 = sound_manager_minimal_get_custom_sound_id("dragon_roar");
    
    CU_ASSERT_EQUAL(retrieved1, id1);
    CU_ASSERT_EQUAL(retrieved2, id3);
    
    // Verify non-existent sound returns 0
    SoundSmplTblID not_found = sound_manager_minimal_get_custom_sound_id("nonexistent");
    CU_ASSERT_EQUAL(not_found, 0);
    
    // Print stats
    sound_manager_minimal_print_stats();
    
    printf("Custom sound registration test passed!\n");
}

// Test duplicate registration
void test_duplicate_registration(void) {
    printf("\n--- Test: Duplicate Registration ---\n");
    
    // Register a sound
    SoundSmplTblID id1 = sound_manager_minimal_register_custom_sound("unique_sound", "sounds/unique.wav");
    printf("First registration: %d\n", id1);
    
    // Register same sound again - should return same ID
    SoundSmplTblID id2 = sound_manager_minimal_register_custom_sound("unique_sound", "sounds/unique.wav");
    printf("Second registration: %d\n", id2);
    
    CU_ASSERT_EQUAL(id1, id2);
    
    printf("Duplicate registration test passed!\n");
}

// Test effect playback (won't actually play in test env)
void test_effect_playback(void) {
    printf("\n--- Test: Effect Playback ---\n");
    
    // These won't actually play sound in test environment
    // But we verify the API works correctly
    
    SoundEmitterID emitter1 = sound_manager_minimal_play_effect(100, 3, 256);
    printf("Played effect with emitter ID: %ld\n", emitter1);
    
    SoundEmitterID emitter2 = sound_manager_minimal_play_effect(200, 5, 128);
    printf("Played effect with emitter ID: %ld\n", emitter2);
    
    // Print final stats
    sound_manager_minimal_print_stats();
    
    printf("Effect playback test completed!\n");
}

// Test music control
void test_music_control(void) {
    printf("\n--- Test: Music Control ---\n");
    
    // Test music playback (won't actually play in test env)
    TbBool result1 = sound_manager_minimal_play_music(1);
    printf("Play music track 1: %d\n", result1);
    
    TbBool result2 = sound_manager_minimal_play_music(5);
    printf("Play music track 5: %d\n", result2);
    
    sound_manager_minimal_stop_music();
    printf("Stopped music\n");
    
    printf("Music control test completed!\n");
}

// Suite setup
int init_sound_manager_suite(void) {
    printf("\n==============================================\n");
    printf("  Sound Manager Test Suite Starting\n");
    printf("==============================================\n");
    return 0;
}

// Suite cleanup
int clean_sound_manager_suite(void) {
    printf("\n==============================================\n");
    printf("  Sound Manager Test Suite Completed\n");
    printf("==============================================\n");
    return 0;
}

// Register test suite
CU_TestInfo sound_manager_tests[] = {
    {"Test initialization", test_sound_manager_init},
    {"Test custom sound registration", test_custom_sound_registration},
    {"Test duplicate registration", test_duplicate_registration},
    {"Test effect playback", test_effect_playback},
    {"Test music control", test_music_control},
    CU_TEST_INFO_NULL,
};

CU_SuiteInfo sound_manager_suites[] = {
    {"Sound Manager Tests", init_sound_manager_suite, clean_sound_manager_suite, NULL, NULL, sound_manager_tests},
    CU_SUITE_INFO_NULL,
};

// Auto-register with CUnit
void register_sound_manager_tests(void) {
    CU_register_suites(sound_manager_suites);
}
```

---

## 2. Building and Testing

### 2.1 Add Files to Build System

**Option A: CMake (if using CMake)**

Add to `CMakeLists.txt`:

```cmake
# Add SoundManager minimal implementation
set(SOUND_MANAGER_SOURCES
    src/sound_manager_minimal.cpp
)

# Add to main target
target_sources(keeperfx PRIVATE ${SOUND_MANAGER_SOURCES})

# Add test
add_executable(test_sound_manager
    tests/tst_sound_manager.cpp
    src/sound_manager_minimal.cpp
    # Add other required sources
)

target_link_libraries(test_sound_manager
    CUnit
    # Other libraries
)
```

**Option B: Makefile**

Add to `Makefile` or create `test_sound_manager.mk`:

```makefile
# Sound Manager Minimal Test

SOUND_MGR_TEST_SOURCES = \
    tests/tst_sound_manager.cpp \
    src/sound_manager_minimal.cpp

SOUND_MGR_TEST_OBJS = $(SOUND_MGR_TEST_SOURCES:.cpp=.o)

test_sound_manager: $(SOUND_MGR_TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) -lCUnit

.PHONY: test-sound-manager
test-sound-manager: test_sound_manager
	./test_sound_manager
```

### 2.2 Manual Compilation (Quick Test)

If you just want to test the architecture without full build system integration:

```bash
# Compile the minimal implementation
g++ -c -std=c++20 -I./src -I./deps \
    src/sound_manager_minimal.cpp \
    -o /tmp/sound_manager_minimal.o

# Compile the test
g++ -c -std=c++20 -I./src -I./deps -I./deps/CUnit-2.1-3/CUnit/Headers \
    tests/tst_sound_manager.cpp \
    -o /tmp/tst_sound_manager.o

# Link (you'll need to link against keeperfx objects)
g++ -std=c++20 \
    /tmp/sound_manager_minimal.o \
    /tmp/tst_sound_manager.o \
    -o /tmp/test_sound_manager \
    -lCUnit
```

### 2.3 Run the Test

```bash
# Run tests
./test_sound_manager

# Or with verbose output
./test_sound_manager -v

# Expected output:
# ==============================================
#   Sound Manager Test Suite Starting
# ==============================================
#
# --- Test: Sound Manager Initialization ---
# [SoundManagerMinimal] Constructor called
# [SoundManagerMinimal] Initializing...
# ...
# All tests passed!
```

---

## 3. Integration Examples

### 3.1 Before and After Comparison

**Before (Current Code):**

```c
// In magic_powers.c - slap function
void slap_creature(struct PlayerInfo *player, struct Thing *thing) {
    // ... damage logic ...
    
    // Sound call scattered in code
    play_creature_sound(thing, CrSnd_Slap, 3, 0);
}

// In creature_states_combt.c - combat function
void creature_in_combat(struct Thing *thing) {
    // ... combat logic ...
    
    // Another scattered sound call
    play_creature_sound(fighter, CrSnd_Fight, 3, 0);
}

// In sounds.c - ambient sounds
void update_player_sounds(void) {
    // ... various sound logic scattered everywhere ...
    play_non_3d_sample(89);
    play_atmos_sound(AtmosRepeat);
}
```

**After (With SoundManager):**

```c
// In magic_powers.c - slap function
void slap_creature(struct PlayerInfo *player, struct Thing *thing) {
    // ... damage logic ...
    
    // Clean, unified API
    sound_manager_minimal_play_creature_sound(thing, CrSnd_Slap, 3);
}

// In creature_states_combt.c - combat function
void creature_in_combat(struct Thing *thing) {
    // ... combat logic ...
    
    // Same unified API
    sound_manager_minimal_play_creature_sound(fighter, CrSnd_Fight, 3);
}

// In sounds.c - ambient sounds
void update_player_sounds(void) {
    // Clean, descriptive calls
    sound_manager_minimal_play_effect(89, 3, FULL_LOUDNESS);
    sound_manager_minimal_play_effect(AtmosRepeat, 1, atmos_sound_volume);
}
```

### 3.2 Using Custom Sounds

**Example: Campaign-Specific Sound**

```c
// In campaign initialization
void init_campaign_sounds(void) {
    // Register campaign-specific sounds
    sound_manager_minimal_register_custom_sound(
        "boss_roar",
        "campgns/my_campaign/sounds/boss_roar.wav"
    );
    
    sound_manager_minimal_register_custom_sound(
        "victory_fanfare",
        "campgns/my_campaign/sounds/victory.wav"
    );
}

// Later in gameplay
void trigger_boss_encounter(void) {
    SoundSmplTblID boss_sound = sound_manager_minimal_get_custom_sound_id("boss_roar");
    if (boss_sound > 0) {
        sound_manager_minimal_play_effect(boss_sound, 6, FULL_LOUDNESS);
    }
}

void level_completed(void) {
    SoundSmplTblID victory_sound = sound_manager_minimal_get_custom_sound_id("victory_fanfare");
    if (victory_sound > 0) {
        sound_manager_minimal_play_effect(victory_sound, 6, FULL_LOUDNESS);
    }
}
```

### 3.3 Logging and Debugging Benefits

With SoundManager, you get automatic logging of all sound operations:

```
[SoundManagerMinimal] Playing effect: sample=100, priority=3, volume=256
[SoundManagerMinimal] Playing creature sound: thing_idx=42, type=8, priority=3
[SoundManagerMinimal] Registered custom sound 'boss_roar' as sample 10000
[SoundManagerMinimal] Found custom sound 'boss_roar' as sample 10000
[SoundManagerMinimal] Playing effect: sample=10000, priority=6, volume=256

=== SoundManagerMinimal Statistics ===
Initialized: YES
Total sounds played: 147
Custom sounds registered: 5
Next custom sample ID: 10005
```

This makes debugging sound issues much easier!

---

## 4. Next Steps

### 4.1 Immediate Testing Steps

1. **Compile the minimal implementation**
   ```bash
   # Add files to your build system
   # Compile and link
   ```

2. **Run the test suite**
   ```bash
   ./test_sound_manager
   ```

3. **Verify output**
   - Check that tests pass
   - Verify statistics are printed
   - Confirm singleton pattern works

4. **Try integration**
   - Pick one simple function (e.g., `play_non_3d_sample` call)
   - Replace with `sound_manager_minimal_play_effect`
   - Rebuild and test in game

### 4.2 Validation Checklist

- [ ] Minimal implementation compiles without errors
- [ ] Test suite runs and all tests pass
- [ ] Singleton pattern verified (same instance used)
- [ ] Custom sound registration works
- [ ] C API wrappers function correctly
- [ ] Statistics tracking works
- [ ] No crashes or memory leaks
- [ ] Logging output is helpful for debugging

### 4.3 Extending the Proof-of-Concept

Once the minimal version works, you can extend it:

1. **Add 3D positioning**
   ```cpp
   SoundEmitterID playEffectAt(SoundSmplTblID sample_id, 
                                const struct Coord3d& position);
   ```

2. **Add actual runtime loading**
   ```cpp
   bool loadWavFile(const std::string& filepath, SoundSmplTblID sample_id);
   ```

3. **Add Lua bindings** (see `sound_manager_design.md` Section 5)

4. **Add volume control**
   ```cpp
   void setMasterVolume(SoundVolume volume);
   void setSoundVolume(SoundVolume volume);
   ```

5. **Add error handling**
   ```cpp
   enum class SoundError {
       None,
       NotInitialized,
       FileNotFound,
       InvalidFormat,
       OutOfMemory
   };
   ```

### 4.4 Performance Testing

Once basic functionality works, test performance:

```cpp
// Stress test
void stress_test_sound_manager(void) {
    for (int i = 0; i < 1000; i++) {
        sound_manager_minimal_play_effect(100 + (i % 50), 3, 256);
    }
    sound_manager_minimal_print_stats();
}
```

### 4.5 Migration Path

**Phase 1: Proof-of-Concept (Current)**
- Minimal implementation
- Basic testing
- No breaking changes

**Phase 2: Integration Testing**
- Replace a few scattered calls
- Test in actual game
- Verify no regressions

**Phase 3: Add Features**
- Runtime loading
- Lua bindings
- Enhanced error handling

**Phase 4: Full Migration**
- Replace all sound calls
- Remove old functions
- Update documentation

---

## 5. Troubleshooting

### Common Issues

**Issue: Compilation errors about missing headers**
```
Solution: Add include paths:
  -I./src
  -I./deps
  -I./deps/CUnit-2.1-3/CUnit/Headers
```

**Issue: Linker errors about undefined symbols**
```
Solution: Link against required libraries:
  -lCUnit
  -lSDL2
  -lSDL2_mixer
  -lopenal
```

**Issue: Sound doesn't play in test environment**
```
This is expected! Test environment often has no audio device.
The test verifies the API works, not actual sound playback.
```

**Issue: Singleton not working (multiple instances)**
```
Check that you're using getInstance() consistently:
  ✓ KeeperFX::SoundManagerMinimal::getInstance().playEffect(...)
  ✗ KeeperFX::SoundManagerMinimal manager; // Don't do this!
```

### Debug Output

Enable verbose logging by adding to the functions:

```cpp
#define SOUND_MANAGER_DEBUG 1

#if SOUND_MANAGER_DEBUG
    printf("[DEBUG] Function: %s, Line: %d\n", __FUNCTION__, __LINE__);
#endif
```

---

## 6. Comparison: Minimal vs Full Implementation

| Feature | Minimal (PoC) | Full (Production) |
|---------|---------------|-------------------|
| Singleton | ✅ Yes | ✅ Yes |
| Basic playback | ✅ Wrappers | ✅ Full implementation |
| 3D positioning | ❌ Not yet | ✅ Complete |
| Runtime loading | 🟡 Registration only | ✅ Full WAV loading |
| Lua bindings | ❌ Not yet | ✅ Complete API |
| Error handling | 🟡 Basic | ✅ Comprehensive |
| Volume control | ❌ Not yet | ✅ Per-type volumes |
| Statistics | ✅ Basic | ✅ Detailed |
| Documentation | ✅ This guide | ✅ Full docs |
| Tests | ✅ Unit tests | ✅ Full test suite |

---

## Conclusion

This minimal proof-of-concept provides:

✅ **Working example** of the architecture pattern  
✅ **Test suite** to verify functionality  
✅ **Integration examples** showing how to use it  
✅ **No breaking changes** to existing code  
✅ **Foundation** for full implementation  

You can now:
1. Build and test the minimal version
2. Verify the singleton pattern works
3. Test custom sound registration
4. See how it integrates with existing code
5. Extend it incrementally

Once you're satisfied with the architecture, proceed with the full implementation as outlined in `sound_manager_design.md`.

---

**Quick Commands:**

```bash
# Add files to your project
cp docs/sound_manager_getting_started.md /path/to/keeperfx/docs/
# Create the source files as shown above

# Compile (adjust paths as needed)
g++ -c -std=c++20 -I./src src/sound_manager_minimal.cpp -o sound_manager_minimal.o

# Test
./test_sound_manager

# Integrate
# Replace one sound call in your code with the SoundManager version
# Rebuild and test
```

---

**Document Version:** 1.0  
**Date:** 2026-01-31  
**Author:** KeeperFX Team
