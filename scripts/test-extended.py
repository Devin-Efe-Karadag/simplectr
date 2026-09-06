#!/usr/bin/env python3
"""Real kernel tests for user mappings, PTYs, exec and TCP publishing; run as root."""
import errno
import fcntl
import os
from pathlib import Path
import select
import signal
import socket
import struct
import subprocess as sp
import termios
import time

ROOT = Path(__file__).resolve().parent.parent
R = str(ROOT / 'bin/simplectr')
USER = os.environ.get('SIMPLECTR_MAP_USER', os.environ.get('SUDO_USER', 'testuser'))
processes = []

def run(*args, code=0):
    result = sp.run([R, *args], stdout=sp.PIPE, stderr=sp.STDOUT, text=True, timeout=40)
    assert result.returncode == code, (args, result.returncode, result.stdout)
    return result.stdout

def wait_for(fn, timeout=10):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        try:
            value = fn()
            if value:
                return value
        except (OSError, AssertionError):
            pass
        time.sleep(.05)
    raise AssertionError('timed out waiting for condition')

def start(name, options, command):
    p = sp.Popen([R, 'run', '--name', name, *options, '--', *command], stdout=sp.PIPE, stderr=sp.STDOUT)
    processes.append(p)
    wait_for(lambda: 'READY' in run('logs', name))
    return p

def inspect(name):
    text = run('inspect', name)
    return dict(line.split('=', 1) for line in text.splitlines()[1:] if '=' in line)

def exchange(port, payload=b'published\n'):
    with socket.create_connection(('127.0.0.1', port), timeout=2) as s:
        s.sendall(payload)
        return s.recv(4096) == payload

def read_until(fd, pattern, timeout=8):
    data = b''
    end = time.monotonic() + timeout
    while pattern not in data and time.monotonic() < end:
        if select.select([fd], [], [], .1)[0]:
            try:
                chunk = os.read(fd, 8192)
            except OSError as e:
                if e.errno == errno.EIO:
                    break
                raise
            if not chunk:
                break
            data += chunk
    assert pattern in data, (pattern, data)
    return data

def interactive(args):
    master, slave = os.openpty()
    fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack('HHHH', 25, 90, 0, 0))
    original = termios.tcgetattr(slave)
    def session():
        os.setsid()
        fcntl.ioctl(0, termios.TIOCSCTTY, 0)
    p = sp.Popen([R, *args], stdin=slave, stdout=slave, stderr=slave, preexec_fn=session)
    processes.append(p)
    try:
        # A prompt confirms the inner shell has started before sending terminal input.
        read_until(master, b'# ')
        os.write(master, b'echo TTY_READY; stty size\n')
        read_until(master, b'\r\n25 90\r\n')
        fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack('HHHH', 33, 101, 0, 0))
        os.kill(p.pid, signal.SIGWINCH)
        time.sleep(.1)
        os.write(master, b'stty size\n')
        read_until(master, b'\r\n33 101\r\n')
        os.write(master, b'sleep 30\n')
        time.sleep(.2)
        os.write(master, b'\x03')
        time.sleep(.1)
        os.write(master, b'echo AFTER_INTERRUPT; exit 23\n')
        read_until(master, b'\r\nAFTER_INTERRUPT\r\n')
        assert p.wait(timeout=8) == 23
        assert termios.tcgetattr(slave) == original, 'host terminal was not restored'
    finally:
        if p.poll() is None:
            p.kill()
            p.wait()
        os.close(master)
        os.close(slave)

