#!/bin/bash
# create the varcosv package
# createpkg <-folders>

BUILD_DIR=Debug
APP_DIR=../../../01_software/rasp_varcosv
DATA_DIR=../../../01_software/varcosv
WEB_DIR=../../../11_web
PKG_DIR=./pkg
FILES_BASE_DIR=./files
UPDATEPKG_FNAME=varcosv
APPLICATION_NAME=varcosv

MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
INITIALDIR=`pwd`
#..............................
# prepare package
echo "executing from: $MYDIR"

rm $UPDATEPKG_FNAME*.tar.gz
cd $MYDIR

# remove the previous one
rm -rf $PKG_DIR

mkdir $PKG_DIR

# create some folders
# these folders shoulb be present already in the SD
if [ $1 == '-folders' ]; then
	mkdir $PKG_DIR/database
	chmod 777 $PKG_DIR/database
	mkdir $PKG_DIR/restore_db
	chmod 777 $PKG_DIR/restore_db
	mkdir $PKG_DIR/firmware_sched
	chmod 777 $PKG_DIR/firmware_sched
	mkdir $PKG_DIR/database_bkp
	mkdir $PKG_DIR/firmware 
	mkdir $PKG_DIR/firmware_new
	mkdir $PKG_DIR/sv_scripts 
	mkdir $PKG_DIR/update
 
	mkdir $PKG_DIR/utils
	mkdir $PKG_DIR/webapp
	mkdir $PKG_DIR/database
	mkdir $PKG_DIR/.backup
fi

# copy application
cp $APP_DIR/$BUILD_DIR/$APPLICATION_NAME $PKG_DIR
if [ -e $FILES_BASE_DIR/$APPLICATION_NAME.cfg ]; then
	cp $FILES_BASE_DIR/$APPLICATION_NAME.cfg $PKG_DIR
else
	cp $APP_DIR/$APPLICATION_NAME.cfg $PKG_DIR
fi

# get application version
VERSION=`strings "$PKG_DIR/$APPLICATION_NAME" | grep 'fw_version\[[0-9]\{2\}\.[0-9]\{2\}\]' | grep -oh '[0-9]\{2\}\.[0-9]\{2\}'`
IFS='.' read -a ver <<< "$VERSION"

# get webapp version
WEB_VERSION=`grep -Po '"v.* build .*"' $WEB_DIR/inc/global.php`
#echo "webapp version: $WEB_VERSION"

# copy startup script
if [ -e $FILES_BASE_DIR/sv_startup.sh ]; then
	cp $FILES_BASE_DIR/sv_startup.sh $PKG_DIR
else
	cp $APP_DIR/$APPLICATION_NAME.cfg $PKG_DIR
fi
chmod 755  $PKG_DIR/sv_startup.sh

# copy webapp
mkdir $PKG_DIR/webapp
cp -r $WEB_DIR/* $PKG_DIR/webapp/

# copy database (if need)
if [ -d $FILES_BASE_DIR/database ]; then
	mkdir $PKG_DIR/database
	cp $FILES_BASE_DIR/database/* $PKG_DIR/database/
	chmod 666  $PKG_DIR/database/*
fi
# .. or copy scripts
if [ -e $FILES_BASE_DIR/db_varcolan_cmd.sql ] || [ -e $FILES_BASE_DIR/db_events_cmd.sql ]; then
	echo "!! scripts to alter DB found"
	cp $FILES_BASE_DIR/*.sql $PKG_DIR
fi


# sv scripts files
if [ -d $FILES_BASE_DIR/sv_scripts ]; then
	mkdir $PKG_DIR/sv_scripts
	cp $FILES_BASE_DIR/sv_scripts/* $PKG_DIR/sv_scripts/
fi

# firmware updates folder contents
if [ -d $FILES_BASE_DIR/firmware ]; then
	mkdir $PKG_DIR/firmware
	cp $FILES_BASE_DIR/firmware/unpkg.sh $PKG_DIR/firmware/
	chmod 755  $PKG_DIR/firmware/unpkg.sh
fi

# script in utils
if [ -d $FILES_BASE_DIR/utils ]; then
	mkdir $PKG_DIR/utils
fi
if [ -e $FILES_BASE_DIR/utils/unpkg.sh ]; then
	
	cp $FILES_BASE_DIR/utils/unpkg.sh $PKG_DIR/utils/
	chmod 755  $PKG_DIR/utils/unpkg.sh
fi
if [ -e $FILES_BASE_DIR/utils/killme.sh ]; then
	cp $FILES_BASE_DIR/utils/killme.sh $PKG_DIR/utils
	chmod 755 $PKG_DIR/utils/killme.sh
fi
if [ -e $FILES_BASE_DIR/utils/checklogsize.sh ]; then
	cp $FILES_BASE_DIR/utils/checklogsize.sh $PKG_DIR/utils
	chmod 755 $PKG_DIR/utils/checklogsize.sh
fi
if [ -e $FILES_BASE_DIR/utils/log.sh ]; then
	cp $FILES_BASE_DIR/utils/log.sh $PKG_DIR/utils
	chmod 755 $PKG_DIR/utils/log.sh
fi

# the installer
cp $FILES_BASE_DIR/install.sh $PKG_DIR/
chmod 755 $PKG_DIR/install.sh

#.............................

# create md5sum
cd $PKG_DIR
md5sum $APPLICATION_NAME > $APPLICATION_NAME.txt

tar -czvf ../"$UPDATEPKG_FNAME"_${ver[0]}${ver[1]}.tar.gz *

cd $MYDIR
#mv "$UPDATEPKG_FNAME"_${ver[0]}${ver[1]}.tar.gz .
rm -rf $PKG_DIR

cd $INITIALDIR
echo "*** DONE ***"
echo "package created for $VERSION version of varcolan supervisor (webapp ver. $WEB_VERSION)"

