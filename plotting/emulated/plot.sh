#!/bin/bash
TCP=$1
buffer=$2
x1=$3
x2=$4

tcpdump -ttttt -r emulated_core-2-0.pcap | grep "length 1460" > sequence_send.raw
tcpdump -ttttt -r emulated_core-2-0.pcap | grep "length 0" > sequence_ack.raw
./tcpdump.py
gnuplot -e "TCP='$TCP';buffer='$buffer';x1='$x1';x2='$x2'" plot.gnu

