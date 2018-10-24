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
# delete all
#--------------------------
echo "** delete previous version"
cd $INSTALL_DIR
rm varcosv varcosv.txt 2>/dev/null
rm -rf webapp/*
cd $MYDIR

#--------------------------
# install the new package
#--------------------------
echo "** installing..."
cp -r * $INSTALL_DIR

#--------------------------
# alter database
#--------------------------
if [ -e $INSTALL_DIR/db_varcolan_cmd.sql ]; then
	cp $INSTALL_DIR/database/varcolan.db $INSTALL_DIR/database_bkp
	sqlite3 $INSTALL_DIR/database/varcolan.db < $INSTALL_DIR/db_varcolan_cmd.sql
	rm $INSTALL_DIR/db_varcolan_cmd.sql
fi
if [ -e $INSTALL_DIR/db_events_cmd.sql ]; then
	cp $INSTALL_DIR/database/varcolan_events.db $INSTALL_DIR/database_bkp
	sqlite3 $INSTALL_DIR/database/varcolan_events.db < $INSTALL_DIR/db_events_cmd.sql
	rm $INSTALL_DIR/db_events_cmd.sql
fi

#--------------------------
# clean up
#--------------------------
echo "** clean up"
rm -rf $INSTALL_DIR/update/*
rm $INSTALL_DIR/install.sh
rm $INSTALL_DIR/varcosv.txt
sync
sleep 2
#--------------------------
# reboot
#--------------------------
echo "** reboot"
sudo reboot

echo "--- DONE ---"

