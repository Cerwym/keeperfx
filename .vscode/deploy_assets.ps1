#!/usr/bin/env pwsh
# KeeperFX Asset Deployment Script
# Deploys compiled assets to layered deployment directory (.deploy/)
#
# Usage Examples:
#   .\.vscode\deploy_assets.ps1 -All
#   .\.vscode\deploy_assets.ps1 -DeployGraphics -DeploySounds
#   .\.vscode\deploy_assets.ps1 -DeployExecutable

param(
    [string]$GameDirectory = (Join-Path $PSScriptRoot ".." ".deploy"),
    [switch]$DeployExecutable,
    [switch]$DeployGraphics,
    [switch]$DeploySounds,
    [switch]$DeployLocalization,
    [switch]$DeployConfig,
    [switch]$All,
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"

if ($All) {
    $DeployExecutable = $true
    $DeployGraphics = $true
    $DeploySounds = $true
    $DeployLocalization = $true
    $DeployConfig = $true
}

Write-Host ""
Write-Host "=" * 80 -ForegroundColor Cyan
Write-Host "🚀 KeeperFX Asset Deployment (Layered)" -ForegroundColor Cyan
Write-Host "=" * 80 -ForegroundColor Cyan
Write-Host ""
Write-Host "Target: $GameDirectory" -ForegroundColor Yellow
Write-Host ""

# Verify deployment directory exists
if (-not (Test-Path $GameDirectory)) {
    Write-Host "❌ Error: Deployment directory not found" -ForegroundColor Red
    Write-Host "   Path: $GameDirectory" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Run .\.vscode\init_layered_deploy.ps1 first to create deployment" -ForegroundColor Yellow
    exit 1
}

$deployedCount = 0

# ============================================================================
# Deploy Executable
# ============================================================================

if ($DeployExecutable) {
    Write-Host "[1/5] 📦 Deploying Executable" -ForegroundColor Green
    Write-Host ""
    
    $exePath = "bin\keeperfx.exe"
    if (Test-Path $exePath) {
        Copy-Item $exePath "$GameDirectory\" -Force
        Write-Host "  ✓ keeperfx.exe" -ForegroundColor Green
        $deployedCount++
    } else {
        Write-Host "  ⚠️  keeperfx.exe not found (compile first?)" -ForegroundColor Yellow
    }
    Write-Host ""
}

# ============================================================================
# Deploy Graphics
# ============================================================================

if ($DeployGraphics) {
    Write-Host "[2/5] 🎨 Deploying Graphics" -ForegroundColor Green
    Write-Host ""
    
    # Ensure target directories exist
    $dataDir = Join-Path $GameDirectory "data"
    $ldataDir = Join-Path $GameDirectory "ldata"
    
    if (-not (Test-Path $dataDir)) { New-Item -ItemType Directory -Path $dataDir | Out-Null }
    if (-not (Test-Path $ldataDir)) { New-Item -ItemType Directory -Path $ldataDir | Out-Null }
    
    # GUI sprites (gui2-64.dat)
    $guiPath = "pkg\data\gui2-64.dat"
    if (Test-Path $guiPath) {
        Copy-Item $guiPath "$dataDir\" -Force
        Write-Host "  ✓ gui2-64.dat" -ForegroundColor Green
        $deployedCount++
    } else {
        Write-Host "  ⚠️  gui2-64.dat not found" -ForegroundColor Yellow
    }
    
    # Engine textures (tmap*.dat)
    $tmapFiles = Get-ChildItem "pkg\data\tmap*.dat" -ErrorAction SilentlyContinue
    if ($tmapFiles) {
        foreach ($file in $tmapFiles) {
            Copy-Item $file.FullName "$dataDir\" -Force
            Write-Host "  ✓ $($file.Name)" -ForegroundColor Green
            $deployedCount++
        }
    } else {
        Write-Host "  ⚠️  No tmap*.dat files found" -ForegroundColor Yellow
    }
    
    # Land views (ldata/*.dat)
    $ldataFiles = Get-ChildItem "pkg\ldata\*.dat" -ErrorAction SilentlyContinue
    if ($ldataFiles) {
        foreach ($file in $ldataFiles) {
            Copy-Item $file.FullName "$ldataDir\" -Force
            if ($Verbose) {
                Write-Host "  ✓ ldata\$($file.Name)" -ForegroundColor Green
            }
            $deployedCount++
        }
        if (-not $Verbose) {
            Write-Host "  ✓ $($ldataFiles.Count) land view files" -ForegroundColor Green
        }
    } else {
        Write-Host "  ⚠️  No land view files found" -ForegroundColor Yellow
    }
    
    Write-Host ""
}

# ============================================================================
# Deploy Sounds
# ============================================================================

if ($DeploySounds) {
    Write-Host "[3/5] 🔊 Deploying Sounds" -ForegroundColor Green
    Write-Host ""
    
    $soundPath = "pkg\data\sound.dat"
    if (Test-Path $soundPath) {
        $dataDir = Join-Path $GameDirectory "data"
        if (-not (Test-Path $dataDir)) { New-Item -ItemType Directory -Path $dataDir | Out-Null }
        
        Copy-Item $soundPath "$dataDir\" -Force
        Write-Host "  ✓ sound.dat" -ForegroundColor Green
        $deployedCount++
    } else {
        Write-Host "  ⚠️  sound.dat not found" -ForegroundColor Yellow
    }
    
    # Speech banks (speech_*.dat)
    $speechFiles = Get-ChildItem "pkg\data\speech_*.dat" -ErrorAction SilentlyContinue
    if ($speechFiles) {
        foreach ($file in $speechFiles) {
            Copy-Item $file.FullName "$dataDir\" -Force
            if ($Verbose) {
                Write-Host "  ✓ $($file.Name)" -ForegroundColor Green
            }
            $deployedCount++
        }
        if (-not $Verbose) {
            Write-Host "  ✓ $($speechFiles.Count) speech files" -ForegroundColor Green
        }
    }
    
    Write-Host ""
}

# ============================================================================
# Deploy Localization
# ============================================================================

if ($DeployLocalization) {
    Write-Host "[4/5] 🌍 Deploying Localization" -ForegroundColor Green
    Write-Host ""
    
    $langPath = "pkg\lang"
    if (Test-Path $langPath) {
        $targetLangDir = Join-Path $GameDirectory "lang"
        if (-not (Test-Path $targetLangDir)) { New-Item -ItemType Directory -Path $targetLangDir | Out-Null }
        
        $moFiles = Get-ChildItem "$langPath\*.mo" -ErrorAction SilentlyContinue
        if ($moFiles) {
            foreach ($file in $moFiles) {
                Copy-Item $file.FullName "$targetLangDir\" -Force
                if ($Verbose) {
                    Write-Host "  ✓ $($file.Name)" -ForegroundColor Green
                }
                $deployedCount++
            }
            if (-not $Verbose) {
                Write-Host "  ✓ $($moFiles.Count) translation files" -ForegroundColor Green
            }
        } else {
            Write-Host "  ⚠️  No .mo files found" -ForegroundColor Yellow
        }
    } else {
        Write-Host "  ⚠️  pkg\lang directory not found" -ForegroundColor Yellow
    }
    
    Write-Host ""
}

# ============================================================================
# Deploy Configuration
# ============================================================================

if ($DeployConfig) {
    Write-Host "[5/5] ⚙️  Deploying Configuration" -ForegroundColor Green
    Write-Host ""
    
    $fxdataDir = Join-Path $GameDirectory "config\fxdata"
    if (-not (Test-Path $fxdataDir)) { 
        New-Item -ItemType Directory -Path $fxdataDir -Force | Out-Null 
    }
    
    # terrain.cfg
    $terrainPath = "config\fxdata\terrain.cfg"
    if (Test-Path $terrainPath) {
        Copy-Item $terrainPath "$fxdataDir\" -Force
        Write-Host "  ✓ terrain.cfg" -ForegroundColor Green
        $deployedCount++
    } else {
        Write-Host "  ⚠️  terrain.cfg not found" -ForegroundColor Yellow
    }
    
    # slabset.toml
    $slabsetPath = "config\fxdata\slabset.toml"
    if (Test-Path $slabsetPath) {
        Copy-Item $slabsetPath "$fxdataDir\" -Force
        Write-Host "  ✓ slabset.toml" -ForegroundColor Green
        $deployedCount++
    } else {
        Write-Host "  ⚠️  slabset.toml not found" -ForegroundColor Yellow
    }
    
    # keeperfx.cfg (main config)
    $mainConfigPath = "config\keeperfx.cfg"
    if (Test-Path $mainConfigPath) {
        $targetConfigDir = Join-Path $GameDirectory "config"
        if (-not (Test-Path $targetConfigDir)) { 
            New-Item -ItemType Directory -Path $targetConfigDir | Out-Null 
        }
        Copy-Item $mainConfigPath "$targetConfigDir\" -Force
        Write-Host "  ✓ keeperfx.cfg" -ForegroundColor Green
        $deployedCount++
    }
    
    Write-Host ""
}

# ============================================================================
# Summary
# ============================================================================

Write-Host "=" * 80 -ForegroundColor Green
if ($deployedCount -gt 0) {
    Write-Host "✨ Deployment Complete! ($deployedCount files)" -ForegroundColor Green
} else {
    Write-Host "⚠️  No files deployed" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Tips:" -ForegroundColor Yellow
    Write-Host "  - Compile first: wsl make all" -ForegroundColor White
    Write-Host "  - Specify what to deploy: -DeployGraphics, -DeploySounds, etc." -ForegroundColor White
}
Write-Host "=" * 80 -ForegroundColor Green
Write-Host ""
