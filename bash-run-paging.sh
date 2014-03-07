#!/bin/bash

./lte-paging paging_100_50RB_512KB 130 -1 radio_dev2 1

for ((i = 0; i < 2000; ))
do 
j=$i
i=$(($i+200))
echo "i = $i, j = $j"
perl -pi -e "s/pagingDelay = $j/pagingDelay = $i/g" src/lte/model/epc-sgw-pgw-application.cc
./lte-paging paging_$i\_50RB_512KB 130 -1 radio_dev2 1
done


#perl -pi -e "s/pagingDelay = 1000/pagingDelay = 400/g" src/lte/model/epc-sgw-pgw-application.cc
#./lte-paging paging_400_25RB_700KB 150 -1 radio_dev2 1



