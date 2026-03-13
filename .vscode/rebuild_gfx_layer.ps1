param(
    [Parameter(Mandatory=$true)]
    [string]$WorkspaceFolder
)

$ErrorActionPreference = "Stop"

$workspace = (Resolve-Path $WorkspaceFolder).Path
$deployDir = Join-Path $workspace ".deploy"
$deployData = Join-Path $deployDir "data"
$composeFile = Join-Path $workspace "docker/compose.yml"

if (-not (Test-Path $deployDir)) {
    throw ".deploy does not exist. Run 'Init .deploy/' first."
}
if (-not (Test-Path $composeFile)) {
    throw "Compose file not found: $composeFile"
}

Write-Host "Rebuilding gfx package in linux container..." -ForegroundColor Cyan
docker compose -f "$composeFile" run --rm linux bash -lc "make pkg-gfx"
if ($LASTEXITCODE -ne 0) {
    throw "make pkg-gfx failed."
}

$pkgData = Join-Path $workspace "pkg/data"
$pkgLdata = Join-Path $workspace "pkg/ldata"

if (-not (Test-Path $pkgData) -and -not (Test-Path $pkgLdata)) {
    throw "No gfx package output found under pkg/data or pkg/ldata."
}

New-Item -ItemType Directory -Force -Path $deployData | Out-Null
if (Test-Path $pkgData) {
    Copy-Item "$pkgData\*" $deployData -Recurse -Force
}
if (Test-Path $pkgLdata) {
    Copy-Item "$pkgLdata\*" $deployData -Recurse -Force
}

Write-Host "GFX layer refreshed in .deploy/data." -ForegroundColor Green
