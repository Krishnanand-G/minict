#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${MINICT_BIN:-$ROOT/build/minict}"
export MINICT_SIM="${MINICT_SIM:-1}"
start=$(date +%s%3N)
"$BIN" run --name "bench-$$" --memory 64m --cpu 50 /bin/true
end=$(date +%s%3N)
echo "wall start latency: $((end-start))ms (target: <500ms)"
