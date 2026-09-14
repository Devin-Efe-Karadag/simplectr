# simplectr

`simplectr` is a small container runtime written in C. I built it to understand
what Docker and similar tools have to set up before an isolated process can
start.

A container is not a separate virtual machine. It is a normal Linux process
that shares the host kernel but receives its own view of processes, mounts,
users, and networking. `simplectr` creates those boundaries directly with
Linux system calls.

## What you need

This project is intended for an ARM64 Ubuntu VM. It requires root access and
changes namespaces, cgroups, mounts, network interfaces, and nftables rules.
Use a disposable VM while experimenting; this is a learning project, not a
security boundary for untrusted programs.

Install the build and runtime dependencies:

```sh
sudo apt install clang make libmnl-dev libseccomp-dev libcap-dev \
  curl tar nftables iproute2 uidmap
```

Then build and run the checks that do not require root:

```sh
make
make test
sudo ./bin/simplectr doctor
```

`doctor` reports whether the kernel features and host tools needed by the
runtime are available.

## Run your first container

Download the small Alpine root filesystem used by the examples:

```sh
sudo ./bin/simplectr pull alpine
```

Now start a container that prints a message and exits:

```sh
sudo ./bin/simplectr run --name hello -- \
  /bin/sh -c 'echo "hello from the container"; uname -m'
```

The command after `--` runs inside the container. Commands must use an absolute
path such as `/bin/sh`, because the process sees the container filesystem
rather than the host filesystem.

For an interactive shell:

```sh
sudo ./bin/simplectr run --name shell1 --tty \
  --memory 256M --cpus 0.5 --pids 64 -- /bin/sh -i
```

Type `exit` or press `Ctrl-D` to leave the shell. The memory, CPU, and process
limits are enforced through cgroup v2.

## Inspect and manage containers

```sh
sudo ./bin/simplectr list
sudo ./bin/simplectr inspect shell1
sudo ./bin/simplectr logs shell1
sudo ./bin/simplectr exec --tty shell1 -- /bin/sh -i
sudo ./bin/simplectr stop shell1
sudo ./bin/simplectr cleanup
```

`list` shows known containers, `inspect` prints one container's saved state,
and `logs` prints its captured output. `cleanup` removes stopped containers and
runtime-owned resources left behind after an interrupted run.

## Try bridge networking

`--net none` gives the container only a loopback interface. `--net bridge`
connects it to the `simplectr0` bridge and enables outbound NAT. A published
port maps a host TCP port to a container TCP port.

The included HTTP example starts a listener on container port 8080 and
publishes it as host port 18080:

```sh
# Terminal 1
sudo ./scripts/http-demo.sh

# Terminal 2
curl http://127.0.0.1:18080
```

The response should be `published`. Stop the example with `Ctrl-C` in the
first terminal.

## What happens during `run`

1. The parent prepares the cgroup, root filesystem, user mapping, and network.
2. The child enters new Linux namespaces.
3. OverlayFS gives the container a writable layer over the cached Alpine image.
4. The child mounts private `/proc` and `/dev`, drops capabilities, and installs
   a seccomp filter.
5. The requested command becomes PID 1 while the parent forwards signals,
   collects logs, and cleans up afterward.

For kernel-level and end-to-end tests, use `sudo make privileged-test` and
`sudo make integration-test`. Run them only in an idle test VM because they
create real namespaces, cgroups, mounts, and network devices.
