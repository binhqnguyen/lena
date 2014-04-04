#!/bin/bash
#ENV='/var/tmp/ln_result'
if [ $# -lt 2 ];
	then
	echo "Usage  <TCP NAME e.i. CUBIC> <path to processing folder i.e. radio>"
	exit 1;
fi

RADIO=$2
TCP_NAME=$1


#cd $ENV/$RADIO/
cd $RADIO

# cd ~/Documents/ln_result/radio
pwd


######### queues logs from log file ###########
echo "***Queue preparation ...."
grep "Queue:" TCP_LOG > queue
grep -w "2 Queue:Dequeue(): m_traceDequeue" queue > endhost_dev_dequeue_tmp.txt
grep -w "1 Queue:Enqueue(): m_traceEnqueue" queue > enb_dev_enqueue_tmp.txt	
grep -w "1 Queue:Dequeue(): m_traceDequeue" queue > enb_dev_dequeue_tmp.txt	
grep -w "1 Queue:Drop(): m_traceDrop" queue > enb_dev_queue_drop_tmp.txt
###############################################


############################TCP CUBIC#######################
echo "***CUBIC cwnd preparation ...."
grep "RemoteAdd=" TCP_LOG > cubic_all.raw
grep -w "10\.1\.3\.1"  cubic_all.raw > cubic.dat
grep -w "102\.102\.102\.102" cubic_all.raw > cubic_ack.dat

######Send and ack flow#####
echo "***Sending flow and acking flow put preparation ...."
grep -w "10\.1\.3\.1" tcp-put.dat > tcp_send.dat
grep -w "10\.1\.2\.2" tcp-put.dat > tcp_ack.dat
grep -w "10\.1\.3\.1"  udp-put.dat > udp_downlink.dat
grep -w "10\.1\.2\.2" udp-put.dat > udp_uplink.dat

############# post processing: *_tmp files processing ########
echo "***Running scripts for post processing ...."
./get_queue_time.py

########calculate retrans from cubic.dat##########
./retrans_count.py 1

./plot-sequence.sh 1

######plot and move graph files######
echo "***Plotting graphs ...."
gnuplot  plot-averaged
gnuplot -e "x1=0;x2=50" multiple-plot.gnu
