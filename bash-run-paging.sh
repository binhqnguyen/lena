#!/bin/bash

./lte-paging paging_0 150 -1 radio 1

for ((i = 0; i < 1000; ))
do 
j=$i
i=$(($i+50))
echo "i = $i, j = $j"
perl -pi -e "s/pagingDelay = $j/pagingDelay = $i/g" src/lte/model/epc-sgw-pgw-application.cc
./lte-paging paging_$i 150 -1 radio 1
done



