#include "pre_inc.h"
#include "platform/PlatformVita.h"
#ifdef PLATFORM_VITA
#include <psp2/io/dirent.h>
#include <string.h>
#include <stdlib.h>
#endif
#include "post_inc.h"

#ifdef PLATFORM_VITA

// TbFileFind is defined here; it is an opaque type to all callers.
struct TbFileFind {
    SceUID      handle;
    char        namebuf[256];
    char        pattern[256]; // empty means no filtering
};

// ----- OS information -----

const char* PlatformVita::GetOSVersion() const
{
    return "PS Vita";
}

const void* PlatformVita::GetImageBase() const
{
    return nullptr;
}

const char* PlatformVita::GetWineVersion() const
{
    return nullptr;
}

const char* PlatformVita::GetWineHost() const
{
    return nullptr;
}

// ----- Crash / error parachute -----

void PlatformVita::InstallExceptionHandler()
{
}

void PlatformVita::ErrorParachuteInstall()
{
}

void PlatformVita::ErrorParachuteUpdate()
{
}

// ----- File enumeration helpers -----

static bool vita_name_matches_pattern(const char* name, const char* pattern)
{
    if (pattern[0] == '\0') {
        return true;
    }
    // Simple '*' wildcard matching (case-insensitive not attempted on Vita)
    const char* p = pattern;
    const char* n = name;
    while (*p && *n) {
        if (*p == '*') {
            p++;
            if (*p == '\0') {
                return true; // trailing '*' matches anything
            }
            while (*n) {
                if (vita_name_matches_pattern(n, p)) {
                    return true;
                }
                n++;
            }
            return false;
        }
        if (*p != *n) {
            return false;
        }
        p++;
        n++;
    }
    return *p == '\0' && *n == '\0';
}

static bool vita_find_next_entry(TbFileFind* ff, TbFileEntry* fe)
{
    SceIoDirent de;
    while (sceIoDread(ff->handle, &de) > 0) {
        if (!SCE_S_ISREG(de.d_stat.st_mode)) {
            continue;
        }
        if (!vita_name_matches_pattern(de.d_name, ff->pattern)) {
            continue;
        }
        strncpy(ff->namebuf, de.d_name, sizeof(ff->namebuf) - 1);
        ff->namebuf[sizeof(ff->namebuf) - 1] = '\0';
        fe->Filename = ff->namebuf;
        return true;
    }
    return false;
}

// ----- File enumeration -----

TbFileFind* PlatformVita::FileFindFirst(const char* filespec, TbFileEntry* fe)
{
    // Determine directory and optional pattern from filespec
    const char* slash   = strrchr(filespec, '/');
    const char* pattern = slash ? slash + 1 : filespec;
    char        dir[256];
    if (slash && slash != filespec) {
        size_t len = (size_t)(slash - filespec);
        if (len >= sizeof(dir)) {
            return nullptr;
        }
        strncpy(dir, filespec, len);
        dir[len] = '\0';
    } else {
        strncpy(dir, ".", sizeof(dir));
    }

    SceUID dfd = sceIoDopen(dir);
    if (dfd < 0) {
        return nullptr;
    }

    auto ff = static_cast<TbFileFind*>(malloc(sizeof(TbFileFind)));
    if (!ff) {
        sceIoDclose(dfd);
        return nullptr;
    }
    ff->handle = dfd;
    strncpy(ff->pattern, strchr(filespec, '*') ? pattern : "", sizeof(ff->pattern) - 1);
    ff->pattern[sizeof(ff->pattern) - 1] = '\0';
    ff->namebuf[0] = '\0';

    if (vita_find_next_entry(ff, fe)) {
        return ff;
    }
    sceIoDclose(ff->handle);
    free(ff);
    return nullptr;
}

int32_t PlatformVita::FileFindNext(TbFileFind* ff, TbFileEntry* fe)
{
    if (!ff) {
        return -1;
    }
    return vita_find_next_entry(ff, fe) ? 1 : -1;
}

void PlatformVita::FileFindEnd(TbFileFind* ff)
{
    if (ff) {
        sceIoDclose(ff->handle);
        free(ff);
    }
}

// ----- CDROM / Redbook audio (no-ops on Vita) -----

void PlatformVita::SetRedbookVolume(SoundVolume) {}
TbBool PlatformVita::PlayRedbookTrack(int) { return false; }
void PlatformVita::PauseRedbookTrack() {}
void PlatformVita::ResumeRedbookTrack() {}
void PlatformVita::StopRedbookTrack() {}

// ----- Path provider -----

void        PlatformVita::SetArgv(int, char**) {} // argv[0] unused on Vita
const char* PlatformVita::GetDataPath() const { return "ux0:data/keeperfx"; }
const char* PlatformVita::GetSavePath() const { return "ux0:data/keeperfx/save"; }

#endif // PLATFORM_VITA
