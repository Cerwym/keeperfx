#include "pre_inc.h"
#include "platform/PlatformLinux.h"
#include <string>
#include <memory>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>
#include "post_inc.h"

// TbFileFind is defined here; it is an opaque type to all callers.
struct TbFileFind {
    std::string filespec;
    std::string path;
    std::string namebuf;
    DIR*        handle = nullptr;
    bool        is_pattern = false;
};

// ----- OS information -----

const char* PlatformLinux::GetOSVersion() const
{
    return "Linux";
}

const void* PlatformLinux::GetImageBase() const
{
    return nullptr;
}

const char* PlatformLinux::GetWineVersion() const
{
    return nullptr;
}

const char* PlatformLinux::GetWineHost() const
{
    return nullptr;
}

// ----- Crash / error parachute -----

void PlatformLinux::InstallExceptionHandler()
{
    // TODO: install signal handler
}

void PlatformLinux::ErrorParachuteInstall()
{
    // TODO: implement backtrace logging on crash
}

void PlatformLinux::ErrorParachuteUpdate()
{
    // TODO: implement backtrace logging on crash
}

// ----- File enumeration -----

static bool filespec_is_pattern(const char* filespec)
{
    return strchr(filespec, '*') != nullptr;
}

static std::string directory_from_filespec(const char* filespec)
{
    const auto sep = strrchr(filespec, '/');
    if (sep && sep != filespec) {
        return std::string(filespec, sep - filespec);
    }
    return ".";
}

static bool find_file(TbFileFind* ff, TbFileEntry* fe)
{
    while (true) {
        auto de = readdir(ff->handle);
        if (!de) {
            return false;
        }
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }
        const std::string path = ff->path + "/" + de->d_name;
        ff->namebuf = de->d_name;
        fe->Filename = ff->namebuf.c_str();
        if (ff->is_pattern) {
            if (fnmatch(ff->filespec.c_str(), path.c_str(), FNM_FILE_NAME | FNM_CASEFOLD) != 0) {
                continue;
            }
        }
        struct stat sb;
        if (stat(path.c_str(), &sb) < 0) {
            continue;
        }
        if (!S_ISREG(sb.st_mode)) {
            continue;
        }
        return true;
    }
}

TbFileFind* PlatformLinux::FileFindFirst(const char* filespec, TbFileEntry* fe)
{
    try {
        auto ff = std::make_unique<TbFileFind>();
        ff->is_pattern = filespec_is_pattern(filespec);
        ff->filespec = filespec;
        if (ff->is_pattern) {
            ff->path   = directory_from_filespec(filespec);
            ff->handle = opendir(ff->path.c_str());
        } else {
            ff->path   = filespec;
            ff->handle = opendir(filespec);
        }
        if (ff->handle && find_file(ff.get(), fe)) {
            return ff.release();
        }
    } catch (...) {}
    return nullptr;
}

int32_t PlatformLinux::FileFindNext(TbFileFind* ff, TbFileEntry* fe)
{
    try {
        if (find_file(ff, fe)) {
            return 1;
        }
    } catch (...) {}
    return -1;
}

void PlatformLinux::FileFindEnd(TbFileFind* ff)
{
    if (ff) {
        closedir(ff->handle);
    }
    delete ff;
}

// ----- CDROM / Redbook audio -----

void PlatformLinux::SetRedbookVolume(SoundVolume)
{
    // TODO: implement CDROM features
}

TbBool PlatformLinux::PlayRedbookTrack(int)
{
    // TODO: implement CDROM features
    return false;
}

void PlatformLinux::PauseRedbookTrack()
{
    // TODO: implement CDROM features
}

void PlatformLinux::ResumeRedbookTrack()
{
    // TODO: implement CDROM features
}

void PlatformLinux::StopRedbookTrack()
{
    // TODO: implement CDROM features
}
