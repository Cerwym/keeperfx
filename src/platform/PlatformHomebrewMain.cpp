#include "pre_inc.h"
#include "platform.h"
#include "platform/PlatformManager.h"
#ifdef PLATFORM_VITA
#include "platform/PlatformVita.h"
#elif defined(PLATFORM_3DS)
#include "platform/Platform3DS.h"
#elif defined(PLATFORM_SWITCH)
#include "platform/PlatformSwitch.h"
#endif
#include "post_inc.h"

#if defined(PLATFORM_VITA) || defined(PLATFORM_3DS) || defined(PLATFORM_SWITCH)

int main(int argc, char* argv[]) {
#if defined(PLATFORM_VITA)
    PlatformManager::Set(new PlatformVita());
#elif defined(PLATFORM_3DS)
    PlatformManager::Set(new Platform3DS());
#elif defined(PLATFORM_SWITCH)
    PlatformManager::Set(new PlatformSwitch());
#endif
    return kfxmain(argc, argv);
}

#endif // PLATFORM_VITA || PLATFORM_3DS || PLATFORM_SWITCH
