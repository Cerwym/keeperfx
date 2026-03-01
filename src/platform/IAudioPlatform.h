#ifndef IAUDIOPLATFORM_H
#define IAUDIOPLATFORM_H

/** Abstract interface for platform-specific FMV audio output.
 *
 *  Obtained via IPlatform::GetAudio().  Returns nullptr on platforms that
 *  have no native audio path (desktop CI, headless builds) — callers must
 *  check for null and fall back to SDL or silent playback.
 *
 *  Currently covers FMV (Smacker) audio only.  Extend with SFX/music methods
 *  as the full platform audio abstraction matures.
 */
class IAudioPlatform {
public:
    virtual ~IAudioPlatform() = default;

    // ----- FMV audio streaming -----

    /** Open an audio output port for FMV playback.
     *  @param freq     Sample rate in Hz (e.g. 48000).
     *  @param channels Number of output channels (1 or 2).
     *  @return true on success; false if the port could not be opened
     *          (caller should fall back to silent playback). */
    virtual bool FmvAudioOpen(int freq, int channels) = 0;

    /** Queue a block of decoded PCM audio for output.
     *  @param pcm   Pointer to interleaved S16 PCM samples.
     *  @param bytes Number of bytes in the buffer.
     *  On Vita this calls sceAudioOutOutput which blocks until the DMA
     *  buffer is consumed — call from the FMV decode thread only. */
    virtual void FmvAudioQueue(const void* pcm, int bytes) = 0;

    /** Close the FMV audio port opened by FmvAudioOpen(). */
    virtual void FmvAudioClose() = 0;

    /** Returns nanoseconds of audio submitted to the hardware DMA since open,
     *  or -1 if this platform does not provide an audio clock.
     *  Use as the master clock for video PTS synchronisation. */
    virtual int64_t FmvAudioPtsNs() { return -1; }
};

#endif // IAUDIOPLATFORM_H
