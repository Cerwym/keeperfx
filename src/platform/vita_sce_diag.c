/* vita_sce_diag.c — taiHEN SCE subsystem diagnostic hooks.
 *
 * Active only in vita-debug builds (-DVITA_SCE_DIAG=1).
 * Compiles to an empty translation unit on all other platforms/presets.
 *
 * Pattern: hook our eboot's import-table entries so every SCE call made by
 * KeeperFX (including those made by statically-linked vitaGL) is intercepted.
 * taiHookFunctionImport on TAI_MAIN_MODULE patches the import stub in our
 * process; it does NOT hook the shared library exports at 0xE0000000+.
 *
 * All output goes to kfx_preinit.log via the SCE_DIAG_LOG macro.
 *
 * NIDs sourced from vitasdk share/vita-headers/db/360/
 */

#ifdef VITA_SCE_DIAG

#include <taihen.h>
#include <psp2/gxm.h>
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/sysmodule.h>
#include <psp2/io/fcntl.h>
#include <psp2/audioout.h>
#include <stdio.h>
#include <stdint.h>

/* ---------------------------------------------------------------------------
 * Logging
 * --------------------------------------------------------------------------*/
#define SCE_DIAG_LOG(fmt, ...) do { \
    FILE *_f = fopen("ux0:data/keeperfx/kfx_preinit.log", "a"); \
    if (_f) { fprintf(_f, "[SCE_DIAG] " fmt "\n", ##__VA_ARGS__); fclose(_f); } \
} while (0)

/* ---------------------------------------------------------------------------
 * Hook bookkeeping — max 16 hooks
 * --------------------------------------------------------------------------*/
#define VITA_DIAG_MAX_HOOKS 16
static SceUID         s_hook_uid[VITA_DIAG_MAX_HOOKS];
static tai_hook_ref_t s_hook_ref[VITA_DIAG_MAX_HOOKS];
static int            s_hook_count = 0;

static SceUID vita_diag_hook(tai_hook_ref_t *ref,
                              uint32_t lib_nid, uint32_t func_nid,
                              const void *hook_fn)
{
    SceUID uid = taiHookFunctionImport(ref, TAI_MAIN_MODULE, lib_nid, func_nid, hook_fn);
    if (uid >= 0 && s_hook_count < VITA_DIAG_MAX_HOOKS) {
        s_hook_uid[s_hook_count] = uid;
        s_hook_ref[s_hook_count] = *ref;
        s_hook_count++;
    } else if (uid < 0) {
        SCE_DIAG_LOG("hook FAIL lib=0x%08X func=0x%08X ret=0x%08X",
                     (unsigned)lib_nid, (unsigned)func_nid, (unsigned)uid);
    }
    return uid;
}

/* ---------------------------------------------------------------------------
 * GXM domain (lib NID 0xF76B66BD)
 * --------------------------------------------------------------------------*/
static tai_hook_ref_t s_gxm_init_ref;
static int hook_sceGxmInitialize(const SceGxmInitializeParams *params) {
    int ret = TAI_CONTINUE(int, s_gxm_init_ref, params);
    SCE_DIAG_LOG("sceGxmInitialize params=%p ret=0x%08X", (void *)params, (unsigned)ret);
    return ret;
}

static tai_hook_ref_t s_gxm_ctx_ref;
static int hook_sceGxmCreateContext(const SceGxmContextParams *params, SceGxmContext **ctx) {
    int ret = TAI_CONTINUE(int, s_gxm_ctx_ref, params, ctx);
    SCE_DIAG_LOG("sceGxmCreateContext ret=0x%08X", (unsigned)ret);
    return ret;
}

static tai_hook_ref_t s_gxm_term_ref;
static int hook_sceGxmTerminate(void) {
    int ret = TAI_CONTINUE(int, s_gxm_term_ref);
    SCE_DIAG_LOG("sceGxmTerminate ret=0x%08X", (unsigned)ret);
    return ret;
}

/* ---------------------------------------------------------------------------
 * Module loading domain
 * SceLibKernel lib NID 0xCAE9ACE6 / SceSysmodule lib NID 0x0958E8D1
 * --------------------------------------------------------------------------*/
static tai_hook_ref_t s_loadmod_ref;
static SceUID hook_sceKernelLoadStartModule(const char *path, SceSize args,
                                             void *argp, int flags,
                                             SceKernelLMOption *opt, int *status)
{
    SceUID ret = TAI_CONTINUE(SceUID, s_loadmod_ref, path, args, argp, flags, opt, status);
    SCE_DIAG_LOG("sceKernelLoadStartModule path=%s ret=0x%08X",
                 path ? path : "(null)", (unsigned)ret);
    return ret;
}

static tai_hook_ref_t s_sysmod_ref;
static int hook_sceSysmoduleLoadModule(uint16_t id) {
    int ret = TAI_CONTINUE(int, s_sysmod_ref, id);
    SCE_DIAG_LOG("sceSysmoduleLoadModule id=0x%04X ret=0x%08X", (unsigned)id, (unsigned)ret);
    return ret;
}

/* ---------------------------------------------------------------------------
 * Memory domain (SceSysmem lib NID 0x3380B323)
 * --------------------------------------------------------------------------*/
static tai_hook_ref_t s_alloc_ref;
static SceUID hook_sceKernelAllocMemBlock(const char *name, SceKernelMemBlockType type,
                                           SceSize size, void *opt)
{
    SceUID ret = TAI_CONTINUE(SceUID, s_alloc_ref, name, type, size, opt);
    SCE_DIAG_LOG("sceKernelAllocMemBlock name=%s type=0x%08X size=0x%X uid=0x%08X",
                 name ? name : "(null)", (unsigned)type, (unsigned)size, (unsigned)ret);
    return ret;
}

/* ---------------------------------------------------------------------------
 * File IO domain (SceIofilemgr lib NID 0x9642948C)
 * --------------------------------------------------------------------------*/
static tai_hook_ref_t s_ioopen_ref;
static SceUID hook_sceIoOpen(const char *path, int flags, SceMode mode) {
    SceUID ret = TAI_CONTINUE(SceUID, s_ioopen_ref, path, flags, mode);
    SCE_DIAG_LOG("sceIoOpen path=%s flags=0x%X ret=0x%08X",
                 path ? path : "(null)", (unsigned)flags, (unsigned)ret);
    return ret;
}

/* ---------------------------------------------------------------------------
 * Display domain (SceDisplay lib NID 0x3F05296F)
 * --------------------------------------------------------------------------*/
static tai_hook_ref_t s_display_ref;
static int hook_sceDisplaySetFrameBuf(const SceDisplayFrameBuf *fb,
                                       SceDisplaySetBufSync sync)
{
    int ret = TAI_CONTINUE(int, s_display_ref, fb, sync);
    SCE_DIAG_LOG("sceDisplaySetFrameBuf base=%p pitch=%u ret=0x%08X",
                 fb ? fb->base : NULL, fb ? (unsigned)fb->pitch : 0u, (unsigned)ret);
    return ret;
}

/* ---------------------------------------------------------------------------
 * Audio domain (SceAudio lib NID 0x0DA7714A)
 * --------------------------------------------------------------------------*/
static tai_hook_ref_t s_audio_ref;
static int hook_sceAudioOutOpenPort(int portType, int len, int freq, int param) {
    int ret = TAI_CONTINUE(int, s_audio_ref, portType, len, freq, param);
    SCE_DIAG_LOG("sceAudioOutOpenPort type=%d len=%d freq=%d ret=0x%08X",
                 portType, len, freq, (unsigned)ret);
    return ret;
}

/* ---------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------*/
void vita_diag_install_hooks(void)
{
    s_hook_count = 0;
    SCE_DIAG_LOG("=== SCE_DIAG hooks installing ===");

    /* GXM */
    vita_diag_hook(&s_gxm_init_ref,  0xF76B66BD, 0xB0F1E4EC, hook_sceGxmInitialize);
    vita_diag_hook(&s_gxm_ctx_ref,   0xF76B66BD, 0xE84CE5B4, hook_sceGxmCreateContext);
    vita_diag_hook(&s_gxm_term_ref,  0xF76B66BD, 0xB627DE66, hook_sceGxmTerminate);
    /* Module loading */
    vita_diag_hook(&s_loadmod_ref,   0xCAE9ACE6, 0x2DCC4AFA, hook_sceKernelLoadStartModule);
    vita_diag_hook(&s_sysmod_ref,    0x0958E8D1, 0x79A0160A, hook_sceSysmoduleLoadModule);
    /* Memory */
    vita_diag_hook(&s_alloc_ref,     0x3380B323, 0xB9D5EBDE, hook_sceKernelAllocMemBlock);
    /* File IO */
    vita_diag_hook(&s_ioopen_ref,    0x9642948C, 0x6C60AC61, hook_sceIoOpen);
    /* Display */
    vita_diag_hook(&s_display_ref,   0x3F05296F, 0x7A410B64, hook_sceDisplaySetFrameBuf);
    /* Audio */
    vita_diag_hook(&s_audio_ref,     0x0DA7714A, 0x5BC341E4, hook_sceAudioOutOpenPort);

    SCE_DIAG_LOG("=== %d/%d hooks active ===", s_hook_count, VITA_DIAG_MAX_HOOKS);
}

void vita_diag_unhook_all(void)
{
    int i;
    for (i = 0; i < s_hook_count; i++)
        taiHookRelease(s_hook_uid[i], s_hook_ref[i]);
    SCE_DIAG_LOG("=== %d hooks released ===", s_hook_count);
    s_hook_count = 0;
}

#endif /* VITA_SCE_DIAG */
