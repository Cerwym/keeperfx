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
    [string]$WorkspaceFolder = $PSScriptRoot | Split-Path -Parent
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$Color = @{
    Reset = "`e[0m"
    Green = "`e[32m"
    Yellow = "`e[33m"
    Blue = "`e[34m"
    Red = "`e[31m"
    Cyan = "`e[36m"
}

function Write-ColorOutput {
    param([string]$Message, [string]$Color = 'Reset')
    Write-Host "$($Color)$Message$($Color.Reset)"
}

Write-ColorOutput "=== Clean Master Setup ===" $Color.Cyan

# Check if target already exists
if (Test-Path $TargetPath) {
    Write-ColorOutput "Clean master already exists at: $TargetPath" $Color.Yellow
    
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
        Write-ColorOutput "Validation: OK - All required files present" $Color.Green
        
        if (-not $UseExisting) {
            $response = Read-Host "Use this existing installation? (Y/n)"
            if ($response -eq 'n') {
                Write-ColorOutput "Aborted." $Color.Yellow
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
        
        Write-ColorOutput "`nClean master configured: $TargetPath" $Color.Green
        Write-ColorOutput "Next: Run .\init_layered_deploy.ps1 to create deployment" $Color.Blue
        exit 0
    } else {
        Write-ColorOutput "Validation: FAILED - Missing required files" $Color.Red
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
        Write-ColorOutput "ERROR: Source path not found: $SourcePath" $Color.Red
        exit 1
    }
    
    Write-ColorOutput "Copying from: $SourcePath" $Color.Blue
    Write-ColorOutput "         to: $TargetPath" $Color.Blue
    Write-ColorOutput "This may take a minute..." $Color.Yellow
    
    New-Item -ItemType Directory -Path $TargetPath -Force | Out-Null
    Copy-Item "$SourcePath\*" $TargetPath -Recurse -Force
    
    Write-ColorOutput "Copy complete!" $Color.Green
} else {
    Write-ColorOutput "ERROR: No clean master found and no source specified." $Color.Red
    Write-ColorOutput "Usage:" $Color.Yellow
    Write-ColorOutput "  .\setup_clean_master.ps1 -SourcePath 'C:\temp'" $Color.Yellow
    Write-ColorOutput "  .\setup_clean_master.ps1 -UseExisting -TargetPath 'C:\path\to\clean'" $Color.Yellow
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

Write-ColorOutput "`n=== Setup Complete ===" $Color.Cyan
Write-ColorOutput "Clean master: $TargetPath" $Color.Green
Write-ColorOutput "Settings saved to: .vscode\settings.json" $Color.Blue
Write-ColorOutput "`nNext: Run .\init_layered_deploy.ps1 to create deployment" $Color.Yellow
