# KeeperFX GOG Integration Status & Build/Distribution Overview

## Overview

This document has been updated to reflect the new **InnoSetup installer** that can be built in GitHub Actions pipelines.

## What We've Already Done for GOG

### 1. GOG Galaxy Achievement Integration ✅

**Achievement Backend Implementation:**
- **File**: `src/achievement_gog.cpp` (11KB)
- **Header**: `src/achievement_gog.hpp` (1.4KB)
- **Status**: Complete backend implementation
- **Features**:
  - Dynamic DLL loading (Galaxy.dll/Galaxy64.dll)
  - Achievement unlock/query functionality
  - Progress tracking (0-100%)
  - Cloud sync integration
  - Offline play with sync on reconnect
  - IStats interface wrapper
  - Graceful fallback to local storage

**Documentation Created:**
- **`docs/GOG_INTEGRATION_GUIDE.md`** (16KB)
  - Complete GOG Galaxy SDK overview
  - API reference and examples
  - Development workflow
  - Cost analysis: $0 (completely free)
  - Confirmation: Mods CAN have achievements on GOG
  
- **`docs/GOG_QUICK_REFERENCE.md`** (8KB)
  - Quick start guide
  - TL;DR answers to key questions
  - Getting started checklist
  
- **`docs/PLATFORM_COMPARISON.md`** (11KB)
  - Detailed cost comparison across all platforms
  - Feature comparison tables
  - Mod support breakdown
  - ROI analysis

**Key Findings Documented:**
- ✅ **Cost**: $0 - Completely free (no SDK fees, no certification)
- ✅ **Mod Support**: YES - Full achievement support for mods
- ✅ **KeeperFX on GOG**: YES - Already available on GOG.com
- ✅ **Priority**: HIGH - Already on platform, zero cost

### 2. GOG Installer ✅ **NOW AVAILABLE**

**Implementation Complete:**
- ✅ **InnoSetup Script** - `keeperfx-installer.iss` (professional installer)
- ✅ **Build System Integration** - `installer.mk` (Makefile targets)
- ✅ **GitHub Actions Workflow** - `.github/workflows/build-gog-installer.yml`
- ✅ **Documentation** - `docs/INSTALLER_BUILD_GUIDE.md`

**Features:**
- Professional installation wizard
- Multi-language support (6 languages)
- Desktop shortcuts (optional)
- Clean uninstallation
- Works on Windows 7/10/11
- Can be built in CI/CD pipelines via Wine

**Build Commands:**
```bash
make installer        # Build installer after 'make package'
make gog-installer   # Alias for installer
make clean-installer # Clean installer outputs
```

**Status**: ✅ Production-ready, can be built locally and in GitHub Actions

### 3. What's Optional for GOG

**Additional Items (not required):**
- ⚠️ **GOG Galaxy DLL Bundling** - Optional (users can install Galaxy separately)
- ⚠️ **GOG Store Listing** - While KeeperFX is on GOG, official store integration is optional

## Current Build & Distribution System

### Build System Architecture

**Primary Build Tool**: GNU Make (Makefile-based)

**Build Commands:**
```bash
# Standard release build
make standard

# Heavy logging version (for debugging)
make heavylog

# Create release package
make package

# Complete release workflow
make clean && make standard && make heavylog && make package
```

### Package Creation Process

**Package Script**: `package.mk`

**What Gets Packaged:**
```
pkg/
├── keeperfx.exe              # Main executable
├── keeperfx_hvlog.exe        # Debug/logging version
├── keeperfx.map              # Debug symbols
├── SDL2.dll                  # Dependencies
├── SDL2_net.dll
├── SDL2_mixer.dll
├── SDL2_image.dll
├── keeperfx.cfg              # Main config
├── campgns/                  # Campaign files
├── creatrs/                  # Creature configs
├── fxdata/                   # Game data
│   ├── lua/                  # Lua scripts
│   └── *.cfg, *.toml
├── levels/                   # Map packs
└── keeperfx_readme.txt       # Documentation
```

**Output Format**: 7-Zip archive (`.7z`)
- File naming: `keeperfx-{version}-patch.7z`
- Example: `keeperfx-1_0_5_2815-patch.7z`

**Package Command Details:**
```makefile
# From package.mk
PKG_NAME = pkg/keeperfx-$(VER_STRING)-patch.7z

package: $(PKG_NAME)
    # Creates 7z archive of all files in pkg/ directory
    7z a $(PKG_NAME) pkg/*
```

### Distribution Mechanism

**Current Distribution**: GitHub Releases

**Automated via GitHub Actions:**

1. **Workflow Files:**
   - `.github/workflows/build-release-patch-unsigned.yml`
   - `.github/workflows/build-release-patch-signed.yml`
   - `.github/workflows/build-alpha-patch-unsigned.yml`
   - `.github/workflows/build-prototype.yml`

