# Vita Sysmodule Analysis — KeeperFX Port

**Date:** 2026-03-02  
**Branch:** `renderer-abstraction`  
**Purpose:** Research deliverable. Evaluate SCE sysmodules available on HENkaku for relevance to the KeeperFX Vita port. Modules investigated: SceAvPlayer, NGS, SceAtrac, SceLibJson, SceNpTrophy.

---

## SceAvPlayer

**Module constant:** `SCE_SYSMODULE_AVPLAYER`  
**Header:** `psp2/avplayer.h` ✅ present in vitaSDK  
**Stub lib:** `SceAvPlayer_stub` ✅

### API

`sceAvPlayerInit` takes an `SceAvPlayerInitData` struct that allows full replacement of the memory allocator (`SceAvPlayerMemReplacement`) and file I/O layer (`SceAvPlayerFileReplacement`), plus an event callback for state changes. You call `sceAvPlayerAddSource` with a file path, `sceAvPlayerStart`, then poll per-frame:

- `sceAvPlayerGetVideoData(handle, &info)` — returns `SceAvPlayerFrameInfo` with `uint8_t* pData` pointing into the player's internal frame buffer, `uint64_t timeStamp` in milliseconds, and stream details (width, height, aspect ratio)
- `sceAvPlayerGetAudioData(handle, &info)` — returns PCM frame info with channel count, sample rate, size
- `sceAvPlayerCurrentTime(handle)` — returns decode clock in ms; makes A/V sync trivial
- `sceAvPlayerIsActive(handle)` — poll for end-of-stream
- `sceAvPlayerSetLooping`, `sceAvPlayerPause`, `sceAvPlayerResume`, `sceAvPlayerStop`, `sceAvPlayerClose`

Playback is fully hardware-accelerated. The player runs in its own system service context, separate from the app's heap.

### Relevance to KeeperFX

**Direct replacement: not currently possible.** KeeperFX FMV files are Smacker format (`.smk`) from the original Dungeon Keeper CD. SceAvPlayer supports MP4 with H.264/AVC and AAC audio — it does not handle Smacker. FFmpeg handles Smacker natively in software, which is why `bflib_fmvids.cpp` links `libavformat + libavcodec + libswresample`.

**The path that does exist:** transcode all FMV assets to MP4/AVC+AAC as a build-step asset pipeline, ship a Vita-specific data pack. This is technically feasible. SceAvPlayer's custom allocator support (`SceAvPlayerMemReplacement`) means you can bound its memory usage precisely. The hardware decoder runs entirely outside the app's 128MB heap, which matters given current heap pressure.

**Heap savings angle:** the statically-linked FFmpeg bundle is substantial. Offloading decode to SceAvPlayer would reclaim that binary footprint and the decode working memory. If a Vita-native asset pack is ever built, SceAvPlayer is the right path for FMV.

**Verdict:** Not immediately actionable. Revisit when/if a dedicated Vita data pack is on the roadmap.

---

## NGS (Next Generation Sound)

**Module constant:** `SCE_SYSMODULE_NGS` (0x000B)  
**Header:** `psp2/ngs.h` ❌ absent — only `ngs_internal.h` exists, which is empty  
**Stub lib:** not present in public vitaSDK

### What NGS is

NGS is Sony's first-party audio middleware, used in retail Vita titles (Gravity Rush, Tearaway, etc.). It provides a per-voice DSP effects chain, 3D spatial audio, and submix routing buses. It runs on the Vita's dedicated audio DSP.

### Relevance to KeeperFX

**Practically unavailable.** The public vitaSDK does not expose NGS headers or stubs. It has not been meaningfully reverse-engineered to the level required for homebrew use. Even if we reverse-engineered the interface, using undocumented DSP firmware interfaces carries a high risk of breakage across firmware versions.

KeeperFX's audio stack (SDL2 audio output + libvorbis OGG decode + SFX mixing) works correctly on Vita already. There is no gap that NGS fills for us.

**Verdict:** Not available, not relevant.

---

## SceAtrac (AT9)

**Module constant:** `SCE_SYSMODULE_ATRAC`  
**Header:** `psp2/atrac.h` ✅ present  
**Stub lib:** `SceAtrac_stub` ✅ (already linked in KeeperFX Vita build via FFmpeg pipeline)

### What ATRAC9 is

ATRAC9 is Sony's proprietary compressed audio codec. The Vita has a dedicated hardware DSP decoder that processes AT9 essentially free of CPU cost. `psp2/atrac.h` wraps `SceAudiodec` (`SCE_ATRAC_TYPE_AT9`) and exposes a streaming decode loop: create a decoder handle, feed it AT9 frames, receive 16-bit PCM output, submit to `sceAudioOut`. Maximum 2048 PCM samples per output frame, up to 8 output frames buffered, with full loop-point support (`SCE_ATRAC_INFINITE_LOOP_NUM`).

