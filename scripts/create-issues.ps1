# GitHub Issue Creation Script for KeeperFX Rendering Refactoring
# This script automates the creation of GitHub issues for task management
# Prerequisites: GitHub CLI (gh) must be installed and authenticated

Param(
    [Parameter(Mandatory=$false)]
    [string]$Owner = "Cerwym",
    
    [Parameter(Mandatory=$false)]
    [string]$Repo = "keeperfx",
    
    [Parameter(Mandatory=$false)]
    [switch]$DryRun = $false,
    
    [Parameter(Mandatory=$false)]
    [string]$Milestone = "",
    
    [Parameter(Mandatory=$false)]
    [string[]]$Assignees = @()
)

# Color output functions
function Write-Info {
    param([string]$Message)
    Write-Host $Message -ForegroundColor Cyan
}

function Write-Success {
    param([string]$Message)
    Write-Host $Message -ForegroundColor Green
}

function Write-Error-Custom {
    param([string]$Message)
    Write-Host $Message -ForegroundColor Red
}

function Write-Warning-Custom {
    param([string]$Message)
    Write-Host $Message -ForegroundColor Yellow
}

# Check if GitHub CLI is installed
function Test-GitHubCLI {
    try {
        $ghVersion = gh --version 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Success "GitHub CLI is installed: $($ghVersion[0])"
            return $true
        }
    }
    catch {
        Write-Error-Custom "GitHub CLI (gh) is not installed or not in PATH"
        Write-Info "Please install from: https://cli.github.com/"
        return $false
    }
    return $false
}

# Check if authenticated with GitHub
function Test-GitHubAuth {
    try {
        $authStatus = gh auth status 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Success "GitHub CLI is authenticated"
            return $true
        }
    }
    catch {
        Write-Error-Custom "GitHub CLI is not authenticated"
        Write-Info "Please run: gh auth login"
        return $false
    }
    return $false
}

