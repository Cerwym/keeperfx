param(
    [Parameter(Mandatory=$true)]
    [string]$WorkspaceFolder,
    [string]$DkInstallPath
)

$ErrorActionPreference = "Stop"

function Assert-Command {
    param([string]$Name)
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command '$Name' was not found in PATH."
    }
}

Assert-Command docker

$workspace = (Resolve-Path $WorkspaceFolder).Path
$deployDir = Join-Path $workspace ".deploy"
$deployData = Join-Path $deployDir "data"
$deploySound = Join-Path $deployDir "sound"
$composeFile = Join-Path $workspace "docker/compose.yml"
$initDkScript = Join-Path $workspace ".vscode/init_dk_layer.ps1"

if (-not (Test-Path $composeFile)) {
    throw "Compose file not found: $composeFile"
}
if (-not (Test-Path $initDkScript)) {
    throw "Script not found: $initDkScript"
}

Write-Host "Ensuring local DK originals layer exists..." -ForegroundColor Cyan
if ($DkInstallPath) {
    & $initDkScript -WorkspaceFolder $workspace -DkInstallPath $DkInstallPath
} else {
    & $initDkScript -WorkspaceFolder $workspace
}
if ($LASTEXITCODE -ne 0) {
    throw "Failed to initialize DK layer image."
}

Write-Host "Creating .deploy scaffold..." -ForegroundColor Cyan
New-Item -ItemType Directory -Force -Path $deployData | Out-Null
New-Item -ItemType Directory -Force -Path $deploySound | Out-Null

$dkStage = Join-Path $workspace ".local/dk-stage"
$dkConfigPath = Join-Path $workspace ".local/dk-install-path.txt"
if ((Test-Path (Join-Path $dkStage "data")) -and (Test-Path (Join-Path $dkStage "sound"))) {
    Write-Host "Copying DK originals from normalized local stage..." -ForegroundColor Cyan
    Copy-Item (Join-Path $dkStage "data/*") $deployData -Recurse -Force
    Copy-Item (Join-Path $dkStage "sound/*") $deploySound -Recurse -Force
} elseif (Test-Path $dkConfigPath) {
    $dkRoot = (Get-Content $dkConfigPath -Raw).Trim()
    if ([string]::IsNullOrWhiteSpace($dkRoot) -or -not (Test-Path $dkRoot)) {
        throw "Cached DK install path is invalid; rerun Init DK Originals Layer."
    }
    Write-Host "Copying DK originals directly from cached install path..." -ForegroundColor Cyan
    Copy-Item (Join-Path $dkRoot "data/*") $deployData -Recurse -Force
    Copy-Item (Join-Path $dkRoot "sound/*") $deploySound -Recurse -Force
} else {
    throw "No DK source found. Rerun Init DK Originals Layer."
}

Write-Host "Building KeeperFX asset packages (pkg-gfx/pkg-languages/pkg-sfx)..." -ForegroundColor Cyan
docker compose -f "$composeFile" run --rm linux bash -lc "make -j1 pkg-gfx && make -j1 pkg-languages && make -j1 pkg-sfx"
if ($LASTEXITCODE -ne 0) {
    throw "Asset packaging failed."
}

Write-Host "Staging generated pkg assets into .deploy..." -ForegroundColor Cyan
$pkgData = Join-Path $workspace "pkg/data"
$pkgLdata = Join-Path $workspace "pkg/ldata"
$pkgSound = Join-Path $workspace "pkg/sound"

if (Test-Path $pkgData) {
    Copy-Item "$pkgData\*" $deployData -Recurse -Force
}
if (Test-Path $pkgLdata) {
    Copy-Item "$pkgLdata\*" $deployData -Recurse -Force
}
if (Test-Path $pkgSound) {
    Copy-Item "$pkgSound\*" $deploySound -Recurse -Force
}

Write-Host ".deploy is initialized and layered for host runtime testing." -ForegroundColor Green
