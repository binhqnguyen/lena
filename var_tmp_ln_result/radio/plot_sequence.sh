#!/bin/bash
TCP="TCP CUBIC"
buffer="100-pkt buffer"
x1=$1
x2=$2

tcpdump -ttttt -r lena-x2-handover-1-1.pcap | grep "length 1460" > sequence_send.raw
tcpdump -ttttt -r lena-x2-handover-1-1.pcap | grep "length 0" > sequence_ack.raw
./tcpdump.py
gnuplot -e "TCP='$TCP';buffer='$buffer';x1='$x1';x2='$x2'" plot_sequence.gnu

