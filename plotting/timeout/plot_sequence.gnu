
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

reset
set title "Packet sequence ".TCP.", ".buffer
set key inside bottom right box
set xlabel "Time (s)"
#set xtic 10
set ylabel "packet sequence #"
set y2label "count"
set output "sequence-".x1."-".x2.".svg"
set y2tics nomirror tc lt 2
set y2range [0:8]
#set yrange [0:1200]
set xrange [x1:x2]
#set x2range [x1:x2]
#set y2tics nomirror tc lt 4
#set log y2

set terminal svg


plot "sequence_send.dat" using 1:2 title "send seq number" pt 0,\
"sequence_ack.dat" using 1:2 title "ack seq number" pt 0,\
"enb_dev_queue_drop.txt" using 1:(0) title "enb queue droptail" pt 2,\
"retrans.dat" using 1:(0):($2) title "retransmissions" with points pt 6 ps variable
#"sequence_send.dat" using 1:3 title "packets per send" pt 1 axis x1y2
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




