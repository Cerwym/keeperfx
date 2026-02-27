#ifndef PLATFORM_WINDOWS_H
#define PLATFORM_WINDOWS_H

#include "platform/IPlatform.h"

/** Windows implementation of IPlatform using Win32 FindFirstFile/FindNextFile. */
class PlatformWindows : public IPlatform {
public:
    const char* GetOSVersion() const override;
    const void* GetImageBase() const override;
    const char* GetWineVersion() const override;
    const char* GetWineHost() const override;

    void InstallExceptionHandler() override;
    void ErrorParachuteInstall() override;
    void ErrorParachuteUpdate() override;

    TbFileFind* FileFindFirst(const char* filespec, TbFileEntry* entry) override;
    int32_t     FileFindNext(TbFileFind* handle, TbFileEntry* entry) override;
    void        FileFindEnd(TbFileFind* handle) override;

    void   SetRedbookVolume(SoundVolume vol) override;
    TbBool PlayRedbookTrack(int track) override;
    void   PauseRedbookTrack() override;
    void   ResumeRedbookTrack() override;
    void   StopRedbookTrack() override;
};

#endif // PLATFORM_WINDOWS_H
