#!/bin/bash
TCP="TCP CUBIC"
buffer="512-pkt buffer"

if [ $# -lt 1 ]; then
  echo "Usage: <number of UE>"
  exit 1
fi
x1=$2
x2=$3
echo "Running tshark...."
#tcpdump -ttttt -r lte_ues_m-1-1.pcap | grep "length 1460" > sequence_send.all
#tcpdump -ttttt -r lte_ues_m-1-1.pcap | grep "length 0" > sequence_ack.all
tshark -r lte_ues_m-0-2.pcap | grep -w "Len=1460" > sequence_send.all
tshark -r lte_ues_m-0-2.pcap | grep -w "Len=0" > sequence_ack.all

echo "Filter sequence #...."
for ((i=2;i<=$1+2;i++))
do
grep -w "> 7.0.0.$i.10002" sequence_send.all | awk '{print $2, $3, $4, $5, $6, $7, substr($12,5,length($12)), $13, $15}' > sequence_send-700$i.all
grep -w "7.0.0.$i.10002 >" sequence_ack.all | awk '{print $2, $3, $4, $5, $6, $7, substr($13,5,length($13)), $15}' > sequence_ack-700$i.all

#grep -w "> 7.0.0.$i.10002" sequence_send.all > sequence_send-700$i.raw
#grep -w "7.0.0.$i.10002 >" sequence_ack.all > sequence_ack-700$i.raw
done
./tshark.py $1

#for ((i=2;i<=$1+2;i++))
#do
#gnuplot -e "TCP='$TCP';buffer='$buffer';x1='$x1';x2='$x2';ue='700$i'" plot_sequence.gnu
#done


