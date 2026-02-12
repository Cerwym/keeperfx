# Scripts Directory

This directory contains utility scripts for the KeeperFX project.

## Available Scripts

### create-issues.ps1

PowerShell script to automate the creation of GitHub issues for the rendering refactoring project.

#### Prerequisites

- PowerShell 5.1+ (Windows) or PowerShell Core 7+ (cross-platform)
- GitHub CLI (`gh`) installed and authenticated
  - Download from: https://cli.github.com/
  - Authenticate with: `gh auth login`

#### Usage

**Basic usage** (creates issues in the configured repository):
```powershell
.\scripts\create-issues.ps1
```

**Dry run** (preview without creating issues):
```powershell
.\scripts\create-issues.ps1 -DryRun
```

**Specify repository**:
```powershell
.\scripts\create-issues.ps1 -Owner "username" -Repo "reponame"
```

**Add milestone and assignees**:
```powershell
.\scripts\create-issues.ps1 -Milestone "Rendering Refactor v1" -Assignees @("user1", "user2")
```

#### What It Creates

The script creates a comprehensive set of issues for the rendering refactoring project:

1. **Documentation Issues**
   - Document Current Rendering Architecture

2. **Design Issues**
   - Design Rendering Backend Abstraction Layer

3. **Implementation Issues**
   - Implement Rendering Backend Interface
   - Update Build System for Multi-Backend Support

4. **Research Issues**
   - Research OpenGL Backend Requirements
   - Research Vulkan Backend Requirements
   - Research DirectX Backend Requirements
   - Identify Shader Conversion Opportunities

5. **Infrastructure Issues**
   - Evaluate CI/CD Build System Improvements
   - Create GitHub Actions Workflow for Multi-Backend Testing

Each issue includes:
- Detailed description
- Task checklist
- Prerequisites (where applicable)
- Deliverables
- Appropriate labels

#### Customization

To customize the issues being created, edit the `$issues` array in the script. Each issue has:
- `Title` - Issue title
- `Body` - Issue description (supports Markdown)
- `Labels` - Array of label names

#### Troubleshooting

**Error: "gh: command not found"**
- Install GitHub CLI from https://cli.github.com/

**Error: "Not authenticated"**
- Run `gh auth login` and follow the prompts

**Error: "Label not found"**
- Create the required labels in your repository first, or remove them from the issue definitions
- Common labels needed: `documentation`, `rendering`, `architecture`, `enhancement`, `research`, `opengl`, `vulkan`, `directx`, `windows`, `performance`, `analysis`, `infrastructure`, `ci-cd`, `build-system`, `testing`, `implementation`, `design`

## Adding New Scripts

When adding new scripts to this directory:
1. Use descriptive names
2. Add proper documentation (comments and README updates)
3. Follow existing coding style
4. Include usage examples
