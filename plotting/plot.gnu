
##======CONST=======
#TCP="CUBIC"
#BUFF="50 pkts"
#BUFFSIZE=50
#x1 = 0
#x2 = 100
SEGSIZE=1448
PKTSIZE=1500
#CONST_A=BUFFSIZE
#CONST_B=BUFFSIZE-1
#CONST_C=BUFFSIZE-2
#CONST_D=BUFFSIZE-3

CONST_A=1
CONST_B=2
CONST_C=3
CONST_D=4

#=============Functions=============
cnt=0
increment(cnt) = (cnt=cnt+1,cnt)

filter(x)=((x<100||x>1000000)?1/0:x)
filter_1st_ssth(x)=((x<1000000)?x:1/0)

#======== Tx/Rx rate ========
reset
set title "Rate: ".TCP.", Buffer ".buffer
set key inside bottom right box
set xlabel "Time (s)"
#set xtic 10
set xrange [x1:x2]
set x2range [x1:x2]
#set y2label "Congestion window (Bytes)"
#set yrange [0:2000]
#set xrange [0:100]
set ylabel "Rate (kbps)"
set output "rate-".x1."-".x2.".svg"
#set y2tics nomirror tc lt 6

set terminal svg


plot "udp-put.txt" using 1:3 title "Radio bandwidth" with lines,\
"tcp-put.txt" using 1:3 title "TCP throughput (Rx rate)" with lines,\
"tcp-put.txt" using 1:11 title "TCP Tx rate" with lines,\
"retrans.dat" using 1:(0):($2) title "retransmissions" with points pt 6 ps variable
#"cwnd.txt" using 1:5 title "cwnd" with lines axis x2y2

#"tcp-put.txt" using 1:12 title "TCP delay" with lines axis x2y2,\
#"tcp-put.txt" using 1:13 title "TCP ack delay" with lines axis x2y2

##======== Rx rate only ========
reset
set title "Rx rate ".TCP.", Buffer ".buffer
set key inside bottom right box
set xlabel "Time (s)"
#set xtic 5
set xrange [x1:x2]
set x2range [x1:x2]
set ylabel "Rx Rate (kbps)"
set output "rx_rate-".x1."-".x2.".svg"
#set y2tics nomirror tc lt 6

set terminal svg


plot "udp-put.txt" using 1:3 title "Radio bandwidth" with lines,\
"tcp-put.txt" using 1:3 title "TCP throughput (Rx rate)" with lines,\
"retrans.dat" using 1:(0):($2) title "retransmissions" with points pt 6 ps variable
#===========================================

#========= Cwnd ============
reset
set title "Congestion window ".TCP.", ".buffer
set key inside top right box
set xlabel "Time (s)"
#set xtic 10
set ylabel "cwnd (segments)"
set y2label "delay (ms)"
set output "cwnd-detail-".x1."-".x2.".svg"
#set y2tics nomirror tc lt 2
#set y2range [0:150000]
#set yrange [0:1200]
set xrange [x1:x2]
set x2range [x1:x2]
set y2tics nomirror tc lt 2

set terminal svg


plot "cubic.dat" using 1:7 title "cwnd" with lines,\
"tcp-put.txt" using 1:12 title "TCP delay" pt 0 axis x2y2,\
"retrans.dat" using 1:(0):($2) title "retransmissions" with points pt 6 ps variable,\
"queues.txt" using 2:($4/PKTSIZE) title "enb_radio_queue" with lines lt -1,\
"cubic.dat" using 1:15 title "Wmax" pt 0
#"newack_partial.txt" using 3:15 title "newack in FR" pt 0,\
#"newack_fullack.txt" using 3:17 title "received full ack, leaving FR" pt 0,\
#"Dupack_FASTRECOVERY.txt" using 3:15 title "Dupack IN FR" pt 0
#===========================================
reset
set title "Congestion window ".TCP.", ".buffer
set key inside top right box
set xlabel "Time (s)"
#set xtic 10
set ylabel "value (segments)"
set y2label "drops"
set output "cwnd-".x1."-".x2.".svg"
#set y2tics nomirror tc lt 2
#set y2range [0:10]
#set yrange [0:1200]
set xrange [x1:x2]
set x2range [x1:x2]
set y2tics nomirror tc lt 6
#set log y2

set terminal svg

plot "cubic.dat" using 1:7 title "cwnd" with lines,\
"cubic.dat" using 1:(filter_1st_ssth($11)) title "ssthreshold" with lines,\
"queues.txt" using 2:($4/PKTSIZE) title "enb_radio_queue" with lines,\
"enb_dev_queue_drop.txt" using 1:(CONST_C) title "enb queue droptail" with points pt 6 axis x2y2,\
"retrans.dat" using 1:(0):($2) title "retransmissions" with points pt 6 ps variable
#"enb_dev_dequeue.txt" using 1:(CONST_A) title "eNodeB's queue dequeuing pkts" axis x2y2 pt 0,\
#"endhost_dev_dequeue.txt" using 1:(CONST_B) title "endhost's queue dequeuing pkts" axis x2y2 pt 0,\
#===========================================
reset
set title "Packet sequence ".TCP.", ".buffer
set key inside bottom right box
set xlabel "Time (s)"
#set xtic 10
set ylabel "packet sequence #"
#set y2label "queue droptail"
set output "sequence-".x1."-".x2.".svg"
#set y2tics nomirror tc lt 2
#set y2range [0:1]
#set yrange [0:1200]
set xrange [x1:x2]
#set x2range [x1:x2]
#set y2tics nomirror tc lt 4
#set log y2

set terminal svg


plot "sequence_send.dat" using 1:2 title "send seq number" pt 1,\
"sequence_send.dat" using 1:3 title "packets per send" pt 1 axis x1y2,\
"sequence_ack.dat" using 1:2 title "ack seq number" pt 1,\
"enb_dev_queue_drop.txt" using 1:(0) title "enb queue droptail" pt 2 axis x1y2,\
"retrans.dat" using 1:(0):($2) title "retransmissions" with points pt 6 ps variable
#plot "highest_sent_seq.txt" using 1:3 title "highest sent sequence #" pt 1,\


#===========================================
reset
set title "TCP rtt ".TCP.", ".buffer
set key inside top right box
set xlabel "Time (s)"
#set xtic 10
set ylabel "time (ms)"
#set y2label "queue size (Packets)"
set output "rtt-".x1."-".x2.".svg"
#set y2tics nomirror tc lt 2
#set y2range [0:1200]
#set yrange [450:600]
set xrange [x1:x2]
#set x2range [x1:x2]
#set y2tics nomirror tc lt 8
#set log y2

set terminal svg


plot "cubic.dat" using 1:9 title "rtt" with lines,\
"cubic.dat" using 1:17 title "rttvar" with lines,\
"retrans.dat" using 1:(0):($2) title "retransmissions" with points pt 6 ps variable
#"rtt_value.txt" using 1:($2*1000) title "rtt value" pt 1,\
#plot "rtt_estimator_rto.txt" using 1:($2*1000) title "retransmit timeout value" with lines,\
#"measured_rtt.txt" using 1:($2/1000000) title "measured rtt" pt 0 with lines,\




