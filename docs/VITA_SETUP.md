# PlayStation Vita Development Setup for KeeperFX

This guide provides instructions for setting up a development environment to build KeeperFX for PlayStation Vita homebrew.

## Prerequisites

- A computer running Windows, macOS, or Linux
- Visual Studio Code (recommended)
- Git
- CMake 3.20 or higher

## Installing VitaSDK

VitaSDK is the official SDK for PlayStation Vita homebrew development.

### Windows

You can use either MSYS2 or WSL (Windows Subsystem for Linux) to build KeeperFX for Vita on Windows.

#### Option 1: Windows with MSYS2

1. Download and install [MSYS2](https://www.msys2.org/)
2. Open MSYS2 MinGW 64-bit terminal
3. Install dependencies:
   ```bash
   pacman -S make git wget p7zip tar cmake
   ```
4. Clone and setup VitaSDK:
   ```bash
   git clone https://github.com/vitasdk/vdpm
   cd vdpm
   ./bootstrap-vitasdk.sh
   export VITASDK=/usr/local/vitasdk
   export PATH=$VITASDK/bin:$PATH
   ./install-all.sh
   ```
5. Add VitaSDK to your PATH permanently:
   ```bash
   echo 'export VITASDK=/usr/local/vitasdk' >> ~/.bashrc
   echo 'export PATH=$VITASDK/bin:$PATH' >> ~/.bashrc
   source ~/.bashrc
   ```

#### Option 2: Windows with WSL (Bash on Ubuntu on Windows)

WSL is the recommended approach for Windows 10/11 users.

1. Enable WSL by following [Microsoft's guide](https://docs.microsoft.com/en-us/windows/wsl/install)
2. Install Ubuntu from the Microsoft Store
3. Open Ubuntu terminal and follow the Linux instructions below

**Note:** WSL provides a native Linux environment, so all Linux commands work identically.

### Linux

1. Install dependencies:
   ```bash
   # Ubuntu/Debian
   sudo apt-get update
   sudo apt-get install cmake git make pkg-config libtool wget
   
   # Arch Linux
   sudo pacman -S cmake git make pkg-config libtool wget
   ```

2. Clone and setup VitaSDK with vdpm:
   ```bash
   git clone https://github.com/vitasdk/vdpm
   cd vdpm
   ./bootstrap-vitasdk.sh
   export VITASDK=/usr/local/vitasdk
   export PATH=$VITASDK/bin:$PATH
   ```

3. Install all packages (optional, for full development):
   ```bash
   ./install-all.sh
   ```

4. Add VitaSDK to your PATH permanently:
   ```bash
   echo 'export VITASDK=/usr/local/vitasdk' >> ~/.bashrc
   echo 'export PATH=$VITASDK/bin:$PATH' >> ~/.bashrc
   source ~/.bashrc
   ```

### macOS

1. Install Homebrew if not already installed:
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

2. Install dependencies:
   ```bash
   brew install cmake git wget
   ```

3. Clone and setup VitaSDK with vdpm:
   ```bash
   git clone https://github.com/vitasdk/vdpm
   cd vdpm
   ./bootstrap-vitasdk.sh
   export VITASDK=/usr/local/vitasdk
   export PATH=$VITASDK/bin:$PATH
   ```

4. Install all packages (optional, for full development):
   ```bash
   ./install-all.sh
   ```

5. Add VitaSDK to your PATH permanently:
   ```bash
   echo 'export VITASDK=/usr/local/vitasdk' >> ~/.zshrc
   echo 'export PATH=$VITASDK/bin:$PATH' >> ~/.zshrc
   source ~/.zshrc
   ```

## Installing SDL2 for Vita

KeeperFX uses SDL2 for graphics. Install SDL2 libraries for Vita using vdpm:

```bash
# Navigate to your vdpm directory
cd vdpm

# Install SDL2 and related libraries
./vdpm sdl2
./vdpm sdl2_image
./vdpm sdl2_mixer
./vdpm sdl2_net
```

Alternatively, if you ran `./install-all.sh` during VitaSDK setup, SDL2 libraries are already installed.
vdpm install sdl2_mixer
vdpm install sdl2_net
```

## Setting Up Visual Studio Code

1. Install Visual Studio Code from [code.visualstudio.com](https://code.visualstudio.com/)
2. Install recommended extensions:
   - C/C++ Extension Pack (Microsoft)
   - CMake Tools
   - CMake Language Support

### Configure VSCode for Vita Development

1. Open the KeeperFX repository in VSCode
2. Create or edit `.vscode/settings.json`:
   ```json
   {
       "cmake.configureSettings": {
           "CMAKE_TOOLCHAIN_FILE": "${workspaceFolder}/vita.cmake"
       },
       "cmake.buildDirectory": "${workspaceFolder}/build-vita",
       "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools"
   }
   ```

3. Create or edit `.vscode/c_cpp_properties.json`:
   ```json
   {
       "configurations": [
           {
               "name": "Vita",
               "includePath": [
                   "${workspaceFolder}/**",
                   "${env:VITASDK}/arm-vita-eabi/include",
                   "${env:VITASDK}/arm-vita-eabi/include/SDL2"
               ],
               "defines": [
                   "VITA",
                   "__VITA__"
               ],
               "compilerPath": "${env:VITASDK}/bin/arm-vita-eabi-gcc",
               "cStandard": "c11",
               "cppStandard": "c++20",
               "intelliSenseMode": "gcc-arm"
           }
       ],
       "version": 4
   }
   ```

4. Create or edit `.vscode/tasks.json`:
   ```json
   {
       "version": "2.0.0",
       "tasks": [
           {
               "label": "CMake Configure (Vita)",
               "type": "shell",
               "command": "cmake",
               "args": [
                   "-DCMAKE_TOOLCHAIN_FILE=${workspaceFolder}/vita.cmake",
                   "-B",
                   "${workspaceFolder}/build-vita",
                   "-S",
                   "${workspaceFolder}"
               ],
               "group": "build",
               "problemMatcher": []
           },
           {
               "label": "CMake Build (Vita)",
               "type": "shell",
               "command": "cmake",
               "args": [
                   "--build",
                   "${workspaceFolder}/build-vita"
               ],
               "group": {
                   "kind": "build",
                   "isDefault": true
               },
               "problemMatcher": ["$gcc"],
               "dependsOn": ["CMake Configure (Vita)"]
           }
       ]
   }
   ```

## Building KeeperFX for Vita

### Using Command Line

1. Configure the build:
   ```bash
   cmake -DCMAKE_TOOLCHAIN_FILE=vita.cmake -B build-vita
   ```

2. Build the project:
   ```bash
   cmake --build build-vita
   ```

3. The output will be `build-vita/keeperfx.elf`

### Using Visual Studio Code

1. Open the Command Palette (Ctrl+Shift+P / Cmd+Shift+P)
2. Run "CMake: Configure"
3. Run "CMake: Build" or press F7

## Creating a VPK Package

After building, create a VPK (Vita Package) file for installation on your Vita:

1. Create a `sce_sys` directory with required files:
   ```bash
   mkdir -p build-vita/sce_sys
   ```

2. Create `build-vita/sce_sys/param.sfo`:
   ```bash
   vita-mksfoex -s TITLE_ID=KPFX00001 "KeeperFX" build-vita/sce_sys/param.sfo
   ```

3. Create the VPK:
   ```bash
   vita-make-fself -c build-vita/keeperfx.elf build-vita/eboot.bin
   vita-pack-vpk -s build-vita/sce_sys/param.sfo -b build-vita/eboot.bin keeperfx.vpk
   ```

## Installing on PlayStation Vita

1. Ensure your Vita has HENkaku/Ensō or h-encore installed
2. Install VitaShell on your Vita
3. Transfer the VPK to your Vita via USB or FTP
4. Open VitaShell and navigate to the VPK file
5. Press X to install
6. The game will appear in your LiveArea

## Troubleshooting

### "VITASDK environment variable not set"
Make sure you've added the VITASDK path to your shell configuration and restarted your terminal. The path should be `/usr/local/vitasdk`.

### SDL2 headers not found
Navigate to your vdpm directory and run:
```bash
./vdpm sdl2
./vdpm sdl2_image
./vdpm sdl2_mixer
./vdpm sdl2_net
```

Or install all packages at once:
```bash
./install-all.sh
```

### Build fails with "arm-vita-eabi-gcc not found"
Ensure VitaSDK is properly installed and the bin directory is in your PATH. Run:
```bash
export VITASDK=/usr/local/vitasdk
export PATH=$VITASDK/bin:$PATH
```

### vdpm command not found
Make sure you've cloned the vdpm repository and are running commands from within it:
```bash
git clone https://github.com/vitasdk/vdpm
cd vdpm
./bootstrap-vitasdk.sh
```

### Game crashes on Vita
Check the Vita console output via USB debugging or review crash logs in `ux0:/data/`.

### CI Build: vdpm installation fails
If the GitHub Actions workflow fails with vdpm errors:

1. The workflow has a fallback mechanism to use precompiled SDL2 tarballs
2. Download SDL2 libraries for Vita from: https://github.com/vitasdk/packages/releases
3. Place the following files in `deps/vita/`:
   - `sdl2.tar.gz`
   - `sdl2_image.tar.gz`
   - `sdl2_mixer.tar.gz`
   - `sdl2_net.tar.gz`
4. Commit these files to the repository
5. The CI workflow will automatically extract and use them

This is a temporary workaround until vdpm is properly configured in the CI environment.

## Additional Resources

- [VitaSDK Documentation](https://vitasdk.github.io/)
- [Vita Homebrew Wiki](https://henkaku.xyz/)
- [PlayStation Vita Homebrew Discord](https://discord.gg/JXEKeg6)
- [KeeperFX Discord](https://discord.gg/hE4p7vy2Hb)

## Notes

- The Vita has 512 MB of RAM, so memory usage should be optimized
- The Vita's GPU supports OpenGL ES 2.0
- Screen resolution is 960x544
- Performance optimization is crucial for smooth gameplay
