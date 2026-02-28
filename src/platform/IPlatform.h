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
    /** Install platform-specific crash handlers (SEH on Windows, extra POSIX
     *  signals on Linux/Vita).  Called from LbErrorParachuteInstall() after
     *  the ANSI signal set has been registered. */
    virtual void ErrorParachuteInstall() = 0;
    /** Re-arm the exception filter after any SDL/third-party library resets it. */
    virtual void ErrorParachuteUpdate() = 0;

    // ----- File enumeration -----
    virtual TbFileFind* FileFindFirst(const char* filespec, TbFileEntry* entry) = 0;
    virtual int32_t     FileFindNext(TbFileFind* handle, TbFileEntry* entry) = 0;
    virtual void        FileFindEnd(TbFileFind* handle) = 0;

    // ----- File system helpers -----
    /** Returns true if the file or directory at `path` exists and is accessible. */
    virtual TbBool FileExists(const char* path) const = 0;
    /** Creates a single directory component.  Returns 0 on success, -1 on failure
     *  (EEXIST is NOT treated as failure). */
    virtual int MakeDirectory(const char* path) = 0;
    /** Fills `buf` with the current working directory.  Returns 1 on success,
     *  -1 on failure.  Strips a leading drive letter on platforms that use one
     *  (Windows/Vita ux0: style). */
    virtual int GetCurrentDirectory(char* buf, unsigned long buflen) = 0;

    // ----- Path provider -----
    /** Called once at startup with the raw argc/argv before any path queries.
     *  Desktop platforms extract the executable directory from argv[0] here.
     *  Homebrew platforms can ignore this. */
    virtual void        SetArgv(int argc, char** argv) = 0;

    /** Root directory where game data files live.
     *  Desktop: directory containing the executable (argv[0]-relative).
     *  Vita:    "ux0:data/keeperfx"
     *  3DS:     "sdmc:/keeperfx"
     *  Switch:  "sdmc:/keeperfx"
     *  Returned pointer is valid for the lifetime of the platform object. */
    virtual const char* GetDataPath() const = 0;

    /** Directory where save files are written.
     *  Desktop: <data_path>/save
     *  Homebrew: same convention under the data path. */
    virtual const char* GetSavePath() const = 0;

    // ----- CDROM / Redbook audio -----
    virtual void   SetRedbookVolume(SoundVolume vol) = 0;
    virtual TbBool PlayRedbookTrack(int track) = 0;
    virtual void   PauseRedbookTrack() = 0;
    virtual void   ResumeRedbookTrack() = 0;
    virtual void   StopRedbookTrack() = 0;

    // ----- Log output routing -----
    /** Called for every formatted log line (prefix already prepended).
     *  The default is a no-op — override to mirror output to a platform
     *  debug channel (e.g. sceClibPrintf on Vita, debugnet on 3DS). */
    virtual void LogWrite(const char* /*message*/) {}
};

#endif // IPLATFORM_H
