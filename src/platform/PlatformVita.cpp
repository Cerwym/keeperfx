#include "pre_inc.h"
#include "platform/PlatformVita.h"
#ifdef PLATFORM_VITA
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/clib.h>
#include <psp2/power.h>
#include <psp2/kernel/threadmgr.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "bflib_crash.h"
#endif
#include "post_inc.h"

#ifdef PLATFORM_VITA

/* Link-time heap and stack declarations required by vitasdk.
 * These are read by the linker/loader before main() runs. */
int _newlib_heap_size_user  = 256 * 1024 * 1024; // 256 MB — sound banks + game data
int sceUserMainThreadStackSize = 8 * 1024 * 1024; // 8 MB — deep game call stacks

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

static void vita_crash_handler(int sig)
{
    FILE* f = fopen("ux0:data/keeperfx/crash.log", "a");
    if (f) {
        fprintf(f, "KeeperFX crashed: signal %d\n", sig);
        fclose(f);
    }
    sceClibPrintf("KeeperFX CRASH: signal %d\n", sig);
    sceKernelExitProcess(1);
}

void PlatformVita::ErrorParachuteInstall()
{
    signal(SIGHUP,  ctrl_handler);
    signal(SIGQUIT, ctrl_handler);
    // Override crash signals with the Vita handler that writes crash.log + exits
    signal(SIGSEGV, vita_crash_handler);
    signal(SIGABRT, vita_crash_handler);
    signal(SIGFPE,  vita_crash_handler);
    signal(SIGILL,  vita_crash_handler);
}

void PlatformVita::ErrorParachuteUpdate()
{
}

// ----- File system helpers -----

TbBool PlatformVita::FileExists(const char* path) const
{
    SceIoStat stat;
    return sceIoGetstat(path, &stat) >= 0;
}

int PlatformVita::MakeDirectory(const char* path)
{
    int ret = sceIoMkdir(path, 0777);
    if (ret >= 0 || ret == (int)0x80010011 /* SCE_ERROR_ERRNO_EEXIST */) return 0;
    return -1;
}

int PlatformVita::GetCurrentDirectory(char* buf, unsigned long buflen)
{
    // On Vita CWD is app0: (read-only); return the data path instead
    snprintf(buf, buflen, "%s", GetDataPath());
    return 1;
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

void PlatformVita::LogWrite(const char* message)
{
    sceClibPrintf("KeeperFX: %s", message);
}

// ----- Hardware / OS initialisation -----

void PlatformVita::SystemInit()
{
    // Maximise CPU/GPU clocks — default is throttled; same settings as vitaQuakeII.
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);
    // Disable VFP/FPU exception traps on the main thread; without this, any
    // denormal or other edge-case float operation in the game code will trap.
    sceKernelChangeThreadVfpException(0x0800009FU, 0x0);
}

void PlatformVita::FrameTick()
{
    sceKernelPowerTick(0); // prevent screen blanking
}

// ----- Path provider -----

void        PlatformVita::SetArgv(int, char**) {} // argv[0] unused on Vita
const char* PlatformVita::GetDataPath() const { return "ux0:data/keeperfx"; }
const char* PlatformVita::GetSavePath() const { return "ux0:data/keeperfx/save"; }

#endif // PLATFORM_VITA
