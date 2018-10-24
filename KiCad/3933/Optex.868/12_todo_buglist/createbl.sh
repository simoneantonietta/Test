#!/bin/bash
# create a file for todolist and buglist
# specify filename and max value of the template buglist

if (( ("$#" == "3") && ("$1" == "n") )); then
	echo "===============================================================================" > $2
	echo "BUGLIST and TODO" >> $2
	echo "===============================================================================" >> $2
	echo "fill the list with the number in the header and refer to it for the versions" >> $2
	echo "1] [ ] P:?" >> $2
	echo "-------------------" >> $2
	echo "place an 'x' in the '[ ]' to indicates that is solved or done." >> $2
	echo "To indicate the priority put a string after the "P:?" where ? can be:" >> $2
	echo "l (low)" >> $2
	echo "m (medium)" >> $2
	echo "h (high)">> $2
	echo "u (undefined)" >> $2
	echo "_______________________________________________________________________________" >> $2
	echo "*******************************************************************************" >> $2
	echo >> $2
	for ((i = 1; i <= $3; i++)); do
		echo "===================" >> $2
		echo "$i] [ ] P:u" >> $2
		echo "-------------------" >> $2
		echo >> $2
	done
elif (( ("$#" == "4") && ("$1" == "a") )); then
	for ((i = $3; i <= $4; i++)); do
		echo "===================" >> $2
		echo "$i] [ ] P:u" >> $2
		echo "-------------------" >> $2
		echo >> $2
	done
else
	echo "Create the todolist or buglist"
	echo "USAGE:"
	echo "to create a new file:"
	echo "createbl.sh n <filename> <template values>"
	echo "to append some templates to an existing file:"
	echo "create.sh a <filename> <start new value> <end new value>"
	echo "note: <start new value> is the last number in the file +1"
fi

