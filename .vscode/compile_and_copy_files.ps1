# receive params for script (must be first line in file, other than comments)
Param( $workspaceFolder, $launchJsonFile, $compileSettingsFile )

# show param inputs
Write-Host "workspaceFolder: '$workspaceFolder'" -ForegroundColor DarkGray
Write-Host "launchJsonFile: '$launchJsonFile'" -ForegroundColor DarkGray
Write-Host "compileSettingsFile: '$compileSettingsFile'" -ForegroundColor DarkGray

# validate param inputs
if( -not (Test-Path $workspaceFolder))
{
    Write-Host "Invalid workspaceFolder '$workspaceFolder'. Something went wrong." -ForegroundColor Red;
    exit;
}
if( -not (Test-Path $launchJsonFile))
{
    Write-Host "Invalid launchJsonFile '$launchJsonFile'. Something went wrong." -ForegroundColor Red;
    exit;
}
if( -not (Test-Path $compileSettingsFile))
{
    Write-Host "Invalid compileSettingsFile '$compileSettingsFile'. Something went wrong." -ForegroundColor Red;
    exit;
}

# inform user of relevant information
Write-Host ('Source Code directory: ' + "${workspaceFolder}".Replace('\\', '/')) -ForegroundColor White;

# grab game directory via regex
$regexPattern = '\"cwd\"\s*:\s*\"(.*?)\"';
Write-Host "regexPattern: '$regexPattern'" -ForegroundColor DarkGray;

$regexResult = (Get-Content -Raw "$launchJsonFile" | Select-String -Pattern $regexPattern);

if( -not $regexResult.Matches -or $regexResult.Matches.Length -le 0 -or -not $regexResult.Matches.Groups -or $regexResult.Matches.Groups.Length -le 1 )
{
    Write-Host "The current working directory `"cwd`" could not be found in '$launchJsonFile', please edit the file and update it." -ForegroundColor Red;
    Write-Host "Example: `"cwd`": `"D:/Games/DungeonKeeper/`" (be sure to use forward slashes)." -ForegroundColor Red;
    exit;
}

$gameDir = $regexResult.Matches[0].Groups[1].Value.Replace('\\', '/');

