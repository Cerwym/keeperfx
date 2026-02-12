# Rendering System Refactoring Plan

## Overview

This document outlines the plan for refactoring the KeeperFX rendering system to support multiple rendering backends, enabling hardware acceleration and platform-specific optimizations.

## Current Architecture

KeeperFX currently uses a software renderer based on the original Dungeon Keeper's rendering system:

- **Software-based 3D rendering**: All rendering calculations are performed on the CPU
- **8-bit color palette**: Original 256-color palette system
- **SDL2 backend**: For display output and window management
- **Custom polygon rasterization**: CPU-based triangle and polygon rendering
- **Fixed-function pipeline**: No shader support

### Key Rendering Components

- `bflib_render.c/h` - Core software rendering functions
- `bflib_render_gpoly.c` - Gouraud polygon rendering
- `bflib_render_trig.c` - Triangle rendering
- `engine_render.c/h` - Game-specific 3D rendering
- `bflib_video.c/h` - Video output and display management
- `bflib_vidraw.c/h` - Video drawing primitives
- `bflib_vidsurface.c/h` - Surface management

## Goals

1. **Hardware Acceleration**: Move rendering workload from CPU to GPU
2. **Multiple Backends**: Support for OpenGL, Vulkan, and DirectX
3. **Backward Compatibility**: Maintain software renderer as fallback
4. **Platform Support**: Improve support for Windows, Linux, and potentially macOS
5. **Performance**: Reduce CPU usage and increase frame rates
6. **Flexibility**: Enable advanced graphical effects through shaders

## Non-Goals

- Rewriting the entire rendering system in one go
- Breaking compatibility with existing maps and mods
- Changing the visual style of the game
- Supporting ancient hardware (focus on hardware from ~2010+)

## Implementation Phases

### Phase 1: Analysis & Planning ✓ (Current)

**Status**: In Progress

- [x] Create issue tracking system
- [ ] Document current rendering architecture
- [ ] Identify shader conversion opportunities
- [ ] Profile rendering performance bottlenecks
- [ ] Research backend requirements (OpenGL, Vulkan, DirectX)

### Phase 2: Architecture Design

**Prerequisites**: Phase 1 complete

**Goals**:
- Design renderer abstraction layer
- Define interface for rendering operations
- Plan state management and resource handling
- Design shader pipeline architecture
- Document API contracts

**Deliverables**:
- `docs/rendering-abstraction-design.md`
- Interface header files or pseudocode
- Sequence diagrams

### Phase 3: Abstraction Layer Implementation

**Prerequisites**: Phase 2 design approved

**Goals**:
- Implement renderer interface
- Port software renderer to new abstraction
- Add backend selection system
- Ensure zero performance regression
- Maintain 100% visual compatibility

**Deliverables**:
- `src/renderer_interface.h`
- `src/renderer_software.c/h`
- `src/renderer_factory.c/h`
- Performance benchmarks

### Phase 4: OpenGL Backend (Recommended First)

**Prerequisites**: Phase 3 complete

**Why OpenGL First?**
- Widely supported across platforms
- Simpler than Vulkan or DirectX 12
- Good documentation and community support
- Easier to debug than Vulkan

**Goals**:
- Implement OpenGL 3.3+ core profile renderer
- Support both Windows and Linux
- Implement basic shader pipeline
- Achieve performance improvements over software renderer

**Deliverables**:
- `src/renderer_opengl.c/h`
- GLSL shaders
- Platform integration
- Performance comparison

### Phase 5: Additional Backends (Future)

**DirectX Backend** (Windows-specific):
- DirectX 11 recommended over 12 (lower complexity)
- Better integration with Windows-specific features
- Similar performance to OpenGL on Windows

**Vulkan Backend** (Advanced):
- Highest performance potential
- Most complex to implement
- Best for very high-end scenarios
- Consider using helper libraries (vk-bootstrap, volk)

### Phase 6: Shader Effects & Optimization

**Prerequisites**: At least one hardware backend working

**Goals**:
- Port CPU-bound effects to shaders
- Implement post-processing effects
- Add optional graphical enhancements
- Optimize shader performance

**Examples**:
- Lighting calculations
- Fog effects
- Color grading
- Anti-aliasing options
- Enhanced water/lava effects

### Phase 7: Build System & CI/CD

**Goals**:
- Update build system for multi-backend support
- Add runtime backend selection
- Improve CI/CD workflows
- Test automation for each backend
- Cross-platform validation

## Technical Considerations

### Renderer Abstraction Design

Two main approaches:

1. **C-style function pointers** (Fits current codebase)
   ```c
   struct Renderer {
       void (*init)(void);
       void (*shutdown)(void);
       void (*draw_triangle)(/* params */);
       // ...
   };
   ```

