#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
[[ $EUID == 0 ]] || { echo 'Run sudo make integration-test in the VM'; exit 1; }
R=./bin/simplectr
$R doctor
[[ -z "$($R list)" ]] || { echo 'Clean up existing containers before integration testing'; exit 1; }
$R cleanup
$R pull alpine
make bin/test_namespaces bin/test_mounts bin/test_cgroup bin/test_network bin/test_state
./bin/test_state
./bin/test_namespaces
./bin/test_mounts
./bin/test_cgroup
./bin/test_network
out=$(mktemp -d /run/simplectr-tests.XXXXXX)
children=()
finish() {
    for child in "${children[@]}"; do kill -TERM "$child" 2>/dev/null || true; done
    for name in it-peer it-cpu it-pids it-stop it-crash; do $R stop "$name" >/dev/null 2>&1 || true; done
    for child in "${children[@]}"; do wait "$child" 2>/dev/null || true; done
    $R cleanup
    echo "Integration artifacts: $out"
}
trap finish EXIT
expect_status() {
    local expected=$1; shift
    local status=0
    "$@" || status=$?
    [[ $status == "$expected" ]] || { echo "Expected exit $expected, got $status"; exit 1; }
}
ready() {
    for ((i=0;i<100;i++)); do
        if $R logs "$1" 2>/dev/null | grep -q READY; then return; fi
        sleep .05
    done
    echo "Timed out waiting for $1"; $R logs "$1"; return 1
}
$R run --name it-root -- /bin/sh -c 'test "$$" = 1; test "$(hostname)" = it-root; test ! -e /home/testuser/.profile; test ! -e /.oldroot; test ! -e /sys/fs/cgroup; touch /root/private; echo isolated' >"$out/root"
grep -q isolated "$out/root"
[[ ! -e /var/lib/simplectr/images/alpine/root/private ]]
$R run --name it-overlay -- /bin/sh -c 'test ! -e /root/private'
expect_status 7 $R run --name it-args -- /bin/sh -c 'test "$1" = "a b" && exit 7' sh 'a b'
expect_status 127 $R run --name it-missing -- /missing
$R run --name it-hardening -- /bin/sh -ec 'grep -q "CapEff:.*0000000000000000" /proc/self/status; grep -q "NoNewPrivs:.*1" /proc/self/status; grep -q "Seccomp:.*2" /proc/self/status; ! mount -t tmpfs tmpfs /mnt; ! unshare -n /bin/true'
$R run --name it-none -- /bin/sh -ec 'ping -c 1 -W 1 127.0.0.1; ! ping -c 1 -W 1 10.88.0.1; ! ping -c 1 -W 1 1.1.1.1; test "$(ls /sys/class/net 2>/dev/null | wc -l)" = 0'
$R run --name it-peer --net bridge -- /bin/sh -c 'echo READY; exec sleep 30' >"$out/peer" 2>&1 &
children+=("$!")
ready it-peer
expect_status 125 $R run --name it-peer -- /bin/true
peer=$($R inspect it-peer | head -n 1 | sed -n 's/.*ip=//p')
$R run --name it-net --net bridge -- /bin/sh -ec 'ping -c 1 -W 2 10.88.0.1; ping -c 1 -W 2 "$1"; wget -T 15 -qO- http://example.com | grep -q "Example Domain"' sh "$peer"
$R stop it-peer
expect_status 137 wait "${children[0]}"
$R run --name it-cpu --cpus 0.1 -- /bin/sh -c 'echo READY; while :; do :; done' >"$out/cpu" 2>&1 &
children+=("$!")
ready it-cpu
sleep 2
$R inspect it-cpu >"$out/cpu.metrics"
awk '$1=="nr_throttled" && $2>0 {ok=1} END {exit !ok}' "$out/cpu.metrics"
$R stop it-cpu
expect_status 137 wait "${children[1]}"
expect_status 137 $R run --name it-memory --memory 8M -- /bin/dd if=/dev/zero of=/dev/shm/blob bs=1M count=30
$R inspect it-memory >"$out/memory.metrics"
awk '$1=="oom_kill" && $2>0 {ok=1} END {exit !ok}' "$out/memory.metrics"
$R run --name it-pids --pids 8 -- /bin/sh -c 'echo READY; for i in $(seq 1 32); do sleep 30 & done; wait' >"$out/pids" 2>&1 &
children+=("$!")
ready it-pids
sleep .3
$R inspect it-pids >"$out/pids.metrics"
awk '/\[pids.events\]/{p=1;next} p && $1=="max" && $2>0 {ok=1} END {exit !ok}' "$out/pids.metrics"
$R stop it-pids 2>/dev/null || true
wait "${children[2]}" || true
$R run --name it-stop -- /bin/sh -c 'trap "exit 42" TERM; echo READY; while :; do sleep .1; done' >"$out/stop" 2>&1 &
children+=("$!")
ready it-stop
kill -TERM "${children[3]}"
expect_status 42 wait "${children[3]}"
$R run --name it-crash --net bridge -- /bin/sh -c 'echo READY; exec sleep 30' >"$out/crash" 2>&1 &
children+=("$!")
ready it-crash
kill -KILL "${children[4]}"
wait "${children[4]}" || true
sleep .3
$R cleanup
[[ -z "$($R list)" ]]
[[ ! -e /sys/class/net/simplectr0 ]]
[[ ! -e /sys/fs/cgroup/simplectr ]]
! nft list tables | grep -q simplectr_nat
[[ ! -e /run/simplectr/ip_forward ]]
! find /sys/class/net -maxdepth 1 -name 'kl*' | grep -q .
! find /run/simplectr -mindepth 1 -maxdepth 1 -type d | grep -q .
children=()
echo 'PASS: isolation, overlay, hardening, arguments, exit codes, signals, cgroup enforcement, loopback, bridge, peers, NAT, crash recovery and cleanup'