# Define rendering refactoring issues
$issues = @(
    @{
        Title = "Document Current Rendering Architecture"
        Body = @"
## Overview
Create comprehensive documentation of the current software rendering architecture in KeeperFX.

## Tasks
- [ ] Document the rendering pipeline flow
- [ ] Identify all rendering-related source files
- [ ] Map out key rendering functions and their purposes
- [ ] Document current limitations and bottlenecks
- [ ] List CPU-bound operations that could benefit from GPU acceleration

## Files to Analyze
- ``bflib_render.c/h`` - Core rendering functions
- ``engine_render.c/h`` - Game-specific rendering
- ``bflib_video.c/h`` - Video output layer
- ``bflib_vidraw.c/h`` - Video drawing functions

## Deliverables
- Architecture diagram (can be ASCII or image)
- Markdown documentation in ``docs/rendering-architecture.md``
"@
        Labels = @("documentation", "rendering", "architecture")
    },
    @{
        Title = "Design Rendering Backend Abstraction Layer"
        Body = @"
## Overview
Design an abstraction layer to support multiple rendering backends (Software, OpenGL, Vulkan, DirectX).

## Goals
- Abstract away rendering implementation details
- Support multiple backends without code duplication
- Maintain backward compatibility with software renderer
- Enable platform-specific optimizations

## Design Considerations
- Interface design (C-style or C++ virtual functions)
- State management
- Resource handling (textures, buffers, shaders)
- Performance impact of abstraction
- Build system integration

## Tasks
- [ ] Design renderer interface/API
- [ ] Create sequence diagrams for rendering operations
- [ ] Define initialization and cleanup procedures
- [ ] Plan configuration system for backend selection
- [ ] Document design decisions

## Deliverables
- Design document in ``docs/rendering-abstraction-design.md``
- Interface definitions (header files or pseudocode)
"@
        Labels = @("enhancement", "rendering", "architecture", "design")
    },
    @{
        Title = "Implement Rendering Backend Interface"
        Body = @"
## Overview
Implement the rendering backend abstraction interface based on the approved design.

## Prerequisites
- Issue #2 (Design) must be completed and approved

## Tasks
- [ ] Create base renderer interface (C struct or C++ class)
- [ ] Define rendering operation function pointers/virtual methods
- [ ] Implement renderer factory/registration system
- [ ] Add backend selection configuration
- [ ] Implement software renderer using new interface
- [ ] Ensure backward compatibility

## Files to Create/Modify
- ``src/renderer_interface.h`` - Interface definition
- ``src/renderer_software.c/h`` - Software renderer implementation
- ``src/renderer_factory.c/h`` - Backend selection

## Testing
- [ ] Verify software renderer works identically to original
- [ ] Test on Windows platform
- [ ] Test different screen resolutions
- [ ] Profile performance impact

## Deliverables
- Working software renderer using new abstraction
- Unit tests (if test infrastructure exists)
- Performance comparison report
"@
        Labels = @("enhancement", "rendering", "implementation")
    },
    @{
        Title = "Research OpenGL Backend Requirements"
        Body = @"
## Overview
Research and document requirements for implementing an OpenGL rendering backend.

## Research Areas
- [ ] OpenGL version selection (compatibility vs core profile)
- [ ] Required extensions
- [ ] Shader requirements (GLSL version)
- [ ] Platform support (Windows, Linux, macOS)
- [ ] Library dependencies (GLEW, GLAD, or built-in)
- [ ] Texture format compatibility
- [ ] Performance expectations

## Platform Considerations
- Windows: OpenGL via drivers
- Linux: Mesa or proprietary drivers
- macOS: Deprecated but still available (up to 4.1)

## Tasks
- [ ] Document minimum and recommended OpenGL versions
- [ ] List required extensions
- [ ] Create shader pipeline plan
- [ ] Identify potential compatibility issues
- [ ] Research similar projects (OpenMW, GZDoom, etc.)

## Deliverables
- Research document in ``docs/opengl-backend-research.md``
- List of dependencies to add
- Platform compatibility matrix
"@
        Labels = @("research", "rendering", "opengl")
    },
    @{
        Title = "Research Vulkan Backend Requirements"
        Body = @"
## Overview
Research and document requirements for implementing a Vulkan rendering backend.

## Research Areas
- [ ] Vulkan version selection (1.0, 1.1, 1.2, 1.3)
- [ ] Required extensions
- [ ] Validation layers for development
- [ ] Platform support and SDK requirements
- [ ] Shader compilation (SPIR-V)
- [ ] Memory management strategy
- [ ] Synchronization primitives
- [ ] Performance expectations

## Complexity Assessment
Vulkan is significantly more complex than OpenGL. Evaluate if the benefits justify the implementation cost for this project.

## Tasks
- [ ] Document minimum Vulkan version and required features
- [ ] List platform-specific considerations
- [ ] Assess shader compilation strategy (runtime vs offline)
- [ ] Evaluate validation layer integration
- [ ] Research Vulkan helper libraries (volk, vk-bootstrap, etc.)
- [ ] Compare benefits vs implementation cost

## Deliverables
- Research document in ``docs/vulkan-backend-research.md``
- Recommendation on whether to pursue Vulkan backend
- Timeline estimate if approved
"@
        Labels = @("research", "rendering", "vulkan")
    },
    @{
        Title = "Research DirectX Backend Requirements"
        Body = @"
## Overview
Research and document requirements for implementing a DirectX rendering backend.

## DirectX Version Selection
- DirectX 11: Mature, widely supported, easier than 12
- DirectX 12: Modern, lower overhead, more complex
- Consider target Windows versions

## Research Areas
- [ ] DirectX version selection (DX11 vs DX12)
- [ ] SDK requirements
- [ ] Platform compatibility (Windows only)
- [ ] Shader requirements (HLSL)
- [ ] Texture format support
- [ ] Performance expectations vs OpenGL
- [ ] Integration with existing Windows-specific code

## Tasks
- [ ] Document recommended DirectX version
- [ ] List SDK and build system changes
- [ ] Assess shader compilation strategy
- [ ] Evaluate compatibility with existing Windows code
- [ ] Research helper libraries (DirectXTK, etc.)

## Deliverables
- Research document in ``docs/directx-backend-research.md``
- Recommendation on DX11 vs DX12
- Dependencies and build system requirements
"@
        Labels = @("research", "rendering", "directx", "windows")
    },
    @{
        Title = "Identify Shader Conversion Opportunities"
        Body = @"
## Overview
Identify CPU-bound rendering operations that could be offloaded to GPU shaders.

## Prerequisites
- Issue #1 (Architecture Documentation) must be completed

## Areas to Analyze
- Polygon rendering
- Texture mapping
- Lighting calculations
- Fog effects
- Post-processing effects
- Sprite rendering
- Color manipulation

## Tasks
- [ ] Profile current CPU-bound operations
- [ ] Identify shader candidates
- [ ] Estimate performance gains
- [ ] Assess implementation complexity
- [ ] Prioritize by impact vs effort

## Tools
- Use profiling tools to identify bottlenecks
- Analyze frame time breakdowns
- Test with various map sizes and complexities

## Deliverables
- Profiling report in ``docs/rendering-profiling.md``
- Prioritized list of shader opportunities
- Performance gain estimates
"@
        Labels = @("performance", "rendering", "analysis")
    },
    @{
        Title = "Evaluate CI/CD Build System Improvements"
        Body = @"
## Overview
Evaluate and propose improvements to the CI/CD build system, potentially moving away from CMake complexity.

## Current State
- Makefile-based build (primary)
- CMake build system (additional)
- GitHub Actions workflows
- Multiple platform targets (Windows, Linux)
- Cross-compilation support

## Research Areas
- [ ] Evaluate Makefile improvements
- [ ] Research CMake alternatives (Meson, Bazel, xmake, etc.)
- [ ] Assess Premake or similar meta-build systems
- [ ] Analyze build time optimizations
- [ ] Review dependency management (vcpkg integration)

## Tasks
- [ ] Survey modern build systems
- [ ] Compare features and complexity
- [ ] Assess migration effort
- [ ] Create proof-of-concept for top candidates
- [ ] Gather team feedback

## Deliverables
- Build system comparison in ``docs/build-system-evaluation.md``
- Recommendation with justification
- Migration plan if proposing changes
"@
        Labels = @("infrastructure", "ci-cd", "build-system")
    },
    @{
        Title = "Create GitHub Actions Workflow for Multi-Backend Testing"
        Body = @"
## Overview
Create CI/CD workflows to build and test multiple rendering backends.

## Prerequisites
- Backend abstraction must be implemented
- At least one additional backend (beyond software) should exist

## Requirements
- Build all available backends
- Run tests for each backend
- Generate artifacts for each configuration
- Support matrix builds
- Reasonable build times

## Tasks
- [ ] Design workflow matrix strategy
- [ ] Configure build jobs for each backend
- [ ] Set up appropriate test environments
- [ ] Add backend-specific validation
- [ ] Optimize caching for faster builds
- [ ] Document workflow usage

## Deliverables
- Workflow file in ``.github/workflows/build-multi-backend.yml``
- Documentation on workflow usage
"@
        Labels = @("ci-cd", "testing", "infrastructure")
    },
    @{
        Title = "Update Build System for Multi-Backend Support"
        Body = @"
## Overview
Update the build system (Makefile, CMake) to support building multiple rendering backends.

## Requirements
- Conditional compilation based on backend selection
- Support building multiple backends in single build
- Backend selection at runtime (not just compile-time)
- Maintain backward compatibility
- Cross-platform support

## Tasks
- [ ] Add backend selection flags/options
- [ ] Update Makefile rules
- [ ] Update CMake configuration
- [ ] Add preprocessor definitions
- [ ] Update dependency detection
- [ ] Document build options

## Deliverables
- Updated ``Makefile`` and ``CMakeLists.txt``
- Build documentation in ``docs/building-multi-backend.md``
- Example build commands for each backend
"@
        Labels = @("infrastructure", "build-system", "enhancement")
    }
)

