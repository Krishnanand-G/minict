#!/usr/bin/env bash
# quick timing check for simulated starts
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${MINICT_BIN:-$ROOT/build/minict}"
export MINICT_SIM="${MINICT_SIM:-1}"

# date +%N is flaky across distros; just ask python
start=$(python3 -c 'import time; print(int(time.time()*1000))')
"$BIN" run --name "bench-$$" --memory 64m --cpu 50 /bin/true
end=$(python3 -c 'import time; print(int(time.time()*1000))')
echo "wall start latency: $((end-start))ms (target: <500ms)"
