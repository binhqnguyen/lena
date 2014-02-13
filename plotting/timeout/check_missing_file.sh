#!/bin/bash
test -e $1
if [ $? == 0 ]; then
	A=`cat $1`
	if [ $A ]; then
		echo 0
		exit
	fi 
fi
echo 1
