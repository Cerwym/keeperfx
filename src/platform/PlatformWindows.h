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

    void ErrorParachuteInstall() override;
    void ErrorParachuteUpdate() override;

    TbFileFind* FileFindFirst(const char* filespec, TbFileEntry* entry) override;
    int32_t     FileFindNext(TbFileFind* handle, TbFileEntry* entry) override;
    void        FileFindEnd(TbFileFind* handle) override;

    TbBool FileExists(const char* path) const override;
    int    MakeDirectory(const char* path) override;
    // Undefine Windows GetCurrentDirectory macro to avoid name collision with our method.
#undef GetCurrentDirectory
    int    GetCurrentDirectory(char* buf, unsigned long buflen) override;

    void        SetArgv(int argc, char** argv) override;
    const char* GetDataPath() const override;
    const char* GetSavePath() const override;

    void   SetRedbookVolume(SoundVolume vol) override;
    TbBool PlayRedbookTrack(int track) override;
    void   PauseRedbookTrack() override;
    void   ResumeRedbookTrack() override;
    void   StopRedbookTrack() override;

private:
    char data_path_[256] = ".";
    char save_path_[256] = ".";
};

#endif // PLATFORM_WINDOWS_H
