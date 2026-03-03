/*
 * ffmpeg_vita_stubs.c — no-op stubs for symbols absent from fish47/FFmpeg-vita.
 *
 * ff_ac3_find_syncword: returning -1 disables AC3 sync detection. AC3 FMV audio
 * is not supported on Vita (SCE HW decoder handles AAC/MP3); this is a safe no-op.
 */

int ff_ac3_find_syncword(const unsigned char *buf, int buf_size)
{
    (void)buf;
    (void)buf_size;
    return -1;
}
