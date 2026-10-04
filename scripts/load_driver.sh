#!/bin/sh
# Build and load the kernel module, show it in dmesg and /dev.
set -e
cd "$(dirname "$0")/../driver"
make
sudo insmod vsensor.ko || true
sleep 0.3
ls -l /dev/vsensor0
dmesg | tail -n 3
