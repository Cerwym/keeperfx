# KeeperFX Build & Distribution - Visual Overview

## Current Build Pipeline

```
┌─────────────────────────────────────────────────────────────────┐
│                      SOURCE CODE                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐      │
│  │   src/   │  │ config/  │  │campgns/  │  │ levels/  │      │
│  │  *.c *.h │  │  *.cfg   │  │  *.cfg   │  │  *.cfg   │      │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘      │
└─────────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────────┐
│                       BUILD PROCESS                              │
│                                                                  │
│  make clean          → Remove old build files                   │
│  make standard       → Compile bin/keeperfx.exe                 │
│  make heavylog       → Compile bin/keeperfx_hvlog.exe          │
│  make package        → Create 7z archive                       │
│                                                                  │
│  Tools: MinGW32, GCC, 7-Zip                                    │
└─────────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────────┐
│                    PACKAGE STRUCTURE                             │
│                                                                  │
│  pkg/                                                           │
│  ├── keeperfx.exe           ← Main executable                  │
│  ├── keeperfx_hvlog.exe     ← Debug version                    │
│  ├── keeperfx.map           ← Debug symbols                    │
│  ├── *.dll                  ← SDL dependencies                 │
│  ├── keeperfx.cfg           ← Configuration                    │
│  ├── campgns/               ← Campaign data                    │
│  ├── creatrs/               ← Creature configs                 │
│  ├── fxdata/                ← Game data                        │
│  └── levels/                ← Maps                             │
│                                                                  │
│  Output: keeperfx-{version}-patch.7z                           │
└─────────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────────┐
│                  GITHUB ACTIONS (Automated)                      │
│                                                                  │
│  Triggers: Push to main, PR, Manual dispatch                   │
│                                                                  │
│  Workflow Steps:                                               │
│  1. Checkout code                                              │
│  2. Setup build environment                                    │
│  3. Run: make standard                                         │
│  4. Run: make heavylog                                         │
│  5. Run: make package                                          │
│  6. Upload artifact to GitHub                                  │
│                                                                  │
│  Workflows:                                                    │
│  - build-release-patch-unsigned.yml                            │
│  - build-release-patch-signed.yml                              │
│  - build-alpha-patch-unsigned.yml                              │
└─────────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────────┐
│                  DISTRIBUTION (Current)                          │
│                                                                  │
│  GitHub Releases                                               │
│  ├── keeperfx-{version}-patch.7z                               │
│  ├── keeperfx-{version}-alpha.7z                               │
│  └── Release notes, changelog                                  │
│                                                                  │
│  Users Download:                                               │
│  1. Go to github.com/dkfans/keeperfx/releases                 │
│  2. Download .7z file                                          │
│  3. Extract to Dungeon Keeper directory                       │
│  4. Run keeperfx.exe                                           │
└─────────────────────────────────────────────────────────────────┘
```

## GOG Integration Status

```
┌─────────────────────────────────────────────────────────────────┐
│                   GOG ACHIEVEMENT SYSTEM                         │
│                        ✅ COMPLETE                              │
│                                                                  │
│  Implementation:                                               │
│  ├── src/achievement_gog.cpp     (11KB) ✅                     │
│  ├── src/achievement_gog.hpp     (1.4KB) ✅                    │
│  └── Galaxy.dll loading          ✅                             │
│                                                                  │
│  Documentation:                                                │
│  ├── docs/GOG_INTEGRATION_GUIDE.md   (16KB) ✅                │
│  ├── docs/GOG_QUICK_REFERENCE.md     (8KB) ✅                 │
│  └── docs/PLATFORM_COMPARISON.md     (11KB) ✅                │
│                                                                  │
│  Features:                                                     │
│  ✅ Achievement unlock/query                                   │
│  ✅ Progress tracking (0-100%)                                 │
│  ✅ Cloud sync                                                 │
│  ✅ Offline mode support                                       │
│  ✅ Graceful fallback to local storage                         │
│                                                                  │
│  Cost: $0 (completely free)                                    │
│  Mod Support: Full support                                     │
│  KeeperFX on GOG: Yes (already available)                      │
└─────────────────────────────────────────────────────────────────┘
```

## What's Missing for GOG Distribution

```
┌─────────────────────────────────────────────────────────────────┐
│                   GOG INSTALLER                                  │
│                     ❌ NOT CREATED                              │
│                                                                  │
│  What We Have:                                                 │
│  ✅ 7z patch files                                             │
│  ✅ Manual extraction process                                  │
│  ✅ Works on GOG's Dungeon Keeper                              │
│                                                                  │
│  What's Missing:                                               │
│  ❌ Installer executable (.exe)                                │
│  ❌ Installation wizard                                        │
│  ❌ Automatic shortcuts                                        │
│  ❌ Uninstaller                                                │
│  ❌ GOG Galaxy DLL bundled                                     │
│                                                                  │
│  To Create Installer:                                          │
│  Option 1: InnoSetup (recommended)                             │
│  Option 2: NSIS                                                │
│  Option 3: WiX Toolset                                         │
│                                                                  │
│  Estimated Effort: 4-8 hours                                   │
└─────────────────────────────────────────────────────────────────┘
```

## Proposed GOG Installer Pipeline

