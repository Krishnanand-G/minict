# minict

Small container runtime I wrote to learn Linux namespaces and cgroup v2.

It sets up pid, mount, uts, ipc, and net namespaces, applies `memory.max` / `cpu.max`, then starts a command. No daemon, no OCI, just the pieces I wanted to understand.

I do a lot of work from Windows, so `MINICT_SIM=1` records the same steps under `.minict/` without calling `unshare`. Real isolation needs Linux (WSL2 is fine) and usually root.

## Build

```bash
make all
MINICT_SIM=1 make test
```

Windows / MinGW:

```bat
build.bat
```

## Usage

```bash
MINICT_SIM=1 ./build/minict run --memory 64m --cpu 50 --name demo /bin/sh
./build/minict ps
./build/minict stats
./build/minict kill demo
```

Real mode on WSL/Linux:

```bash
sudo ./build/minict run --memory 64m --cpu 50 --name demo /bin/sh
```

Rootfs (sim just drops a marker; Linux runs `tar`):

```bash
./build/minict pull-rootfs ubuntu-rootfs.tar ubuntu
```

## Status page

`ui/` is a plain table over the status JSON. `tools/statusd.py` serves `.minict/status.json` on `:7474`.

```bash
python3 tools/statusd.py &
python3 -m http.server 8080 -d ui
```

Open http://127.0.0.1:8080 — if statusd isn't up it falls back to `sample-status.json`.

`scripts/bench_start.sh` times a simulated start. I was aiming for under 500ms in sim; measure your own box for the real path.

## Layout

```
src/           runtime bits
include/       headers
tests/         minitest suite (run with MINICT_SIM=1)
ui/            status table
tools/statusd.py
```

## Notes

- cgroup writes go under `/sys/fs/cgroup/minict/<name>` when not simulating
- state lives in `.minict/` (override with `MINICT_STATE_DIR`)
- this is a learning project, not a docker replacement
