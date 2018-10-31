#!/bin/bash
FILE="buglist_todo.txt"
CMD=$1
OPT="."
PRI="."
if [ $# -ne 1 ]; then
	echo "list the todo/buglist items"
	echo "USAGE:"
	echo "./$0 <option><priority>"
	echo "options:"
	echo "-a      all"
	echo "-d[p]   done/fixed"
	echo "-n[p]   to be done/fixed"
	echo "priority [p]:"
	echo "a       all"
	echo "u       undefined"
	echo "l       low"
	echo "m       medium"
	echo "h       high"
	exit
else
	CMD=$1
	optch=${CMD:1:1}
	prich=${CMD:2:1}
	
	# options
	if [ "$optch" == "a" ]; then
		OPT="."
	elif [ "$optch" == "d" ]; then
		OPT="x"
	elif [ "$optch" == "n" ]; then
		OPT="[[:space:]]"
	fi

	# priority
	if [ "$prich" == 'a' ]; then
		PRI="."
	elif [ "$prich" == 'u' ]; then
		PRI="u"
	elif [ "$prich" == 'l' ]; then
		PRI="l"
	elif [ "$prich" == 'm' ]; then
		PRI="m"
	elif [ "$prich" == 'h' ]; then
		PRI="h"
	fi
fi
#sed -n '/^[0-9]*\] \["$OPT")\] P:"$PRI"/,/===================/p' $FILE
sed -n "/^[0-9]*\] \["$OPT"\] P:"$PRI"/,/===================/p" $FILE


