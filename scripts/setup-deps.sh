#!/bin/sh
set -eu
[ "$(uname -m)" = aarch64 ] || { echo 'Requires ARM64 Linux'; exit 1; }
apt-get update