# Expand VS Code variables
$gameDir = $gameDir.Replace('${workspaceFolder}', $workspaceFolder.Replace('\', '/'));

Write-Host "Found current working directory (cwd): '$gameDir' in '$launchJsonFile'" -ForegroundColor White;
if( -not (Test-Path $gameDir) )
{
    Write-Host "Directory '$gameDir' invalid, make sure it exists on-disk and the path is spelled correctly." -ForegroundColor Red;
    Write-Host "Example: `"cwd`": `"D:/Games/DungeonKeeper/`" (be sure to use forward slashes)." -ForegroundColor Red;
    exit;
}
else
{
    Write-Host "Directory '$gameDir' valid, exists on-disk" -ForegroundColor DarkGray;
}

$debugFlag      = 'DEBUG=0';
$debugFlagFTest = 'FTEST_DEBUG=0';
$packageSuffix  = '';

$compileSetting = (Get-Content "$compileSettingsFile" -Raw).Trim();
if ($compileSetting -like '*DEBUG=1*')
{
    $debugFlag = 'DEBUG=1';
}
if ($compileSetting -like '*FTEST_DEBUG=1*')
{
    $debugFlagFTest = 'FTEST_DEBUG=1';
}
if ($compileSetting -match 'PACKAGE_SUFFIX\s*=\s*(.+)')
{
    $customSuffix = $Matches[1].Trim();
    $branchName = wsl git rev-parse --abbrev-ref HEAD 2>$null;
    if ($branchName) {
        $branchName = $branchName.Trim();
        # Replace forward slashes and other special characters with hyphens
        $branchName = $branchName -replace '[/\\:]', '-';
        $packageSuffix = "PACKAGE_SUFFIX=$customSuffix-$branchName";
    } else {
        $packageSuffix = "PACKAGE_SUFFIX=$customSuffix";
    }
}

if ($debugFlag -eq 'DEBUG=1')
{
    Write-Host 'Compiling with DEBUG=1' -ForegroundColor Yellow;
}
else
{
    Write-Host 'Compiling with DEBUG=0' -ForegroundColor Green;
}

if ($debugFlagFTest -eq 'FTEST_DEBUG=1')
{
    Write-Host 'Compiling with FTEST_DEBUG=1' -ForegroundColor Magenta;
}
if ($packageSuffix)
{
    Write-Host "Compiling with $packageSuffix" -ForegroundColor Cyan;
}
wsl make all -j`nproc` $debugFlag $debugFlagFTest $packageSuffix;
if ($?) {
    Write-Host 'Compilation successful!' -ForegroundColor Green;
}
else
{
    Write-Host 'Compilation failed!' -ForegroundColor Red;
    exit 1;
}

# Deploy to .deploy/ directory instead of copying to game directory
$deployPath = Join-Path $workspaceFolder ".deploy"

# Initialize .deploy/ if it doesn't exist
if (-not (Test-Path $deployPath)) {
    Write-Host 'Initializing layered deployment (.deploy/)...' -ForegroundColor Cyan;
    $initScript = Join-Path $workspaceFolder ".vscode\init_layered_deploy.ps1"
    if (Test-Path $initScript) {
        & $initScript
        if (-not $?) {
            Write-Host 'Failed to initialize deployment!' -ForegroundColor Red;
            exit 1;
        }
    } else {
        Write-Host 'ERROR: init_layered_deploy.ps1 not found!' -ForegroundColor Red;
        exit 1;
    }
}

# Deploy compiled assets
Write-Host 'Deploying assets to .deploy/...' -ForegroundColor Cyan;
Copy-Item -Path "${workspaceFolder}\\bin\\keeperfx.exe" -Destination $deployPath -Force;
Copy-Item -Path "${workspaceFolder}\\bin\\*.dll" -Destination $deployPath -Force -ErrorAction SilentlyContinue;

# Deploy config files to root (keeperfx expects them there, not in config/)
Write-Host 'Deploying config files to root...' -ForegroundColor Cyan;

# Copy keeperfx.cfg to root
$sourceConfigFile = Join-Path $workspaceFolder "config\\keeperfx.cfg"
if (Test-Path $sourceConfigFile) {
    Copy-Item -Path $sourceConfigFile -Destination (Join-Path $deployPath "keeperfx.cfg") -Force
    Write-Host '  keeperfx.cfg deployed to root' -ForegroundColor DarkGray
}

# Copy mods/ directory to root
$sourceModsPath = Join-Path $workspaceFolder "config\\mods"
$targetModsPath = Join-Path $deployPath "mods"
if (Test-Path $sourceModsPath) {
    Copy-Item -Path $sourceModsPath -Destination $targetModsPath -Recurse -Force
    
    # Create load_order.cfg if only _load_order.cfg exists (disabled by default)
    $loadOrderFile = Join-Path $targetModsPath "load_order.cfg"
    $disabledLoadOrder = Join-Path $targetModsPath "_load_order.cfg"
    if (-not (Test-Path $loadOrderFile) -and (Test-Path $disabledLoadOrder)) {
        Copy-Item -Path $disabledLoadOrder -Destination $loadOrderFile -Force
        Write-Host '  Created load_order.cfg from template' -ForegroundColor DarkGray
    }
    
    Write-Host '  mods/ deployed to root' -ForegroundColor DarkGray
}

# Copy creatrs/ directory to root
$sourceCreatrsPath = Join-Path $workspaceFolder "config\\creatrs"
$targetCreatrsPath = Join-Path $deployPath "creatrs"
if (Test-Path $sourceCreatrsPath) {
    Copy-Item -Path $sourceCreatrsPath -Destination $targetCreatrsPath -Recurse -Force
    Write-Host '  creatrs/ deployed to root' -ForegroundColor DarkGray
}

Write-Host 'Deployment complete!' -ForegroundColor Green;
