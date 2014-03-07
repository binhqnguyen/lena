#!/bin/bash

./lte-paging paging_319_50RB_512KB_latest 130 -1 radio_dev2 1


#for ((i = 0; i < 2000; ))
#do 
#j=$i
#i=$(($i+200))
#echo "i = $i, j = $j"
#perl -pi -e "s/pagingDelay = $j/pagingDelay = $i/g" src/lte/model/epc-sgw-pgw-application.cc
#./lte-paging paging_$i\_50RB_512KB 130 -1 radio_dev2 1
#done


perl -pi -e "s/pagingDelay = 319/pagingDelay = 558/g" src/lte/model/epc-sgw-pgw-application.cc
./lte-paging paging_558_50RB_512KB_latest 130 -1 radio_dev2 1



