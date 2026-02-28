#include "pre_inc.h"
#include "platform/PlatformManager.h"
#include "platform.h"
#include "bflib_fileio.h"
#include "post_inc.h"

// ----- Singleton storage -----

IPlatform* PlatformManager::s_instance = nullptr;

IPlatform* PlatformManager::Get()
{
    return s_instance;
}

void PlatformManager::Set(IPlatform* platform)
{
    delete s_instance;
    s_instance = platform;
}

// ----- C-compatible wrappers -----
// Only functions previously duplicated in linux.cpp / windows.cpp are wrapped
// here.  Functions with dedicated cross-platform source files (bflib_crash.c,
// cdrom.cpp) are left to those files.

extern "C" const char* get_os_version()
{
    return PlatformManager::Get()->GetOSVersion();
}

extern "C" const void* get_image_base()
{
    return PlatformManager::Get()->GetImageBase();
}

extern "C" const char* get_wine_version()
{
    return PlatformManager::Get()->GetWineVersion();
}

extern "C" const char* get_wine_host()
{
    return PlatformManager::Get()->GetWineHost();
}

extern "C" void install_exception_handler()
{
    PlatformManager::Get()->InstallExceptionHandler();
}

extern "C" TbFileFind* LbFileFindFirst(const char* filespec, TbFileEntry* fe)
{
    return PlatformManager::Get()->FileFindFirst(filespec, fe);
}

extern "C" int32_t LbFileFindNext(TbFileFind* ff, TbFileEntry* fe)
{
    return PlatformManager::Get()->FileFindNext(ff, fe);
}

extern "C" void LbFileFindEnd(TbFileFind* ff)
{
    PlatformManager::Get()->FileFindEnd(ff);
}

extern "C" const char* PlatformManager_GetDataPath()
{
    return PlatformManager::Get()->GetDataPath();
}

extern "C" const char* PlatformManager_GetSavePath()
{
    return PlatformManager::Get()->GetSavePath();
}

extern "C" void PlatformManager_SetArgv(int argc, char** argv)
{
    PlatformManager::Get()->SetArgv(argc, argv);
}

