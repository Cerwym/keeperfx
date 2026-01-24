# Setup script for vendoring ImGui into KeeperFX
# This script downloads ImGui and copies the necessary files

param(
    [string]$ImGuiVersion = "docking",
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$depsDir = Join-Path $PSScriptRoot "deps"
$imguiDir = Join-Path $depsDir "imgui"
$tempDir = Join-Path $depsDir "imgui-temp"

Write-Host "KeeperFX ImGui Setup" -ForegroundColor Cyan
Write-Host "===================" -ForegroundColor Cyan
Write-Host ""

# Check if imgui already exists
if (Test-Path $imguiDir) {
    if (-not $Force) {
        Write-Host "ImGui is already set up at: $imguiDir" -ForegroundColor Yellow
        Write-Host "Use -Force to re-download" -ForegroundColor Yellow
        exit 0
    }
    Write-Host "Removing existing ImGui..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $imguiDir
}

Write-Host "Downloading ImGui ($ImGuiVersion branch)..." -ForegroundColor Green

# Clone ImGui
try {
    Push-Location $depsDir
    git clone --depth 1 --branch $ImGuiVersion https://github.com/ocornut/imgui.git imgui-temp
    
    if (-not $?) {
        throw "Failed to clone ImGui repository"
    }
    
    Write-Host "Copying ImGui files..." -ForegroundColor Green
    
    # Create directories
    New-Item -ItemType Directory -Force -Path $imguiDir | Out-Null
    New-Item -ItemType Directory -Force -Path (Join-Path $imguiDir "backends") | Out-Null
    
    # Copy main files
    Copy-Item (Join-Path $tempDir "*.cpp") $imguiDir
    Copy-Item (Join-Path $tempDir "*.h") $imguiDir
    
    # Copy SDL2 backends
    Copy-Item (Join-Path $tempDir "backends/imgui_impl_sdl2.*") (Join-Path $imguiDir "backends")
    Copy-Item (Join-Path $tempDir "backends/imgui_impl_sdlrenderer2.*") (Join-Path $imguiDir "backends")
    
    # Clean up
    Write-Host "Cleaning up..." -ForegroundColor Green
    Remove-Item -Recurse -Force $tempDir
    
    Pop-Location
    
    Write-Host ""
    Write-Host "✓ ImGui setup complete!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Cyan
    Write-Host "1. Build with: wsl make ENABLE_IMGUI=1" -ForegroundColor White
    Write-Host "2. Run the game and press F3 to toggle debug overlay" -ForegroundColor White
    Write-Host ""
    Write-Host "Note: Don't commit the imgui directory to git!" -ForegroundColor Yellow
    
} catch {
    Write-Host "Error: $_" -ForegroundColor Red
    if (Test-Path $tempDir) {
        Remove-Item -Recurse -Force $tempDir
    }
    exit 1
}
