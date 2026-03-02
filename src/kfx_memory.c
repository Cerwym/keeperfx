#include "pre_inc.h"
#include "kfx_memory.h"
#include "post_inc.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Forward declaration -- avoids dragging in PlatformManager.h and its
 * C++-only type dependencies (TbFileHandle, TbBool, etc.) into C code. */
extern size_t PlatformManager_GetScratchSize(void);

/* ===== OOM handler ===== */
static void kfx_oom(size_t size, const char* file, int line)
{
    fprintf(stderr, "KfxAlloc: OUT OF MEMORY: %zu bytes at %s:%d\n",
            size, file ? file : "?", line);
    fflush(stderr);
    abort();
}

/* ===================================================================
 * RELEASE build -- thin wrappers, zero overhead
 * =================================================================== */
#ifndef KFX_DEBUG_MEMORY

void* KfxAlloc(size_t size)
{
    void* p = malloc(size);
    if (!p && size) kfx_oom(size, NULL, 0);
    return p;
}

void* KfxCalloc(size_t count, size_t size)
{
    void* p = calloc(count, size);
    if (!p && count && size) kfx_oom(count * size, NULL, 0);
    return p;
}

void* KfxRealloc(void* ptr, size_t size)
{
    void* p;
    if (size == 0) { free(ptr); return NULL; }
    p = realloc(ptr, size);
    if (!p) kfx_oom(size, NULL, 0);
    return p;
}

void KfxFree(void* ptr)
{
    free(ptr);
}

char* KfxStrDup(const char* s)
{
    char* p;
    size_t len;
    if (!s) return NULL;
    len = strlen(s) + 1;
    p = (char*)malloc(len);
    if (!p) kfx_oom(len, NULL, 0);
    memcpy(p, s, len);
    return p;
}

void KfxMemDump(void) { /* no-op in release */ }

#else /* KFX_DEBUG_MEMORY ============================================
 * DEBUG build -- per-callsite accounting via __FILE__ / __LINE__
 * =================================================================== */

#define KFX_MAX_SITES 256

typedef struct {
    const char* file;
    size_t      live_bytes;
    size_t      total_bytes;
    size_t      alloc_count;
} KfxSite;

static KfxSite s_sites[KFX_MAX_SITES];
static int     s_nsites    = 0;
static size_t  s_total_live = 0;

static KfxSite* get_site(const char* file)
{
    int i;
    /* string-literal pointer identity is sufficient for same TU */
    for (i = 0; i < s_nsites; i++)
        if (s_sites[i].file == file) return &s_sites[i];
    if (s_nsites < KFX_MAX_SITES) {
        s_sites[s_nsites].file = file;
        return &s_sites[s_nsites++];
    }
    return NULL;
}

static void record(size_t size, const char* file, int line)
{
    KfxSite* s;
    (void)line;
    s = file ? get_site(file) : NULL;
    if (s) { s->live_bytes += size; s->total_bytes += size; s->alloc_count++; }
    s_total_live += size;
}

void* KfxAlloc_impl(size_t size, const char* file, int line)
{
    void* p = malloc(size);
    if (!p && size) kfx_oom(size, file, line);
    record(size, file, line);
    return p;
}

void* KfxCalloc_impl(size_t count, size_t size, const char* file, int line)
{
    void* p = calloc(count, size);
    if (!p && count && size) kfx_oom(count * size, file, line);
    record(count * size, file, line);
    return p;
}

void* KfxRealloc_impl(void* ptr, size_t size, const char* file, int line)
{
    void* p;
    if (size == 0) { free(ptr); return NULL; }
    p = realloc(ptr, size);
    if (!p) kfx_oom(size, file, line);
    record(size, file, line);
    return p;
}

void KfxFree(void* ptr) { free(ptr); }

char* KfxStrDup_impl(const char* s, const char* file, int line)
{
    char* p;
    size_t len;
    if (!s) return NULL;
    len = strlen(s) + 1;
    p = (char*)malloc(len);
    if (!p) kfx_oom(len, file, line);
    memcpy(p, s, len);
    record(len, file, line);
    return p;
}

void KfxMemDump(void)
{
    int i;
    fprintf(stderr, "=== KfxMemDump: %zu bytes live ===\n", s_total_live);
    for (i = 0; i < s_nsites; i++) {
        const char* f = s_sites[i].file;
        const char* sl = strrchr(f, '/');
        if (!sl) sl = strrchr(f, '\\');
        fprintf(stderr, "  %-45s  live=%9zu  allocs=%zu\n",
                sl ? sl + 1 : f,
                s_sites[i].live_bytes,
                s_sites[i].alloc_count);
    }
}

#endif /* KFX_DEBUG_MEMORY */

/* ===================================================================
 * Scratch / arena allocator (shared release + debug)
 * =================================================================== */
static unsigned char* s_scratch_base = NULL;
static size_t         s_scratch_cap  = 0;
static size_t         s_scratch_used = 0;

void KfxMemInit(void)
{
    size_t cap = PlatformManager_GetScratchSize();
    s_scratch_base = (unsigned char*)malloc(cap);
    if (!s_scratch_base) kfx_oom(cap, __FILE__, __LINE__);
    s_scratch_cap  = cap;
    s_scratch_used = 0;
}

void KfxMemShutdown(void)
{
    free(s_scratch_base);
    s_scratch_base = NULL;
    s_scratch_cap  = 0;
    s_scratch_used = 0;
}

void* KfxScratch(size_t size)
{
    void* p;
    /* align to 8 bytes */
    size = (size + 7u) & ~7u;
    if (s_scratch_base && s_scratch_used + size <= s_scratch_cap) {
        p = s_scratch_base + s_scratch_used;
        s_scratch_used += size;
        return p;
    }
    /* overflow: fall back to heap -- caller must not KfxFree this */
    fprintf(stderr, "KfxScratch: overflow (used=%zu cap=%zu req=%zu)\n",
            s_scratch_used, s_scratch_cap, size);
    return malloc(size);
}

void KfxScratchReset(void)
{
    s_scratch_used = 0;
}

size_t KfxScratchUsed(void)
{
    return s_scratch_used;
}
