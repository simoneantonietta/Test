#!/bin/bash
# C/C++ version
# (by L. Mini)
# 
# USAGE:
# <exe> filename [-p]
# where:
# <exe>        is this script
# filename     is the name of the file to modify
# [-p]         (optional) print the file after the modifications


echo "Build number increment (C/C++ version)"
echo "actual dir:"
pwd

# check the command line
if [ $# -lt 1 ]; then
	echo "USAGE:"
	echo "$0 filename [-p]"
	echo "where:"
	echo "filename       name of the file to modify"
	echo "[-p]           (optional) print the file after the modifications"
	exit
fi

TMPFILE="_tempfile_.tmp"

#-----------------------------------------
# script that act the modifications
awk '
BEGIN { 
buildnumb="[0-9]+";
}
$1 == "#define" && $2 == "VERSION_BUILDNUMBER" {
if (match($3, buildnumb)) {
	val = substr($3,RSTART,RLENGTH);
	val++;
	newval=sprintf("%04d",val);
	# substitute to the line
	sub(buildnumb,newval);
	}
}
{ print } 
' $1 > $TMPFILE
#-----------------------------------------

# replace the file with the newer one
cp $TMPFILE $1
rm $TMPFILE

# print the modified file (if requested)
if [ "$2" == "-p" ]; then
	cat $1
fi

