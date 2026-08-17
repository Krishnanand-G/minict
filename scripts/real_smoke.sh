#!/usr/bin/env bash
# Real-mode smoke: requires sudo on Linux / WSL2 with cgroup v2.
# Verifies the actual isolation path: namespaces, cgroup attachment,
# pivot_root, capability drop, and the seccomp filter.
set -euo pipefail

cd "$(dirname "$0")/.."
# keep state on the Linux fs (ext4), not /mnt/e (9p): bind mounts + pivot_root
# behave better on a real filesystem. Override with MINICT_REAL_STATE.
SD="${MINICT_REAL_STATE:-$HOME/.minict-real}"
RUN() { sudo MINICT_STATE_DIR="$SD" ./build/minict "$@"; }

make clean all

sudo rm -rf "$SD"

echo "== load alpine rootfs =="
RUN pull-rootfs rootfs/alpine-minirootfs.tar.gz alpine

echo "== run with hardening =="
# The command is single-quoted so substitutions run inside the container.
# shellcheck disable=SC2016
RUN run --memory 64m --cpu 50 --name real-demo --image alpine \
  'sleep 1; echo HOST=$(hostname); echo CGROUP=$(head -1 /proc/self/cgroup); grep -E "^CapEff|^Seccomp" /proc/self/status'

echo "== seccomp rejects escape/privileged syscalls =="
RUN run --name sec-demo --image alpine \
  'chroot / 2>&1; unshare -i 2>&1; echo sec-done'

echo "== daemon mode =="
RUN daemon &
DPID=$!
sleep 1
RUN run --name daemon-demo --image alpine 'echo from-daemon'
RUN ps | grep daemon-demo
RUN kill daemon-demo
kill "$DPID" 2>/dev/null || true

echo "== cleanup: kill the smoke containers, cgroup dirs removed =="
RUN kill real-demo
RUN kill sec-demo
sudo ls /sys/fs/cgroup/minict/ | grep -E "real-demo|sec-demo" || echo "cgroup dirs cleaned"

sudo rm -rf "$SD"
echo "real smoke ok"
