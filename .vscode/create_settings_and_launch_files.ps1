# Called on folderOpen. Creates settings.json and launch.json from templates (silent, no prompts).
Param( $workspaceFolder, $templateSettingsFile, $templateLaunchFile, $settingsJsonFile, $launchJsonFile )

$workspaceFolder      = $workspaceFolder.Replace('\', '/')
$templateSettingsFile = $templateSettingsFile.Replace('\', '/')
$templateLaunchFile   = $templateLaunchFile.Replace('\', '/')
$settingsJsonFile     = $settingsJsonFile.Replace('\', '/')
$launchJsonFile       = $launchJsonFile.Replace('\', '/')
$driveLetter          = (Split-Path -Path $workspaceFolder -Qualifier).Split(":").Get(0)
$driveLetterLower     = $driveLetter.ToLower()

Write-Host "workspaceFolder: '$workspaceFolder'" -ForegroundColor DarkGray

if (-not (Test-Path $workspaceFolder)) {
    Write-Host "Invalid workspaceFolder '$workspaceFolder'." -ForegroundColor Red; exit
}
if (-not (Test-Path $templateSettingsFile)) {
    Write-Host "Invalid templateSettingsFile '$templateSettingsFile'." -ForegroundColor Red; exit
}
if (-not (Test-Path $templateLaunchFile)) {
    Write-Host "Invalid templateLaunchFile '$templateLaunchFile'." -ForegroundColor Red; exit
}

# ── settings.json ─────────────────────────────────────────────────────────────
if (Test-Path $settingsJsonFile) {
    Write-Host "settings.json already exists — skipping" -ForegroundColor DarkGray
} else {
    Copy-Item $templateSettingsFile $settingsJsonFile
    Write-Host "settings.json created from template." -ForegroundColor White
}

# ── launch.json ───────────────────────────────────────────────────────────────
if (Test-Path $launchJsonFile) {
    Write-Host "launch.json already exists — skipping" -ForegroundColor DarkGray
} else {
    # Patch sourceFileMap drive letter from template, then write
    $newLaunchFileContents = Get-Content $templateLaunchFile
    $sourceFileMapRegexReplace = '(\/\/)(\"sourceFileMap\"\s*:\s*{\s*\"\/mnt\/)(.*?)(\/\"\s*:\s*\")(.*?)(:\/\")'
    $newLaunchFileContents = ($newLaunchFileContents) -replace $sourceFileMapRegexReplace, ('$2{0}$4{1}$6' -f "${driveLetterLower}", "${driveLetter}")
    [System.IO.File]::WriteAllLines($launchJsonFile, $newLaunchFileContents)
    Write-Host "launch.json created from template." -ForegroundColor White
}

# ── Hint: one-time DK image setup (only if image is missing) ─────────────────
$imageCheck = docker image inspect "keeperfx-dk-originals:local" 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "┌─────────────────────────────────────────────────────────┐" -ForegroundColor Yellow
    Write-Host "│  One-time setup required: DK original files             │" -ForegroundColor Yellow
    Write-Host "│  Run: .\.vscode\init_dk_layer.ps1                       │" -ForegroundColor Yellow
    Write-Host "│  Then: .\.vscode\init_layered_deploy.ps1                │" -ForegroundColor Yellow
    Write-Host "└─────────────────────────────────────────────────────────┘" -ForegroundColor Yellow
} elseif (-not (Test-Path (Join-Path $workspaceFolder ".deploy"))) {
    Write-Host ""
    Write-Host "DK layer image found. Run '.\.vscode\init_layered_deploy.ps1' to set up .deploy/." -ForegroundColor Cyan
} else {
    Write-Host "Workspace ready. Press Ctrl+Shift+B to build." -ForegroundColor Green
}
