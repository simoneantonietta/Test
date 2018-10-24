#!/bin/bash
# startup script
# this file must resides in /data

INSTALL_DIR=/data
DIR_UPDATE=update
DIR_UPDATE_SCHED=firmware_sched
DIR_RESTOREDB=restore_db
DIR_DATABASE=database
UPDATEPKG_FNAME=varcosv
APPLICATION_NAME=varcosv
RUNUSER=pi

MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

UPDATE_FILE=""

#..............................................
# check for the presence of an update
function check_update {
# if that is in the update dir, may be one in the scheduled one
if [ "$(ls -A $INSTALL_DIR/$DIR_UPDATE_SCHED/$UPDATEPKG_*.tar.gz 2>/dev/null)" ]; then
	for f in $INSTALL_DIR/$DIR_UPDATE_SCHED/$UPDATEPKG_*.tar.gz
		do
			echo "new software found: $f but in scheduled, will be moved to update folder"
			mv $f $INSTALL_DIR/$DIR_UPDATE
			sync
		done
fi

# check if the update folder is empty
#VAR=[ "$(ls -A .. 2>/dev/null)" ] && echo "Not Empty" || echo "Empty"
if [ "$(ls -A $INSTALL_DIR/$DIR_UPDATE/$UPDATEPKG_*.tar.gz 2>/dev/null)" ]; then
	for f in $INSTALL_DIR/$DIR_UPDATE/$UPDATEPKG_*.tar.gz
		do
			echo "new software found: $f"
			UPDATE_FILE=$f
		done
fi

}

#..............................................
# check for the presence of a db to be restored
function check_restoredb {
if [ -f $INSTALL_DIR/$DIR_RESTOREDB/varcolan.db ]; then
	rm -f $INSTALL_DIR/$DIR_DATABASE/varcolan.db
	mv $INSTALL_DIR/$DIR_RESTOREDB/varcolan.db $INSTALL_DIR/$DIR_DATABASE/
	chmod 666 $INSTALL_DIR/$DIR_DATABASE/varcolan.db
fi
}

#..............................................
# install a new software
function install_update {

cd $INSTALL_DIR/$DIR_UPDATE
tar xvzf $1
res=`md5sum -c $APPLICATION_NAME.txt | grep OK`
if [ -n "$res" ]; then
    echo "integrity check: OK"
   	# executes the install script
	if [ $UID -eq 0 ]; then
		runuser $RUNUSER -c './install.sh'
	else
   	./install.sh
	fi
else
    echo "integrity check: FAIL"
fi
}

#..............................................
# run the application and respawn if it crashes
function run_application {

# start
while true
do 
	cd $INSTALL_DIR

	
	if [ $UID -eq 0 ]; then
		runuser $RUNUSER -c './varcosv >> /tmp/varcosv.log'
	else
		./$APPLICATION_NAME
	fi
	
	echo "***********************************************"
	echo "application terminated or crashed -> respawn..."
	echo "***********************************************"
	sleep 1
	
	# save the event database
	cp /tmp/varcolan_events.db $INSTALL_DIR/$DIR_DATABASE/
	sync
	
	# check for update
	UPDATE_FILE=""
	check_update
	if [ -n "$UPDATE_FILE" ]; then
		install_update $UPDATE_FILE
	fi
	
	check_restoredb
done
}

#----------------------------------------------
# MAIN
#----------------------------------------------
check_update

# need to apache2
mkdir /tmp/sessions
chmod 777 /tmp/sessions

if [ -n "$UPDATE_FILE" ]; then
	install_update $UPDATE_FILE
fi

# add permission to database to ensure webapp to use it
chmod 777 $INSTALL_DIR/$DIR_DATABASE/*.db

run_application



