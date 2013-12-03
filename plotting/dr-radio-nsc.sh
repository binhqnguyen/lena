#!/bin/bash
ENV='/Users/binh/Documents/ln_result'
if [ "$1" != "" ];
	then
		E_NAME=$1
	else
		echo "Usage ./dr-radio-nsc.sh <TCP NAME e.i. CUBIC>"
		exit;
fi
cd $ENV\/radio/
# cd ~/Documents/ln_result/radio
pwd

echo "Removing radio_error_rx.dat ...."
rm radio_error_rx.dat


######### queues logs from log file ###########
cat < TCP_LOG | grep "Queue:" > queue
grep "0 Queue:GetTotalReceivedBytes():" queue > ue_dev_total_received.txt
grep "2 Queue:Dequeue(): m_traceDequeue" queue > endhost_dev_dequeue_tmp.txt
grep "1 Queue:Enqueue(): m_traceEnqueue" queue > enb_dev_enqueue_tmp.txt	
grep "1 Queue:Dequeue(): m_traceDequeue" queue > enb_dev_dequeue_tmp.txt	
grep "1 Queue:Drop(): m_traceDrop" queue > enb_dev_queue_drop_tmp.txt
###############################################


############################TCP CUBIC#######################
cat TCP_LOG | grep "RemoteAdd=" > cubic_all.raw
cat cubic_all.raw | grep "7.0.0.2" | grep "node 1" > cubic.dat
cat cubic_all.raw | grep "102.102.102.102" | grep "node 3" > cubic_ack.dat

################ RLC UM log ###########
cat TCP_LOG | grep "2 LteRlcUm:" > enb_rlc_um.raw
cat enb_rlc_um.raw | grep "TxBuffer is full. RLC SDU discarded" > enb_rlcum_txqueue_drop_tmp.tmp
cat enb_rlc_um.raw | grep "txBufferSize = " > enb_rlcum_txqueue_size_tmp.tmp


############# post processing: *_tmp files processing ########
./check_radio_error.py
./rlc_um_processing.py
./get_queue_time.py
########calculate retrans from cubic.dat##########
./retrans_count.py
######plot and move graph files######
gnuplot plot-averaged.txt

#######backing up#########
# cp *.svg graphs
cp -rf ../plotting-data $ENV\/backup/radio/$E_NAME
# cp  $EVN\/backup/radio/$E_NAME