assert os.geteuid() == 0, 'Run sudo make integration-test'
assert not run('list').strip(), 'Runtime must be idle and cleaned before extended tests'
try:
    mapped = start('xt-map', ['--userns', USER, '--net', 'bridge'], ['/bin/sh', '-c', 'echo READY; exec sleep 120'])
    state = inspect('xt-map')
    hostpid = int(state['pid'])
    uid_base, gid_base = int(state['uid_base']), int(state['gid_base'])
    assert uid_base >= 65536 and gid_base >= 65536
    host_status = Path(f'/proc/{hostpid}/status').read_text()
    assert f'Uid:\t{uid_base}\t{uid_base}\t{uid_base}\t{uid_base}' in host_status
    assert Path('/var/lib/simplectr/images/alpine/bin/busybox').stat().st_uid == 0
    command = 'test "$$" != 1; test "$(hostname)" = xt-map; test "$(id -u)" = 0; touch /root/exec-file; grep -q "NoNewPrivs:.*1" /proc/self/status; grep -q "CapEff:.*0000000000000000" /proc/self/status; ! mount -t tmpfs tmpfs /mnt; ping -c 1 -W 2 10.88.0.1; echo EXEC_OK'
    assert 'EXEC_OK' in run('exec', 'xt-map', '--', '/bin/sh', '-ec', command)
    run('exec', 'xt-map', '--', '/bin/sh', '-c', 'test "$1" = "a b" && exit 17', 'sh', 'a b', code=17)
    run('exec', 'xt-map', '--', '/no-such-command', code=127)
    cg = Path(state['cgroup'])
    observed = run('exec', 'xt-map', '--', '/bin/sh', '-c', 'cat /proc/self/cgroup')
    assert state['id'] in observed
    interactive(['run', '--name', 'xt-tty', '--userns', USER, '--tty', '--', '/bin/sh', '-i'])
    interactive(['exec', '--tty', 'xt-map', '--', '/bin/sh', '-i'])
    sleeper = sp.Popen([R, 'exec', 'xt-map', '--', '/bin/sleep', '60'], stdout=sp.PIPE, stderr=sp.STDOUT)
    processes.append(sleeper)
    wait_for(lambda: int((cg / 'pids.current').read_text()) >= 2)
    run('stop', 'xt-map')
    assert mapped.wait(timeout=10) == 137
    assert sleeper.wait(timeout=10) == 137
    run('exec', 'xt-map', '--', '/bin/true', code=125)
    print('PASS: mapped host UID, unchanged image, exec isolation/limits/security, PTY resize/Ctrl-C/restoration', flush=True)

    server = start('xt-pub', ['--userns', USER, '--net', 'bridge', '--publish', '18081:8080', '--publish', '18082:8080'], ['/bin/sh', '-c', 'echo READY; exec /bin/busybox nc -lk -p 8080 -e /bin/cat'])
    wait_for(lambda: exchange(18081))
    wait_for(lambda: exchange(18082))
    run('run', '--name', 'xt-conflict', '--net', 'bridge', '--publish', '18081:80', '--', '/bin/true', code=125)
    assert exchange(18081)
    run('run', '--name', 'xt-hairpin', '--net', 'bridge', '--', '/bin/sh', '-ec', 'test "$(echo hairpin | busybox nc -w 1 10.88.0.1 18081)" = hairpin')
    with socket.socket() as occupied:
        occupied.bind(('0.0.0.0', 18083))
        occupied.listen()
        run('run', '--name', 'xt-host-port', '--net', 'bridge', '--publish', '18083:80', '--', '/bin/true', code=125)
    # Lost supervisor closes reservation sockets; stale records must still reserve published ports.
    server.kill()
    assert server.wait(timeout=10) == -signal.SIGKILL
    time.sleep(.3)
    run('run', '--name', 'xt-stale-port', '--net', 'bridge', '--publish', '18081:80', '--', '/bin/true', code=125)
    run('cleanup')
    assert not run('list').strip()
    tables = sp.check_output(['/usr/sbin/nft', 'list', 'tables'], text=True)
    assert 'simplectr_' not in tables
    with socket.socket() as released:
        released.bind(('0.0.0.0', 18081))
    print('PASS: multiple TCP mappings, localhost/hairpin, collisions, stale reservations and crash cleanup', flush=True)
finally:
    for p in processes:
        if p.poll() is None:
            p.kill()
    for p in processes:
        try:
            p.wait(timeout=10)
        except sp.TimeoutExpired:
            pass
    time.sleep(.1)
    run('cleanup')
