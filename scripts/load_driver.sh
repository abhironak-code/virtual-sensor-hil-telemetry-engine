
set -e
cd "$(dirname "$0")/../driver"
make
sudo insmod vsensor.ko || true
sleep 0.3
ls -l /dev/vsensor0
dmesg | tail -n 3
