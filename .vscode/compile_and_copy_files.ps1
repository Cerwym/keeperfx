#!/usr/bin/env pwsh
# Called by the "Compile, Copy Files" VS Code task (triggered by launch.json F5 debugger).
# Builds keeperfx.exe via Docker and deploys it to .deploy/.

Param(
    [string]$workspaceFolder,
    [string]$launchJsonFile,
    [string]$compileSettingsFile
)

Write-Host "workspaceFolder:    '$workspaceFolder'"    -ForegroundColor DarkGray
Write-Host "launchJsonFile:     '$launchJsonFile'"     -ForegroundColor DarkGray
Write-Host "compileSettingsFile:'$compileSettingsFile'" -ForegroundColor DarkGray

if (-not (Test-Path $workspaceFolder))    { Write-Host "Invalid workspaceFolder." -ForegroundColor Red; exit 1 }
if (-not (Test-Path $launchJsonFile))     { Write-Host "Invalid launchJsonFile."  -ForegroundColor Red; exit 1 }
if (-not (Test-Path $compileSettingsFile)){ Write-Host "Invalid compileSettingsFile." -ForegroundColor Red; exit 1 }

# ── Parse debug / package-suffix flags ───────────────────────────────────────
$debugFlag      = 'DEBUG=0'
$debugFlagFTest = 'FTEST_DEBUG=0'
$cmakeExtraArgs = @()

$compileSetting = (Get-Content "$compileSettingsFile" -Raw).Trim()

if ($compileSetting -like '*DEBUG=1*') {
    $debugFlag = 'DEBUG=1'
    $cmakeExtraArgs += '-DDEBUG=1'
    Write-Host 'Compiling with DEBUG=1' -ForegroundColor Yellow
} else {
    Write-Host 'Compiling with DEBUG=0' -ForegroundColor Green
}

if ($compileSetting -like '*FTEST_DEBUG=1*') {
    $debugFlagFTest = 'FTEST_DEBUG=1'
    $cmakeExtraArgs += '-DFTEST_DEBUG=1'
    Write-Host 'Compiling with FTEST_DEBUG=1' -ForegroundColor Magenta
}

# ── Build via Docker ──────────────────────────────────────────────────────────
$PRESET  = 'windows-x86-release'
$COMPOSE = Join-Path $workspaceFolder 'docker\compose.yml'

$cmakeConfigArgs = "cmake --preset $PRESET" + (($cmakeExtraArgs | ForEach-Object { " $_" }) -join '')
$cmakeBuildArgs  = "cmake --build --preset $PRESET"

Write-Host ""
Write-Host "Building via Docker (preset: $PRESET)..." -ForegroundColor Cyan

docker compose -f $COMPOSE run --rm mingw32 bash -c "$cmakeConfigArgs && $cmakeBuildArgs"

if ($LASTEXITCODE -ne 0) {
    Write-Host "Compilation failed!" -ForegroundColor Red
    exit 1
}

Write-Host "Compilation successful!" -ForegroundColor Green

# ── Deploy to .deploy/ ────────────────────────────────────────────────────────
$deployPath = Join-Path $workspaceFolder ".deploy"

if (-not (Test-Path $deployPath)) {
    Write-Host "Initializing layered deployment (.deploy/)..." -ForegroundColor Cyan
    & "$PSScriptRoot\init_layered_deploy.ps1" -WorkspaceFolder $workspaceFolder
    if ($LASTEXITCODE -ne 0) { Write-Host "Deployment init failed!" -ForegroundColor Red; exit 1 }
}

$exeSrc = Join-Path $workspaceFolder "out\build\$PRESET\keeperfx.exe"
if (Test-Path $exeSrc) {
    Copy-Item $exeSrc (Join-Path $deployPath "keeperfx.exe") -Force
    Write-Host "Deployed: keeperfx.exe" -ForegroundColor Green
} else {
    Write-Host "WARNING: keeperfx.exe not found at expected path: $exeSrc" -ForegroundColor Yellow
}

$hvlogSrc = Join-Path $workspaceFolder "out\build\$PRESET\keeperfx_hvlog.exe"
if (Test-Path $hvlogSrc) {
    Copy-Item $hvlogSrc (Join-Path $deployPath "keeperfx_hvlog.exe") -Force
    Write-Host "Deployed: keeperfx_hvlog.exe" -ForegroundColor DarkGray
}

Write-Host "Deployment complete!" -ForegroundColor Green
