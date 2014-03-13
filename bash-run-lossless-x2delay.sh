#!/bin/bash
radio="radio"
last_buffer=64000
log_file="a2a4_lossless_buffer_change"
back_up="/var/tmp/ln_result/from_cade"
rm $log_file
for ((buffer=$last_buffer;buffer<=4096000;buffer=$buffer*2))
do
	echo "values = $last_buffer , $buffer" >> $log_file
	perl -pi -e "s/MaxTxBufferSize \"$last_buffer\"/MaxTxBufferSize \"$buffer\"/g" lte-handover.in
	total_folder=$back_up/total/a2a4-lossless-$buffer
	mkdir $total_folder
	echo "total-folder = $total_folder" >> $log_file
	for ((i=1;i<=10;i++))
	do
		let bf=buffer/1000
		folder_name=a2a4-lossless-$bf-KB-$i
		echo "backup-folder = $folder_name" >> $log_file
		./lte-auto-ho $folder_name 50 0 $radio 1
		cp $back_up/$folder_name/$radio/TCP_LOG $total_folder/TCP_LOG-$i
		cp $back_up/$folder_name/$radio/tcp-put.dat $total_folder/tcp-put-$i.dat
	done
	last_buffer=$buffer
done

#perl -pi -e 's/EpsBearerToRlcMapping \"RlcAmAlways\"/EpsBearerToRlcMapping \"RlcUmAlways\"/g' lte.in
