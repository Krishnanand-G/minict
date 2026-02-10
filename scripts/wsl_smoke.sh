#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
make clean all

export MINICT_STATE_DIR=".minict-smoke"
rm -rf "$MINICT_STATE_DIR"

./build/minict load-oci tests/fixtures/oci-tiny alpine-smoke
./build/minict daemon &
DPID=$!
sleep 0.5

./build/minict run --memory 32m --name smoke-demo --image alpine-smoke echo hi
./build/minict ps | grep smoke-demo
./build/minict kill smoke-demo

kill "$DPID" 2>/dev/null || true
rm -rf "$MINICT_STATE_DIR"
echo "wsl smoke ok"
