#!/bin/bash
# create the varcosv package
# createpkg <installer_script> <-app|-full> 

BUILD_DIR=Debug
APP_DIR=../01_software/rasp_varcosv
DATA_DIR=../01_software/varcosv
WEB_DIR=../11_web
PKG_DIR=./pkg
UPDATEPKG_FNAME=varcosv
APPLICATION_NAME=varcosv
NEW_PACKAGE_DIR=../01_software/packages
OLD_PACKAGES_DIR=../01_software/packages/old_packages

MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
INITIALDIR=`pwd`
#..............................
# prepare package
cd $MYDIR

mv $NEW_PACKAGE_DIR/$UPDATEPKG_FNAME_*.tar.gz $OLD_PACKAGES_DIR 2>/dev/null

# remove the previous one
rm -rf $PKG_DIR

mkdir $PKG_DIR

# create some folders
if [ $1 == '-full' ]; then
	mkdir $PKG_DIR/database
	chmod 777 $PKG_DIR/database
	mkdir $PKG_DIR/database_bkp
	mkdir $PKG_DIR/firmware 
	mkdir $PKG_DIR/firmware_new
	mkdir $PKG_DIR/firmware_sched
	chmod 777 $PKG_DIR/firmware_sched
	mkdir $PKG_DIR/sv_scripts 
	mkdir $PKG_DIR/update
fi
mkdir $PKG_DIR/utils
mkdir $PKG_DIR/webapp
mkdir -p $PKG_DIR/.backup

# copy data to be packaged
cp $APP_DIR/$BUILD_DIR/$APPLICATION_NAME $PKG_DIR
if [ $1 == '-full' ]; then
	cp $APP_DIR/$APPLICATION_NAME.cfg $PKG_DIR
fi

# get the version
VERSION=`strings "$PKG_DIR/$APPLICATION_NAME" | grep 'fw_version\[[0-9]\{2\}\.[0-9]\{2\}\]' | grep -oh '[0-9]\{2\}\.[0-9]\{2\}'`
IFS='.' read -a ver <<< "$VERSION"

if [ $1 == '-full' ]; then
	cp $DATA_DIR/database/* $PKG_DIR/database/
	chmod 666  $PKG_DIR/database/*
	cp $DATA_DIR/sv_scripts/* $PKG_DIR/sv_scripts/
fi

cp -r $WEB_DIR/* $PKG_DIR/webapp/

cp sv_startup.sh $PKG_DIR
chmod 755  $PKG_DIR/sv_startup.sh

if [ $1 == '-full' ]; then
	cp unpkg.sh $PKG_DIR/firmware/
	chmod 755  $PKG_DIR/unpkg.sh
fi

cp unpkg.sh $PKG_DIR/utils/
chmod 755  $PKG_DIR/utils/unpkg.sh

# select the installer
if [ $# -eq 2 ]; then
    echo "Use the installer: $2"
    cp $2 $PKG_DIR/install.sh
else
    echo "Use the default install.sh"
    cp install.sh $PKG_DIR/
fi
chmod 755 $PKG_DIR/install.sh

cp checklogsize.sh $PKG_DIR/utils/
chmod 755 $PKG_DIR/utils/checklogsize.sh

cp killme.sh $PKG_DIR/utils
chmod 755 $PKG_DIR/utils/killme.sh

# create md5sum
cd $PKG_DIR
md5sum $APPLICATION_NAME > $APPLICATION_NAME.txt

tar -czvf ../"$UPDATEPKG_FNAME"_${ver[0]}${ver[1]}.tar.gz *

cd $MYDIR
mv "$UPDATEPKG_FNAME"_${ver[0]}${ver[1]}.tar.gz $NEW_PACKAGE_DIR
rm -rf $PKG_DIR

cd $INITIALDIR

