#!/bin/bash
cd ~/ln/
SIM_TIME=100
INC=100


for ((d=50; d<= 550; d+=$INC))
do
	if [ $d -le 1000 ]; then
		INC=20
	fi
	#if [ $d -gt 1000 -a $d -le 2000 ]; then
	#	INC=100
	#fi
	#if [ $d -gt 2000 -a $d -le 2500 ]; then
	#	INC=30
	#fi
	#if [ $d -gt 12000 -a $d -le 17000 ]; then
	#	INC=100
	#fi
	echo "$d $INC"
	./rp-radio-nsc.sh PED_reno_$d $SIM_TIME $d 1
	#./rp-radio-nsc.sh VEH_$d $SIM_TIME $d 0
done





