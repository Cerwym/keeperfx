#!/usr/bin/env bash
set -euo pipefail

workspace="${1:-$(pwd)}"
deploy_dir="$workspace/.deploy"
compose_file="$workspace/docker/compose.yml"

if [[ ! -d "$deploy_dir" ]]; then
  echo ".deploy does not exist. Run Init .deploy/ first."
  exit 1
fi

if [[ ! -f "$compose_file" ]]; then
  echo "Compose file not found: $compose_file"
  exit 1
fi

echo "Rebuilding gfx package in linux container..."
docker compose -f "$compose_file" run --rm linux bash -lc "make pkg-gfx"

echo "Staging pkg/data and pkg/ldata into .deploy/data..."
mkdir -p "$deploy_dir/data"
if [[ -d "$workspace/pkg/data" ]]; then
  cp -a "$workspace/pkg/data/." "$deploy_dir/data/"
fi
if [[ -d "$workspace/pkg/ldata" ]]; then
  cp -a "$workspace/pkg/ldata/." "$deploy_dir/data/"
fi

echo "GFX layer refreshed in .deploy/data."
