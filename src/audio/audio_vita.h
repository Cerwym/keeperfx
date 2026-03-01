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
 *  Blocks until the DMA buffer is consumed — call from FMV decode thread. */
void vita_fmv_audio_queue(const void *pcm, int bytes);

/** Close the FMV audio port. */
void vita_fmv_audio_close(void);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_VITA */
#endif /* AUDIO_VITA_H */
