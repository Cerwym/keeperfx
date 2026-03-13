#!/usr/bin/env bash
set -euo pipefail

workspace="${1:-$(pwd)}"
dk_install_path="${2:-}"

deploy_dir="$workspace/.deploy"
deploy_data="$deploy_dir/data"
deploy_sound="$deploy_dir/sound"
compose_file="$workspace/docker/compose.yml"

mkdir -p "$deploy_data" "$deploy_sound"

if [[ -n "$dk_install_path" ]]; then
  "$workspace/.vscode/init_dk_layer.sh" "$workspace" "$dk_install_path"
else
  "$workspace/.vscode/init_dk_layer.sh" "$workspace"
fi

if [[ -d "$workspace/.local/dk-stage/data" && -d "$workspace/.local/dk-stage/sound" ]]; then
  echo "Copying DK originals from normalized local stage ..."
  cp -a "$workspace/.local/dk-stage/data/." "$deploy_data/"
  cp -a "$workspace/.local/dk-stage/sound/." "$deploy_sound/"
else
  echo "Normalized stage unavailable; copying originals directly from cached DK path ..."
  config_path="$workspace/.local/dk-install-path.txt"
  if [[ ! -f "$config_path" ]]; then
    echo "Missing cached DK path; rerun Init DK Originals Layer." >&2
    exit 1
  fi
  dk_root="$(tr -d '\r\n' < "$config_path")"
  cp -a "$dk_root/data/." "$deploy_data/"
  cp -a "$dk_root/sound/." "$deploy_sound/"
fi

echo "Building KeeperFX asset packages ..."
if command -v docker >/dev/null 2>&1 && [[ -f "$compose_file" ]]; then
  docker compose -f "$compose_file" run --rm linux bash -lc "make -j1 pkg-gfx && make -j1 pkg-languages && make -j1 pkg-sfx"
else
  (
    cd "$workspace"
    make -j1 pkg-gfx
    make -j1 pkg-languages
    make -j1 pkg-sfx
  )
fi

if [[ -d "$workspace/pkg/data" ]]; then
  cp -a "$workspace/pkg/data/." "$deploy_data/"
fi
if [[ -d "$workspace/pkg/ldata" ]]; then
  cp -a "$workspace/pkg/ldata/." "$deploy_data/"
fi
if [[ -d "$workspace/pkg/sound" ]]; then
  cp -a "$workspace/pkg/sound/." "$deploy_sound/"
fi

echo ".deploy is initialized and layered for host runtime testing."
