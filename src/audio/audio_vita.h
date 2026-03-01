#ifndef AUDIO_VITA_H
#define AUDIO_VITA_H

#ifdef PLATFORM_VITA

#ifdef __cplusplus
extern "C" {
#endif

/** Open a dedicated SCE audio port for FMV (Smacker) playback.
 *  @return 1 on success, 0 on failure. */
int  vita_fmv_audio_open(int freq, int channels);

/** Queue a block of S16 stereo PCM for FMV audio output.
 *  Blocks until a ring buffer slot is free — use as back-pressure to pace
 *  the decode thread to the hardware audio clock. */
void vita_fmv_audio_queue(const void *pcm, int bytes);

/** Close the FMV audio port. */
void vita_fmv_audio_close(void);

/** Returns nanoseconds of audio submitted to the DMA hardware since open.
 *  Use as the master clock for video PTS synchronisation. */
int64_t vita_fmv_audio_pts_ns(void);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_VITA */
#endif /* AUDIO_VITA_H */
