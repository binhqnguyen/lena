	#!/bin/bash
	ENV="/var/tmp/"
	# ENV="~/Documents/ln_result"
 	E_NAME=$1
 	# PY_DIR=$2
	cd $ENV\ln_result/emulated/
	pwd

	TCP='CUBIC'

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
cat cubic_all.raw | grep "10.1.3.1" | grep "node 2" > cubic.dat
cat cubic_all.raw | grep "102.102.102.102" | grep "node 0" > cubic_ack.dat

############# post processing: *_tmp files processing ########
	./get_queue_time.py 
########calculate retrans from cubic.dat##########
	./retrans_count.py 

#######get (relative) sequence number from pcap files######
tcpdump  -ttttt -r emulated_core-2-0.pcap | grep "length 0" > sequence_ack.raw
tcpdump  -ttttt -r emulated_core-2-0.pcap | grep "length 1460" > sequence_send.raw
./tcpdump.py
######plot and move graph files######
	gnuplot plot.gnu

	#######backing up#########
	cp *.svg graphs
	cp *.* $ENV\ln_result/backup/emulated/$E_NAME
	cp graphs/*.* $ENV\ln_result/backup/emulated/$E_NAME

