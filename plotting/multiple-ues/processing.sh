#!/bin/bash
#ENV='/var/tmp/ln_result'
RLC="LteRlcAm"
ENB_NODE="2" #ENB = 2, ... , 6 for HO.
UE_NODE="3"
ULPORT=20000
if [ $# -lt 3 ];
	then
	echo "Usage  <TCP NAME e.i. CUBIC> <path to processing folder i.e. radio> <total number of UEs during experiment>"
	exit 1;
fi

NUM_UE=$3
RADIO=$2
TCP_NAME=$1


#cd $ENV/$RADIO/
cd $RADIO

# cd ~/Documents/ln_result/radio
pwd

echo "***Removing radio_error_rx.dat ...."
rm radio_error_rx.dat


######### queues logs from log file ###########
echo "***Queue preparation ...."
grep "Queue:" TCP_LOG > queue
grep -w "0 Queue:GetTotalReceivedBytes():" queue > ue_dev_total_received.txt
grep -w "2 Queue:Dequeue(): m_traceDequeue" queue > endhost_dev_dequeue_tmp.txt
grep -w "1 Queue:Enqueue(): m_traceEnqueue" queue > enb_dev_enqueue_tmp.txt	
grep -w "1 Queue:Dequeue(): m_traceDequeue" queue > enb_dev_dequeue_tmp.txt	
grep -w "1 Queue:Drop(): m_traceDrop" queue > enb_dev_queue_drop_tmp.txt
###############################################


############################TCP CUBIC#######################
echo "***CUBIC cwnd preparation ...."
grep "RemoteAdd=" TCP_LOG > cubic_all.raw
for ((ue = 2; ue < $NUM_UE+2; ue++))
do
	grep -w "7\.0\.0\.$ue"  cubic_all.raw > cubic-700$ue.dat
	grep -w "102\.102\.102\.102" cubic_all.raw | grep "node $((ue+2))" > cubic_ack-700$ue.dat
done

################ RLC UM log ###########
echo "***RLC preparation ...."
grep -w " $RLC:" TCP_LOG > enb_rlc_um.raw
grep -w "TxBuffer is full. RLC SDU discarded" enb_rlc_um.raw  > enb_rlcum_txqueue_drop_tmp.tmp
grep -w "$RLC:DoTransmitPdcpPdu(): txBufferSize" TCP_LOG | grep -v "5 $RLC:DoTransmitPdcpPdu(): txBufferSize" > enb_rlcum_txqueue_size_tmp.tmp

######Send and ack flow#####
echo "***Sending flow and acking flow put preparation ...."
for ((ue = 2; ue < $NUM_UE+2; ue++))
do
	grep -w "7\.0\.0\.$ue" tcp-put.dat > tcp_send-700$ue.dat
	grep -w "1\.0\.0\.2" tcp-put.dat | grep $((ULPORT+ue-1)) > tcp_ack-700$ue.dat
	grep -w "1\.0\.0\.2" udp-put.dat | grep $((ULPORT+ue-1)) > udp_uplink-700$ue.dat
	grep -w "7\.0\.0\.$ue" udp-put.dat > udp_downlink-700$ue.dat
done

#####Joining event#######
grep "Server and " debugger.dat > join_time.dat
grep "UE attachs" debugger.dat > join_ue.dat
paste join_time.dat join_ue.dat > join.dat

###### Handover #####
grep "start handover of UE" handover.dat > handover_time.dat

########RLC AM processing#######
echo "***RLC AM parameter preparation ...."
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
echo "***Running scripts for post processing ...."
./check_radio_error.py
./rlc_um_processing.py
./get_queue_time.py

########calculate retrans from cubic.dat##########
./retrans_count.py $NUM_UE

######plot and move graph files######
echo "***Plotting graphs ...."
for ((ue_ip=0; ue_ip < $NUM_UE; ue_ip++))
do
	ue=$((ue_ip+7002))
	gnuplot -e "ue='$ue'" plot-averaged
done

./plot_sequence.sh

#######backing up#########
#BACKUP_FOLDER="$ENV/from_cade/$E_NAME"
#cp $ENV/$2 $ENV/from_cade
# cp *.svg graphs
# cp  $EVN\/backup/radio/$E_NAME
