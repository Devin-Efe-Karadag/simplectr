}
grep -q isolated "$out/root"
$R run --name it-overlay -- /bin/sh -c 'test ! -e /root/private'
expect_status 127 $R run --name it-missing -- /missing
$R run --name it-none -- /bin/sh -ec 'ping -c 1 -W 1 127.0.0.1; ! ping -c 1 -W 1 10.88.0.1; ! ping -c 1 -W 1 1.1.1.1; test "$(ls /sys/class/net 2>/dev/null | wc -l)" = 0'
ready it-peer
peer=$($R inspect it-peer | head -n 1 | sed -n 's/.*ip=//p')
$R stop it-peer
$R run --name it-cpu --cpus 0.1 -- /bin/sh -c 'echo READY; while :; do :; done' >"$out/cpu" 2>&1 &
ready it-cpu
$R inspect it-cpu >"$out/cpu.metrics"
$R stop it-cpu
expect_status 137 $R run --name it-memory --memory 8M -- /bin/dd if=/dev/zero of=/dev/shm/blob bs=1M count=30
awk '$1=="oom_kill" && $2>0 {ok=1} END {exit !ok}' "$out/memory.metrics"
children+=("$!")
sleep .3
awk '/\[pids.events\]/{p=1;next} p && $1=="max" && $2>0 {ok=1} END {exit !ok}' "$out/pids.metrics"
wait "${children[2]}" || true
children+=("$!")
kill -TERM "${children[3]}"
$R run --name it-crash --net bridge -- /bin/sh -c 'echo READY; exec sleep 30' >"$out/crash" 2>&1 &
ready it-crash
wait "${children[4]}" || true
$R cleanup
[[ ! -e /sys/class/net/simplectr0 ]]
! nft list tables | grep -q simplectr_nat
! find /sys/class/net -maxdepth 1 -name 'kl*' | grep -q .
children=()
