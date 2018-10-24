#!/bin/bash
#sudo dd bs=4M if=raspbian-varcolansv.img of=/dev/mmcblk0
# USAGE:
# [-f|-sf] [device]
# ex:
# sdcreate.sh -f sdc

# list of all partitions
#ls -a /dev | grep sdc

DEV=$2

if [ "$1" == "-sf" ]; then
	echo "skip formatting"
else
	echo "formatting"
	# umount partitions
	sudo umount /dev/$DEV1
	# remove partition
	sudo parted /dev/$DEV rm 1
	# create ext4 partition
	sudo parted /dev/$DEV mkpart primary ext4 0% 100%
	# format
	#sudo mkfs -V -t ext4 /dev/SDEV1
fi

# create sd
echo "creating the sd...please wait"
sudo dd bs=4M if=raspbian-varcolansv.img of=/dev/$DEV
echo "sync-ing..."
sync

# verify
echo "verifying...please wait"
sudo md5sum -c md5sum.txt

