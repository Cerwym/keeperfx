#include "pre_inc.h"
#include "platform.h"
#include "platform/PlatformManager.h"
#include "bflib_basics.h"
#include <SDL2/SDL_log.h>
#include <stdio.h>
#ifdef PLATFORM_VITA
#include "platform/PlatformVita.h"
#include <unistd.h>
#elif defined(PLATFORM_3DS)
#include "platform/Platform3DS.h"
#elif defined(PLATFORM_SWITCH)
#include "platform/PlatformSwitch.h"
#endif
#include "post_inc.h"

#if defined(PLATFORM_VITA)
extern "C" void input_vita_initialize(void);
extern "C" void audio_vita_initialize(void);
#endif

#if defined(PLATFORM_VITA) || defined(PLATFORM_3DS) || defined(PLATFORM_SWITCH)

static FILE* sdl_log_file = nullptr;

static void sdl_log_callback(void* /*userdata*/, int /*category*/, SDL_LogPriority /*priority*/, const char* message)
{
    if (sdl_log_file) {
        fprintf(sdl_log_file, "%s\n", message);
        fflush(sdl_log_file);
    }
}

int main(int argc, char* argv[]) {
#if defined(PLATFORM_VITA)
    PlatformManager::Set(new PlatformVita());
    LbErrorLogSetup(PlatformManager_GetDataPath(), "keeperfx.log", 5);
    // Set the working directory to the data path so relative paths (e.g. "data/creature.tab")
    // resolve against ux0:data/keeperfx/ rather than the app bundle.
    chdir(PlatformManager_GetDataPath());
    input_vita_initialize();
    audio_vita_initialize();
#elif defined(PLATFORM_3DS)
    PlatformManager::Set(new Platform3DS());
    LbErrorLogSetup(PlatformManager_GetDataPath(), "keeperfx.log", 5);
#elif defined(PLATFORM_SWITCH)
    PlatformManager::Set(new PlatformSwitch());
    LbErrorLogSetup(PlatformManager_GetDataPath(), "keeperfx.log", 5);
#endif

    char sdl_log_path[DISKPATH_SIZE];
    snprintf(sdl_log_path, sizeof(sdl_log_path), "%s/sdl.log", PlatformManager_GetDataPath());
    sdl_log_file = fopen(sdl_log_path, "w");
    SDL_LogSetOutputFunction(sdl_log_callback, nullptr);

    return kfxmain(argc, argv);
}

#endif // PLATFORM_VITA || PLATFORM_3DS || PLATFORM_SWITCH
