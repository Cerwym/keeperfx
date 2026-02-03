# KeeperFX Installer - Build Instructions

## Overview

KeeperFX now includes an InnoSetup-based installer that provides a professional installation experience for users. The installer can be built locally or automatically via GitHub Actions.

## What's Included

The installer includes:
- Main executable (`keeperfx.exe`)
- Debug/logging version (`keeperfx_hvlog.exe`)
- SDL2 libraries (SDL2.dll, SDL2_net.dll, SDL2_mixer.dll, SDL2_image.dll)
- Configuration files
- Campaigns, creatures, game data
- Map packs and levels
- Documentation

## Building the Installer

### Prerequisites

**Windows:**
- [InnoSetup 6](https://jrsoftware.org/isinfo.php) (free)
- Standard KeeperFX build tools (MinGW, Make)

**Linux/macOS:**
- Wine (to run InnoSetup)
- InnoSetup installed via Wine
- Standard KeeperFX build tools

### Quick Build Instructions

**1. Build the game first:**
```bash
make clean
make standard
make heavylog
make package
```

**2. Build the installer:**
```bash
make installer
```

The installer will be created in `installer/keeperfx-{version}-setup.exe`

### Manual Build (if needed)

If you need to build manually with custom settings:

```bash
# Set version (optional, auto-detected from version.mk)
export KEEPERFX_VERSION=1.3.1.2900

# Compile with InnoSetup
iscc keeperfx-installer.iss

# On Linux with Wine:
wine "~/.wine/drive_c/Program Files (x86)/Inno Setup 6/ISCC.exe" keeperfx-installer.iss
```

## GitHub Actions / CI/CD

The installer is automatically built by GitHub Actions when code changes are pushed. The workflow:

1. Builds the game executables
2. Creates the package (populates `pkg/` directory)
3. Installs Wine and InnoSetup (on Linux runner)
4. Compiles the installer
5. Uploads the installer as an artifact

**Workflow file:** `.github/workflows/build-gog-installer.yml`

### Triggering the Build

The installer workflow runs automatically on:
- Pushes to `main` or `develop` branches
- Changes to source code, configs, or installer files
- Manual workflow dispatch

### Downloading Artifacts

After the workflow completes:
1. Go to the Actions tab in GitHub
2. Click on the completed workflow run
3. Download the installer artifact (e.g., `keeperfx-1.3.1.2900-setup`)

## Installing InnoSetup on Linux (for CI/CD)

The GitHub Actions workflow handles this automatically, but for local Linux development:

```bash
# Install Wine
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install -y wine

# Download InnoSetup
wget -O innosetup.exe "https://jrsoftware.org/download.php/is.exe"

# Install via Wine (silent mode)
wine innosetup.exe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP-

# Wait for installation to complete
sleep 10

# Verify installation
ls "$HOME/.wine/drive_c/Program Files (x86)/Inno Setup 6/"
```

## Customizing the Installer

Edit `keeperfx-installer.iss` to customize:

- App name, version, publisher
- Installation directory
- Icons and shortcuts
- Files to include/exclude
- Installation wizard appearance
- Languages supported

Key sections:
- `[Setup]` - Basic installer configuration
- `[Files]` - Which files to include
- `[Icons]` - Desktop/menu shortcuts
- `[Languages]` - Supported languages
- `[Tasks]` - Optional installation tasks

## Installer Features

✅ **Professional Installation Wizard**
- Modern UI
- License agreement display
- Directory selection
- Component selection
- Progress tracking

✅ **Multi-Language Support**
- English, French, German, Spanish, Polish, Russian
- Easily extensible to more languages

✅ **Optional Components**
- Desktop shortcut (optional)
- Quick Launch icon (optional)
- Debug logging version included

✅ **Clean Uninstallation**
- Complete removal of installed files
- Registry cleanup
- Uninstall shortcut in Start Menu

✅ **User-Friendly**
- No administrator rights required (installs in user directory)
- Works on Windows 7/10/11
- Both 32-bit and 64-bit compatible

## Makefile Targets

```bash
# Build the installer (requires pkg/ directory from 'make package')
make installer

# Alias for GOG-specific installer
make gog-installer

# Clean installer outputs
make clean-installer

# Complete release build (executables + package + installer)
make clean && make standard && make heavylog && make package && make installer
```

## Troubleshooting

### InnoSetup Not Found

**Error:** `Error: InnoSetup not found`

**Solution:**
- Windows: Install InnoSetup from https://jrsoftware.org/isinfo.php
- Linux: Install Wine and InnoSetup as described above
- CI/CD: The workflow handles this automatically

### Wine Issues on Linux

**Error:** Wine crashes or hangs

**Solution:**
```bash
# Reinitialize Wine
rm -rf ~/.wine
wine --version
winecfg  # Let Wine initialize
```

### Version Not Set

**Error:** Version appears as "1.3.1.0" (default)

**Solution:**
The version is automatically read from `version.mk`. To override:
```bash
export KEEPERFX_VERSION=1.3.1.2900
make installer
```

Or let the build system calculate it:
```bash
BUILD_NUMBER=$(git rev-list --count HEAD)
make BUILD_NUMBER=$BUILD_NUMBER installer
```

### Package Not Built

**Error:** Files missing from `pkg/` directory

**Solution:**
Build the package first:
```bash
make clean
make standard
make heavylog
make package
make installer
```

## File Locations

```
keeperfx/
├── keeperfx-installer.iss       # InnoSetup script
├── installer.mk                 # Makefile rules for installer
├── installer/                   # Output directory (auto-created)
│   └── keeperfx-*.exe          # Generated installer
├── pkg/                         # Staging directory (from 'make package')
│   ├── keeperfx.exe
│   ├── campgns/
│   ├── fxdata/
│   └── ...
└── .github/workflows/
    └── build-gog-installer.yml  # CI/CD workflow
```

## Testing the Installer

**Before Release:**
1. Build the installer
2. Test on clean Windows VM
3. Install to default location
4. Verify all files copied correctly
5. Test shortcuts work
6. Run the game
7. Test uninstaller
8. Verify clean removal

**Automated Testing:**
The GitHub Actions workflow validates:
- Installer compiles successfully
- No missing files
- Correct version in filename
- Installer size is reasonable (~40-50 MB)

## Distribution

**GitHub Releases:**
- Upload the installer alongside 7z patches
- Users can choose installer (easier) or patch (smaller)
- Both methods work equally well

**GOG Store (if pursuing):**
- The installer is ready for GOG submission
- Meets GOG's requirements for professional distribution
- Includes uninstaller as required

**Direct Download:**
- Host on keeperfx.net or similar
- Provide alongside traditional patch files

## Advantages Over 7z Patches

**Installer Benefits:**
✅ No extraction needed
✅ Wizard-guided setup
✅ Automatic shortcuts
✅ Professional appearance
✅ Easy uninstallation
✅ Better for new users

**7z Patch Benefits:**
✅ Smaller file size (~15-20 MB vs ~40-50 MB)
✅ Faster to build
✅ Familiar to existing users
✅ Works on any OS (with 7zip)

**Recommendation:** Provide both options for users to choose.

## Support

For issues with the installer:
- GitHub Issues: https://github.com/dkfans/keeperfx/issues
- Discord: https://discord.gg/hE4p7vy2Hb

For InnoSetup-specific help:
- InnoSetup Documentation: https://jrsoftware.org/ishelp/
- InnoSetup Forum: https://groups.google.com/g/innosetup

---

**Last Updated:** 2026-02-03
**Version:** 1.0
**Status:** Production-ready
