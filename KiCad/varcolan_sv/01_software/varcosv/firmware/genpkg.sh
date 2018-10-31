#!/bin/bash
# pass as parameter the full path of the fw without extension

echo "Generate MD5 of binary file"

#fwDIR="../02_firmware/FW/varcolan/Debug"
BINARYAXF="$1.axf"
BINARYBIN="$1.bin"
FILENAME="$1.txt"
ARCHIVE="$1.tar"


if [ -z $1 ]; then
	echo "please pass the name of the binary file without extension"
	echo "$0 <binary_filename>"
	exit
fi

#remove output file
rm $FILENAME

pwd
#convert axf -> bin
#if [ ! -f $BINARYNAME ]; then
if [ -e $BINARYAXF ]; then
	arm-none-eabi-objcopy -O binary varcolan.axf $BINARYNAME
fi
if [ -e $BINARYBIN ]; then
	#caluclate md5 and write it into output file
	md5sum -b varcolan.bin > $FILENAME

	#read Firmware versione and store it into temporary file
	VERSION=`strings "$BINARYBIN" | grep 'fw_version\[[0-9]\{2\}\.[0-9]\{2\}\]' | grep -oh '[0-9]\{2\}\.[0-9]\{2\}'`
	printf "#" >> $FILENAME
	echo "$VERSION" >> $FILENAME
	#strings "$BINARYBIN" | grep 'fw_version\[[0-9]\{2\}\.[0-9]\{2\}\]' | grep -oh '[0-9]\{2\}\.[0-9]\{2\}' >> $FILENAME	
	
	IFS='.' read -a ver <<< "$VERSION"
	
	tar -cf $1_${ver[0]}${ver[1]}.tar $BINARYBIN $FILENAME
	rm $BINARYAXF $BINARYBIN $FILENAME
fi
echo "-- DONE--"	
	



