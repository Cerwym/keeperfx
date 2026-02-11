#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Creates a layered deployment environment using Windows junctions and hard links.

.DESCRIPTION
    Initializes a .deploy/ directory with:
    - Layer 0 (Base): Junctions to clean master directories (read-only view)
    - Layer 1 (Overlay): Hard links to files that may be modified
    - Layer 2 (Modified): Space for your changed files (exe, DATs, configs)
    
    Result: Full game installation view with 95% disk savings (20MB vs 500MB).

.PARAMETER CleanMasterPath
    Path to clean KeeperFX installation (e.g., C:\Users\peter\source\repos\keeperfx-clean\1.3.1)

.EXAMPLE
    .\init_layered_deploy.ps1 -CleanMasterPath "C:\Users\peter\source\repos\keeperfx-clean\1.3.1"
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory=$false)]
    [string]$CleanMasterPath,
    
    [Parameter(Mandatory=$false)]
    [string]$WorkspaceFolder
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Set default workspace folder if not provided
if (-not $WorkspaceFolder) {
    $WorkspaceFolder = Split-Path -Parent $PSScriptRoot
}

# ANSI color codes for output
$script:Colors = @{
    Reset = "`e[0m"
    Green = "`e[32m"
    Yellow = "`e[33m"
    Blue = "`e[34m"
    Red = "`e[31m"
    Cyan = "`e[36m"
}

function Write-ColorOutput {
    param([string]$Message, [string]$ColorCode = 'Reset')
    Write-Host "$($script:Colors[$ColorCode])$Message$($script:Colors.Reset)" -NoNewline
    Write-Host ""
}

function Test-AdminPrivileges {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

# Main script
Write-ColorOutput "=== KeeperFX Layered Deployment Initializer ===" 'Cyan'

# Get or validate clean master path
if (-not $CleanMasterPath) {
    $settingsPath = Join-Path $WorkspaceFolder ".vscode\settings.json"
    if (Test-Path $settingsPath) {
        $settings = Get-Content $settingsPath -Raw | ConvertFrom-Json
        $CleanMasterPath = $settings.'keeperfx.cleanMasterPath'
    }
}

if (-not $CleanMasterPath) {
    Write-ColorOutput "ERROR: Clean master path not specified." 'Red'
    Write-ColorOutput "Run setup_clean_master.ps1 first or provide -CleanMasterPath parameter." 'Yellow'
    exit 1
}

if (-not (Test-Path $CleanMasterPath)) {
    Write-ColorOutput "ERROR: Clean master not found at: $CleanMasterPath" 'Red'
    exit 1
}

Write-ColorOutput "Clean master: $CleanMasterPath" 'Blue'

# Verify clean master has expected structure
$requiredFiles = @('keeperfx.exe', 'data', 'ldata', 'levels', 'fxdata')
foreach ($item in $requiredFiles) {
    $itemPath = Join-Path $CleanMasterPath $item
    if (-not (Test-Path $itemPath)) {
        Write-ColorOutput "ERROR: Clean master missing: $item" 'Red'
        exit 1
    }
}

# Setup deployment directory
$deployPath = Join-Path $WorkspaceFolder ".deploy"
if (Test-Path $deployPath) {
    Write-ColorOutput "Deployment already exists at: $deployPath" 'Yellow'
    $response = Read-Host "Delete and recreate? (y/N)"
    if ($response -ne 'y') {
        Write-ColorOutput "Aborted." 'Yellow'
        exit 0
    }
    
    Write-ColorOutput "Removing existing deployment..." 'Yellow'
    & "$PSScriptRoot\reset_layered_deploy.ps1" -Force
}

Write-ColorOutput "Creating deployment directory..." 'Green'
New-Item -ItemType Directory -Path $deployPath -Force | Out-Null

# Create metadata directory
$metadataPath = Join-Path $deployPath ".overlay"
New-Item -ItemType Directory -Path $metadataPath -Force | Out-Null

# Layer 0: Create junctions for large read-only directories
Write-ColorOutput "`nLayer 0: Creating junctions (base directories)..." 'Green'

$junctionDirs = @('ldata', 'levels', 'fxdata', 'lang', 'campgns')
foreach ($dir in $junctionDirs) {
    $sourcePath = Join-Path $CleanMasterPath $dir
    $targetPath = Join-Path $deployPath $dir
    
    if (Test-Path $sourcePath) {
        Write-Host "  Junction: $dir -> " -NoNewline
        cmd /c mklink /J "$targetPath" "$sourcePath" 2>&1 | Out-Null
        if ($LASTEXITCODE -eq 0) {
            Write-ColorOutput "OK" 'Green'
        } else {
            Write-ColorOutput "FAILED" 'Red'
        }
    }
}

# Layer 1: Create hard links for data/*.dat files (frequently modified)
Write-ColorOutput "`nLayer 1: Creating hard links (modifiable files)..." 'Green'

$dataSourcePath = Join-Path $CleanMasterPath "data"
$dataTargetPath = Join-Path $deployPath "data"
New-Item -ItemType Directory -Path $dataTargetPath -Force | Out-Null

$datFiles = Get-ChildItem -Path $dataSourcePath -Filter "*.dat" -File
foreach ($file in $datFiles) {
    $sourcePath = $file.FullName
    $targetPath = Join-Path $dataTargetPath $file.Name
    
    Write-Host "  Hardlink: data\$($file.Name) -> " -NoNewline
    cmd /c mklink /H "$targetPath" "$sourcePath" 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-ColorOutput "OK" 'Green'
    } else {
        Write-ColorOutput "FAILED" 'Red'
    }
}

