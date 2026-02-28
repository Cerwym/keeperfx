/******************************************************************************/
// Bullfrog Engine Emulation Library - for use to remake classic games like
// Syndicate Wars, Magic Carpet or Dungeon Keeper.
/******************************************************************************/
/** @file bflib_cpu_stub.c
 *     CPU detection stub for non-x86 platforms.
 * @par Purpose:
 *     Provides no-op / default implementations of CPU detection functions
 *     for architectures where CPUID is unavailable (ARM, MIPS, etc.).
 *     Selected by CMake instead of bflib_cpu.c on non-x86 targets.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "bflib_cpu.h"

#include "bflib_basics.h"
#include "post_inc.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/

void cpu_detect(struct CPU_INFO *cpu)
{
    static char const anonvendor[] = "AnonymousCPU";
    snprintf(cpu->vendor, sizeof(cpu->vendor), "%s", anonvendor);
    cpu->timeStampCounter = 0;
    cpu->feature_intl = 0;
    cpu->feature_edx = 0;
    cpu->BrandString = 0;
    cpu->brand[0] = '\0';
}

unsigned char cpu_get_type(struct CPU_INFO *cpu)
{
    if (cpu->feature_intl != 0)
        return (cpu->feature_intl>>12) & 0x3;
    else
        return CPUID_TYPE_OEM;
}

unsigned char cpu_get_family(struct CPU_INFO *cpu)
{
    if (cpu->feature_intl != 0)
    {
        unsigned char family = (cpu->feature_intl>>8) & 0xF;
        if (family == 15)
            return ((cpu->feature_intl>>20) & 0xFF) + family;
        else
            return family;
    }
    else
    {
        return CPUID_FAMILY_486;
    }
}

unsigned char cpu_get_model(struct CPU_INFO *cpu)
{
    unsigned char family = cpu_get_family(cpu);
    unsigned char model = ((cpu->feature_intl>>4) & 0xF);
    if ( (family == 6) || (family == 15) )
        return (((cpu->feature_intl>>16 & 0xF)) << 4) + model;
    else
        return model;
}

unsigned char cpu_get_stepping(struct CPU_INFO *cpu)
{
    return (cpu->feature_intl) & 0xF;
}

/******************************************************************************/
#ifdef __cplusplus
}
#endif
