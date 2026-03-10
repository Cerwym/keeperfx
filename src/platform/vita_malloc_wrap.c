/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file vita_malloc_wrap.c
 *     vitaGL unified-heap malloc wrappers for PlayStation Vita.
 * @par Purpose:
 *     Routes all stdlib heap calls through vitaGL's allocator when vitaGL is
 *     active.  This unifies the C stdlib heap and the vitaGL GPU memory pool
 *     into a single allocator, preventing cross-pool fragmentation — a large
 *     texture upload can use memory a game object just freed, and vice versa.
 * @par Comment:
 *     Activated via GCC --wrap linker flags added by CMakeLists for Vita
 *     builds.  The __wrap_* convention
 *     means the linker silently replaces every call to malloc/free/etc. in the
 *     entire binary (game code + all linked libraries) with these functions,
 *     with zero source changes needed elsewhere.
 *     Technique from d3es-vita neo/sys/linux/main.cpp.
 * @author   KeeperFX Team
 * @date     03 Mar 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include <stdint.h>
#include <string.h>
#include <vitaGL.h>

/* Forward declarations of the real allocator provided by the linker's --wrap
 * mechanism.  When vglMalloc / vglFree / vglRealloc call stdlib internally
 * (e.g. for their own bookkeeping), those calls are also intercepted by
 * --wrap and re-enter __wrap_malloc.  The recursion guard detects this and
 * routes the inner call through __real_malloc (the original libc malloc),
 * keeping VitaGL's internals out of the VGL pool while game-code allocations
 * still go through vglMalloc. */
extern void *__real_malloc(uint32_t size);
extern void  __real_free(void *addr);
extern void *__real_realloc(void *ptr, uint32_t size);
extern void *__real_memalign(uint32_t alignment, uint32_t size);

/* Plain (non-TLS) flags: on ARM Vita _Thread_local uses emutls which
 * allocates its per-thread slot via malloc — putting it in a malloc wrapper
 * creates infinite recursion before the guard can even be checked.
 * The game's rendering/allocation work is single-threaded so a plain
 * static flag is sufficient. */
static int _in_vgl_malloc  = 0;
static int _in_vgl_free    = 0;
static int _in_vgl_realloc = 0;

void *__wrap_malloc(uint32_t size)
{
    if (_in_vgl_malloc) return __real_malloc(size);
    _in_vgl_malloc = 1;
    void *ptr = vglMalloc(size);
    _in_vgl_malloc = 0;
    return ptr;
}
void  __wrap_free(void *addr)
{
    if (_in_vgl_free) { __real_free(addr); return; }
    _in_vgl_free = 1;
    vglFree(addr);
    _in_vgl_free = 0;
}
void *__wrap_calloc(uint32_t nmemb, uint32_t size)
{
    /* Do NOT call vglCalloc(): it internally calls calloc() → __wrap_calloc
     * → infinite recursion.  Use __wrap_malloc (which carries the recursion
     * guard) and zero the block manually. */
    size_t total = (size_t)nmemb * size;
    void *ptr = __wrap_malloc(total);
    if (ptr != NULL)
        memset(ptr, 0, total);
    return ptr;
}
void *__wrap_realloc(void *ptr, uint32_t size)
{
    if (_in_vgl_realloc) return __real_realloc(ptr, size);
    _in_vgl_realloc = 1;
    void *p = vglRealloc(ptr, size);
    _in_vgl_realloc = 0;
    return p;
}
static int _in_vgl_memalign = 0;

void *__wrap_memalign(uint32_t alignment, uint32_t size)
{
    if (_in_vgl_memalign) return __real_memalign(alignment, size);
    _in_vgl_memalign = 1;
    void *ptr = vglMemalign(alignment, size);
    _in_vgl_memalign = 0;
    return ptr;
}
