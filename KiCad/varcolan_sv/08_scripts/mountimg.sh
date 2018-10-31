#!/bin/bash
fdisk -l raspbian-varcolansv.img
sudo mount -t ext4 -o loop,offset=$((2658304 * 512)) raspbian-varcolansv.img /mnt/loop

echo "*** data partition mounted in /mnt/loop ***"
ls /mnt/loop
