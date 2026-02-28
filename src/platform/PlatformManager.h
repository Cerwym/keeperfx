#ifndef PLATFORM_MANAGER_H
#define PLATFORM_MANAGER_H

#ifdef __cplusplus
#  include "IPlatform.h"

/** Singleton owner and accessor for the active IPlatform implementation.
 *
 *  Call PlatformManager::Set() once at program startup (before kfxmain) to
 *  register the platform implementation.  The C-compatible wrappers defined
 *  in PlatformManager.cpp then delegate to the registered instance.
 */
class PlatformManager {
public:
    static IPlatform* Get();
    static void       Set(IPlatform* platform);

private:
    static IPlatform* s_instance;
};
#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

/** C-callable wrappers — safe to include and call from C translation units. */
const char* PlatformManager_GetDataPath(void);
const char* PlatformManager_GetSavePath(void);
void        PlatformManager_SetArgv(int argc, char** argv);
void        PlatformManager_ErrorParachuteInstall(void);
void        PlatformManager_ErrorParachuteUpdate(void);
TbBool      PlatformManager_FileExists(const char* path);
int         PlatformManager_MakeDirectory(const char* path);
int         PlatformManager_GetCurrentDirectory(char* buf, unsigned long buflen);
void        PlatformManager_LogWrite(const char* message);
/** Called once per frame to allow the platform to perform per-frame housekeeping
 *  (e.g. prevent screen blanking on Vita via sceKernelPowerTick). */
void        PlatformManager_FrameTick(void);

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_MANAGER_H
