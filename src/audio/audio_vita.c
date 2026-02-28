/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file audio_vita.c
 *     PlayStation Vita audio implementation.
 * @par Purpose:
 *     Provides audio interface implementation using Vita audio API.
 * @par Comment:
 *     Uses libSceAudio for audio output and 3D positioning.
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
#include "audio_interface.h"

#ifdef PLATFORM_VITA

#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>
#include <string.h>
#include <math.h>

#include "../bflib_sound.h"
#include "../post_inc.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/

#define MAX_AUDIO_CHANNELS 8
#define SAMPLES_PER_BUFFER 512
#define SAMPLE_RATE 48000

/******************************************************************************/
// Channel state
typedef struct {
    int port;                   // Audio port handle
    int16_t buffer[2][SAMPLES_PER_BUFFER * 2];  // Stereo buffers
    int currentBuffer;
    bool active;
    float volume;
    float pan;  // -1.0 (left) to 1.0 (right)
} AudioChannel;

/******************************************************************************/
// Global state
static AudioChannel s_channels[MAX_AUDIO_CHANNELS];
static int s_musicChannel = -1;
static float s_masterVolume = 1.0f;
static float s_musicVolume = 1.0f;
static float s_sfxVolume = 1.0f;
static bool s_initialized = false;

/******************************************************************************/
// Helper functions

static int find_free_channel(void) __attribute__((unused));
static int find_free_channel(void)
{
    for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
        if (!s_channels[i].active) {
            return i;
        }
    }
    return -1;
}

static float calculate_3d_volume(long x, long y, long z, int base_volume)
{
    // Simple distance-based attenuation
    float distance = sqrtf((float)(x*x + y*y + z*z));
    float attenuation = 1.0f / (1.0f + distance * 0.0001f);
    float volume = (base_volume / 256.0f) * attenuation * s_sfxVolume * s_masterVolume;
    return fminf(1.0f, fmaxf(0.0f, volume));
}

static float calculate_3d_pan(long x, long y, long z)
{
    // Simple stereo panning based on X position
    float pan = (float)x / 10000.0f;
    return fminf(1.0f, fmaxf(-1.0f, pan));
}

static void apply_volume_and_pan(int16_t* buffer, int samples, float volume, float pan) __attribute__((unused));
static void apply_volume_and_pan(int16_t* buffer, int samples, float volume, float pan)
{
    for (int i = 0; i < samples; i++) {
        // Calculate left and right channel volumes
        float left_vol = volume * (1.0f - fmaxf(0.0f, pan));
        float right_vol = volume * (1.0f + fminf(0.0f, pan));

        // Apply volume (stereo interleaved)
        buffer[i * 2 + 0] = (int16_t)(buffer[i * 2 + 0] * left_vol);
        buffer[i * 2 + 1] = (int16_t)(buffer[i * 2 + 1] * right_vol);
    }
}

/******************************************************************************/
// Audio interface implementation

static TbResult audio_vita_init(void)
{
    if (s_initialized)
        return Lb_SUCCESS;

    // Initialize channels
    for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
        s_channels[i].port = sceAudioOutOpenPort(
            SCE_AUDIO_OUT_PORT_TYPE_MAIN,
            SAMPLES_PER_BUFFER,
            SAMPLE_RATE,
            SCE_AUDIO_OUT_MODE_STEREO
        );

        if (s_channels[i].port < 0) {
            // Failed to open port, clean up and return error
            for (int j = 0; j < i; j++) {
                if (s_channels[j].port >= 0)
                    sceAudioOutReleasePort(s_channels[j].port);
            }
            return Lb_FAIL;
        }

        s_channels[i].currentBuffer = 0;
        s_channels[i].active = false;
        s_channels[i].volume = 1.0f;
        s_channels[i].pan = 0.0f;
        memset(s_channels[i].buffer, 0, sizeof(s_channels[i].buffer));
    }

    // Reserve first channel for music
    s_musicChannel = 0;

    s_initialized = true;
    return Lb_SUCCESS;
}

