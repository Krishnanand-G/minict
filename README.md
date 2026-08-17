# minict

A small container runtime I wrote from scratch in C++14 to learn Linux
namespaces, cgroup v2, and what a real runtime (runc) has to get right.

`minict run` creates **new pid, mount, uts, ipc, and net namespaces** via
`clone(2)`, applies **cgroup v2** memory/cpu limits, `pivot_root`s into a
rootfs, mounts a private `/proc`, **drops every capability**, and installs a
**seccomp filter** — then executes the command. A thin daemon serves the same
CLI over a Unix socket when `.minict/minict.sock` exists, so containers are
reaped by the daemon and never left as zombies.

I do a lot of work from Windows, so `MINICT_SIM=1` records the same steps under
`.minict/` without touching the kernel. Real isolation needs Linux (WSL2 is
fine) and root.

## What runs when you `minict run`

```
client CLI
   |  (socket up? -> JSON over AF_UNIX -> daemon)
   v
run_container()
   |  namespaces: clone(2) child with CLONE_NEWPID|NEWNS|NEWUTS|NEWIPC|NEWNET
   |  limits:     write memory.max + cpu.max, then attach pid to cgroup.procs
   |  child:
   |    set hostname (UTS ns)
   |    pivot_root into rootfs, mount private /proc   [mount ns]
   |    drop all capabilities (capset + bounding set + securebits)
   |    install seccomp filter (no_new_privs, block ~30 syscalls)
   v
   exec /bin/sh -c "<command>"
```

State lives in plain files: `.minict/containers.tsv` (name, command, state,
memory, cpu, latency_ms, pid, rootfs) and `.minict/status.json` for the UI.
This is intentional — everything is inspectable without a debugger.

### Security model

- **`pivot_root`, not `chroot`** (`src/sandbox.cpp`). `chroot` is escapable and
  doesn't change the mount table; `pivot_root` swaps the entire root mount and
  the old root is unmounted. The mount ns is made `MS_SLAVE` first so nothing
  propagates back to the host.
- **Private `/proc`** — the container mounts its own procfs instead of reading
  the host's process table.
- **Zero capabilities** — the child empties effective/permitted/inheritable
  (`capset`), drains the bounding set (`PR_CAPBSET_DROP`, dropping
  `CAP_SETPCAP` last), and sets `SECBIT_NOROOT|NO_SETUID_FIXUP` so a setuid
  binary inside can't re-gain privileges.
- **Seccomp** — a classic BPF filter: arch check, then a curated ~30-syscall
  blocklist returned as `EPERM`: `mount`/`umount2`/`pivot_root`/`chroot`
  (escape), `unshare`/`setns` (namespace escape), `ptrace`/`bpf`/`userfaultfd`
  (introspection), module load, `swapon`, `reboot`/`kexec`, time/audit setters.
  Installed with `PR_SET_NO_NEW_PRIVS` so it can't be disabled.
- `CLONE_NEWPID` makes the command **PID 1** inside the container.

### What a production runtime adds (and why this isn't one)

minict is a learning project, not a Docker replacement. Honest gaps:

| minict | runc / production |
|---|---|
| `pivot_root` + private `/proc` | same, plus proper `devtmpfs`, bind mounts per spec |
| static seccomp blocklist | configurable per-container seccomp profiles + `seccomp notify` |
| all capabilities dropped | capability *sets* configured via OCI spec |
| no user namespaces — must run as root | rootless via `CLONE_NEWUSER` (uid/gid mapping) |
| `/bin/sh` is PID 1 | separate `runc init` child that re-execs, owns stdio, reaps |
| `SIGTERM` on kill | full signal lifecycle (`SIGCHLD` → wait, `KILL` escalation) |
| no network setup | CNI: `veth` pairs, bridge, port mapping |
| no AppArmor/SELinux | LSM labels, `nosuid`/`noexec` mounts |
| hand-rolled JSON/TSV state | OCI runtime-spec config + `runc state` |

