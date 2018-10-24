#!/bin/bash
# check if the varcosv log file is too big, if yes
# creates a backup and start a new one

FILENAME=/tmp/varcosv.log
RUNUSER=pi

FILESIZE=$(stat -c%s "$FILENAME")
echo "Size of $FILENAME = $FILESIZE bytes."

if [ $FILESIZE -ge 100000 ]; then
	echo "log file reach the maximum size"
	if [ $UID -eq 0 ]; then
		runuser $RUNUSER -c 'mv /tmp/varcosv.log /tmp/varcosv.log.old'
	else
		mv /tmp/varcosv.log /tmp/varcosv.log.old
	fi
fi