2. **C++ virtual functions** (More modern, but requires C++)
   ```cpp
   class IRenderer {
   public:
       virtual void Init() = 0;
       virtual void Shutdown() = 0;
       virtual void DrawTriangle(/* params */) = 0;
       // ...
   };
   ```

**Recommendation**: C-style function pointers to maintain consistency with existing codebase.

### Backend Selection

Runtime backend selection through configuration:
- `keeperfx.cfg` option: `RENDERING_BACKEND = <software|opengl|vulkan|directx>`
- Command-line override: `--renderer=opengl`
- Automatic fallback if preferred backend unavailable
- Validation on startup

### Shader Pipeline

For hardware backends:
- Vertex shaders for transformations
- Fragment shaders for texturing and lighting
- Geometry shaders for specific effects (optional)
- Compute shaders for heavy calculations (advanced)

### Memory Management

Hardware backends require:
- Vertex buffer management
- Texture uploading and caching
- Uniform buffer handling
- Resource lifecycle management

### Platform Considerations

**Windows**:
- DirectX: Native support
- OpenGL: Via drivers
- Vulkan: Via SDK and drivers

**Linux**:
- OpenGL: Mesa or proprietary drivers
- Vulkan: Good support on modern systems

**macOS**:
- OpenGL: Deprecated (max 4.1)
- Vulkan: Via MoltenVK (translation layer)
- Metal: Native (not planned)

## Performance Expectations

Based on similar projects:

- **Software → OpenGL**: 2-5x improvement in complex scenes
- **Software → Vulkan**: 3-8x improvement with proper optimization
- **Software → DirectX 11**: 2-4x improvement on Windows

Actual results depend heavily on:
- Scene complexity
- Number of draw calls
- Texture sizes
- Screen resolution
- Hardware capabilities

## Risk Assessment

### High Risk
- Breaking visual compatibility
- Performance regression
- Platform-specific bugs

### Medium Risk
- Shader compilation issues
- Driver compatibility problems
- Increased complexity

### Low Risk
- Build system complexity (mitigated by modular approach)
- Additional dependencies (manageable with vcpkg)

## Mitigation Strategies

1. **Extensive Testing**: Automated visual regression testing
2. **Incremental Rollout**: Optional feature initially
3. **Strong Abstraction**: Minimize backend-specific code leakage
4. **Community Feedback**: Early alpha/beta testing
5. **Fallback Support**: Always maintain working software renderer

## Timeline Estimate

Conservative estimates (1-2 developers):

- Phase 1 (Analysis): 2-3 weeks
- Phase 2 (Design): 2-3 weeks
- Phase 3 (Abstraction): 4-6 weeks
- Phase 4 (OpenGL): 8-12 weeks
- Phase 5 (Other backends): 8-12 weeks each
- Phase 6 (Shaders): 4-8 weeks
- Phase 7 (Build/CI): 2-4 weeks

**Total**: 6-12 months for full implementation with testing

## Success Criteria

1. **Functionality**: All rendering backends produce identical output
2. **Performance**: Hardware backends show measurable improvements
3. **Stability**: No increase in crash rates
4. **Compatibility**: Works with all existing maps and mods
5. **Maintainability**: Code is clean and well-documented
6. **Platform Support**: Builds successfully on all target platforms

## Resources

### Similar Projects
- [OpenMW](https://openmw.org/) - Open source Morrowind engine with modern rendering
- [GZDoom](https://www.zdoom.org/) - Doom source port with hardware rendering
- [OpenRCT2](https://openrct2.org/) - RollerCoaster Tycoon 2 reimplementation

### Documentation
- [OpenGL Documentation](https://www.opengl.org/documentation/)
- [Vulkan Guide](https://vkguide.dev/)
- [DirectX 11 Documentation](https://docs.microsoft.com/en-us/windows/win32/direct3d11/)
- [SDL2 Documentation](https://wiki.libsdl.org/)

### Tools
- [RenderDoc](https://renderdoc.org/) - Graphics debugger
- [Nsight Graphics](https://developer.nvidia.com/nsight-graphics) - NVIDIA GPU debugger
- [PIX](https://devblogs.microsoft.com/pix/) - DirectX debugger

## Next Steps

1. Run `scripts/create-issues.ps1` to create tracking issues
2. Begin Phase 1 documentation work
3. Set up performance profiling baseline
4. Research hardware backend requirements
5. Create proof-of-concept for abstraction layer

## Questions & Discussion

For questions or discussions about the rendering refactoring:
- Open an issue with the `rendering` label
- Join the Keeper Klan Discord development channel
- Review and comment on design documents in pull requests

---

*Last Updated*: 2026-02-12  
*Status*: Planning Phase  
*Next Review*: After Phase 1 completion