static void audio_vita_shutdown(void)
{
    if (!s_initialized)
        return;

    // Release all audio ports
    for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
        if (s_channels[i].port >= 0) {
            sceAudioOutReleasePort(s_channels[i].port);
            s_channels[i].port = -1;
        }
    }

    s_initialized = false;
}

static void audio_vita_play_sample(int id, long x, long y, long z, int volume)
{
    if (!s_initialized)
        return;

    // Find a free channel (skip music channel)
    int channel = -1;
    for (int i = 1; i < MAX_AUDIO_CHANNELS; i++) {
        if (!s_channels[i].active) {
            channel = i;
            break;
        }
    }

    if (channel < 0)
        return;  // No free channels

    // Calculate 3D audio parameters
    float vol = calculate_3d_volume(x, y, z, volume);
    float pan = calculate_3d_pan(x, y, z);

    s_channels[channel].volume = vol;
    s_channels[channel].pan = pan;
    s_channels[channel].active = true;

    // Note: In a real implementation, you would:
    // 1. Load the sample data based on id
    // 2. Decode/convert the sample to 16-bit stereo PCM
    // 3. Fill the buffer with sample data
    // 4. Apply volume and panning
    // 5. Output to audio port with sceAudioOutOutput
    //
    // For now, this is a placeholder showing the interface structure

    // Example output (would be called after filling buffer):
    // apply_volume_and_pan(s_channels[channel].buffer[0], SAMPLES_PER_BUFFER, vol, pan);
    // sceAudioOutOutput(s_channels[channel].port, s_channels[channel].buffer[0]);
}

static void audio_vita_play_music(int track)
{
    if (!s_initialized || s_musicChannel < 0)
        return;

    AudioChannel* channel = &s_channels[s_musicChannel];

    // Stop current music
    channel->active = false;
    memset(channel->buffer, 0, sizeof(channel->buffer));

    // Note: In a real implementation, you would:
    // 1. Load the music track data based on track ID
    // 2. Setup streaming decoder (e.g., for OGG or MP3)
    // 3. Create a streaming thread that continuously:
    //    - Decodes music data
    //    - Fills buffers
    //    - Outputs to audio port
    // 4. Handle looping and seeking
    //
    // For now, this is a placeholder showing the interface structure

    channel->volume = s_musicVolume * s_masterVolume;
    channel->pan = 0.0f;  // Center
    channel->active = true;

    // Example streaming output (would be in a separate thread):
    // while (channel->active) {
    //     decode_music_to_buffer(channel->buffer[channel->currentBuffer]);
    //     apply_volume_and_pan(channel->buffer[channel->currentBuffer],
    //                          SAMPLES_PER_BUFFER, channel->volume, channel->pan);
    //     sceAudioOutOutput(channel->port, channel->buffer[channel->currentBuffer]);
    //     channel->currentBuffer = 1 - channel->currentBuffer;
    // }
}

static void audio_vita_set_listener(long x, long y, long z, int angle)
{
    // Store listener position for 3D audio calculations
    // In a real implementation, you would store these values
    // and use them in calculate_3d_volume and calculate_3d_pan
}

static void audio_vita_set_volume(int master, int music, int sfx)
{
    if (master >= 0)
        s_masterVolume = master / 256.0f;
    if (music >= 0)
        s_musicVolume = music / 256.0f;
    if (sfx >= 0)
        s_sfxVolume = sfx / 256.0f;

    // Update music channel volume immediately
    if (s_initialized && s_musicChannel >= 0) {
        s_channels[s_musicChannel].volume = s_musicVolume * s_masterVolume;
    }
}

static AudioInterface audio_vita_impl = {
    audio_vita_init,
    audio_vita_shutdown,
    audio_vita_play_sample,
    audio_vita_play_music,
    audio_vita_set_listener,
    audio_vita_set_volume,
};

void audio_vita_initialize(void)
{
    g_audio = &audio_vita_impl;
}

/******************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // PLATFORM_VITA

// Stubs for SDL audio functions excluded on Vita (from bflib_sndlib.cpp)
#ifdef PLATFORM_VITA
int InitialiseSDLAudio(void) { return 1; }
void ShutDownSDLAudio(void) {}
#endif
