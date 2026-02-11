#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Sets up the clean KeeperFX master installation location.

.DESCRIPTION
    Configures the path to a clean KeeperFX installation used as base layer
    for layered deployments. Can copy from temporary location or use existing.

.PARAMETER SourcePath
    Path to copy clean master from (e.g., C:\temp)

.PARAMETER TargetPath
    Where to store clean master permanently (default: C:\Users\peter\source\repos\keeperfx-clean\1.3.1)

.PARAMETER UseExisting
    Use existing installation at TargetPath without copying.

.EXAMPLE
    .\setup_clean_master.ps1 -SourcePath "C:\temp" -TargetPath "C:\Users\peter\source\repos\keeperfx-clean\1.3.1"
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory=$false)]
    [string]$SourcePath,
    
    [Parameter(Mandatory=$false)]
    [string]$TargetPath = "C:\Users\peter\source\repos\keeperfx-clean\1.3.1",
    
    [Parameter(Mandatory=$false)]
    [switch]$UseExisting,
    
    [Parameter(Mandatory=$false)]
    [string]$WorkspaceFolder
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Set default workspace folder if not provided
if (-not $WorkspaceFolder) {
    $WorkspaceFolder = Split-Path -Parent $PSScriptRoot
}

$Color = @{
    Reset = "`e[0m"
    Green = "`e[32m"
    Yellow = "`e[33m"
    Blue = "`e[34m"
    Red = "`e[31m"
    Cyan = "`e[36m"
}

function Write-ColorOutput {
    param([string]$Message, [string]$ColorCode = 'Reset')
    $colors = @{
        Reset = "`e[0m"; Green = "`e[32m"; Yellow = "`e[33m"
        Blue = "`e[34m"; Red = "`e[31m"; Cyan = "`e[36m"
    }
    Write-Host "$($colors[$ColorCode])$Message$($colors.Reset)"
}

Write-ColorOutput "=== Clean Master Setup ===" 'Cyan'

# Check if target already exists
if (Test-Path $TargetPath) {
    Write-ColorOutput "Clean master already exists at: $TargetPath" 'Yellow'
    
    # Verify it has expected structure
    $requiredFiles = @('keeperfx.exe', 'data', 'ldata', 'levels')
    $isValid = $true
    foreach ($file in $requiredFiles) {
        if (-not (Test-Path (Join-Path $TargetPath $file))) {
            $isValid = $false
            break
        }
    }
    
    if ($isValid) {
        Write-ColorOutput "Validation: OK - All required files present" 'Green'
        
        if (-not $UseExisting) {
            $response = Read-Host "Use this existing installation? (Y/n)"
            if ($response -eq 'n') {
                Write-ColorOutput "Aborted." 'Yellow'
                exit 1
            }
        }
        
        # Save to settings
        $settingsPath = Join-Path $WorkspaceFolder ".vscode\settings.json"
        $settings = @{}
        if (Test-Path $settingsPath) {
            $settings = Get-Content $settingsPath -Raw | ConvertFrom-Json -AsHashtable
        }
        $settings.'keeperfx.cleanMasterPath' = $TargetPath
        $settings | ConvertTo-Json -Depth 10 | Set-Content $settingsPath -Encoding UTF8
        
        Write-ColorOutput "`nClean master configured: $TargetPath" 'Green'
        Write-ColorOutput "Next: Run .\init_layered_deploy.ps1 to create deployment" 'Blue'
        exit 0
    } else {
        Write-ColorOutput "Validation: FAILED - Missing required files" 'Red'
        $response = Read-Host "Delete and start fresh? (y/N)"
        if ($response -ne 'y') {
            exit 1
        }
        Remove-Item $TargetPath -Recurse -Force
    }
}

# Copy from source if provided
if ($SourcePath) {
    if (-not (Test-Path $SourcePath)) {
        Write-ColorOutput "ERROR: Source path not found: $SourcePath" 'Red'
        exit 1
    }
    
    Write-ColorOutput "Copying from: $SourcePath" 'Blue'
    Write-ColorOutput "         to: $TargetPath" 'Blue'
    Write-ColorOutput "This may take a minute..." 'Yellow'
    
    New-Item -ItemType Directory -Path $TargetPath -Force | Out-Null
    Copy-Item "$SourcePath\*" $TargetPath -Recurse -Force
    
    Write-ColorOutput "Copy complete!" 'Green'
} else {
    Write-ColorOutput "ERROR: No clean master found and no source specified." 'Red'
    Write-ColorOutput "Usage:" 'Yellow'
    Write-ColorOutput "  .\setup_clean_master.ps1 -SourcePath 'C:\temp'" 'Yellow'
    Write-ColorOutput "  .\setup_clean_master.ps1 -UseExisting -TargetPath 'C:\path\to\clean'" 'Yellow'
    exit 1
}

# Save to settings
$settingsPath = Join-Path $WorkspaceFolder ".vscode\settings.json"
$settings = @{}
if (Test-Path $settingsPath) {
    $settings = Get-Content $settingsPath -Raw | ConvertFrom-Json -AsHashtable
}
$settings.'keeperfx.cleanMasterPath' = $TargetPath
$settings | ConvertTo-Json -Depth 10 | Set-Content $settingsPath -Encoding UTF8

Write-ColorOutput "`n=== Setup Complete ===" 'Cyan'
Write-ColorOutput "Clean master: $TargetPath" 'Green'
Write-ColorOutput "Settings saved to: .vscode\settings.json" 'Blue'
Write-ColorOutput "`nNext: Run .\init_layered_deploy.ps1 to create deployment" 'Yellow'