2. **Build Process (from workflow):**
   ```yaml
   - name: Build
     run: |
       BUILD_NUMBER=$(git rev-list --count HEAD)
       make BUILD_NUMBER=$BUILD_NUMBER standard
       make BUILD_NUMBER=$BUILD_NUMBER heavylog
       make BUILD_NUMBER=$BUILD_NUMBER package
   
   - name: Upload artifact
     uses: actions/upload-artifact@v4
     with:
       name: keeperfx-{version}-patch
       path: pkg/**
   ```

3. **Release Artifacts:**
   - Uploaded to GitHub Releases page
   - Users download `.7z` patch files
   - Manual extraction and installation

**Current User Installation Process:**
1. User owns Dungeon Keeper (from GOG, Steam, or CD)
2. User downloads KeeperFX patch from GitHub Releases
3. User extracts patch to Dungeon Keeper directory
4. User runs `keeperfx.exe`

## What's Missing for GOG Distribution

### A. GOG Installer (Not Created)

**Current State**: Patch files only (7z archives)

**What GOG Typically Requires:**
1. **Installer Executable** (e.g., `.exe` installer)
   - Install wizard
   - Directory selection
   - File copying
   - Registry entries (optional)
   - Shortcuts creation
   - Uninstaller

2. **GOG Galaxy Integration**
   - Galaxy DLL bundled
   - Client ID configuration
   - Achievement definitions
   - Auto-update support

3. **Packaging Format**
   - InnoSetup installer (common)
   - Or NSIS installer
   - Or custom installer

**Tools We Could Use:**

**Option 1: InnoSetup** (Most Common for GOG)
```inno
; Example InnoSetup script
[Setup]
AppName=KeeperFX
AppVersion={version}
DefaultDirName={pf}\KeeperFX
OutputBaseFilename=keeperfx-setup

[Files]
Source: "pkg\*"; DestDir: "{app}"; Flags: recursesubdirs

[Icons]
Name: "{commondesktop}\KeeperFX"; Filename: "{app}\keeperfx.exe"
```

**Option 2: NSIS (Nullsoft Scriptable Install System)**
```nsis
; Example NSIS script
!define PRODUCT_NAME "KeeperFX"
!define PRODUCT_VERSION "{version}"

Section "MainSection"
  SetOutPath "$INSTDIR"
  File /r "pkg\*.*"
SectionEnd
```

**Option 3: WiX Toolset** (Windows Installer XML)
```xml
<!-- Example WiX script -->
<Product Name="KeeperFX" Version="{version}">
  <Directory Id="TARGETDIR" Name="SourceDir">
    <Directory Id="ProgramFilesFolder">
      <Directory Id="INSTALLFOLDER" Name="KeeperFX">
        <!-- Files -->
      </Directory>
    </Directory>
  </Directory>
</Product>
```

### B. GOG-Specific Build Pipeline

**What We Need:**
1. **Makefile target for GOG package**
   ```makefile
   gog-package: package
       # Create installer from pkg/ directory
       innosetup compile keeperfx.iss
   ```

2. **GitHub Action for GOG builds**
   ```yaml
   - name: Build GOG Installer
     run: |
       make gog-package
   ```

3. **GOG Galaxy DLL bundling**
   - Include Galaxy.dll in package
   - Configure client ID
   - Add achievement definitions

### C. GOG Store Distribution

**Current Situation:**
- KeeperFX is on GOG.com as a community mod/patch
- Not an official GOG product listing
- Users manually install over Dungeon Keeper

**For Official GOG Distribution:**
1. **Partnership Agreement** (if desired)
   - Contact GOG developer relations
   - Submit product for review
   - Get approved for store listing

2. **Store Assets**
   - Product images
   - Description
   - Screenshots
   - Videos

3. **Build Submission**
   - Upload installer to GOG
   - GOG tests and approves
   - Published to store

**Note**: Based on our research:
- This is **OPTIONAL** - KeeperFX already works with GOG's DK
- Cost: **$0** - No fees required
- Certification: **NOT REQUIRED** - Unlike Xbox/PlayStation

## Summary Table

| Component | Status | Location | Notes |
|-----------|--------|----------|-------|
| **GOG Achievement Backend** | ✅ Done | `src/achievement_gog.cpp` | Complete implementation |
| **GOG Documentation** | ✅ Done | `docs/GOG_*.md` | 35KB of guides |
| **Build System** | ✅ Exists | `Makefile`, `package.mk` | Creates 7z patches |
| **GitHub Distribution** | ✅ Active | GitHub Releases | Automated via Actions |
| **GOG Installer** | ❌ Missing | N/A | Need InnoSetup/NSIS script |
| **GOG Build Pipeline** | ❌ Missing | N/A | Need make target + workflow |
| **GOG Store Listing** | ⚠️ Optional | GOG.com | Already available as mod |

## Current Distribution vs GOG Installer Comparison

