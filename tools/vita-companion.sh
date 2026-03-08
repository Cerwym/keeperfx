#!/usr/bin/env bash
# vita-companion.sh — Manage vitacompanion, noASLR, and PrincessLog on a PS Vita
#
# Prerequisites: curl, nc (netcat) on the host; FTP server on the Vita (port 1337).
# vitacompanion itself provides port 1337 once installed; for first-time setup
# use VitaShell's built-in FTP (press SELECT in VitaShell).
#
# Usage:
#   ./tools/vita-companion.sh setup          # Download plugins + upload + patch taiHEN config
#   ./tools/vita-companion.sh deploy-eboot   # Upload eboot.bin from latest build
#   ./tools/vita-companion.sh launch         # Kill + relaunch KeeperFX
#   ./tools/vita-companion.sh deploy-launch  # deploy-eboot + launch in one step
#   ./tools/vita-companion.sh reboot         # Reboot the Vita
#   ./tools/vita-companion.sh log [port]     # Listen for PrincessLog output (default 8080)
#   ./tools/vita-companion.sh screen <on|off>
#
# Environment:
#   VITA_IP   — Vita IP address (default: 192.168.0.66)
#   VITA_FTP  — FTP port         (default: 1337)
#   VITA_CMD  — Command port     (default: 1338)
#   VITA_PRESET — Build preset   (default: vita-reldebug)

set -euo pipefail

VITA_IP="${VITA_IP:-192.168.0.66}"
VITA_FTP="${VITA_FTP:-1337}"
VITA_CMD="${VITA_CMD:-1338}"
VITA_PRESET="${VITA_PRESET:-vita-reldebug}"
TITLE_ID="KFXV00001"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE="$(cd "$SCRIPT_DIR/.." && pwd)"
PLUGIN_DIR="$WORKSPACE/out/vita-plugins"
BUILD_DIR="$WORKSPACE/out/build/$VITA_PRESET"

# Colours
RED='\033[0;31m'; GREEN='\033[0;32m'; CYAN='\033[0;36m'; YELLOW='\033[1;33m'; NC='\033[0m'

info()  { echo -e "${CYAN}[vita]${NC} $*"; }
ok()    { echo -e "${GREEN}[vita]${NC} $*"; }
warn()  { echo -e "${YELLOW}[vita]${NC} $*"; }
err()   { echo -e "${RED}[vita]${NC} $*" >&2; }

vita_cmd() {
    # Send a command to vitacompanion's TCP command server
    local cmd="$1"
    if command -v nc &>/dev/null; then
        echo "$cmd" | nc -w 2 "$VITA_IP" "$VITA_CMD" 2>/dev/null || true
    else
        # Fallback: bash /dev/tcp (works without netcat installed)
        (echo "$cmd" > "/dev/tcp/${VITA_IP}/${VITA_CMD}") 2>/dev/null || true
    fi
}

ftp_upload() {
    # Upload a local file to a path on the Vita
    local local_file="$1"
    local remote_path="$2"
    if [[ ! -f "$local_file" ]]; then
        err "File not found: $local_file"
        return 1
    fi
    info "Uploading $(basename "$local_file") → $remote_path"
    curl -s --ftp-method nocwd --ftp-create-dirs \
        -T "$local_file" \
        "ftp://${VITA_IP}:${VITA_FTP}/${remote_path}" || {
        err "FTP upload failed. Is the Vita FTP server running on ${VITA_IP}:${VITA_FTP}?"
        err "For first-time setup, open VitaShell and press SELECT to start its FTP server."
        return 1
    }
}

ftp_download() {
    # Download a file from the Vita
    local remote_path="$1"
    local local_file="$2"
    mkdir -p "$(dirname "$local_file")"
    curl -s --ftp-method nocwd \
        -o "$local_file" \
        "ftp://${VITA_IP}:${VITA_FTP}/${remote_path}" || {
        err "FTP download failed: $remote_path"
        return 1
    }
}

# ── SETUP: Download plugin releases and upload to Vita ────────────────

