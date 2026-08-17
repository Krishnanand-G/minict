#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
make clean all

export MINICT_SIM=1
STATE_DIR="$(mktemp -d)"
export MINICT_STATE_DIR="$STATE_DIR"
DPID=""
cleanup() {
  if [[ -n "$DPID" ]]; then kill "$DPID" 2>/dev/null || true; fi
  rm -rf "$STATE_DIR"
}
trap cleanup EXIT

./build/minict load-oci tests/fixtures/oci-tiny alpine-smoke
./build/minict daemon &
DPID=$!
sleep 0.5

./build/minict run --memory 32m --name smoke-demo --image alpine-smoke echo hi
./build/minict ps | grep smoke-demo
./build/minict kill smoke-demo

echo "wsl smoke ok"
