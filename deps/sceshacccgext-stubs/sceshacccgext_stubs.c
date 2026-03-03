/* Stub for SceShaccCgExt symbols referenced by libvitashark.a.
 * vitashark calls sceShaccCgExtEnable/DisableExtensions only when compiling
 * shaders at runtime.  KeeperFX uses pre-compiled .gxp binaries so these
 * functions are never reached; the stubs exist only to satisfy the linker. */
#include <stdint.h>

int32_t sceShaccCgExtEnableExtensions(void)  { return 0; }
int32_t sceShaccCgExtDisableExtensions(void) { return 0; }
