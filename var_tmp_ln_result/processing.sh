#!/bin/bash
ENV='/var/tmp/ln_result'
RLC="LteRlcAm"
ENB_NODE="2" #ENB = 2, ... , 6 for HO.
UE_NODE="3"
if [ "$1" != "" -a "$2" != "" ];
	then
		E_NAME=$1
	else
		echo "Usage ./dr-radio-nsc-cade.sh <TCP NAME e.i. CUBIC> <Processing folder i.e. radio>"
		exit;
fi
cd $ENV/$2/

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
cat TCP_LOG | grep "2 $RLC:" > enb_rlc_um.raw
cat enb_rlc_um.raw | grep "TxBuffer is full. RLC SDU discarded" > enb_rlcum_txqueue_drop_tmp.tmp
cat TCP_LOG | grep "$RLC:DoTransmitPdcpPdu(): txBufferSize" | grep -v "5 $RLC:DoTransmitPdcpPdu(): txBufferSize" > enb_rlcum_txqueue_size_tmp.tmp

######Send and ack flow#####
cat tcp-put.dat | grep "7.0.0.2" > tcp_send.dat
cat tcp-put.dat | grep "1.0.0.2" > tcp_ack.dat
cat udp-put.dat | grep "1.0.0.2" > udp_uplink.dat
cat udp-put.dat | grep "7.0.0.2" > udp_downlink.dat


cat handover.dat | grep "start handover of UE" > handover_time.dat

########RLC AM processing#######
#Get ACKed Rlc Pdu Seq reported at enodeb:
grep "$ENB_NODE LteRlcAm:DoReceivePdu(): ACKed SN =" TCP_LOG | awk '{ print substr($1,1,length($1)-1), $2,$3, $4, $5, $6;}' > ack_seq.enb
#Get VT(S) and VT(A) of enodeb's Rlc Pdus using buffer report function. VT(S) = send sequence #, VT(A) = ack seq #.
grep "$ENB_NODE LteRlcAm:DoReportBufferStatus(): VT(S) = " TCP_LOG | awk '{print substr($1,1,length($1)-1), $2,$3, $4, $5, $6;}' > vtS.enb
grep "$ENB_NODE LteRlcAm:DoReportBufferStatus(): VT(A) = " TCP_LOG | awk '{print substr($1,1,length($1)-1), $2,$3, $4, $5, $6;}' > vtA.enb
#Get receiver (UE) Rlc Pdus status:
#VR(R) = received SN, lower edge of RlcAm window.
#VR(MR) = VR(R) + RlcAm_window size.
#VR(X) = SN at which reordering happen (missing received Pdus detected).
#VR(MS) = maximum SN that the STATUS PDU can ack.
#VR(H) = most recent received Rlc Pdu SN. 
grep "$UE_NODE LteRlcAm:DoReceivePdu(): VR(R)" TCP_LOG | awk '{print substr($1,1,length($1)-1), $2,$3, $4, $5, $6;}' > vrR.ue
grep "$UE_NODE LteRlcAm:DoReceivePdu(): VR(MR)" TCP_LOG | awk '{print substr($1,1,length($1)-1), $2,$3, $4, $5, $6;}' > vrMR.ue
grep "$UE_NODE LteRlcAm:DoReceivePdu(): VR(X)" TCP_LOG | awk '{print substr($1,1,length($1)-1), $2,$3, $4, $5, $6;}' > vrX.ue
grep "$UE_NODE LteRlcAm:DoReceivePdu(): VR(MS)" TCP_LOG | awk '{print substr($1,1,length($1)-1), $2,$3, $4, $5, $6;}' > vrMS.ue
grep "$UE_NODE LteRlcAm:DoReceivePdu(): VR(H)" TCP_LOG | awk '{print substr($1,1,length($1)-1), $2,$3, $4, $5, $6;}' > vrH.ue


############# post processing: *_tmp files processing ########
./check_radio_error.py
./rlc_um_processing.py
./get_queue_time.py
########calculate retrans from cubic.dat##########
./retrans_count.py
######plot and move graph files######
gnuplot plot-averaged
./plot_sequence.sh

#######backing up#########
# cp *.svg graphs
#cp -rf ../ $ENV/backup/radio/$E_NAME
# cp  $EVN\/backup/radio/$E_NAME
