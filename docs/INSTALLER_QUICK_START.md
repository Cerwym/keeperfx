# KeeperFX Installer - Quick Start Guide

## What Is This?

KeeperFX now has a professional Windows installer built with InnoSetup that can be automatically created in GitHub Actions pipelines.

## Quick Facts

✅ **Free**: Uses InnoSetup (no cost)
✅ **Automated**: Builds in GitHub Actions via Wine
✅ **Professional**: Installation wizard, shortcuts, uninstaller
✅ **Multi-language**: English, French, German, Spanish, Polish, Russian
✅ **No Admin Required**: Installs in user directory

## Build Commands

```bash
# Build installer (after building game and package)
make installer

# Complete build from scratch
make clean && make standard && make heavylog && make package && make installer

# Clean installer outputs
make clean-installer
```

## Output

The installer will be created at:
```
installer/keeperfx-{version}-setup.exe
```

Example: `installer/keeperfx-1.3.1.2900-setup.exe` (~40-50 MB)

## GitHub Actions

**Automatic Builds:**
- Triggers on push to `main` or `develop`
- Triggers on changes to installer files
- Can be manually triggered

**Download Artifacts:**
1. Go to Actions tab in GitHub
2. Click on "Build GOG Installer" workflow
3. Download the installer artifact

## Requirements

**Windows:**
- InnoSetup 6 (download from https://jrsoftware.org/isinfo.php)

**Linux/CI:**
- Wine (automatically installed by GitHub Actions)
- InnoSetup via Wine (automatically installed by GitHub Actions)

**Build Dependencies:**
- Standard KeeperFX build tools (MinGW, Make)
- 7zip (for package creation)
- Game must be built first (`make standard && make heavylog`)

## How It Works

1. **You build the game:**
   ```bash
   make standard
   make heavylog
   ```

2. **You create the package:**
   ```bash
   make package
   ```
   This populates the `pkg/` directory with all game files.

3. **You build the installer:**
   ```bash
   make installer
   ```
   InnoSetup compiles the installer from `pkg/` directory.

4. **Result:**
   ```
   installer/keeperfx-{version}-setup.exe
   ```

## What Gets Installed

The installer includes:
- Main executable (`keeperfx.exe`)
- Debug version (`keeperfx_hvlog.exe`)
- SDL2 libraries (DLLs)
- Configuration files
- Campaigns and maps
- Creature configs
- Game data (fxdata)
- Documentation

## Installation Experience

**User sees:**
1. Welcome screen
2. License agreement
3. Installation directory selection
4. Optional desktop shortcut
5. Progress bar
6. Completion with option to launch game

**User gets:**
- Start menu shortcuts
- Desktop shortcut (if selected)
- Uninstaller entry

## Comparison: Installer vs 7z Patch

| Feature | Installer | 7z Patch |
|---------|-----------|----------|
| Size | ~40-50 MB | ~15-20 MB |
| Setup | Wizard | Manual extract |
| Shortcuts | Automatic | Manual |
| Uninstall | One-click | Manual delete |
| User-friendly | ✅ Yes | ⚠️ Advanced users |
| Build time | ~2 min | ~30 sec |
| Platform | Windows only | Any OS |

**Recommendation:** Provide both! Let users choose.

## Customization

Edit `keeperfx-installer.iss` to customize:

**Common changes:**
```pascal
; Change app name
#define MyAppName "KeeperFX Enhanced Edition"

; Change default install directory
DefaultDirName={autopf}\MyCustomPath

; Add/remove languages
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"

; Change icon
SetupIconFile=res\custom_icon.ico
```

## Troubleshooting

**Problem:** `InnoSetup not found`
- **Windows:** Install InnoSetup from https://jrsoftware.org/isinfo.php
- **Linux:** The GitHub Actions workflow installs it automatically

**Problem:** `pkg/ directory not found`
- **Solution:** Run `make package` first to create the staging directory

**Problem:** Version shows as "1.3.1.0"
- **Solution:** Version is read from `version.mk` and `BUILD_NUMBER`
- Set explicitly: `export KEEPERFX_VERSION=1.3.1.2900 && make installer`

**Problem:** Wine crashes on Linux
- **Solution:** Reinitialize Wine: `rm -rf ~/.wine && winecfg`

**Problem:** Files missing from installer
- **Solution:** Check that all files exist in `pkg/` before building installer
- Verify paths in `keeperfx-installer.iss` match your `pkg/` structure

## GitHub Actions Workflow

**File:** `.github/workflows/build-gog-installer.yml`

**What it does:**
1. ✅ Checks out code
2. ✅ Installs build tools (MinGW, libpng, 7zip)
3. ✅ Installs Wine
4. ✅ Installs InnoSetup via Wine
5. ✅ Builds game graphics
6. ✅ Builds executables (standard + heavylog)
7. ✅ Creates package (pkg/ directory)
8. ✅ Compiles installer with InnoSetup
9. ✅ Uploads installer as artifact

**Artifact retention:** 90 days

**Trigger manually:**
1. Go to Actions tab
2. Select "Build GOG Installer" workflow
3. Click "Run workflow"
4. Select branch
5. Click "Run workflow" button

## Testing Checklist

Before releasing an installer:

**Build Test:**
- [ ] Installer compiles without errors
- [ ] File size is reasonable (~40-50 MB)
- [ ] Version number is correct in filename

**Installation Test:**
- [ ] Test on clean Windows VM
- [ ] Install to default location
- [ ] Install to custom location
- [ ] Desktop shortcut works
- [ ] Start menu shortcuts work
- [ ] Game launches successfully
- [ ] Debug version launches

**Uninstall Test:**
- [ ] Uninstaller runs without errors
- [ ] All files removed
- [ ] Shortcuts removed
- [ ] No registry leftovers

**Compatibility Test:**
- [ ] Windows 7
- [ ] Windows 10
- [ ] Windows 11

## Distribution Options

**1. GitHub Releases** (Recommended)
- Upload installer alongside 7z patches
- Tag with version number
- Write release notes

**2. Direct Download**
- Host on keeperfx.net
- Provide both installer and 7z patch

**3. GOG Store** (Optional)
- Submit installer to GOG for approval
- Professional distribution channel
- No cost to submit

## Support

**Documentation:**
- Complete guide: `docs/INSTALLER_BUILD_GUIDE.md`
- Build system: `docs/BUILD_AND_DISTRIBUTION.md`

**Help:**
- GitHub Issues: https://github.com/dkfans/keeperfx/issues
- Discord: https://discord.gg/hE4p7vy2Hb

**InnoSetup:**
- Documentation: https://jrsoftware.org/ishelp/
- Examples: Check InnoSetup installation directory

## Key Files

```
keeperfx/
├── keeperfx-installer.iss          # InnoSetup script (edit this)
├── installer.mk                    # Makefile rules (build logic)
├── installer/                      # Output directory (auto-created)
│   └── keeperfx-*.exe             # Generated installer
├── docs/
│   ├── INSTALLER_BUILD_GUIDE.md   # Complete documentation
│   └── BUILD_AND_DISTRIBUTION.md  # Build system overview
└── .github/workflows/
    └── build-gog-installer.yml    # CI/CD automation
```

## Summary

**In 3 Commands:**
```bash
make standard && make heavylog && make package  # Build game
make installer                                   # Build installer
```

**Result:**
```
installer/keeperfx-{version}-setup.exe
```

**Use Case:**
- Easier for new users (wizard-guided)
- Professional appearance
- GOG-ready distribution
- Still have 7z patches for advanced users

---

**Status:** ✅ Ready to use
**Last Updated:** 2026-02-03
**Version:** 1.0
