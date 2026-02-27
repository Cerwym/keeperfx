#ifndef PLATFORM_MANAGER_H
#define PLATFORM_MANAGER_H

#include "IPlatform.h"

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

#endif // PLATFORM_MANAGER_H
