#!/bin/sh
sudo rmmod vsensor && dmesg | tail -n 2
