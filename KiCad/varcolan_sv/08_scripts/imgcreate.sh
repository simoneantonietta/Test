#!/bin/bash
#sudo dd bs=4M if=/dev/mmcblk0 of=raspbian-varcolansv.img
DEV=$1
echo "removing previous image"
rm -f raspbian-varcolansv.img md5sum.txt

echo "creating image from $DEV"
sudo dd bs=4M if=/dev/$DEV of=raspbian-varcolansv.img
echo "sync-ing"
sync

md5sum raspbian-varcolansv.img > md5sum.txt
sed -i -e "s/raspbian-varcolansv.img/\/dev\/$DEV/g" md5sum.txt

echo "DONE"