### Why it is not relevant to KeeperFX now

KeeperFX ships all music and sound banks as Ogg Vorbis files, decoded by `libvorbis` + `libogg` which are already statically linked. Using AT9 would require:

1. Re-encoding every audio asset to AT9 format
2. Shipping a Vita-specific game data pack with the re-encoded files

Ogg Vorbis decode on a 444MHz ARM Cortex-A9 is cheap. It does not appear as a profiling bottleneck. The hardware decode benefit is real but the asset pipeline cost is disproportionate to the gain.

The `SceAtrac_stub` is already in the build only because it is pulled in transitively by the FFmpeg audio decode pipeline — KeeperFX is not actually calling any `sceAtrac*` functions directly.

**Verdict:** Relevant only if a dedicated Vita asset pack is built. If that happens, AT9 for looping music tracks is the natural choice — loop-point support in the API is explicit (`SCE_ATRAC_AT9_MIN_LOOP_SAMPLES`), unlike Vorbis loop metadata.

---

## SceLibJson

**Module constant:** `SCE_SYSMODULE_JSON` (0x001F)  
**Header:** `psp2/json.h` ✅ present  
**Stub libs:** `libSceLibJson_stub.a` and `libSceLibJson_stub_weak.a` ✅  
**Available from:** firmware 3.60

### API

The entire API is C++ object-based inside the `sce::Json` namespace. From the NID database, the exported symbols are all mangled C++ names: `sce::Json::Initializer::initialize`, `sce::Json::Value::set`, `sce::Json::Array::push_back`, `sce::Json::Object`, etc. Allocation is controlled via a virtual `sce::Json::MemAllocator` subclass. The module must be loaded at startup with `sceSysmoduleLoadModule(SCE_SYSMODULE_JSON)`.

### Relevance to KeeperFX

KeeperFX uses centijson — a flat C callback/DOM library included from `deps/centijson/`. It is used in `src/api.c` (`json_dom_dump`, `JSON_INPUT_POS`) and `src/custom_sprites.c`. 

**Structural incompatibility:** centijson is a C API; SceLibJson is a C++ object API. They are not interchangeable at the call site. Replacing one with the other would require an abstraction shim — a platform-neutral JSON interface (`JsonNode* json_parse(const char*, size_t)` etc.) with a centijson implementation for PC and a SceLibJson implementation for Vita.

**Cost vs. benefit:** centijson is small — a few KB of `.text`, no persistent heap beyond the parsed document tree (which SceLibJson would also allocate). The gain is eliminating that few KB of compiled code. On a device with 128MB heap, this is not a meaningful saving. The abstraction layer itself costs more in maintenance than it saves.

**Potential long-term relevance:** if a platform JSON abstraction layer is desired for *other* reasons (e.g. 3DS has its own quirks, Nintendo Switch has `nn::json`), then building the shim and plugging SceLibJson in as the Vita backend would be a natural extension. But building it solely to save a few KB is not justified.

**Verdict:** Not worth the abstraction cost for the saving it delivers. Revisit only if a cross-platform JSON layer is needed for other platforms.

---

## SceNpTrophy

**Module constant:** `SCE_SYSMODULE_NP_TROPHY` (0x0025)  
**Header:** `psp2/trophy.h` ❌ absent from public vitaSDK headers (must be written from scene docs)  
**Stub libs:** `libSceNpTrophy_stub.a` and `libSceNpTrophy_stub_weak.a` ✅  
**Available from:** firmware 3.60

### API (from NID database — all symbols confirmed in stub)

```
sceNpTrophyInit()
sceNpTrophyTerm()
sceNpTrophyCreateContext(ctx, np_comm_id, passphrase, flags)
sceNpTrophyDestroyContext(ctx)
sceNpTrophyCreateHandle(handle)
sceNpTrophyDestroyHandle(handle)
sceNpTrophyAbortHandle(handle)
sceNpTrophyUnlockTrophy(handle, ctx, trophy_id, &platinum_id)
sceNpTrophyGetTrophyUnlockState(handle, ctx, &state, &count)
sceNpTrophyGetTrophyInfo(handle, ctx, trophy_id, &info, &icon)
sceNpTrophyGetGameInfo(handle, ctx, &info, &icon)
sceNpTrophyGetGroupInfo(handle, ctx, group_id, &info, &icon)
sceNpTrophyGetTrophyIcon(handle, ctx, trophy_id, &icon)
sceNpTrophyGetGameIcon(handle, ctx, &icon)
sceNpTrophyGetGroupIcon(handle, ctx, group_id, &icon)
```