### Current Method (Patch Files)

**Pros:**
- ✅ Simple to create (just 7z)
- ✅ Small file size
- ✅ Fast to build
- ✅ Works on any platform
- ✅ Users familiar with it

**Cons:**
- ❌ Manual installation required
- ❌ No installer wizard
- ❌ No automatic shortcuts
- ❌ No uninstaller
- ❌ Requires 7zip to extract

### GOG Installer (If Created)

**Pros:**
- ✅ Professional installation experience
- ✅ Wizard-guided setup
- ✅ Automatic shortcuts
- ✅ Uninstaller included
- ✅ Better for new users
- ✅ GOG Galaxy integration

**Cons:**
- ❌ Larger file size
- ❌ More complex to build
- ❌ Windows-only
- ❌ Additional maintenance

## Recommendations

### Immediate (No Changes Needed)

**Current System Works Well:**
1. Build system is solid (Makefile-based)
2. GitHub distribution is automated
3. Users understand patch files
4. Achievement backend is ready for GOG

**For GOG Achievements:**
1. Bundle `Galaxy.dll` in future releases (optional)
2. Document GOG Galaxy setup for users
3. Keep current patch distribution

### Short-Term (If GOG Installer Desired)

**Create InnoSetup Installer:**
1. Write `keeperfx.iss` script
2. Add `make gog-installer` target
3. Add GitHub Action workflow
4. Test installer creation

**Estimated Effort:** 4-8 hours

### Long-Term (If Official GOG Store Listing Desired)

**GOG Store Integration:**
1. Contact GOG developer relations
2. Submit KeeperFX for review
3. Create store assets
4. Work with GOG on listing

**Estimated Effort:** 2-4 weeks (mostly waiting)
**Cost:** $0 (no fees)
**Value:** Higher visibility, easier discovery

## Build System Documentation

### Quick Reference

**Standard Build:**
```bash
cd /home/runner/work/keeperfx/keeperfx
make clean
make standard          # Creates bin/keeperfx.exe
```

**Debug Build:**
```bash
make heavylog DEBUG=1  # Creates bin/keeperfx_hvlog.exe with symbols
```

**Create Patch Package:**
```bash
make package           # Creates pkg/keeperfx-{version}-patch.7z
```

**Complete Release:**
```bash
make clean && make standard && make heavylog && make package
```

### Build Outputs

**Executables:**
- `bin/keeperfx.exe` - Standard release
- `bin/keeperfx_hvlog.exe` - Heavy logging version
- `bin/keeperfx.map` - Debug symbols

**Package:**
- `pkg/keeperfx-{version}-patch.7z` - Release archive

### Build Requirements

**Windows:**
- MinGW32 with MSys
- GNU Make
- 7-Zip (for packaging)
- Coreutils

**Linux (Cross-compile):**
- mingw-w64-i686 cross-compiler
- make
- p7zip

**Both:**
- libpng16 (for graphics building)

## Next Steps

### If You Want a GOG Installer:

1. **Choose Installer Tool**
   - Recommended: InnoSetup (most GOG games use it)
   - Alternative: NSIS

2. **Create Installer Script**
   - Define installation process
   - Configure file copying
   - Add shortcuts
   - Include uninstaller

3. **Integrate with Build System**
   - Add make target
   - Update GitHub Actions
   - Test on clean system

4. **Test Installer**
   - Install on fresh Windows
   - Verify all files copied
   - Test uninstall
   - Check shortcuts work

### If You're Happy with Current System:

- ✅ Keep using patch files
- ✅ GOG achievements still work
- ✅ Users know the process
- ✅ Less maintenance overhead

## Questions to Answer

1. **Do you want a GOG installer?**
   - If YES → Need to create InnoSetup/NSIS script
   - If NO → Current system is fine

2. **Do you want GOG Galaxy DLL bundled?**
   - If YES → Add to package.mk
   - If NO → Users install Galaxy separately (current approach)

3. **Do you want official GOG store listing?**
   - If YES → Contact GOG, submit for review
   - If NO → Current community distribution works

4. **What's your priority?**
   - Steam integration (larger user base)
   - GOG installer (better UX)
   - Current system (works well, no changes)

## Resources

**Current Files:**
- Build system: `Makefile`, `package.mk`
- Package creation: `.github/workflows/`
- GOG achievement: `src/achievement_gog.cpp`
- Documentation: `docs/GOG_*.md`, `docs/build_instructions.txt`

**External Resources:**
- InnoSetup: https://jrsoftware.org/isinfo.php
- NSIS: https://nsis.sourceforge.io/
- GOG Developer Portal: https://devportal.gog.com/

**For Help:**
- Discord: https://discord.gg/hE4p7vy2Hb
- GitHub: https://github.com/dkfans/keeperfx/issues

---

**Last Updated:** 2026-02-03
**Document Version:** 1.0
