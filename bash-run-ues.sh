#!/bin/bash

###CUBIC###
./lte-ues-m inc_join_1_from_1_withtimeout 170 -1 radio 4

perl -pi -e 's/uint16_t join_ue = 1/uint16_t join_ue = 3/g' scratch/lte-ues-m.cc
./lte-ues-m inc_join_3_from_1_withtimeout 170 -1 radio 10

perl -pi -e 's/uint16_t join_ue = 3/uint16_t join_ue = 6/g' scratch/lte-ues-m.cc
./lte-ues-m inc_join_6_from_1_withtimeout 170 -1 radio 19

perl -pi -e 's/uint16_t join_ue = 6/uint16_t join_ue = 9/g' scratch/lte-ues-m.cc
./lte-ues-m inc_join_9_from_1_withtimeout 170 -1 radio 28

perl -pi -e 's/uint16_t join_ue = 9/uint16_t join_ue = 12/g' scratch/lte-ues-m.cc
./lte-ues-m inc_join_12_from_1_withtimeout 170 -1 radio 37

perl -pi -e 's/uint16_t join_ue = 12/uint16_t join_ue = 15/g' scratch/lte-ues-m.cc
./lte-ues-m inc_join_15_from_1_withtimeout 170 -1 radio 46

#perl -pi -e 's/uint16_t join_ue = 15/uint16_t join_ue = 18/g' scratch/lte-ues-m.cc
#./lte-ues-m inc_join_18 170 -1 radio 73

#perl -pi -e 's/uint16_t join_ue = 18/uint16_t join_ue = 21/g' scratch/lte-ues-m.cc
#./lte-ues-m inc_join_21 170 -1 radio 85

#perl -pi -e 's/uint16_t join_ue = 21/uint16_t join_ue = 30/g' scratch/lte-ues-m.cc
#./lte-ues-m inc_join_30 300 -1 radio 121


