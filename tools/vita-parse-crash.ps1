<#
.SYNOPSIS
    Fetch and parse the latest KeeperFX crash dump from the PS Vita.

.DESCRIPTION
    Lists .psp2dmp files on the Vita FTP, downloads the latest (or a chosen one),
    and runs vita-parse-core via WSL to produce a readable stack trace.

.PARAMETER VitaFTP
    Vita FTP address. Default: 192.168.0.66:1337

.PARAMETER All
    Show all dumps, not just eboot.bin ones.

.EXAMPLE
    .\tools\vita-parse-crash.ps1
    .\tools\vita-parse-crash.ps1 -VitaFTP 192.168.0.100:1337
#>
param(
    [string]$VitaFTP = "192.168.0.66:1337",
    [switch]$All
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path $PSScriptRoot -Parent

# --- Resolve ELF: prefer reldebug (has symbols), fall back to release ---
$ElfPaths = @(
    "out/build/vita-reldebug/keeperfx",
    "out/build/vita-release/keeperfx"
)
$ElfFile = $null
foreach ($p in $ElfPaths) {
    $full = Join-Path $RepoRoot $p
    if (Test-Path $full) { $ElfFile = $full; break }
}
if (-not $ElfFile) {
    Write-Error "No Vita ELF found. Build vita-reldebug or vita-release first."
    exit 1
}
Write-Host "Using ELF: $ElfFile" -ForegroundColor Cyan

# --- List dumps on Vita ---
Write-Host "Fetching dump list from ftp://$VitaFTP/ux0:/data/ ..." -ForegroundColor Cyan
$listing = wsl bash -c "curl --disable-epsv -s 'ftp://$VitaFTP/ux0:/data/' 2>&1"
$pattern = if ($All) { "psp2dmp$" } else { "eboot\.bin\.psp2dmp$" }
$dumps = $listing | Select-String $pattern | ForEach-Object {
    $fields = ($_ -split '\s+')
    [PSCustomObject]@{
        Size     = [long]$fields[4]
        Date     = "$($fields[5]) $($fields[6]) $($fields[7])"
        Filename = $fields[8]
    }
}

if (-not $dumps) {
    Write-Error "No complete .psp2dmp files found on Vita (only .tmp incomplete ones may exist)."
    exit 1
}

# --- Pick latest ---
Write-Host "`nAvailable dumps:" -ForegroundColor Yellow
$i = 0
foreach ($d in $dumps) {
    Write-Host "  [$i] $($d.Filename)  ($($d.Size) bytes, $($d.Date))"
    $i++
}
Write-Host ""

$choice = Read-Host "Select dump number [default: $($dumps.Count - 1) = latest]"
if ($choice -eq "") { $choice = $dumps.Count - 1 }
$selected = $dumps[[int]$choice]
Write-Host "Selected: $($selected.Filename)" -ForegroundColor Green

# --- Download ---
$DumpDir = Join-Path $RepoRoot "out\vita-dumps"
New-Item -ItemType Directory -Force -Path $DumpDir | Out-Null
$LocalDump = Join-Path $DumpDir $selected.Filename

Write-Host "Downloading..." -ForegroundColor Cyan
wsl bash -c "curl --disable-epsv -s 'ftp://$VitaFTP/ux0:/data/$($selected.Filename)' -o '/mnt/c/Users/peter/source/repos/keeperfx.worktrees/renderer-abstraction/out/vita-dumps/$($selected.Filename)'"
Write-Host "Saved to: $LocalDump" -ForegroundColor Green

# --- Ensure WSL tooling is ready ---
$SetupScript = @'
set -e
# Persistent venv
if [ ! -f ~/venv-vita/bin/activate ]; then
    echo "Creating ~/venv-vita..."
    python3 -m venv ~/venv-vita
    source ~/venv-vita/bin/activate
    pip install -q 'pyelftools==0.29'
else
    source ~/venv-vita/bin/activate
fi

# vita-parse-core
if [ ! -f ~/vita-parse-core/main.py ]; then
    echo "Cloning vita-parse-core..."
    git clone -q https://github.com/xyzz/vita-parse-core ~/vita-parse-core
    # Python 3 compat patches
    sed -i 's/from elftools.common.py3compat import str2bytes/def str2bytes(s): return s.encode("utf-8") if isinstance(s, str) else s/' ~/vita-parse-core/util.py
    sed -i "s/buf\[off\] != '\\\\0'/buf[off] != 0/" ~/vita-parse-core/util.py
    sed -i "s/out += buf\[off\]/out += chr(buf[off]) if isinstance(buf[off], int) else buf[off]/" ~/vita-parse-core/util.py
fi
'@

Write-Host "`nChecking WSL tooling..." -ForegroundColor Cyan
$SetupScript | wsl bash

# --- Run vita-parse-core ---
$WslDump = $LocalDump -replace '\\','/' -replace 'C:','/mnt/c'
$WslElf  = $ElfFile   -replace '\\','/' -replace 'C:','/mnt/c'

Write-Host "`n=== vita-parse-core output ===" -ForegroundColor Yellow
wsl bash -c "source ~/venv-vita/bin/activate && export PATH=/usr/local/vitasdk/bin:`$PATH && python3 ~/vita-parse-core/main.py '$WslDump' '$WslElf' 2>&1"
Write-Host "==============================" -ForegroundColor Yellow
