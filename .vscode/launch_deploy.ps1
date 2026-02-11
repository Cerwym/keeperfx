#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Launches KeeperFX from the layered deployment directory.

.DESCRIPTION
    Starts keeperfx.exe from .deploy/ directory with proper working directory.

.EXAMPLE
    .\launch_deploy.ps1
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory=$false)]
    [string]$WorkspaceFolder = $PSScriptRoot | Split-Path -Parent
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$deployPath = Join-Path $WorkspaceFolder ".deploy"
$exePath = Join-Path $deployPath "keeperfx.exe"

if (-not (Test-Path $deployPath)) {
    Write-Host "ERROR: Deployment not found at: $deployPath" -ForegroundColor Red
    Write-Host "Run init_layered_deploy.ps1 first" -ForegroundColor Yellow
    exit 1
}

if (-not (Test-Path $exePath)) {
    Write-Host "ERROR: keeperfx.exe not found at: $exePath" -ForegroundColor Red
    Write-Host "Build and deploy first with: wsl make && .\.vscode\deploy_assets.ps1 -All" -ForegroundColor Yellow
    exit 1
}

Write-Host "Launching KeeperFX from deployment..." -ForegroundColor Green
Write-Host "Working directory: $deployPath" -ForegroundColor Blue

# Launch in deployment directory
Push-Location $deployPath
try {
    & .\keeperfx.exe
} finally {
    Pop-Location
}
