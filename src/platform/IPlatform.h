#ifndef IPLATFORM_H
#define IPLATFORM_H

#include "bflib_basics.h"
#include "bflib_fileio.h"
#include "bflib_sound.h"

/** Abstract interface for OS/platform-specific operations.
 *
 *  Implement this class for each target platform and register it with
 *  PlatformManager::Set() at startup.  All callers access functionality
 *  through the C-compatible wrappers in PlatformManager.cpp, keeping
 *  existing C-code call-sites unchanged.
 */
class IPlatform {
public:
    virtual ~IPlatform() = default;

    // ----- OS information -----
    virtual const char* GetOSVersion() const = 0;
    virtual const void* GetImageBase() const = 0;
    virtual const char* GetWineVersion() const = 0;
    virtual const char* GetWineHost() const = 0;

    // ----- Crash / error parachute -----
    virtual void InstallExceptionHandler() = 0;
    virtual void ErrorParachuteInstall() = 0;
    virtual void ErrorParachuteUpdate() = 0;

    // ----- File enumeration -----
    virtual TbFileFind* FileFindFirst(const char* filespec, TbFileEntry* entry) = 0;
    virtual int32_t     FileFindNext(TbFileFind* handle, TbFileEntry* entry) = 0;
    virtual void        FileFindEnd(TbFileFind* handle) = 0;

    // ----- CDROM / Redbook audio -----
    virtual void   SetRedbookVolume(SoundVolume vol) = 0;
    virtual TbBool PlayRedbookTrack(int track) = 0;
    virtual void   PauseRedbookTrack() = 0;
    virtual void   ResumeRedbookTrack() = 0;
    virtual void   StopRedbookTrack() = 0;
};

#endif // IPLATFORM_H
