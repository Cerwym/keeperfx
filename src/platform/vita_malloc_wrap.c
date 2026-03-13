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
#include <errno.h>
#include <limits.h>
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

/* Simple recursion guards: when vglMalloc/vglFree call stdlib internally,
 * these flags prevent infinite recursion by routing inner calls to __real_malloc.
 * We use volatile to ensure the compiler doesn't optimize out the flag checks. */
static volatile int _in_vgl_malloc = 0;
static volatile int _in_vgl_free = 0;
static volatile int _in_vgl_realloc = 0;
static volatile int _in_vgl_memalign = 0;

/* Fallback small-allocation threshold: if vglMalloc exhausts the GPU pool,
 * allocations below this size will try __real_malloc as a fallback. This
 * prevents crashes when the vglMalloc pool is full but CPU heap is available.
 * Textures and large buffers still prefer vglMalloc (GPU-optimal). */
#define FALLBACK_ALLOC_THRESHOLD (5 * 1024 * 1024)  // 5 MB threshold

void *__wrap_malloc(uint32_t size)
{
    if (_in_vgl_malloc)
        return __real_malloc(size);
    
    _in_vgl_malloc = 1;
    void *ptr = vglMalloc(size);
    _in_vgl_malloc = 0;
    
    /* If vglMalloc exhausted but the allocation is small, try fallback.
     * This prevents crashes when a small buffer request fails due to pool
     * exhaustion while CPU heap still has space. */
    if (!ptr && size < FALLBACK_ALLOC_THRESHOLD) {
        ptr = __real_malloc(size);
    }
    return ptr;
}

void __wrap_free(void *addr)
{
    if (!addr) return;  // NULL is safe to free
    
    if (_in_vgl_free) {
        __real_free(addr);
        return;
    }
    
    _in_vgl_free = 1;
    vglFree(addr);
    _in_vgl_free = 0;
}

void *__wrap_calloc(uint32_t nmemb, uint32_t size)
{
    /* Do NOT call vglCalloc(): it internally calls calloc() → __wrap_calloc
     * → infinite recursion.  Use __wrap_malloc (which carries the recursion
     * guard) and zero the block manually. 
     * 
     * CRITICAL: Check for integer overflow in nmemb * size (standard C behavior).
     * Without this check, nmemb=0x100000000 size=0x100000000 silently wraps to 0.
     * This matches the overflow protection in standard libc calloc().
     */
    if (nmemb && size > 0xFFFFFFFFU / nmemb) {
        errno = ENOMEM;
        return NULL;
    }
    size_t total = (size_t)nmemb * size;
    void *ptr = __wrap_malloc(total);
    if (ptr != NULL)
        memset(ptr, 0, total);
    return ptr;
}

void *__wrap_realloc(void *ptr, uint32_t size)
{
    if (_in_vgl_realloc)
        return __real_realloc(ptr, size);
    
    _in_vgl_realloc = 1;
    void *p = vglRealloc(ptr, size);
    _in_vgl_realloc = 0;
    
    /* If vglRealloc fails and size is small, try fallback to __real_malloc.
     * This follows the same logic as __wrap_malloc. */
    if (!p && size < FALLBACK_ALLOC_THRESHOLD) {
        p = __real_realloc(ptr, size);
    }
    return p;
}

void *__wrap_memalign(uint32_t alignment, uint32_t size)
{
    if (_in_vgl_memalign)
        return __real_memalign(alignment, size);
    
    _in_vgl_memalign = 1;
    void *ptr = vglMemalign(alignment, size);
    _in_vgl_memalign = 0;
    
    /* If vglMemalign fails and size is small, try fallback. */
    if (!ptr && size < FALLBACK_ALLOC_THRESHOLD) {
        ptr = __real_memalign(alignment, size);
    }
    return ptr;
}
