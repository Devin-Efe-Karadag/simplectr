# simplectr

I built `simplectr` to learn what a container runtime has to do below the
Docker interface. It is a small C program for ARM64 Linux that works directly
with namespaces, cgroup v2, OverlayFS, seccomp, rtnetlink, and nftables.

It is meant to run in a disposable VM. It needs root and shares the host
kernel, so it should not be used as a security boundary for untrusted code.

## Getting it running

On Ubuntu:

```sh
sudo apt install clang make libmnl-dev libseccomp-dev libcap-dev \
  curl tar nftables iproute2 uidmap
make
sudo ./bin/simplectr doctor
sudo ./bin/simplectr pull alpine
```

`pull alpine` downloads and verifies a small ARM64 root filesystem and keeps a
read-only cached copy. A container gets its own writable OverlayFS layer over
that copy.

## Starting a container

```sh
sudo ./bin/simplectr run --name shell1 --tty \
  --memory 256M --cpus 0.5 --pids 64 -- /bin/sh -i
```

For a run, the parent sets up the cgroup, filesystem, user mapping, and network
before releasing the child. The child switches root, mounts its private
`/proc` and `/dev`, drops capabilities, installs the seccomp filter, and then
executes the command as PID 1. The supervising process forwards signals, reaps
children, saves logs and metrics, and cleans up the resources it owns.

Bridge networking creates a veth pair on `simplectr0`. Outbound traffic is
masqueraded with nftables, and `--publish HOST:CONTAINER` adds a TCP port
mapping. `--net none` leaves only loopback in the network namespace.

The other useful commands are:

```sh
sudo ./bin/simplectr list
sudo ./bin/simplectr inspect shell1
sudo ./bin/simplectr exec --tty shell1 -- /bin/sh -i
sudo ./bin/simplectr logs shell1
sudo ./bin/simplectr stop shell1
sudo ./bin/simplectr cleanup
```

Run `make test` for the unprivileged tests. Namespace and end-to-end tests are
available through `sudo make privileged-test` and `sudo make integration-test`.