Trophies use **integer IDs** (0-based index into the `TROPHY.TRP` definition file). They are binary (locked/unlocked) — there is no native partial-progress type. The platinum trophy (`sceNpTrophyUnlockTrophy` returns its ID in the `platinum_id` out-param) is awarded automatically when all other trophies in the set are unlocked.

On HENkaku, trophies are stored locally by the OS even without PSN sign-in. The first-run trophy setup dialog (`SCE_SYSMODULE_INTERNAL_TROPHY_SETUP_DIALOG`) appears automatically. The `_stub_weak.a` variant allows graceful degradation if the NP system is unavailable on a given firmware.

### Why this is directly actionable

The `achievements` worktree (`feature/achievements-and-gog-integration` branch) already implements a complete cross-platform achievement abstraction. The `AchievementBackend` struct in `achievement_api.h` defines function pointers for `init`, `shutdown`, `unlock`, `is_unlocked`, `set_progress`, `get_progress`, `sync`. The `AchPlat_PlayStation` case already exists in the `AchievementPlatform` enum. What does not exist yet is the `platform_vita_trophy.c` file that implements that interface.

### Mapping AchievementBackend → SceNpTrophy

| AchievementBackend | SceNpTrophy |
|---|---|
| `init` | `sceNpTrophyInit()` + `sceNpTrophyCreateContext(&ctx, "NPWR00000_00", passphrase, 0)` + `sceNpTrophyCreateHandle(&handle)` |
| `shutdown` | `sceNpTrophyDestroyHandle(handle)` + `sceNpTrophyDestroyContext(ctx)` + `sceNpTrophyTerm()` |
| `unlock(id)` | Look up integer ID via `s_trophy_map`, call `sceNpTrophyUnlockTrophy(handle, ctx, int_id, NULL)` |
| `is_unlocked(id)` | `sceNpTrophyGetTrophyUnlockState(handle, ctx, &state, &count)` → check bitmask bit for this trophy |
| `set_progress` | Local tracking only; call `unlock` when progress reaches 1.0 |
| `sync` | No-op — auto-syncs with PSN when online |

### String ID → integer trophy ID bridge

```c
static const struct { const char* ach_id; int trophy_id; } s_trophy_map[] = {
    { "keeperfx.complete_level_1", 0 },
    { "keeperfx.no_casualties",    1 },
    /* ... one entry per defined achievement, matching TROPHY.TRP order */
    { NULL, -1 }
};
```

### Platform detection

In `achievement_api.c`, `achievements_detect_platform()` — add before the `AchPlat_Local` fallback:

```c
#ifdef PLATFORM_VITA
    JUSTLOG("Achievement platform: PlayStation (Vita NpTrophy)");
    return AchPlat_PlayStation;
#endif
```

### TROPHY.TRP prerequisite

A `TROPHY.TRP` file must be packaged at `app0:sce_sys/trophy/` in the VPK. This is an SQLite database containing trophy names, descriptions, icon PNGs, types (bronze/silver/gold/platinum), and the NP communication ID. For homebrew, use the community tool `trophygenesis` to author it. The NP communication ID can be a dummy value — local trophy storage works regardless of PSN status.

**This is the only non-code prerequisite.** It must be authored once, with one entry per KeeperFX achievement, in the same order as the integer IDs in `s_trophy_map`.

### Implementation scope

The entire Vita trophy backend is:
- One new source file: `src/platform/platform_vita_trophy.c`
- One `#ifdef PLATFORM_VITA` block in `achievements_detect_platform()`
- One `achievements_register_backend()` call in `PlatformHomebrewMain.cpp`
- `libSceNpTrophy_stub.a` added to the Vita link libs in `CMakeLists.txt`
- `SCE_SYSMODULE_NP_TROPHY` load call in `PlatformVita::SystemInit()`
- `TROPHY.TRP` packaged in the VPK CMake step

No cross-platform code is touched. Full implementation notes in `achievements` worktree: `docs/dev_vita_trophy.md`.

**Verdict: Directly actionable once the achievements branch is rebased onto renderer-abstraction.**

---

## Summary

| Module | Available | Actionable now | Notes |
|---|---|---|---|
| SceAvPlayer | ✅ full API | ❌ | Requires Smacker→MP4 asset transcoding pipeline |
| NGS | ❌ no header/stub | ❌ | Not exposed in public vitaSDK |
| SceAtrac (AT9) | ✅ full API | ❌ | Requires re-encoding all audio assets to AT9 |
| SceLibJson | ✅ full API | ❌ | C++ API incompatible with centijson; saving too small to justify abstraction |
| SceNpTrophy | ✅ stubs only (no header) | ✅ | Achievement backend abstraction already exists; one new file to implement |
