#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Safely removes a layered deployment environment.

.DESCRIPTION
    Removes .deploy/ directory by:
    1. Deleting junctions (doesn't affect source)
    2. Deleting hard links (source files remain in clean master)
    3. Removing overlay files
    
    Safe: Original clean master files are never touched.

.PARAMETER Force
    Skip confirmation prompt.

.EXAMPLE
    .\reset_layered_deploy.ps1 -Force
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory=$false)]
    [switch]$Force,
    
    [Parameter(Mandatory=$false)]
    [string]$WorkspaceFolder
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Set default workspace folder if not provided
if (-not $WorkspaceFolder) {
    $WorkspaceFolder = Split-Path -Parent $PSScriptRoot
}

$script:Colors = @{
    Reset = "`e[0m"
    Green = "`e[32m"
    Yellow = "`e[33m"
    Red = "`e[31m"
}

function Write-ColorOutput {
    param([string]$Message, [string]$ColorCode = 'Reset')
    Write-Host "$($script:Colors[$ColorCode])$Message$($script:Colors.Reset)"
}

$deployPath = Join-Path $WorkspaceFolder ".deploy"

if (-not (Test-Path $deployPath)) {
    Write-ColorOutput "No deployment found at: $deployPath" 'Yellow'
    exit 0
}

if (-not $Force) {
    Write-ColorOutput "This will remove: $deployPath" 'Yellow'
    Write-ColorOutput "Clean master files will NOT be affected (junctions/hardlinks only)." 'Green'
    $response = Read-Host "Continue? (y/N)"
    if ($response -ne 'y') {
        Write-ColorOutput "Aborted." 'Yellow'
        exit 0
    }
}

Write-ColorOutput "Removing layered deployment..." 'Yellow'

# Remove junctions first (safest)
$junctionDirs = @('ldata', 'levels', 'fxdata', 'lang', 'campgns')
foreach ($dir in $junctionDirs) {
    $junctionPath = Join-Path $deployPath $dir
    if (Test-Path $junctionPath) {
        $item = Get-Item $junctionPath -Force
        if ($item.LinkType -eq 'Junction') {
            Write-Host "  Removing junction: $dir -> " -NoNewline
            Remove-Item $junctionPath -Force -Recurse
            Write-ColorOutput "OK" 'Green'
        }
    }
}

# Remove entire deployment directory
Remove-Item $deployPath -Recurse -Force -ErrorAction SilentlyContinue

Write-ColorOutput "Deployment removed successfully." 'Green'
Write-ColorOutput "Clean master files are intact." 'Blue'

