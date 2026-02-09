# minict

small linux container runtime for learning namespaces + cgroup v2

## build
make all

## try (sim)
MINICT_SIM=1 ./build/minict run --memory 64m --cpu 50 --name demo /bin/sh
./build/minict ps
./build/minict kill demo