# Main execution
Write-Info "=== KeeperFX GitHub Issue Creation Script ==="
Write-Info "Repository: $Owner/$Repo"

if ($DryRun) {
    Write-Warning-Custom "DRY RUN MODE - No issues will be created"
}

# Validate prerequisites
if (-not (Test-GitHubCLI)) {
    exit 1
}

if (-not (Test-GitHubAuth)) {
    exit 1
}

Write-Info "`nIssues to create: $($issues.Count)"

# Confirm with user
if (-not $DryRun) {
    $confirmation = Read-Host "`nDo you want to create these issues? (yes/no)"
    if ($confirmation -ne "yes") {
        Write-Warning-Custom "Operation cancelled by user"
        exit 0
    }
}

# Create issues
$createdIssues = @()
$issueNumber = 1

foreach ($issue in $issues) {
    Write-Info "`n--- Processing Issue $issueNumber/$($issues.Count) ---"
    Write-Host "Title: $($issue.Title)"
    
    # Build gh command
    $ghArgs = @(
        "issue", "create",
        "--repo", "$Owner/$Repo",
        "--title", $issue.Title,
        "--body", $issue.Body
    )
    
    # Add labels
    foreach ($label in $issue.Labels) {
        $ghArgs += "--label"
        $ghArgs += $label
    }
    
    # Add milestone if specified
    if ($Milestone -ne "") {
        $ghArgs += "--milestone"
        $ghArgs += $Milestone
    }
    
    # Add assignees if specified
    if ($Assignees.Count -gt 0) {
        foreach ($assignee in $Assignees) {
            $ghArgs += "--assignee"
            $ghArgs += $assignee
        }
    }
    
    if ($DryRun) {
        Write-Host "Would create issue with command:" -ForegroundColor DarkGray
        Write-Host "gh $($ghArgs -join ' ')" -ForegroundColor DarkGray
        $createdIssues += @{
            Number = "DRY-RUN-$issueNumber"
            Title = $issue.Title
            Url = "https://github.com/$Owner/$Repo/issues/DRY-RUN-$issueNumber"
        }
    }
    else {
        try {
            $result = & gh @ghArgs
            if ($LASTEXITCODE -eq 0) {
                Write-Success "✓ Issue created: $result"
                $createdIssues += @{
                    Number = ($result -split '/')[-1]
                    Title = $issue.Title
                    Url = $result
                }
            }
            else {
                Write-Error-Custom "✗ Failed to create issue: $($issue.Title)"
            }
        }
        catch {
            Write-Error-Custom "✗ Error creating issue: $_"
        }
    }
    
    $issueNumber++
    Start-Sleep -Milliseconds 500  # Rate limiting
}

# Summary
Write-Info "`n=== Summary ==="
Write-Success "Created $($createdIssues.Count) issues:"
foreach ($created in $createdIssues) {
    Write-Host "  #$($created.Number): $($created.Title)" -ForegroundColor White
    Write-Host "    $($created.Url)" -ForegroundColor DarkGray
}

Write-Info "`n✓ Done!"
