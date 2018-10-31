#!/bin/bash
# installer for varcosv
# create ad-hoc for each update

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
# save data
#--------------------------
echo "** save data"
if [ -f $INSTALL_DIR/database/varcolan.db ]; then
	cp $INSTALL_DIR/database/varcolan.db $MYDIR/database/
	chmod 666 $MYDIR/database/varcolan.db
fi

if [ -f $INSTALL_DIR/database/varcolan_events.db ]; then
	cp $INSTALL_DIR/database/varcolan_events.db $MYDIR/database/
	chmod 666 $MYDIR/database/varcolan_events.db
fi	

if [ -f $INSTALL_DIR/varcosv.cfg ]; then
	# this to avoid overwrites
	cp $INSTALL_DIR/varcosv.cfg $MYDIR/
fi
cp $INSTALL_DIR/database_bkp/varcolan.db $MYDIR/database_bkp/ 2>/dev/null
cp $INSTALL_DIR/database_bkp/varcolan_events.db $MYDIR/database_bkp/ 2>/dev/null
chmod 666 $MYDIR/database_bkp/* 2>/dev/null

#--------------------------
# delete all
#--------------------------
echo "** delete previous version"
cd $INSTALL_DIR
rm varcosv varcosv.cfg varcosv.txt 2>/dev/null
rm -rf database database_bkp firmware firmware_new firmware_sched sv_scripts utils webapp
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

