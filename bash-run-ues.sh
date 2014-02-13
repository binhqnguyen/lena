#!/bin/bash

./lte-ues-m inc_join_3_from_3_reno_withtimeout 170 -1 radio 12

perl -pi -e 's/uint16_t join_ue = 3/uint16_t join_ue = 6/g' scratch/lte-ues-m.cc
./lte-ues-m inc_join_6_from_3_reno_withtimeout 170 -1 radio 21

perl -pi -e 's/uint16_t join_ue = 6/uint16_t join_ue = 9/g' scratch/lte-ues-m.cc
./lte-ues-m inc_join_9_from_3_reno_withtimeout 170 -1 radio 30


###CUBIC###
perl -pi -e 's/uint16_t join_ue = 9/uint16_t join_ue = 3/g' scratch/lte-ues-m.cc
perl -pi -e 's/TCP_VERSION=\"reno\"/TCP_VERSION=\"cubic\"/g' scratch/lte-ues.h
./lte-ues-m inc_join_3_from_3_withtimeout 170 -1 radio 12

perl -pi -e 's/uint16_t join_ue = 3/uint16_t join_ue = 6/g' scratch/lte-ues-m.cc
./lte-ues-m inc_join_6_from_3_withtimeout 170 -1 radio 21

perl -pi -e 's/uint16_t join_ue = 6/uint16_t join_ue = 9/g' scratch/lte-ues-m.cc
./lte-ues-m inc_join_9_from_3_withtimeout 170 -1 radio 30

perl -pi -e 's/uint16_t join_ue = 9/uint16_t join_ue = 12/g' scratch/lte-ues-m.cc
./lte-ues-m inc_join_12_from_3_withtimeout 170 -1 radio 39

perl -pi -e 's/uint16_t join_ue = 12/uint16_t join_ue = 15/g' scratch/lte-ues-m.cc
./lte-ues-m inc_join_15_from_3_withtimeout 170 -1 radio 48

#perl -pi -e 's/uint16_t join_ue = 15/uint16_t join_ue = 18/g' scratch/lte-ues-m.cc
#./lte-ues-m inc_join_18 170 -1 radio 73

#perl -pi -e 's/uint16_t join_ue = 18/uint16_t join_ue = 21/g' scratch/lte-ues-m.cc
#./lte-ues-m inc_join_21 170 -1 radio 85

#perl -pi -e 's/uint16_t join_ue = 21/uint16_t join_ue = 30/g' scratch/lte-ues-m.cc
#./lte-ues-m inc_join_30 300 -1 radio 121


