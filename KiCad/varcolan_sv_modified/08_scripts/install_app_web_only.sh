#!/bin/bash
# installer for varcosv
# create ad-hoc for each update
#
# simple app and webapp version
#

INSTALL_DIR=/data
APPLICATION_NAME=varcosv
UPDATEPKG_FNAME="varcosv"
BACKUP_DIR=.backup

MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
#--------------------------
# backup previous version
#--------------------------
echo "** backup old version"
OLD_VERSION=`strings "$APPLICATION_NAME" | grep 'fw_version\[[0-9]\{2\}\.[0-9]\{2\}\]' | grep -oh '[0-9]\{2\}\.[0-9]\{2\}'`
tar czvf $INSTALL_DIR/.backup/$APPLICATION_NAME_$OLD_VERSION.tar.gz $INSTALL_DIR/* --exclude='$BACKUP_DIR' --exclude='firmware_new' --exclude='firmware_sched' --exclude='restore_db' --exclude='firmware' --exclude='update'

#--------------------------
# remove myself package
#--------------------------
echo "** remove package"
rm $UPDATEPKG_FNAME_*.tar.gz

#--------------------------
# delete all
#--------------------------
echo "** delete previous version"
cd $INSTALL_DIR
rm varcosv varcosv.txt 2>/dev/null
rm -rf webapp
cd $MYDIR

#--------------------------
# install the new package
#--------------------------
echo "** installing..."
rm install.sh
cp -r * $INSTALL_DIR

#--------------------------
# clean up
#--------------------------
echo "** clean up"
rm -rf $INSTALL_DIR/update/*
sync
sleep 2
#--------------------------
# reboot
#--------------------------
echo "** reboot"
sudo reboot

echo "--- DONE ---"

