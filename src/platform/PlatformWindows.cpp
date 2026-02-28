#include "pre_inc.h"
#include "platform/PlatformWindows.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "post_inc.h"

// TbFileFind is defined here; it is an opaque type to all callers.
struct TbFileFind {
    HANDLE handle;
    char*  namebuf;
    int    namebuflen;
};

// ----- OS information -----

const char* PlatformWindows::GetOSVersion() const
{
    static char buffer[256];
    OSVERSIONINFO v;
    v.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&v)) {
        snprintf(buffer, sizeof(buffer), "%s %ld.%ld.%ld",
            (v.dwPlatformId == VER_PLATFORM_WIN32_NT) ? "Windows NT" : "Windows",
            v.dwMajorVersion, v.dwMinorVersion, v.dwBuildNumber);
        return buffer;
    }
    return "unknown";
}

const void* PlatformWindows::GetImageBase() const
{
    return GetModuleHandle(NULL);
}

const char* PlatformWindows::GetWineVersion() const
{
    const auto module = GetModuleHandle("ntdll.dll");
    if (module) {
        const auto wine_get_version =
            (const char* (WINAPI*)())(void*)GetProcAddress(module, "wine_get_version");
        if (wine_get_version) {
            return wine_get_version();
        }
    }
    return nullptr;
}

const char* PlatformWindows::GetWineHost() const
{
    const auto module = GetModuleHandle("ntdll.dll");
    static char buffer[256];
    if (module) {
        const auto wine_get_host_version =
            (void (WINAPI*)(const char**, const char**))(void*)
            GetProcAddress(module, "wine_get_host_version");
        if (wine_get_host_version) {
            const char* sys_name     = nullptr;
            const char* release_name = nullptr;
            wine_get_host_version(&sys_name, &release_name);
            snprintf(buffer, sizeof(buffer), "%s %s",
                sys_name     ? sys_name     : "unknown",
                release_name ? release_name : "unknown");
            return buffer;
        }
    }
    return nullptr;
}

// ----- Crash / error parachute -----

void PlatformWindows::InstallExceptionHandler()
{
    // Exception handler is registered in WinMain via AddVectoredExceptionHandler.
}

void PlatformWindows::ErrorParachuteInstall()
{
    // TODO: implement Windows crash logging
}

void PlatformWindows::ErrorParachuteUpdate()
{
    // TODO: implement Windows crash logging
}

// ----- File enumeration -----

TbFileFind* PlatformWindows::FileFindFirst(const char* filespec, TbFileEntry* fentry)
{
    auto ffind = static_cast<TbFileFind*>(malloc(sizeof(TbFileFind)));
    if (!ffind) {
        return nullptr;
    }
    WIN32_FIND_DATA fd;
    ffind->handle = FindFirstFile(filespec, &fd);
    if (ffind->handle == INVALID_HANDLE_VALUE) {
        free(ffind);
        return nullptr;
    }
    const int namelen = strlen(fd.cFileName);
    ffind->namebuf = static_cast<char*>(malloc(namelen + 1));
    if (!ffind->namebuf) {
        FindClose(ffind->handle);
        free(ffind);
        return nullptr;
    }
    memcpy(ffind->namebuf, fd.cFileName, namelen + 1);
    ffind->namebuflen   = namelen;
    fentry->Filename    = ffind->namebuf;
    return ffind;
}

int32_t PlatformWindows::FileFindNext(TbFileFind* ffind, TbFileEntry* fentry)
{
    if (!ffind) {
        return -1;
    }
    WIN32_FIND_DATA fd;
    if (!FindNextFile(ffind->handle, &fd)) {
        return -1;
    }
    const int namelen = strlen(fd.cFileName);
    if (namelen > ffind->namebuflen) {
        auto buf = static_cast<char*>(realloc(ffind->namebuf, namelen + 1));
        if (!buf) {
            return -1;
        }
        ffind->namebuf    = buf;
        ffind->namebuflen = namelen;
    }
    memcpy(ffind->namebuf, fd.cFileName, namelen + 1);
    fentry->Filename = ffind->namebuf;
    return 1;
}

void PlatformWindows::FileFindEnd(TbFileFind* ffind)
{
    if (ffind) {
        FindClose(ffind->handle);
        free(ffind->namebuf);
        free(ffind);
    }
}

// ----- Path provider -----

void PlatformWindows::SetArgv(int argc, char** argv)
{
    if (argc < 1 || !argv || !argv[0] || argv[0][0] == '\0') {
        return;
    }
    snprintf(data_path_, sizeof(data_path_), "%s", argv[0]);
    // Strip filename component, keeping the directory.
    char* end = strrchr(data_path_, '\\');
    if (!end) end = strrchr(data_path_, '/');
    if (end) {
        *end = '\0';
    } else {
        snprintf(data_path_, sizeof(data_path_), ".");
    }
    snprintf(save_path_, sizeof(save_path_), "%s", data_path_);
}

const char* PlatformWindows::GetDataPath() const { return data_path_; }
const char* PlatformWindows::GetSavePath() const { return save_path_; }

// ----- CDROM / Redbook audio -----

void PlatformWindows::SetRedbookVolume(SoundVolume)
{
    // TODO: implement CDROM features
}

TbBool PlatformWindows::PlayRedbookTrack(int)
{
    // TODO: implement CDROM features
    return false;
}

void PlatformWindows::PauseRedbookTrack()
{
    // TODO: implement CDROM features
}

void PlatformWindows::ResumeRedbookTrack()
{
    // TODO: implement CDROM features
}

void PlatformWindows::StopRedbookTrack()
{
    // TODO: implement CDROM features
}
