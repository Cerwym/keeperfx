# Rendering Refactoring Quick Start Guide

Welcome to the KeeperFX rendering refactoring project! This guide will help you get started quickly.

## Overview

We're refactoring the rendering system to support multiple backends (OpenGL, Vulkan, DirectX) while maintaining the existing software renderer. See `rendering-refactoring-plan.md` for the full plan.

## Getting Started

### 1. Create GitHub Issues

Use our automation script to create tracking issues:

```powershell
# Windows PowerShell or PowerShell Core
cd /path/to/keeperfx
.\scripts\create-issues.ps1 -DryRun  # Preview first
.\scripts\create-issues.ps1           # Create issues
```

**Prerequisites**:
- Install GitHub CLI: https://cli.github.com/
- Authenticate: `gh auth login`

### 2. Choose Your Starting Point

Pick a task based on your skills and interests:

#### 📚 Documentation & Analysis (Good First Task)
- **Document Current Rendering Architecture**
  - Read through `src/bflib_render.c`, `src/engine_render.c`
  - Create flow diagrams
  - Identify key functions and data structures
  - Document in `docs/rendering-architecture.md`

- **Profile Rendering Performance**
  - Use profiling tools to identify bottlenecks
  - Test with various maps (simple to complex)
  - Document findings in `docs/rendering-profiling.md`

#### 🔬 Research (Technical Analysis)
- **OpenGL Backend Requirements**
  - Research OpenGL versions and extensions
  - Investigate similar projects (OpenMW, GZDoom)
  - Document in `docs/opengl-backend-research.md`

- **Vulkan/DirectX Requirements**
  - Research platform-specific considerations
  - Evaluate complexity vs benefits
  - Document in respective research files

#### 🏗️ Architecture Design (Advanced)
- **Design Rendering Abstraction Layer**
  - Create interface for renderer backends
  - Design state management system
  - Plan resource handling (textures, buffers)
  - Document in `docs/rendering-abstraction-design.md`

#### 💻 Implementation (Advanced, After Design)
- **Implement Renderer Interface**
  - Create base renderer interface
  - Port software renderer to new abstraction
  - Ensure zero regression

## Key Files to Understand

### Current Rendering System

```
src/
├── bflib_render.c/h          # Core rendering (polygon drawing)
├── bflib_render_gpoly.c      # Gouraud shaded polygons
├── bflib_render_trig.c       # Triangle rendering
├── engine_render.c/h         # Game rendering (3D world)
├── bflib_video.c/h           # Video output (SDL2)
├── bflib_vidraw.c/h          # Drawing primitives
├── bflib_vidraw_spr_*.c      # Sprite rendering variations
└── bflib_vidsurface.c/h      # Surface management
```

### Key Structures

- `struct PolyPoint` - Screen space polygon point
- `struct GtBlock` - Texture/graphics block
- `struct EngineCoord` - 3D coordinate with view transformation
- `poly_pool[]` - Memory pool for polygon rendering

## Building and Testing

### Build the Project

```bash
# Linux (using Make)
make

# Windows (using Make with MinGW)
make

# Using CMake
cmake -B build
cmake --build build
```

See `docs/build_instructions.txt` for detailed instructions.

### Run Tests

```bash
# Build with tests
make tests

# Run test executable
./keeperfx --run-tests
```

### Profile Performance

```bash
# Build with profiling enabled
make PROFILE=1

# Run with profiling
./keeperfx --profile-rendering

# Or use external profilers
valgrind --tool=callgrind ./keeperfx
gprof ./keeperfx gmon.out > analysis.txt
```

## Development Workflow

1. **Pick an Issue**: Choose from GitHub issues (created by `create-issues.ps1`)
2. **Create a Branch**: `git checkout -b feature/your-feature-name`
3. **Document First**: If designing, write the design doc first
4. **Small Commits**: Make incremental, focused changes
5. **Test Thoroughly**: Verify no visual regressions
6. **Open PR**: Submit for review with clear description
7. **Iterate**: Address review feedback

## Coding Guidelines

### Existing Code Style

KeeperFX follows these conventions:

```c
// Function names: lowercase with underscores
void render_3d_scene(void);

// Struct names: CamelCase
struct RenderState {
    int field_name;  // member names: lowercase with underscores
};

// Constants: UPPERCASE with underscores
#define MAX_POLYGONS 16777216

// File organization
// - Headers: function declarations, structures, constants
// - Source: implementations
// - Keep related code together
```

See `docs/coding_style_for_eclipse.xml` for detailed formatting rules.

### New Abstraction Code

For new abstraction layer, follow similar conventions but consider:

```c
// Renderer interface (function pointer style)
struct IRenderer {
    void (*init)(void);
    void (*shutdown)(void);
    void (*begin_frame)(void);
    void (*end_frame)(void);
    void (*draw_triangle)(struct PolyPoint *points);
    // ... more functions
};

// Backend registration
void renderer_register_backend(const char *name, struct IRenderer *renderer);
struct IRenderer *renderer_get_backend(const char *name);
```

## Common Pitfalls

### ⚠️ Don't Break Visual Compatibility
- New backends must produce identical output (initially)
- Test with various maps and configurations
- Use screenshot comparison tools

### ⚠️ Don't Change Too Much at Once
- Incremental changes are easier to review
- Easier to identify what broke if something fails
- Reduces merge conflicts

### ⚠️ Don't Ignore Performance
- Profile before and after changes
- Document performance impact
- Consider both CPU and GPU usage

### ⚠️ Don't Skip Documentation
- Document design decisions
- Comment complex algorithms
- Update docs/ files when appropriate

## Testing Checklist

Before submitting a PR:

- [ ] Code compiles without warnings
- [ ] Runs without crashes
- [ ] No visual regressions (compare screenshots)
- [ ] Performance is same or better
- [ ] Works on target platforms (Windows/Linux)
- [ ] Documentation is updated
- [ ] Code follows project style

## Resources

### Learning Resources

**Graphics Programming**:
- [Learn OpenGL](https://learnopengl.com/) - Excellent OpenGL tutorial
- [Vulkan Tutorial](https://vulkan-tutorial.com/) - Step-by-step Vulkan guide
- [DirectX 11 Tutorial](http://www.rastertek.com/tutdx11.html) - DirectX 11 basics

**Similar Projects**:
- [OpenMW](https://gitlab.com/OpenMW/openmw) - Morrowind engine reimplementation
- [GZDoom](https://github.com/coelckers/gzdoom) - Doom source port with hardware rendering
- [OpenRCT2](https://github.com/OpenRCT2/OpenRCT2) - RollerCoaster Tycoon 2 reimplementation

**Tools**:
- [RenderDoc](https://renderdoc.org/) - Graphics debugger (OpenGL, Vulkan, DX11/12)
- [Nsight Graphics](https://developer.nvidia.com/nsight-graphics) - NVIDIA debugging
- [Visual Studio Graphics Debugger](https://docs.microsoft.com/en-us/visualstudio/debugger/graphics/) - DirectX debugging

### Project Resources

- **Discord**: [Keeper Klan](https://discord.gg/hE4p7vy2Hb) - Join #development channel
- **GitHub Issues**: Track progress and discussions
- **Wiki**: https://github.com/dkfans/keeperfx/wiki

## Getting Help

1. **Check Documentation**: Read `rendering-refactoring-plan.md` first
2. **Search Issues**: Someone might have asked already
3. **Ask on Discord**: Join the development channel
4. **Open an Issue**: For specific problems or questions

## Example: First Contribution

Here's a simple first contribution to get familiar with the codebase:

### Task: Document a Rendering Function

1. Pick a function from `src/bflib_render.c`, for example `draw_gpoly()`
2. Read the code and understand what it does
3. Add comprehensive documentation:

```c
/******************************************************************************/
/**
 * Draws a Gouraud-shaded polygon to the screen.
 * 
 * This function rasterizes a polygon with per-vertex lighting (Gouraud shading).
 * The polygon is defined by a set of points in screen space, each with its own
 * color/brightness value that is interpolated across the surface.
 * 
 * @param points Array of polygon vertices in screen space
 * @param npoints Number of vertices (3 or more)
 * @param color Base color of the polygon
 * 
 * @note This is a CPU-based rasterization function. In future GPU backends,
 *       this will be replaced with shader-based rendering.
 * 
 * @see struct PolyPoint for vertex definition
 * @see draw_triangle() for simpler non-shaded triangles
 */
void draw_gpoly(struct PolyPoint *points, int npoints, TbPixel color)
{
    // ... implementation
}
```

4. Submit as PR with title: "docs: Add documentation for draw_gpoly function"

This helps you:
- Understand the codebase
- Learn the coding style
- Make your first contribution
- Help future developers (including yourself!)

## Next Steps

1. ✅ Read this guide
2. ✅ Set up your development environment
3. ✅ Build the project successfully
4. ✅ Run the game and verify it works
5. 📋 Create GitHub issues with `scripts/create-issues.ps1`
6. 🎯 Pick your first issue
7. 💻 Start coding!

Good luck, and welcome to the KeeperFX rendering refactoring project! 🎮

---

*Last Updated*: 2026-02-12  
*Questions?* Open an issue or ask on Discord
