#!/bin/bash

MOUNT_DIR=/tmp/mnt
RECOVERY_FILE=sv_recovery.txt
MOUNT_DEV=/dev/sda
RECOVERY_DEST_DIR=/tmp

#take a list of attached devices
USBKEY_EXISTS=`sudo blkid | grep -c sda`
if [ "$USBKEY_EXISTS" == "1" ]; then
	mkdir $MOUNT_DIR
	sudo mount $MOUNT_DEV $MOUNT_DIR
	# check for the recovery file
	if [ -f $MOUNT_DIR/$RECOVERY_FILE ]; then
		echo "recovery starting..."
		cp $MOUNT_DIR/$RECOVERY_FILE $RECOVERY_DEST_DIR
		sync
		rm $MOUNT_DIR/$RECOVERY_FILE
		sync
	fi
	sudo umount $MOUNT_DIR
fi