# Copy keeperfx.exe (will be overwritten by builds)
Write-ColorOutput "`nCopying executable..." 'Green'
$exeSource = Join-Path $CleanMasterPath "keeperfx.exe"
$exeTarget = Join-Path $deployPath "keeperfx.exe"
Copy-Item $exeSource $exeTarget -Force
Write-ColorOutput "  keeperfx.exe copied" 'Green'

# Copy config files (user may want to modify)
Write-ColorOutput "`nCopying configuration files..." 'Green'
$configPath = Join-Path $deployPath "config"
New-Item -ItemType Directory -Path $configPath -Force | Out-Null

$configSource = Join-Path $CleanMasterPath "config\keeperfx.cfg"
if (Test-Path $configSource) {
    Copy-Item $configSource (Join-Path $configPath "keeperfx.cfg") -Force
    Write-ColorOutput "  keeperfx.cfg copied" 'Green'
}

# Create manifest
$manifest = @{
    created = Get-Date -Format "yyyy-MM-ddTHH:mm:ss"
    cleanMaster = $CleanMasterPath
    workspace = $WorkspaceFolder
    branch = (git branch --show-current 2>$null)
    junctions = $junctionDirs
    hardlinks = $datFiles.Name
}

$manifestPath = Join-Path $metadataPath "manifest.json"
$manifest | ConvertTo-Json -Depth 10 | Set-Content $manifestPath -Encoding UTF8

Write-ColorOutput "`n=== Deployment Ready ===" 'Cyan'
Write-ColorOutput "Location: $deployPath" 'Blue'
Write-ColorOutput "Disk usage: ~20MB (junctions + hard links)" 'Green'
Write-ColorOutput "`nTo deploy your builds:" 'Yellow'
Write-ColorOutput "  1. Build: wsl make" 'Yellow'
Write-ColorOutput "  2. Deploy: .\.vscode\deploy_assets.ps1" 'Yellow'
Write-ColorOutput "  3. Run: .\.vscode\launch_deploy.ps1" 'Yellow'
Write-ColorOutput "`nManifest saved to: .deploy\.overlay\manifest.json" 'Blue'

