# KeeperFX Sound System Architecture

## Overview

This document provides a comprehensive explanation of how sound effects are played in KeeperFX, covering:
- How sounds are loaded and stored
- How creature sounds are configured and triggered
- How room ambient sounds work
- Sound file formats and locations
- How to implement custom sounds at model, campaign, or creature level

## Table of Contents

1. [Sound Loading & Storage System](#1-sound-loading--storage-system)
2. [Creature Sound Configuration](#2-creature-sound-configuration)
3. [Room Ambient Sound System](#3-room-ambient-sound-system)
4. [Sound Triggering Mechanisms](#4-sound-triggering-mechanisms)
5. [Sound File Formats & Locations](#5-sound-file-formats--locations)
6. [3D Sound System](#6-3d-sound-system)
7. [Custom Sound Implementation Guide](#7-custom-sound-implementation-guide)

---

## 1. Sound Loading & Storage System

### 1.1 Sound Bank Files

KeeperFX uses a packed sound bank system with two main files:

- **`sound.dat`** - Primary sound effects bank containing all game SFX samples
- **`speech.dat`** (or `speech_[LANG].dat`) - Speech and voice samples for different languages

Both files are stored in the `FGrp_LrgSound` directory (typically `/data/sound/` or game root).

### 1.2 Sound Bank Format

The sound bank uses a custom binary format:

**Header Structure** (18 bytes):
- 14-byte signature
- 32-bit version number

**Entry Tables** (9 banks total):
Each `SoundBankEntry` is 16 bytes containing:
- `first_sample_offset` - Offset to first sample metadata
- `first_data_offset` - Offset to first audio data
- `total_samples_size` - Total size of all samples in bytes
- `entries_count` - Number of samples in this bank

**Sample Metadata** (32 bytes per sample):
- `filename` (18 bytes) - Original filename
- `data_offset` - Offset to audio data in file
- `sample_rate` - Sample rate in Hz (e.g., 22050)
- `data_size` - Size of audio data in bytes
- `sfxid` - Sound effect identifier
- `format_flags` - Audio format flags

### 1.3 Loading Process

**Function:** `load_sound_banks()` in `src/bflib_sndlib.cpp` (lines 418-433)

The loading sequence:

1. Opens `sound.dat` and `speech.dat` files
2. Reads header offset from the last 4 bytes of the file
3. Parses `SoundBankEntry` tables (directory index 2 is used for samples)
4. Creates global sound banks (`g_banks[0]` and `g_banks[1]`) as `std::vector<sound_sample>`
5. Each sample is stored in an OpenAL buffer with wave format conversion

### 1.4 Memory Management

Sound data is managed through:
- **Game Heap** (`src/game_heap.c`) - Large memory pool for expensive sound/graphics data
- **OpenAL Buffers** - Audio data stored in OpenAL buffer objects
- **Source Pool** - Reusable OpenAL sources (limited by `max_number_of_samples`)

---

## 2. Creature Sound Configuration

### 2.1 CreatureSounds Structure

**Location:** `src/creature_control.h` (lines 429-442)

Each creature model has a `CreatureSounds` structure containing 12 sound types:

```c
struct CreatureSounds {
    struct CreatureSound foot;      // Walking/footsteps (CrSnd_Foot = 10)
    struct CreatureSound hit;       // Hit attack (CrSnd_Hit = 2)
    struct CreatureSound happy;     // Happy mood (CrSnd_Happy = 3)
    struct CreatureSound sad;       // Sad mood (CrSnd_Sad = 4)
    struct CreatureSound hurt;      // Taking damage (CrSnd_Hurt = 1)
    struct CreatureSound die;       // Death sound (CrSnd_Die = 9)
    struct CreatureSound hang;      // Hanging/dragging (CrSnd_Hang = 5)
    struct CreatureSound drop;      // Being dropped (CrSnd_Drop = 6)
    struct CreatureSound torture;   // During torture (CrSnd_Torture = 7)
    struct CreatureSound slap;      // When slapped (CrSnd_Slap = 8)
    struct CreatureSound fight;     // Combat (CrSnd_Fight = 11)
    struct CreatureSound piss;      // Urinating (CrSnd_Piss = 12)
};
```

Each `CreatureSound` contains:
- `index` - Starting sample ID in the sound bank
- `count` - Number of sound variations (for random selection)

### 2.2 Sound Type Enumeration

**Location:** `src/creature_control.h` (lines 58-72)

```c
enum CreatureSoundTypes {
    CrSnd_None      = 0,
    CrSnd_Hurt      = 1,   // When creature takes damage
    CrSnd_Hit       = 2,   // When creature hits an enemy
    CrSnd_Happy     = 3,   // When creature is happy
    CrSnd_Sad       = 4,   // When creature is sad/unhappy
    CrSnd_Hang      = 5,   // When creature is picked up/hanging
    CrSnd_Drop      = 6,   // When creature is dropped
    CrSnd_Torture   = 7,   // When creature is being tortured
    CrSnd_Slap      = 8,   // When creature is slapped
    CrSnd_Die       = 9,   // When creature dies
    CrSnd_Foot      = 10,  // Footstep sounds
    CrSnd_Fight     = 11,  // During combat
    CrSnd_Piss      = 12,  // When creature is urinating
};
```

### 2.3 Configuration Storage

Creature sounds are stored in the global game configuration:

**Location:** `game.conf.crtr_conf.creature_sounds[CREATURE_MODEL]`

- Array indexed by creature model (max 128 types)
- Configured in creature config files (e.g., `config/fxdata/creature.cfg`)
- Can be overridden via level scripts

### 2.4 Configuration File Format

**File:** `config/fxdata/creature.cfg`

Example creature sound configuration:

```ini
[creature_wizard]
Name = WIZARD
; ... other attributes ...

[sounds_wizard]
Foot = 100 4        ; Starting index 100, 4 variations
Hit = 110 2         ; Starting index 110, 2 variations
Happy = 115 3       ; Starting index 115, 3 variations
Sad = 120 2         ; Starting index 120, 2 variations
Hurt = 125 3        ; Starting index 125, 3 variations
Die = 130 2         ; Starting index 130, 2 variations
Hang = 135 1        ; Starting index 135, 1 variation
Drop = 136 1        ; Starting index 136, 1 variation
Torture = 137 2     ; Starting index 137, 2 variations
Slap = 140 2        ; Starting index 140, 2 variations
Fight = 145 3       ; Starting index 145, 3 variations
Piss = 150 1        ; Starting index 150, 1 variation
```

### 2.5 Accessing Creature Sounds

**Function:** `get_creature_sound()` in `src/creature_control.c` (lines 222-228)

```c
struct CreatureSound *get_creature_sound(struct Thing *thing, long snd_idx)
{
    ThingModel cmodel = thing->model;
    if ((cmodel < 1) || (cmodel >= game.conf.crtr_conf.model_count))
    {
        ERRORLOG("Trying to get sound for undefined creature type %d", (int)cmodel);
        return &game.conf.crtr_conf.creature_sounds[0].foot;
    }
    struct CreatureSounds *crsounds = &game.conf.crtr_conf.creature_sounds[cmodel];
    // Return appropriate CreatureSound based on snd_idx (CrSnd_Hurt, CrSnd_Slap, etc.)
    switch (snd_idx) {
        case CrSnd_Hurt: return &crsounds->hurt;
        case CrSnd_Slap: return &crsounds->slap;
        // ... etc for all sound types
    }
}
```

---

## 3. Room Ambient Sound System

### 3.1 Room Sound Configuration

Each room type has an associated ambient sound configured in the room stats:

**Location:** `game.conf.slab_conf.room_cfgstats[room_kind]`

**Structure:** `RoomConfigStats` in `src/config_terrain.h`
- `ambient_snd_smp_id` - Sound sample ID to play as ambient loop

Example room configurations:
- **Training Room** - Training sounds (grunts, weapon clashing)
- **Workshop** - Hammering and crafting sounds
- **Library** - Page turning and whispers
- **Hatchery** - Eating and creature noises
- **Treasury** - Clinking coins

### 3.2 Ambient Sound Playback

**Function:** `find_nearest_rooms_for_ambient_sound()` in `src/sounds.c` (lines 219-262)

**Process:**
1. Gets player camera position
2. Searches in a **spiral pattern** (11×11 slabs) around camera
3. Checks if slab is a player-owned room
4. If room has `ambient_snd_smp_id > 0`, plays ambient sound
5. Sound positioned at room center (subtile coordinates)

**Function:** `set_room_playing_ambient_sound()` in `src/sounds.c` (lines 179-217)

**Process:**
1. Uses dedicated "ambient sound object" (`game.ambient_sound_thing_idx`)
2. Moves sound emitter to room position
3. Plays sample with:
   - Pitch: `NORMAL_PITCH` (100)
   - Repeats: `-1` (infinite loop)
   - Channel type: 3
   - Priority: 6
   - Volume: `FULL_LOUDNESS` (256)

### 3.3 Campaign Ambient Sounds

Campaign-level ambient sounds are configured separately:

**Location:** `game.campaign` structure

- `campaign.ambient_good` - Good/light faction ambient sound
- `campaign.ambient_bad` - Evil/dark faction ambient sound

These play globally and are not tied to specific rooms.

---

## 4. Sound Triggering Mechanisms

### 4.1 Core Sound Playback Functions

#### A. `thing_play_sample()`

**Header:** `src/sounds.h` (line 77)

```c
void thing_play_sample(
    struct Thing *thing,          // The thing emitting the sound
    SoundSmplTblID smptbl_idx,   // Sound sample table index
    SoundPitch pitch,             // Sound pitch (100 = normal)
    char repeats,                 // Loop count (-1 = infinite, 0 = once)
    unsigned char ctype,          // Channel type (3 = normal creature)
    unsigned char flags,          // Special flags (0 = normal, 8 = preserve on load)
    long priority,                // Sound priority (higher plays first)
    SoundVolume loudness          // Volume (0-256, FULL_LOUDNESS = 256)
);
```

**Implementation:** `src/sounds.c` (lines 70-100)

**Key Features:**
- Applies sound volume settings from game configuration
- Checks distance to sound receiver (camera)
- Creates or reuses sound emitter for the thing
- Adds sample to emitter with 3D spatial audio

#### B. `play_creature_sound()`

**Header:** `src/creature_control.h`

**Implementation:** `src/creature_control.c` (lines 291-309)

```c
void play_creature_sound(struct Thing *thing, long snd_idx, long a3, long a4)
{
    SYNCDBG(8,"Starting");
    if (playing_creature_sound(thing, snd_idx)) {
        return;  // Don't overlap same sound
    }
    struct CreatureSound* crsound = get_creature_sound(thing, snd_idx);
    if (crsound->index <= 0) {
        SYNCDBG(19,"No sample %ld for creature %d", snd_idx, thing->model);
        return;
    }
    long i = SOUND_RANDOM(crsound->count);  // Pick random variant
    SoundSmplTblID smptbl_idx = crsound->index + i;
    thing_play_sample(thing, smptbl_idx, NORMAL_PITCH, 0, 3, 0, a3, FULL_LOUDNESS);
}
```

**Features:**
- Prevents sound overlap by checking if already playing
- Randomly selects from configured sound variants
- Uses configured sound index and count from creature config

### 4.2 Creature Action Sound Triggers

#### Slapping Sound

**Function:** `slap_creature()` in `src/magic_powers.c` (lines 604-642)

```c
void slap_creature(struct PlayerInfo *player, struct Thing *thing)
{
    // ... damage and state logic ...
    play_creature_sound(thing, CrSnd_Slap, 3, 0);  // Priority 3
}
```

**Trigger:** When player uses power hand to slap a creature

#### Combat Sounds

**File:** `src/creature_states_combt.c`

**Fight Sound (Attacking):**
```c
play_creature_sound(fighter, CrSnd_Fight, 3, 0);  // Priority 3
```
**Trigger:** During active melee combat

**Hurt Sound (Taking Damage):**
```c
play_creature_sound(target, CrSnd_Hurt, 3, 0);  // Priority 3
```
**Trigger:** When creature receives damage from attacks/projectiles

**File:** `src/thing_shots.c` - Hurt sounds when hit by projectiles

#### Walking/Footstep Sounds

**Function:** `play_thing_walking()` in `src/sounds.c` (lines 118-177)

```c
void play_thing_walking(struct Thing *thing)
{
    // Check distance to camera
    // Handle flying creatures (buzzing sound for diptera)
    if (get_foot_creature_has_down(thing))
    {
        int smpl_variant = foot_down_sound_sample_variant[...];
        struct CreatureSound* crsound = get_creature_sound(thing, CrSnd_Foot);
        smpl_idx = crsound->index + smpl_variant;
        thing_play_sample(thing, smpl_idx, crconf->footstep_pitch, 0, 3, 3, 1, loudness);
    }
}
```

**Trigger:** Every game tick when creature is moving

**Special Cases:**
- Snow terrain: Uses different sound samples (181-184)
- Water terrain: Adds splash sounds
- Flying creatures: Buzzing sound for insects (diptera)

#### Death Sound

**File:** `src/thing_corpses.c`

```c
play_creature_sound(thing, CrSnd_Die, 3, 0);  // Priority 3
```
**Trigger:** When creature health reaches 0

#### Torture Sound

**File:** `src/creature_states_tortr.c`

```c
play_creature_sound(thing, CrSnd_Torture, 2, 0);  // Priority 2
```
**Trigger:** While creature is in torture chamber

### 4.3 Creature Instance Sounds (Spells & Abilities)

Creature instances are special abilities like spells, attacks, or actions.

**Function:** `instf_creature_cast_spell()` in `src/creature_instances.c` (lines 500-528)

**Process:**
1. Creature activates spell instance
2. `creature_cast_spell()` or `creature_cast_spell_at_thing()` is called
3. Sound played from spell configuration

**Sound Trigger:** `src/thing_creature.c`

```c
if (spconf->caster_affect_sound > 0)
{
    thing_play_sample(
        castng, 
        spconf->caster_affect_sound + SOUND_RANDOM(spconf->caster_sounds_count),
        NORMAL_PITCH,  // Pitch 100
        0,             // No repeat
        3,             // Channel type
        0,             // No special flags
        4,             // Priority 4 (higher than most creature sounds)
        FULL_LOUDNESS  // Volume 256
    );
}
```

**Configuration:** `config/fxdata/magic.cfg`

Example:
```ini
[spell_fireball]
CasterAffectSound = 45      ; Sound index for casting
CasterSoundsCount = 3       ; Number of variations
```

### 4.4 Sound Priority System

Sound priority determines which sounds play when source pool is exhausted:

| Priority | Sound Type | Example |
|----------|------------|---------|
| 1 | Low priority | Footsteps |
| 2 | Medium-low | Torture, Mood sounds |
| 3 | Medium | Slap, Hurt, Fight, Die |
| 4 | High | Spell casting, Digging |
| 6 | Very high | Room ambient sounds |

**Behavior:** When all sources are in use, lower priority sounds are kicked out to make room for higher priority sounds.

### 4.5 Common Sound Triggers Table

| Game State | Sound Type | File Location | Priority | Function |
|------------|------------|---------------|----------|----------|
| Slapping | `CrSnd_Slap` | `magic_powers.c:641` | 3 | `slap_creature()` |
| Taking damage | `CrSnd_Hurt` | `thing_shots.c` | 3 | Projectile hit |
| Attacking | `CrSnd_Fight` | `creature_states_combt.c` | 3 | Combat state |
| Dying | `CrSnd_Die` | `thing_corpses.c` | 3 | Death state |
| Walking | `CrSnd_Foot` | `sounds.c:161` | 1 | `play_thing_walking()` |
| Picked up | `CrSnd_Hang` | `power_hand.c` | 2 | Power hand pickup |
| Dropped | `CrSnd_Drop` | `power_hand.c` | 2 | Power hand release |
| Torture | `CrSnd_Torture` | `creature_states_tortr.c` | 2 | Torture state |
| Mood change | `CrSnd_Happy`/`CrSnd_Sad` | `creature_states_mood.c` | 2 | Mood states |
| Digging | Direct sample | `creature_instances.c:679` | 4 | `instf_dig()` |
| Spell casting | Config-based | `thing_creature.c` | 4 | Instance execution |

---

## 5. Sound File Formats & Locations

### 5.1 Primary Format: WAVE (WAV)

KeeperFX uses WAVE audio files parsed using RIFF/WAVE headers.

**Parser:** `wave_file` class in `src/bflib_sndlib.cpp`

**Supported Formats:**
- **PCM (Pulse Code Modulation)**:
  - 8-bit mono
  - 16-bit mono
  - 8-bit stereo
  - 16-bit stereo

- **ADPCM (Adaptive Differential PCM)**:
  - 4-bit MS-ADPCM mono
  - 4-bit MS-ADPCM stereo

**OpenAL Conversion:**
Samples are converted to OpenAL formats:
- `AL_FORMAT_MONO8` / `AL_FORMAT_MONO16`
- `AL_FORMAT_STEREO8` / `AL_FORMAT_STEREO16`
- `AL_FORMAT_MONO_MSADPCM_SOFT`
- `AL_FORMAT_STEREO_MSADPCM_SOFT`

### 5.2 Music Formats

**Supported Formats:**
- **OGG** (Ogg Vorbis) - Preferred format
- **MP3** (MPEG Audio Layer 3)

**Playback:** Via SDL_mixer for streaming

### 5.3 File Storage Locations

#### Source Audio Files

During development, audio files are organized:

```
keeperfx/
├── sfx/
│   └── sound/
│       ├── filelist.txt       # List of sound files to pack
│       ├── sample001.wav      # Individual sound samples
│       ├── sample002.wav
│       └── ...
```

#### Built Sound Banks

After building (`make package` or `pkg_sfx.mk`):

```
keeperfx/
├── sound.dat                  # Packed sound effects
├── speech.dat                 # Packed speech (English)
├── speech_ger.dat            # German speech
├── speech_fre.dat            # French speech
└── ...
```

#### Music Files

```
keeperfx/
├── music/
│   ├── track01.ogg           # Music tracks
│   ├── track02.ogg
│   └── ...
```

### 5.4 Building Sound Banks

**Makefile:** `pkg_sfx.mk`

**Tool:** `sndbanker` utility (in `tools/sndbanker/`)

**Process:**
1. Reads `filelist.txt` containing list of WAV files
2. Packs all samples into `sound.dat` with metadata
3. Creates index/directory structures
4. Optimizes for fast loading

**Command:**
```bash
make package-sfx
```

---

## 6. 3D Sound System

### 6.1 Sound Emitters

Every "thing" (creature, object, effect) that makes sounds has a **sound emitter**.

**Structure:** `SoundEmitter` in `src/bflib_sound.h` (lines 74-83)

```c
struct SoundEmitter {
    unsigned char flags;              // Emi_IsAllocated, Emi_IsPlaying, Emi_IsMoving
    unsigned char emitter_flags;      // Custom flags
    short index;                      // Emitter ID
    struct SoundCoord3d pos;          // 3D position (x, y, z)
    unsigned char reserved[6];
    long pitch_doppler;               // Doppler pitch adjustment
    unsigned char curr_pitch;         // Current pitch
    unsigned char target_pitch;       // Target pitch for interpolation
};
```

**Location:** `thing->snd_emitter_id` - Each thing stores its emitter ID

**Lifecycle:**
1. **Creation:** `S3DCreateSoundEmitterPri()` - Creates emitter at thing's position
2. **Update:** `S3DMoveSoundEmitterTo()` - Updates position when thing moves
3. **Destruction:** `S3DDestroySoundEmitterAndSamples()` - Removes emitter and stops samples

### 6.2 Sound Receiver (Listener)

The **sound receiver** represents the player's camera/ears.

**Structure:** `SoundReceiver` in `src/bflib_sound.h` (lines 85-92)

```c
struct SoundReceiver {
    struct SoundCoord3d pos;          // 3D position
    unsigned short rotation_angle_x;  // X rotation (pitch)
    unsigned short rotation_angle_y;  // Y rotation (yaw)
    unsigned short rotation_angle_z;  // Z rotation (roll)
    unsigned long flags;
    unsigned char sensivity;          // Hearing sensitivity (0-255)
};
```

**Global:** `Receiver` (global variable)

**Update:** `update_3d_sound_receiver()` in `src/sounds.c` (lines 264-288)

**Process:**
1. Gets camera position and orientation
2. Sets receiver position via `S3DSetSoundReceiverPosition()`
3. Sets receiver orientation via `S3DSetSoundReceiverOrientation()`
4. Adjusts sensitivity based on camera mode (zoom affects hearing range)

### 6.3 3D Audio Calculations

**Function:** `process_sound_samples()` in `src/bflib_sound.c`

**Distance Attenuation:**
- Calculates distance between emitter and receiver
- Applies volume falloff based on distance
- Maximum distance: `MaxSoundDistance` (default 5120 units)

**Panning:**
- Calculates angle between receiver and emitter
- Maps angle to stereo pan (-256 to +256)
- Left ear vs. right ear balance

**Doppler Effect:**
- Tracks emitter movement
- Adjusts pitch based on velocity relative to receiver
- Simulates realistic sound behavior

**Line of Sight:**
- Optional callback: `S3D_LineOfSight_Func`
- Can check if walls block sound (not used by default)

### 6.4 Sound Emitter Pool

**Maximum Emitters:** `SOUND_EMITTERS_MAX` (128)

**Global Array:** `emitter[128]` in `src/bflib_sound.c`

**Allocation:**
- First emitter (index 0) is "invalid" emitter
- Emitters allocated on-demand when things play sounds
- Reused when things stop making sounds

### 6.5 Sample Management

**Maximum Concurrent Samples:** Configurable via `max_number_of_samples` (default 100)

**Structure:** `S3DSample` in `src/bflib_sound.h` (lines 94-110)

```c
struct S3DSample {
    unsigned long priority;       // Sound priority
    unsigned long time_turn;      // Game turn when started
    unsigned short smptbl_id;     // Sample table ID
    unsigned char bank_id;        // Sound bank ID
    unsigned short base_pitch;    // Base pitch value
    unsigned short pan;           // Stereo pan
    unsigned short volume;        // Current volume
    SoundMilesID mss_id;         // Miles Sound System ID (OpenAL source)
    struct SoundEmitter *emit_ptr; // Associated emitter
    long emit_idx;                // Emitter index
    char repeat_count;            // Remaining repeats (-1 = infinite)
    unsigned char flags;          // Smp_NoPitchUpdate, Smp_NoVolumeUpdate
    unsigned char is_playing;     // Playing status
    unsigned char sfxid;          // Sound effect ID
    unsigned long base_volume;    // Base volume
};
```

**Global Array:** `SampleList[SOUNDS_MAX_COUNT]` (max 16)

---

## 7. Custom Sound Implementation Guide

This section explains how to add custom sounds at different levels.

### 7.1 Custom Sounds at Creature Level

To add custom sounds for a specific creature:

#### Step 1: Prepare Audio Files

Create WAV files with proper format:
- **Sample rate:** 22050 Hz (standard for DK)
- **Bit depth:** 16-bit PCM
- **Channels:** Mono (preferred for 3D sounds)
- **Naming:** Use descriptive names (e.g., `dragon_hurt_01.wav`)

#### Step 2: Add to Sound Bank

**Option A: Add to existing sound.dat**

1. Place WAV files in `sfx/sound/` directory
2. Add filenames to `filelist.txt`
3. Rebuild sound bank:
   ```bash
   make package-sfx
   ```

**Option B: Use custom sound configuration**

In level script or campaign script:
```
REM Load custom sound for creature
LOAD_SOUND(CREATURE_DRAGON, SOUND_TYPE_HURT, "custom_dragon_hurt_01.wav", 3)
```
(Note: This requires implementing custom sound loading - see section 7.5)

#### Step 3: Configure Creature Sounds

Edit creature configuration (`config/fxdata/creature.cfg`):

```ini
[sounds_dragon]
Foot = 200 4        ; Footsteps: samples 200-203
Hit = 204 2         ; Hit sounds: samples 204-205
Happy = 206 3       ; Happy sounds: samples 206-208
Sad = 209 2         ; Sad sounds: samples 209-210
Hurt = 211 5        ; Hurt sounds: samples 211-215 (5 variations!)
Die = 216 2         ; Death sounds: samples 216-217
Hang = 218 1        ; Hanging sound: sample 218
Drop = 219 1        ; Drop sound: sample 219
Torture = 220 2     ; Torture sounds: samples 220-221
Slap = 222 2        ; Slap sounds: samples 222-223
Fight = 224 3       ; Fight sounds: samples 224-226
Piss = 227 1        ; Piss sound: sample 227
```

**Important Notes:**
- `index` must match position in sound bank
- `count` determines how many sequential samples are used
- Samples are picked randomly from the range [index, index+count-1]

#### Step 4: Test in Game

1. Load level with the creature
2. Trigger various actions (slap, combat, etc.)
3. Verify correct sounds play

### 7.2 Custom Sounds at Campaign Level

To add campaign-specific ambient or global sounds:

#### Step 1: Configure Campaign

Edit campaign configuration (`campgns/[campaign_name]/campaign.cfg`):

```ini
[common]
AMBIENT_GOOD = 500      ; Ambient sound for good faction (sample index)
AMBIENT_BAD = 501       ; Ambient sound for evil faction (sample index)
```

#### Step 2: Configure Map-Specific Sounds

In map script file (`[map_name].txt`):

```
REM Set background ambient sound
SET_AMBIENT_SOUND(1013)    ; Water drips sound

REM Play sound effect at location
IF(PLAYER0, FLAG0 == 1)
    PLAY_MESSAGE(0, 0, "custom_narrator.wav")
ENDIF
```

### 7.3 Custom Sounds at Room Level

To add custom ambient sounds for custom rooms:

#### Step 1: Configure Room

Edit room configuration (`config/fxdata/terrain.cfg`):

```ini
[room50]
NAME = CUSTOM_TEMPLE
COST = 2000
HEALTH = 4000
; ... other properties ...
AMBIENCE_SOUND = 550    ; Custom ambient sound index
```

#### Step 2: Add Sound to Bank

Ensure sample 550 is in sound.dat and contains appropriate looping audio.

**Tips for Looping Sounds:**
- Remove clicks at start/end
- Ensure seamless loop point
- Keep file size reasonable (<100KB for ambient)

### 7.4 Per-Model Sound Configuration

You can create different sound sets for the same creature at different levels:

**Example:** Make Dragons sound different at higher levels

```ini
[sounds_dragon_level_1]
; Basic dragon sounds
Hurt = 211 2

[sounds_dragon_level_5]
; More aggressive dragon sounds  
Hurt = 250 3

[sounds_dragon_level_10]
; Epic dragon sounds
Hurt = 280 5
```

Then in creature configuration:

```ini
[creature_dragon]
; ... other properties ...
SOUNDS_BY_LEVEL = 1:sounds_dragon_level_1, 5:sounds_dragon_level_5, 10:sounds_dragon_level_10
```

(Note: This feature requires custom implementation - see section 7.5)

### 7.5 Advanced: Runtime Sound Loading

For truly custom sound systems, you can implement runtime loading:

#### Modify `src/sounds.c` to add custom loading:

```c
// Add to init_sound() or create new function
TbBool load_custom_creature_sounds(ThingModel crmodel, const char *sound_dir)
{
    char filepath[256];
    
    // Load hurt sounds
    for (int i = 0; i < 5; i++) {
        sprintf(filepath, "%s/hurt_%02d.wav", sound_dir, i);
        if (load_sample_to_bank(filepath, next_sample_id)) {
            game.conf.crtr_conf.creature_sounds[crmodel].hurt.index = next_sample_id;
            game.conf.crtr_conf.creature_sounds[crmodel].hurt.count = i + 1;
            next_sample_id++;
        }
    }
    
    // Load other sound types similarly...
    
    return true;
}
```

#### Extend level script commands:

In `src/lvl_script_commands.c`, add:

```c
long script_support_load_creature_sounds(long crmodel, const char *sound_dir)
{
    if (!load_custom_creature_sounds(crmodel, sound_dir)) {
        ERRORLOG("Failed to load custom sounds for creature %d", crmodel);
        return 0;
    }
    return 1;
}
```

### 7.6 Sound Replacement Best Practices

When replacing or adding sounds:

1. **Match Original Characteristics:**
   - Similar duration to original sounds
   - Similar tone/mood
   - Same sample rate (22050 Hz)

2. **Maintain Sound Priority:**
   - Important sounds (death, hurt) should have priority 3+
   - Ambient sounds should have priority 6
   - Background sounds should have priority 1-2

3. **Test Extensively:**
   - Test in different game situations
   - Verify 3D positioning works correctly
   - Check that sounds don't overlap inappropriately
   - Test with different audio settings (stereo, mono)

4. **Performance Considerations:**
   - Keep file sizes reasonable (<500KB per sound)
   - Use lower sample rates for less important sounds
   - Limit number of variations (2-5 is usually enough)
   - Consider memory usage (sounds loaded into RAM)

5. **Compatibility:**
   - Test with different campaigns
   - Ensure sounds work with save/load
   - Verify multiplayer compatibility

### 7.7 Example: Complete Custom Creature Sound Set

Here's a complete example of adding a new creature with custom sounds:

**1. Prepare Audio Files:**
```
custom_sounds/phoenix/
├── foot_01.wav through foot_04.wav     (Footsteps)
├── hurt_01.wav through hurt_03.wav     (Taking damage)
├── fight_01.wav through fight_02.wav   (Attacking)
├── die_01.wav                          (Death)
├── slap_01.wav                         (Slapped)
├── happy_01.wav                        (Happy)
└── ... (other sounds)
```

**2. Add to Sound Bank:**
Add to `sfx/sound/filelist.txt`:
```
phoenix_foot_01.wav
phoenix_foot_02.wav
phoenix_foot_03.wav
phoenix_foot_04.wav
phoenix_hurt_01.wav
... (etc)
```

**3. Note Sample Indices:**
After adding to filelist, note the indices (e.g., 600-615)

**4. Configure Creature:**
Edit `config/fxdata/creature.cfg`:

```ini
[creature36]
NAME = PHOENIX
; ... other properties ...

[sounds_phoenix]
Foot = 600 4        ; 4 footstep variations
Hit = 604 2         ; 2 hit sounds
Happy = 606 1       ; 1 happy sound
Sad = 607 1         ; 1 sad sound
Hurt = 608 3        ; 3 hurt sounds
Die = 611 1         ; 1 death sound
Hang = 612 1        ; 1 hang sound
Drop = 613 1        ; 1 drop sound
Torture = 614 1     ; 1 torture sound
Slap = 615 1        ; 1 slap sound
Fight = 616 2       ; 2 fight sounds
Piss = 618 1        ; 1 piss sound (optional)
```

**5. Build and Test:**
```bash
make package-sfx
make
# Run game and test phoenix creature
```

---

## Conclusion

The KeeperFX sound system is a comprehensive 3D audio engine built on OpenAL with custom sound bank management. It supports:

- **Packed sound banks** for efficient storage and loading
- **Per-creature sound configuration** with multiple variations
- **3D spatial audio** with distance attenuation and panning
- **Priority-based sample management** to handle limited audio sources
- **Room ambient sounds** with automatic camera-based activation
- **Instance-based sounds** for spells and special abilities

For custom sound implementation:
- **Creature Level:** Modify creature config and rebuild sound banks
- **Campaign Level:** Edit campaign config for ambient sounds
- **Room Level:** Configure room ambient sounds in terrain config
- **Advanced:** Implement runtime loading for complete flexibility

This architecture allows for easy customization at all levels while maintaining performance and compatibility with the original Dungeon Keeper design.

---

## Key Files Reference

### Core Sound Files
- `src/sounds.h` / `src/sounds.c` - Game-specific sound functions
- `src/bflib_sound.h` / `src/bflib_sound.c` - 3D sound system
- `src/bflib_sndlib.h` / `src/bflib_sndlib.cpp` - OpenAL implementation

### Configuration Files
- `config/fxdata/creature.cfg` - Creature sound configuration
- `config/fxdata/terrain.cfg` - Room sound configuration
- `config/fxdata/magic.cfg` - Spell sound configuration

### Sound Triggering
- `src/creature_control.c` - Creature sound playback functions
- `src/magic_powers.c` - Power hand sounds (slapping)
- `src/creature_states_combt.c` - Combat sounds
- `src/creature_instances.c` - Spell/ability sounds
- `src/thing_shots.c` - Projectile impact sounds

### Build System
- `pkg_sfx.mk` - Sound bank building makefile
- `tools/sndbanker/` - Sound bank packer tool

---

**Document Version:** 1.0  
**Date:** 2026-01-30  
**Author:** KeeperFX Team
