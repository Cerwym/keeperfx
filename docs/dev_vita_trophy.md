# Vita Trophy Backend — Integration Notes

**Date:** 2026-03-02  
**Branch:** `feature/achievements-and-gog-integration`  
**Context:** Researched as part of Vita port (`renderer-abstraction` branch) sysmodule investigation.

---

## SceNpTrophy API

Module: `SCE_SYSMODULE_NP_TROPHY` (0x0025)  
Stub libs: `libSceNpTrophy_stub.a` and `libSceNpTrophy_stub_weak.a` — both present in vitaSDK.

Full exported function set (all available from firmware 3.60):

```
sceNpTrophyInit / sceNpTrophyTerm
sceNpTrophyCreateContext / sceNpTrophyDestroyContext
sceNpTrophyCreateHandle / sceNpTrophyDestroyHandle / sceNpTrophyAbortHandle
sceNpTrophyUnlockTrophy
sceNpTrophyGetTrophyUnlockState
sceNpTrophyGetTrophyInfo / sceNpTrophyGetGameInfo / sceNpTrophyGetGroupInfo
sceNpTrophyGetTrophyIcon / sceNpTrophyGetGameIcon / sceNpTrophyGetGroupIcon
```

No functions are missing from what we need. This is the complete API.

---

## Mapping onto AchievementBackend

The `AchievementBackend` struct in `achievement_api.h` maps cleanly onto `SceNpTrophy`. The entire Vita implementation is one new file: `src/platform/platform_vita_trophy.c`.

| AchievementBackend field | SceNpTrophy equivalent |
|---|---|
| `init` | `sceNpTrophyInit()` + `sceNpTrophyCreateContext(&ctx, np_comm_id, passphrase, 0)` + `sceNpTrophyCreateHandle(&handle)` |
| `shutdown` | `sceNpTrophyDestroyHandle(handle)` + `sceNpTrophyDestroyContext(ctx)` + `sceNpTrophyTerm()` |
| `unlock` | `sceNpTrophyUnlockTrophy(handle, ctx, trophy_int_id, NULL)` — see ID mapping below |
| `is_unlocked` | `sceNpTrophyGetTrophyUnlockState(handle, ctx, &state, &count)` — check bitmask |
| `set_progress` | Local tracking only; NP trophies are binary. Call `unlock` when progress reaches 1.0 |
| `sync` | No-op — NpTrophy auto-syncs with PSN whenever user is online |

---

## String ID → Integer Trophy ID mapping

`sceNpTrophyUnlockTrophy` takes an integer ID corresponding to a trophy's position in the `TROPHY.TRP` file. The achievement system uses string IDs (`achievement_id`). The bridge is a static lookup table in `platform_vita_trophy.c`:

```c
static const struct { const char* ach_id; int trophy_id; } s_trophy_map[] = {
    { "keeperfx.complete_level_1",  0 },
    { "keeperfx.no_casualties",     1 },
    /* ... */
    { NULL, -1 }
};

static int find_trophy_id(const char* ach_id) {
    for (int i = 0; s_trophy_map[i].ach_id != NULL; i++) {
        if (strcmp(s_trophy_map[i].ach_id, ach_id) == 0)
            return s_trophy_map[i].trophy_id;
    }
    return -1;
}
```

The integer IDs must match exactly the ordering defined in `TROPHY.TRP`.

---

## Platform detection

`achievements_detect_platform()` in `achievement_api.c` currently falls through to `AchPlat_Local` on non-Windows. Add before the fallback:

```c
#ifdef PLATFORM_VITA
    JUSTLOG("Achievement platform: PlayStation (Vita NpTrophy)");
    return AchPlat_PlayStation;
#endif
```

---

## TROPHY.TRP prerequisite

`SceNpTrophy` requires a `TROPHY.TRP` file packaged in the VPK at `app0:sce_sys/trophy/`. This is an SQLite database containing:
- Trophy names, descriptions, types (bronze/silver/gold/platinum)
- Trophy icons (PNG, embedded)
- The NP communication ID this set belongs to

For homebrew, use `trophygenesis` (community tool) to author the `.trp` file. The NP communication ID can be a dummy value (e.g. `NPWR00000_00`). Trophies will be stored **locally by the OS** regardless of PSN sign-in status. The first-run trophy setup dialog (`SCE_SYSMODULE_INTERNAL_TROPHY_SETUP_DIALOG`) appears automatically on first launch.

**The `.trp` must be authored once, with one entry per achievement KeeperFX defines, in the same order as the integer IDs in `s_trophy_map` above.**

---

## Implementation checklist (when ready to implement)

- [ ] Rebase/merge `feature/achievements-and-gog-integration` onto `renderer-abstraction`
- [ ] Author `TROPHY.TRP` with trophygenesis, matching defined achievement IDs
- [ ] Create `src/platform/platform_vita_trophy.c` implementing `AchievementBackend`
- [ ] Add `#ifdef PLATFORM_VITA` detection to `achievements_detect_platform()`
- [ ] Register Vita backend in `PlatformHomebrewMain.cpp` via `achievements_register_backend()`
- [ ] Add `libSceNpTrophy_stub.a` to `CMakeLists.txt` Vita link libs
- [ ] Add `SCE_SYSMODULE_NP_TROPHY` load call to `PlatformVita::SystemInit()`
- [ ] Package `sce_sys/trophy/TROPHY.TRP` in VPK CMake step

---

## Notes

- No header file for SceNpTrophy exists in the public vitaSDK headers (`psp2/trophy.h` is absent). The stub is present but the header must be written from the NID database or sourced from the scene. The function signatures are well-documented in community resources (vitadev wiki, TrophyHax source, etc.).
- The `_stub_weak.a` variant allows the module to be optional — if `sceNpTrophyInit` fails (e.g. the NP system is not available on this firmware), the backend falls back gracefully to `AchPlat_Local` without crashing.
