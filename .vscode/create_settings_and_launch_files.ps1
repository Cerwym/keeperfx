# receive params for script (must be first line in file, other than comments)
Param( $workspaceFolder, $templateSettingsFile, $templateLaunchFile, $settingsJsonFile, $launchJsonFile )

# sanitize inputs
$workspaceFolder = $workspaceFolder.Replace('\', '/')
$templateSettingsFile = $templateSettingsFile.Replace('\', '/')
$templateLaunchFile = $templateLaunchFile.Replace('\', '/')
$settingsJsonFile = $settingsJsonFile.Replace('\', '/')
$launchJsonFile = $launchJsonFile.Replace('\', '/')
$driveLetter = (Split-Path -Path $workspaceFolder -Qualifier).Split(":").Get(0)
$driveLetterLower = $driveLetter.ToLower()

# show param inputs
Write-Host "workspaceFolder: '$workspaceFolder'" -ForegroundColor DarkGray
Write-Host "templateSettingsFile: '$templateSettingsFile'" -ForegroundColor DarkGray
Write-Host "templateLaunchFile: '$templateLaunchFile'" -ForegroundColor DarkGray
Write-Host "settingsJsonFile: '$settingsJsonFile'" -ForegroundColor DarkGray
Write-Host "launchJsonFile: '$launchJsonFile'" -ForegroundColor DarkGray

# validate param inputs
if( -not (Test-Path $workspaceFolder))
{
    Write-Host "Invalid workspaceFolder '$workspaceFolder'. Something went wrong." -ForegroundColor Red
    exit
}
if( -not (Test-Path $templateSettingsFile))
{
    Write-Host "Invalid templateSettingsFile '$templateSettingsFile'. Something went wrong." -ForegroundColor Red
    exit
}
if( -not (Test-Path $templateLaunchFile))
{
    Write-Host "Invalid templateLaunchFile '$templateLaunchFile'. Something went wrong." -ForegroundColor Red
    exit
}

# inform user of relevant information
Write-Host ('Source Code directory: ' + "${workspaceFolder}") -ForegroundColor White

# import windows gui stuff
Add-Type -AssemblyName System.Windows.Forms

# check for / copy settings.json
if( Test-Path $settingsJsonFile )
{
    Write-Host ("settingsJsonFile '$settingsJsonFile' exists, skipping generation") -ForegroundColor DarkGray
}
else
{
    Copy-Item "${templateSettingsFile}" $settingsJsonFile
    Write-Host ("settingsJsonFile '$settingsJsonFile' was created.") -ForegroundColor White
}

# check for / generate / copy launch.json
if( Test-Path $launchJsonFile )
{
    Write-Host ("launchJsonFile '$launchJsonFile' exists, skipping generation") -ForegroundColor DarkGray
}
else
{
    Write-Host "Generating launch.json from template..." -ForegroundColor White
    
    # With layered deployment, use template as-is (already points to .deploy)
    $newLaunchFileContents = Get-Content ${templateLaunchFile}
    
    # Update sourceFileMap for the current drive
    $sourceFileMapRegexReplace = '(\/\/)(\"sourceFileMap\"\s*:\s*{\s*\"\/mnt\/)(.*?)(\/\"\s*:\s*\")(.*?)(:\/\")'
    $newLaunchFileContents = ($newLaunchFileContents) -replace $sourceFileMapRegexReplace, ('$2{0}$4{1}$6' -f "${driveLetterLower}", "${driveLetter}")
    
    [System.IO.File]::WriteAllLines( ${launchJsonFile}, $newLaunchFileContents)
    Write-Host ("launchJsonFile '$launchJsonFile' was created.") -ForegroundColor White
}

# ============================================================================
# Layered Deployment Setup (for new worktrees)
# ============================================================================

Write-Host "`n=== Checking Layered Deployment Setup ===" -ForegroundColor Cyan

# Check if settings.json has clean master path configured
$settingsContent = @{}
if (Test-Path $settingsJsonFile) {
    try {
        $settingsContent = Get-Content $settingsJsonFile -Raw | ConvertFrom-Json -AsHashtable
    } catch {
        Write-Host "Warning: Could not parse settings.json" -ForegroundColor Yellow
    }
}

$cleanMasterPath = $settingsContent.'keeperfx.cleanMasterPath'
$deployPath = Join-Path $workspaceFolder ".deploy"

# Step 1: Ensure clean master is configured
if (-not $cleanMasterPath -or -not (Test-Path $cleanMasterPath)) {
    Write-Host "`nClean master installation not configured." -ForegroundColor Yellow
    Write-Host "The layered file system requires a clean KeeperFX installation as a base." -ForegroundColor White
    Write-Host "`nPlease provide the path to your clean KeeperFX installation:" -ForegroundColor White
    Write-Host "(e.g., from GOG, Steam, or EA)" -ForegroundColor DarkGray
    
    # Prompt for clean master path
    $FolderBrowser = New-Object System.Windows.Forms.FolderBrowserDialog -Property @{
        Description = 'Select your clean KeeperFX installation folder (containing keeperfx.exe)'
        ShowNewFolderButton = $false
    }
    
    $folderResult = $FolderBrowser.ShowDialog((New-Object System.Windows.Forms.Form -Property @{TopMost = $true }))
    
    if ($folderResult -eq "OK") {
        $cleanMasterPath = $FolderBrowser.SelectedPath.Replace('\', '/')
        
        # Validate it has required files
        $requiredFiles = @('keeperfx.exe', 'data', 'ldata', 'levels')
        $isValid = $true
        foreach ($file in $requiredFiles) {
            if (-not (Test-Path (Join-Path $cleanMasterPath $file))) {
                $isValid = $false
                Write-Host "ERROR: Missing required file/folder: $file" -ForegroundColor Red
                break
            }
        }
        
        if ($isValid) {
            # Save to settings.json
            if (-not $settingsContent) {
                $settingsContent = @{}
            }
            $settingsContent.'keeperfx.cleanMasterPath' = $cleanMasterPath
            $settingsContent | ConvertTo-Json -Depth 10 | Set-Content $settingsJsonFile -Encoding UTF8
            Write-Host "Clean master configured: $cleanMasterPath" -ForegroundColor Green
        } else {
            Write-Host "Invalid clean master installation. Skipping deployment setup." -ForegroundColor Red
            Write-Host "You can run '.\.vscode\setup_clean_master.ps1' manually later." -ForegroundColor Yellow
            exit
        }
    } else {
        Write-Host "Clean master selection cancelled. Skipping deployment setup." -ForegroundColor Yellow
        Write-Host "You can run '.\.vscode\setup_clean_master.ps1' manually later." -ForegroundColor Yellow
        exit
    }
}

# Step 2: Initialize .deploy directory if needed
if (-not (Test-Path $deployPath)) {
    Write-Host "`nInitializing layered deployment (.deploy/)..." -ForegroundColor Cyan
    Write-Host "This creates junctions and hard links to save ~480MB of disk space." -ForegroundColor White
    
    $initScript = Join-Path (Split-Path $settingsJsonFile -Parent) "init_layered_deploy.ps1"
    if (Test-Path $initScript) {
        & $initScript -CleanMasterPath $cleanMasterPath -WorkspaceFolder $workspaceFolder
        
        if ($LASTEXITCODE -eq 0 -and (Test-Path $deployPath)) {
            Write-Host "`nLayered deployment ready!" -ForegroundColor Green
        } else {
            Write-Host "Failed to initialize deployment. You may need to run init_layered_deploy.ps1 manually." -ForegroundColor Red
        }
    } else {
        Write-Host "ERROR: init_layered_deploy.ps1 not found!" -ForegroundColor Red
    }
} else {
    Write-Host ".deploy directory already exists - skipping initialization" -ForegroundColor DarkGray
}

# Step 3: Check if GDB is installed for debugging
Write-Host "`nChecking debugging setup..." -ForegroundColor Cyan
try {
    $gdbCheck = wsl which i686-w64-mingw32-gdb 2>&1
    if ($LASTEXITCODE -eq 0 -and $gdbCheck) {
        Write-Host "GDB debugger found in WSL - breakpoint debugging enabled!" -ForegroundColor Green
    } else {
        Write-Host "GDB not found - Install for breakpoint debugging:" -ForegroundColor Yellow
        Write-Host "  Run: wsl sudo apt install gdb-mingw-w64" -ForegroundColor Yellow
    }
} catch {
    Write-Host "Could not check for GDB (install with: wsl sudo apt install gdb-mingw-w64)" -ForegroundColor DarkGray
}

# Check debug symbols setting
$compileSettings = Join-Path (Split-Path $settingsJsonFile -Parent) "compile_settings.cfg"
if (Test-Path $compileSettings) {
    $content = Get-Content $compileSettings -Raw
    if ($content -match "DEBUG\s*=\s*1") {
        Write-Host "DEBUG=1 enabled - debug symbols will be included" -ForegroundColor Green
    } else {
        Write-Host "DEBUG=0 - No debug symbols (set DEBUG=1 in compile_settings.cfg for breakpoints)" -ForegroundColor DarkGray
    }
}

# Step 4: Configure SSH for git submodules (1Password integration)
Write-Host "`nConfiguring SSH for git operations..." -ForegroundColor Cyan
try {
    # Check if core.sshCommand is set for git
    $gitSshCommand = wsl git config --global --get core.sshCommand 2>&1
    if ($gitSshCommand -eq "ssh.exe") {
        Write-Host "Git already configured to use Windows SSH (1Password integration)" -ForegroundColor Green
    } else {
        # Test if Windows SSH agent is accessible from WSL
        $sshTest = wsl ssh-add.exe -l 2>&1
        if ($LASTEXITCODE -eq 0) {
            # Configure git to use ssh.exe
            wsl git config --global core.sshCommand ssh.exe
            Write-Host "Configured Git to use ssh.exe for 1Password SSH agent" -ForegroundColor Green
        } else {
            Write-Host "1Password SSH agent not accessible from WSL" -ForegroundColor Yellow
            Write-Host "  See .vscode/SSH_SETUP.md for setup instructions" -ForegroundColor Yellow
        }
    }
} catch {
    Write-Host "Could not check SSH configuration" -ForegroundColor DarkGray
}

Write-Host "`n=== Workspace Setup Complete ===" -ForegroundColor Cyan