cmd_setup() {
    info "Setting up vitacompanion + noASLR + PrincessLog on ${VITA_IP}..."
    mkdir -p "$PLUGIN_DIR"

    # --- Download vitacompanion.suprx ---
    if [[ ! -f "$PLUGIN_DIR/vitacompanion.suprx" ]]; then
        info "Downloading vitacompanion.suprx..."
        curl -sL -o "$PLUGIN_DIR/vitacompanion.suprx" \
            "https://github.com/devnoname120/vitacompanion/releases/download/1.00/vitacompanion.suprx"
        ok "Downloaded vitacompanion.suprx"
    else
        ok "vitacompanion.suprx already cached"
    fi

    # --- Download noASLR ---
    if [[ ! -f "$PLUGIN_DIR/noaslr.skprx" ]]; then
        info "Downloading noaslr.skprx..."
        curl -sL -o "$PLUGIN_DIR/noaslr.skprx" \
            "https://github.com/TeamFAPS/PSVita-RE-tools/raw/master/noASLR/release/noaslr.skprx"
        ok "Downloaded noaslr.skprx"
    else
        ok "noaslr.skprx already cached"
    fi

    # --- Download PrincessLog ---
    if [[ ! -f "$PLUGIN_DIR/net_logging_mgr.skprx" ]]; then
        info "Downloading PrincessLog (net_logging_mgr.skprx)..."
        curl -sL -o "$PLUGIN_DIR/net_logging_mgr.skprx" \
            "https://github.com/TeamFAPS/PSVita-RE-tools/raw/master/PrincessLog/build/net_logging_mgr.skprx"
        ok "Downloaded net_logging_mgr.skprx"
    else
        ok "net_logging_mgr.skprx already cached"
    fi

    if [[ ! -f "$PLUGIN_DIR/NetLoggingMgrSettings.vpk" ]]; then
        info "Downloading PrincessLog settings app..."
        curl -sL -o "$PLUGIN_DIR/NetLoggingMgrSettings.vpk" \
            "https://github.com/TeamFAPS/PSVita-RE-tools/raw/master/PrincessLog/build/NetLoggingMgrSettings.vpk"
        ok "Downloaded NetLoggingMgrSettings.vpk"
    else
        ok "NetLoggingMgrSettings.vpk already cached"
    fi

    info ""
    info "=== Plugin Upload ==="
    info "Make sure FTP is running on the Vita (VitaShell → SELECT, or vitacompanion if already installed)."
    info ""

    # Upload plugins to ur0:/tai/
    ftp_upload "$PLUGIN_DIR/vitacompanion.suprx"    "ur0:/tai/vitacompanion.suprx"
    ftp_upload "$PLUGIN_DIR/noaslr.skprx"           "ur0:/tai/noaslr.skprx"
    ftp_upload "$PLUGIN_DIR/net_logging_mgr.skprx"  "ur0:/tai/net_logging_mgr.skprx"

    # Upload PrincessLog settings VPK (user installs via VitaShell)
    ftp_upload "$PLUGIN_DIR/NetLoggingMgrSettings.vpk" "ux0:/data/NetLoggingMgrSettings.vpk"

    info ""
    info "=== taiHEN Config Update ==="
    info "Downloading current ur0:/tai/config.txt..."

    local tai_config="$PLUGIN_DIR/config.txt"
    local tai_backup="$PLUGIN_DIR/config.txt.backup"

    ftp_download "ur0:/tai/config.txt" "$tai_config"
    cp "$tai_config" "$tai_backup"
    ok "Backed up config.txt → $(basename "$tai_backup")"

    local modified=false

    # Add *KERNEL entries (noASLR + PrincessLog kernel module)
    if ! grep -q "noaslr.skprx" "$tai_config"; then
        # Ensure *KERNEL section exists
        if ! grep -q '^\*KERNEL' "$tai_config"; then
            echo -e "\n*KERNEL" >> "$tai_config"
        fi
        # Add after *KERNEL line
        sed -i '/^\*KERNEL/a ur0:tai/noaslr.skprx' "$tai_config"
        modified=true
        ok "Added noaslr.skprx to *KERNEL"
    else
        warn "noaslr.skprx already in config"
    fi

    if ! grep -q "net_logging_mgr.skprx" "$tai_config"; then
        if ! grep -q '^\*KERNEL' "$tai_config"; then
            echo -e "\n*KERNEL" >> "$tai_config"
        fi
        sed -i '/^\*KERNEL/a ur0:tai/net_logging_mgr.skprx' "$tai_config"
        modified=true
        ok "Added net_logging_mgr.skprx to *KERNEL"
    else
        warn "net_logging_mgr.skprx already in config"
    fi

    # Add *main entry (vitacompanion user module — loads for all apps)
    if ! grep -q "vitacompanion.suprx" "$tai_config"; then
        # Ensure *main section exists
        if ! grep -q '^\*main' "$tai_config"; then
            echo -e "\n*main" >> "$tai_config"
        fi
        sed -i '/^\*main/a ur0:tai/vitacompanion.suprx' "$tai_config"
        modified=true
        ok "Added vitacompanion.suprx to *main"
    else
        warn "vitacompanion.suprx already in config"
    fi

    if $modified; then
        info "Uploading updated config.txt..."
        ftp_upload "$tai_config" "ur0:/tai/config.txt"
        ok "taiHEN config updated on Vita"
    else
        ok "No config changes needed"
    fi

    echo ""
    ok "=== Setup Complete ==="
    info "1. Install PrincessLog settings app: open VitaShell → navigate to"
    info "   ux0:/data/NetLoggingMgrSettings.vpk → install it"
    info "2. Launch NetLoggingMgrSettings → set your PC's IP & port (default 8080) → Save"
    info "3. Reboot the Vita (run: ./tools/vita-companion.sh reboot)"
    info "4. After reboot, vitacompanion FTP+commands will be active automatically"
    info "5. To see logs: ./tools/vita-companion.sh log"
}

