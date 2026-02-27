#ifndef PLATFORM_LINUX_H
#define PLATFORM_LINUX_H

#include "platform/IPlatform.h"

/** Linux implementation of IPlatform using POSIX dirent/fnmatch. */
class PlatformLinux : public IPlatform {
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

#endif // PLATFORM_LINUX_H
