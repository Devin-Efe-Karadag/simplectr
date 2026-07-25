#!/bin/sh
set -eu
[ "$(uname -m)" = aarch64 ] || { echo 'Requires ARM64 Linux'; exit 1; }
apt-get update
apt-get install -y clang make clang-format cppcheck libmnl-dev libseccomp-dev libcap-dev curl ca-certificates nftables python3
modprobe overlay