# ── DEPLOY: Upload eboot.bin to the Vita app directory ────────────────

cmd_deploy_eboot() {
    local self_file="$BUILD_DIR/keeperfx.self"
    if [[ ! -f "$self_file" ]]; then
        err "Build output not found: $self_file"
        err "Run 'Build Vita $(echo "$VITA_PRESET" | sed 's/vita-//' | sed 's/.*/\u&/')' task first."
        return 1
    fi
    # eboot.bin on the Vita is the .self renamed
    ftp_upload "$self_file" "ux0:/app/${TITLE_ID}/eboot.bin"
    ok "eboot.bin deployed to ux0:/app/${TITLE_ID}/"
}

# ── LAUNCH: Kill running apps and launch KeeperFX ────────────────────

cmd_launch() {
    info "Killing running apps..."
    vita_cmd "destroy"
    sleep 1
    info "Launching ${TITLE_ID}..."
    vita_cmd "launch ${TITLE_ID}"
    ok "Launch command sent"
}

# ── DEPLOY + LAUNCH ──────────────────────────────────────────────────

cmd_deploy_launch() {
    cmd_deploy_eboot
    cmd_launch
}

# ── REBOOT ───────────────────────────────────────────────────────────

cmd_reboot() {
    info "Rebooting Vita..."
    vita_cmd "reboot"
    ok "Reboot command sent"
}

# ── LOG: Listen for PrincessLog network output ───────────────────────

cmd_log() {
    local port="${1:-8080}"
    info "Listening for PrincessLog output on port ${port}..."
    info "Make sure PrincessLog is configured to send to this machine's IP on port ${port}"
    info "Press Ctrl+C to stop"
    echo "---"
    if command -v nc &>/dev/null; then
        nc -kl "$port"
    elif command -v socat &>/dev/null; then
        socat TCP-LISTEN:"$port",reuseaddr,fork -
    else
        err "No netcat or socat found. Install one with: apt-get install -y netcat-openbsd"
        err "Or run: socat TCP-LISTEN:${port},reuseaddr,fork -"
        return 1
    fi
}

# ── SCREEN ───────────────────────────────────────────────────────────

cmd_screen() {
    local state="${1:-}"
    if [[ "$state" != "on" && "$state" != "off" ]]; then
        err "Usage: $0 screen <on|off>"
        return 1
    fi
    info "Screen ${state}..."
    vita_cmd "screen ${state}"
    ok "Screen command sent"
}

# ── MAIN ─────────────────────────────────────────────────────────────

usage() {
    echo "Usage: $0 <command> [args]"
    echo ""
    echo "Commands:"
    echo "  setup            Download plugins, upload to Vita, patch taiHEN config"
    echo "  deploy-eboot     Upload eboot.bin from build output to Vita"
    echo "  launch           Kill running apps + launch KeeperFX"
    echo "  deploy-launch    Deploy eboot + launch (combined)"
    echo "  reboot           Reboot the Vita"
    echo "  log [port]       Listen for PrincessLog output (default: 8080)"
    echo "  screen <on|off>  Turn Vita screen on or off"
    echo ""
    echo "Environment variables:"
    echo "  VITA_IP=$VITA_IP  VITA_FTP=$VITA_FTP  VITA_CMD=$VITA_CMD  VITA_PRESET=$VITA_PRESET"
}

case "${1:-}" in
    setup)          cmd_setup ;;
    deploy-eboot)   cmd_deploy_eboot ;;
    launch)         cmd_launch ;;
    deploy-launch)  cmd_deploy_launch ;;
    reboot)         cmd_reboot ;;
    log)            cmd_log "${2:-8080}" ;;
    screen)         cmd_screen "${2:-}" ;;
    -h|--help|"")   usage ;;
    *)              err "Unknown command: $1"; usage; exit 1 ;;
esac
