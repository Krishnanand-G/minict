# minict

Small container runtime I wrote to learn Linux namespaces and cgroup v2.

It sets up pid, mount, uts, ipc, and net namespaces, applies `memory.max` / `cpu.max`, then starts a command. A thin daemon listens on a Unix socket; the CLI talks to it when `.minict/minict.sock` exists. Local OCI image layouts can be loaded into a named rootfs.

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

### Daemon (WSL2 / Linux)

Start the daemon in one terminal:

```bash
MINICT_SIM=1 ./build/minict daemon
# real mode: sudo ./build/minict daemon
```

In another terminal, the same commands go through the socket automatically:

```bash
MINICT_SIM=1 ./build/minict run --name demo echo hi
MINICT_SIM=1 ./build/minict ps
MINICT_SIM=1 ./build/minict kill demo
```

If the socket is not up, the CLI falls back to in-process mode (same as before).

### OCI image (local layout)

Load an OCI image layout directory or `oci-archive` tar:

```bash
./build/minict load-oci ./my-image-layout alpine
./build/minict run --image alpine --name demo /bin/sh
```

Sim mode writes a marker under `.minict/rootfs/<name>/`. On Linux it unpacks layer blobs in order.

Fixture for tests: `tests/fixtures/oci-tiny/`.

Real mode on WSL/Linux:

```bash
sudo ./build/minict daemon
sudo ./build/minict load-oci tests/fixtures/oci-tiny alpine
sudo ./build/minict run --image alpine --name demo /bin/sh
```

Rootfs tarball (older path, still works):

```bash
./build/minict pull-rootfs ubuntu-rootfs.tar ubuntu
```

Quick smoke on WSL:

```bash
bash scripts/wsl_smoke.sh
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
src/           runtime bits (ipc, daemon, oci)
include/       headers
tests/         minitest suite (run with MINICT_SIM=1)
ui/            status table
tools/statusd.py
```

## Notes

- daemon socket: `.minict/minict.sock` (override state dir with `MINICT_STATE_DIR`)
- cgroup writes go under `/sys/fs/cgroup/minict/<name>` when not simulating
- state lives in `.minict/` (override with `MINICT_STATE_DIR`)
- OCI v1: local layout / tar only, no registry pull
- this is a learning project, not a docker replacement