### How this maps to Canonical's stack

- **snapd** confinement is the same primitives: each snap gets a mount
  namespace, an AppArmor profile, cgroup limits, and a seccomp filter — the
  same four layers minict applies, with AppArmor standing in for the pure-cap
  drop.
- **LXD** runs full system containers: it's minict's model extended with user
  namespaces, image management, and networking — `CLONE_NEWUSER` is the exact
  next step on this project's roadmap.

### Roadmap / what I'd do next

1. `CLONE_NEWUSER` + uid/gid mapping for rootless operation.
2. Configurable seccomp + capability profiles per container (OCI `config.json`).
3. Network namespaces wired to a `veth` pair and a bridge (no CNI deps).
4. `runc init`-style re-exec so PID 1 handles signals and reaps properly.
5. Benchmark the real start path and drive it under 100ms (sim currently
   targets <500ms).

## Support levels

The project has three deliberately separate support levels:

| Path | Status | Requirements |
|---|---|---|
| `MINICT_SIM=1` | Supported for development and CI | Linux, WSL2, or Windows with a C++ compiler |
| Native WSL/Linux runtime | Experimental and root-only | Linux namespaces, cgroup v2, `sudo`, and a trusted rootfs |
| Rootless runtime, networking, and full OCI runtime-spec compatibility | Roadmap | Not implemented yet |

The simulator exercises orchestration, state, IPC, OCI metadata handling, and
failure paths without making kernel changes. The native path is intentionally
smaller than Docker or `runc`; it is a learning implementation of selected
kernel primitives, not a production security boundary.

CI runs the simulator with GCC and Clang sanitizers. Privileged native smoke
runs should be performed manually in an isolated WSL distribution or virtual
machine.

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

Sim mode writes a marker under `.minict/rootfs/<name>/`. On Linux it unpacks
layer blobs in order.

Fixture for tests: `tests/fixtures/oci-tiny/`.

### Real mode (WSL/Linux, requires root)

```bash
bash scripts/real_smoke.sh
```

The smoke test verifies the whole hardening chain end to end: hostname from the
UTS namespace, `CapEff=0000000000000000` and `Seccomp=2` in the container,
`/proc/self/cgroup` showing the container's cgroup (i.e. the limits apply), and
`swapon`/`unshare` rejected with `Operation not permitted` by the seccomp
filter.

Manual equivalent:

```bash
sudo ./build/minict daemon
sudo ./build/minict pull-rootfs rootfs/alpine-minirootfs.tar.gz alpine
sudo ./build/minict run --memory 64m --image alpine --name demo /bin/sh
```

Rootfs tarball (older path, still works):

```bash
./build/minict pull-rootfs ubuntu-rootfs.tar ubuntu
```

Quick simulated smoke on WSL:

```bash
bash scripts/wsl_smoke.sh
```

## Status page

`ui/` is a plain table over the status JSON. `tools/statusd.py` serves
`.minict/status.json` on `:7474`.

```bash
python3 tools/statusd.py &
python3 -m http.server 8080 -d ui
```

Open http://127.0.0.1:8080 — if statusd isn't up it falls back to
`sample-status.json`.

`scripts/bench_start.sh` times a simulated start.

## Layout

```
src/           runtime bits (ipc, daemon, oci, sandbox)
include/       headers
tests/         minitest suite (run with MINICT_SIM=1)
ui/            status table
tools/statusd.py
scripts/       smoke + benchmark scripts
```

## Notes

- daemon socket: `.minict/minict.sock` (override state dir with `MINICT_STATE_DIR`)
- cgroup writes go under `/sys/fs/cgroup/minict/<name>` when not simulating
- state lives in `.minict/` (override with `MINICT_STATE_DIR`)
- OCI v1: local layout / tar only, no registry pull
- the seccomp filter targets x86_64 (arch-checked in the BPF program)
- this is a learning project, not a docker replacement
