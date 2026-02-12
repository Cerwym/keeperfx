/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file audio_ndsp.c
 *     Nintendo 3DS NDSP audio implementation.
 * @par Purpose:
 *     Provides audio interface implementation using NDSP (Nintendo DSP).
 * @par Comment:
 *     Uses NDSP for 3D audio and music playback on Nintendo 3DS.
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

#ifdef PLATFORM_3DS

#include <3ds.h>
#include <string.h>

#include "../bflib_sound.h"
#include "../post_inc.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/

#define MAX_AUDIO_CHANNELS 24
#define SAMPLE_RATE 22050
#define SAMPLES_PER_BUF 1024

/******************************************************************************/
// Global state
static ndspWaveBuf s_waveBufs[MAX_AUDIO_CHANNELS][2];
static int s_channelActive[MAX_AUDIO_CHANNELS];
static float s_masterVolume = 1.0f;
static float s_musicVolume = 1.0f;
static float s_sfxVolume = 1.0f;

// Music channel (dedicated)
static int s_musicChannel = 0;
static bool s_initialized = false;

/******************************************************************************/
// Helper functions

static void setup_channel(int channel)
{
    if (channel < 0 || channel >= MAX_AUDIO_CHANNELS)
        return;

    // Configure channel for 16-bit stereo PCM
    ndspChnReset(channel);
    ndspChnSetInterp(channel, NDSP_INTERP_LINEAR);
    ndspChnSetRate(channel, SAMPLE_RATE);
    ndspChnSetFormat(channel, NDSP_FORMAT_STEREO_PCM16);

    // Initialize wave buffers
    for (int i = 0; i < 2; i++) {
        memset(&s_waveBufs[channel][i], 0, sizeof(ndspWaveBuf));
    }

    s_channelActive[channel] = 0;
}

static float calculate_3d_volume(long x, long y, long z, int base_volume)
{
    // Simple distance-based attenuation
    // In a real implementation, you'd use actual listener position
    float distance = sqrtf((float)(x*x + y*y + z*z));
    float attenuation = 1.0f / (1.0f + distance * 0.001f);
    float volume = (base_volume / 256.0f) * attenuation * s_sfxVolume * s_masterVolume;
    return fminf(1.0f, fmaxf(0.0f, volume));
}

static void calculate_3d_pan(long x, long y, long z, float* left, float* right)
{
    // Simple stereo panning based on X position
    // Negative X = left, Positive X = right
    float pan = (float)x / 10000.0f;
    pan = fminf(1.0f, fmaxf(-1.0f, pan));

    if (pan < 0.0f) {
        *left = 1.0f;
        *right = 1.0f + pan;
    } else {
        *left = 1.0f - pan;
        *right = 1.0f;
    }
}

/******************************************************************************/
// Audio interface implementation

static TbResult audio_ndsp_init(void)
{
    if (s_initialized)
        return Lb_SUCCESS;

    // Initialize NDSP
    Result res = ndspInit();
    if (R_FAILED(res))
        return Lb_FAIL;

    // Setup all channels
    for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
        setup_channel(i);
    }

    // Reserve channel 0 for music
    s_musicChannel = 0;

    s_initialized = true;
    return Lb_SUCCESS;
}

static void audio_ndsp_shutdown(void)
{
    if (!s_initialized)
        return;

    // Stop all channels
    for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
        ndspChnReset(i);
    }

    ndspExit();
    s_initialized = false;
}

static void audio_ndsp_play_sample(int id, long x, long y, long z, int volume)
{
    if (!s_initialized)
        return;

    // Find a free channel (skip music channel)
    int channel = -1;
    for (int i = 1; i < MAX_AUDIO_CHANNELS; i++) {
        if (!s_channelActive[i] || !ndspChnIsPlaying(i)) {
            channel = i;
            break;
        }
    }

    if (channel < 0)
        return;  // No free channels

    // Calculate 3D audio parameters
    float vol = calculate_3d_volume(x, y, z, volume);
    float left_pan, right_pan;
    calculate_3d_pan(x, y, z, &left_pan, &right_pan);

    // Set channel volume and mix
    float mix[12] = {0};
    mix[0] = vol * left_pan;   // Front left
    mix[1] = vol * right_pan;  // Front right
    ndspChnSetMix(channel, mix);

    // Note: In a real implementation, you would:
    // 1. Load the sample data based on id
    // 2. Setup wave buffer with the sample data
    // 3. Queue the wave buffer with ndspChnWaveBufAdd
    // 
    // For now, this is a placeholder showing the interface structure

    s_channelActive[channel] = 1;
}

static void audio_ndsp_play_music(int track)
{
    if (!s_initialized)
        return;

    // Stop current music
    ndspChnReset(s_musicChannel);

    // Note: In a real implementation, you would:
    // 1. Load the music track data based on track ID
    // 2. Setup streaming buffers for the music
    // 3. Queue buffers and handle buffer completion callbacks
    //
    // For now, this is a placeholder showing the interface structure

    // Set music volume
    float mix[12] = {0};
    mix[0] = s_musicVolume * s_masterVolume;  // Front left
    mix[1] = s_musicVolume * s_masterVolume;  // Front right
    ndspChnSetMix(s_musicChannel, mix);

    s_channelActive[s_musicChannel] = 1;
}

static void audio_ndsp_set_listener(long x, long y, long z, int angle)
{
    // Store listener position for 3D audio calculations
    // In a real implementation, you would store these values
    // and use them in calculate_3d_volume and calculate_3d_pan
}

static void audio_ndsp_set_volume(int master, int music, int sfx)
{
    if (master >= 0)
        s_masterVolume = master / 256.0f;
    if (music >= 0)
        s_musicVolume = music / 256.0f;
    if (sfx >= 0)
        s_sfxVolume = sfx / 256.0f;

    // Update music channel volume immediately
    if (s_initialized && s_channelActive[s_musicChannel]) {
        float mix[12] = {0};
        mix[0] = s_musicVolume * s_masterVolume;
        mix[1] = s_musicVolume * s_masterVolume;
        ndspChnSetMix(s_musicChannel, mix);
    }
}

static AudioInterface audio_ndsp_impl = {
    audio_ndsp_init,
    audio_ndsp_shutdown,
    audio_ndsp_play_sample,
    audio_ndsp_play_music,
    audio_ndsp_set_listener,
    audio_ndsp_set_volume,
};

void audio_ndsp_initialize(void)
{
    g_audio = &audio_ndsp_impl;
}

/******************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // PLATFORM_3DS
