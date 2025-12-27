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
