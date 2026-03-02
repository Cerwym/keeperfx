#include "pre_inc.h"
#include "platform.h"
#include "platform/PlatformManager.h"
#include "bflib_basics.h"
#include "kfx_memory.h"
#include <SDL2/SDL_log.h>
#include <stdio.h>
#ifdef PLATFORM_VITA
#include "platform/PlatformVita.h"
#elif defined(PLATFORM_3DS)
#include "platform/Platform3DS.h"
#elif defined(PLATFORM_SWITCH)
#include "platform/PlatformSwitch.h"
#endif
#include "post_inc.h"

#if defined(PLATFORM_VITA)
extern "C" void input_vita_initialize(void);
extern "C" void audio_vita_initialize(void);
#include <psp2/kernel/modulemgr.h>
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
    { FILE* _f = fopen("ux0:data/keeperfx/kfx_boot.log", "w"); if (_f) { fprintf(_f, "main-enter\n"); fclose(_f); } }
    {
        // Probe: can we load libshacccg.suprx at the absolute earliest point,
        // before SDL2, before any other init? This proves or disproves whether
        // our init sequence (not the BSS itself) exhausts the physical page pool.
        FILE* _f = fopen("ux0:data/keeperfx/kfx_boot.log", "a");
        if (_f) { fprintf(_f, "shacccg-probe-start\n"); fclose(_f); }
        SceUID r1 = sceKernelLoadStartModule("ur0:/data/libshacccg.suprx", 0, NULL, 0, NULL, NULL);
        SceUID r2 = (r1 < 0) ? sceKernelLoadStartModule("ur0:data/external/libshacccg.suprx", 0, NULL, 0, NULL, NULL) : r1;
        _f = fopen("ux0:data/keeperfx/kfx_boot.log", "a");
        if (_f) {
            fprintf(_f, "shacccg-probe: ur0:/data => 0x%08X  ur0:data/external => 0x%08X\n",
                (unsigned)r1, r1 < 0 ? (unsigned)r2 : 0u);
            fprintf(_f, "shacccg-probe-end: %s\n", (r1 >= 0 || r2 >= 0) ? "LOADED" : "FAILED");
            fclose(_f);
        }
        // Do not unload — leave it resident so vitaSHARK can find it later.
    }
    PlatformManager::Set(new PlatformVita());
    { FILE* _f = fopen("ux0:data/keeperfx/kfx_boot.log", "a"); if (_f) { fprintf(_f, "post-ctor\n"); fclose(_f); } }
    PlatformManager::Get()->SystemInit();
    { FILE* _f = fopen("ux0:data/keeperfx/kfx_boot.log", "a"); if (_f) { fprintf(_f, "post-sysinit\n"); fclose(_f); } }
    LbErrorLogSetup(PlatformManager_GetDataPath(), "keeperfx.log", 5);
    { FILE* _f = fopen("ux0:data/keeperfx/kfx_boot.log", "a"); if (_f) { fprintf(_f, "post-logsetup\n"); fclose(_f); } }
    // sceIoChdir to data path is done inside PlatformVita::SystemInit()
    input_vita_initialize();
    { FILE* _f = fopen("ux0:data/keeperfx/kfx_boot.log", "a"); if (_f) { fprintf(_f, "post-input\n"); fclose(_f); } }
    audio_vita_initialize();
    { FILE* _f = fopen("ux0:data/keeperfx/kfx_boot.log", "a"); if (_f) { fprintf(_f, "post-audio\n"); fclose(_f); } }
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
    KfxMemInit();

    return kfxmain(argc, argv);
}

#endif // PLATFORM_VITA || PLATFORM_3DS || PLATFORM_SWITCH