```
┌─────────────────────────────────────────────────────────────────┐
│                  FUTURE: GOG INSTALLER BUILD                     │
│                                                                  │
│  Current Package (7z)                                          │
│        ↓                                                        │
│  pkg/ directory                                                │
│        ↓                                                        │
│  InnoSetup Script (keeperfx.iss)                               │
│        ↓                                                        │
│  InnoSetup Compiler                                            │
│        ↓                                                        │
│  keeperfx-setup.exe                                            │
│        ↓                                                        │
│  Distribution Options:                                         │
│  ├── GitHub Releases (alongside 7z)                            │
│  ├── Direct download (keeperfx.net)                            │
│  └── GOG Store (if approved)                                   │
│                                                                  │
│  Makefile Target:                                              │
│  make gog-installer                                            │
│        ↓                                                        │
│  Creates: pkg/keeperfx-{version}-setup.exe                     │
└─────────────────────────────────────────────────────────────────┘
```

## Distribution Comparison

```
┌──────────────────┬──────────────────┬──────────────────┐
│   Current (7z)   │  GOG Installer   │  GOG Store       │
├──────────────────┼──────────────────┼──────────────────┤
│ ✅ Exists        │ ❌ Not created   │ ⚠️ Optional      │
│ ✅ Automated     │ ❌ Need script   │ ❌ Need approval │
│ ✅ GitHub        │ ✅ Would work    │ ✅ Official      │
│ ✅ Free          │ ✅ Free          │ ✅ Free          │
│ ❌ Manual setup  │ ✅ Wizard        │ ✅ Auto-install  │
│ ✅ Small size    │ ⚠️ Larger        │ ✅ Managed       │
│ ✅ Fast build    │ ⚠️ Slower        │ ❌ Review time   │
│ ✅ Familiar      │ ✅ Professional  │ ✅ Discoverable  │
└──────────────────┴──────────────────┴──────────────────┘
```

## File Size Comparison

```
Current Package:
├── keeperfx.exe              ~8 MB
├── keeperfx_hvlog.exe        ~12 MB
├── Dependencies (DLLs)       ~5 MB
├── Data files                ~2 MB
└── Total (compressed 7z)     ~15-20 MB

With GOG Installer:
├── All above files           ~27 MB (uncompressed)
├── Galaxy.dll (optional)     ~10 MB
├── Installer overhead        ~2 MB
└── Total (installer .exe)    ~40-50 MB
```

## Key Decisions Needed

```
┌─────────────────────────────────────────────────────────────────┐
│                    DECISION POINTS                               │
│                                                                  │
│  1. Create GOG Installer?                                       │
│     ☐ Yes → Need InnoSetup/NSIS script                         │
│     ☐ No → Continue with 7z patches                            │
│                                                                  │
│  2. Bundle GOG Galaxy DLL?                                      │
│     ☐ Yes → Larger file, works immediately                     │
│     ☐ No → Users install Galaxy separately                     │
│                                                                  │
│  3. Pursue GOG Store Listing?                                   │
│     ☐ Yes → Contact GOG, submit for review                     │
│     ☐ No → Continue community distribution                     │
│                                                                  │
│  4. Priority Order?                                             │
│     ☐ 1st: GOG installer (better UX)                           │
│     ☐ 1st: Steam integration (larger user base)                │
│     ☐ 1st: Current system (works, no changes)                  │
└─────────────────────────────────────────────────────────────────┘
```

## Quick Facts

```
┌─────────────────────────────────────────────────────────────────┐
│                      SUMMARY                                     │
├─────────────────────────────────────────────────────────────────┤
│ Build System:        Makefile (GNU Make)                        │
│ Package Format:      7z archive                                 │
│ Distribution:        GitHub Releases                            │
│ Automation:          GitHub Actions                             │
│ GOG Achievements:    ✅ Complete                                │
│ GOG Installer:       ❌ Not created                             │
│ GOG Store:           ⚠️ Optional (community mod exists)         │
│ Cost (everything):   $0                                         │
│ Build Time:          ~10 minutes                                │
│ Package Size:        ~15-20 MB (7z)                             │
│ Platforms:           Windows (cross-compile from Linux)         │
└─────────────────────────────────────────────────────────────────┘
```

## Next Steps

```
If you want a GOG installer:
┌────────────────────────────────────────┐
│ 1. Choose installer tool               │
│    → InnoSetup (recommended)           │
│                                        │
│ 2. Write installer script              │
│    → keeperfx.iss (InnoSetup)          │
│                                        │
│ 3. Add to build system                 │
│    → make gog-installer target         │
│                                        │
│ 4. Test on clean Windows               │
│    → Verify installation works         │
│                                        │
│ 5. Add to GitHub Actions               │
│    → Automate installer creation       │
│                                        │
│ Estimated time: 4-8 hours              │
└────────────────────────────────────────┘

If current system is fine:
┌────────────────────────────────────────┐
│ ✅ Keep using 7z patches               │
│ ✅ GOG achievements work               │
│ ✅ Users know the process              │
│ ✅ Less maintenance                    │
│ ✅ No changes needed                   │
└────────────────────────────────────────┘
```

---

**For More Details:**
- Build System: `docs/build_instructions.txt`
- GOG Integration: `docs/GOG_INTEGRATION_GUIDE.md`
- Complete Overview: `docs/BUILD_AND_DISTRIBUTION.md`
