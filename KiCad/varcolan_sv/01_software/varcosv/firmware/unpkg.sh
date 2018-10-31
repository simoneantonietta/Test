#!/bin/bash

if [ -z $1 ]; then
	echo "$0 <package whithout extension> <result file>"
	exit
fi

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

tar -xf $1.tar
md5sum -c $1.txt > $2

if [ "$3"=="rmpack" ]; then
	rm $1.tar
fi
