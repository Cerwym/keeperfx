/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file audio_openal.c
 *     OpenAL-based audio implementation wrapping bflib_sndlib.
 * @par Purpose:
 *     Provides an audio interface implementation using OpenAL.
 * @par Comment:
 *     None.
 * @author   KeeperFX Team
 * @date     12 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "audio_interface.h"

#include "bflib_sndlib.h"
#include "bflib_sound.h"
#include "post_inc.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/

static TbResult audio_openal_init(void)
{
    // Audio initialization is handled by InitAudio() called elsewhere
    return Lb_OK;
}

static void audio_openal_shutdown(void)
{
    FreeAudio();
}

static void audio_openal_play_sample(int id, long x, long y, long z, int volume)
{
    // This is a simplified wrapper - actual sample playing in KeeperFX
    // is more complex and goes through S3DCreateSoundEmitterPri etc.
    // For now, this is a placeholder for the interface
}

static void audio_openal_play_music(int track)
{
    play_music_track(track);
}

static void audio_openal_set_listener(long x, long y, long z, int angle)
{
    S3DSetSoundReceiverPosition(x, y, z);
    S3DSetSoundReceiverOrientation(angle, 0, 0);
}

static void audio_openal_set_volume(int master, int music, int sfx)
{
    if (master >= 0)
        SetSoundMasterVolume(master);
    if (music >= 0)
        set_music_volume(music);
    // SFX volume is typically handled per-sample in KeeperFX
}

static AudioInterface audio_openal_impl = {
    audio_openal_init,
    audio_openal_shutdown,
    audio_openal_play_sample,
    audio_openal_play_music,
    audio_openal_set_listener,
    audio_openal_set_volume,
};

AudioInterface* g_audio = &audio_openal_impl;

/******************************************************************************/
#ifdef __cplusplus
}
#endif
