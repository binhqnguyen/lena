#!/bin/bash

./lte-ues-m inc_join_3 200 -1 radio 13

perl -pi -e 's/uint16_t join_ue = 3/uint16_t join_ue = 6/g' scratch/lte-ues-m.cc
./lte-ues-m inc_join_6 200 -1 radio 25

perl -pi -e 's/uint16_t join_ue = 6/uint16_t join_ue = 9/g' scratch/lte-ues-m.cc
./lte-ues-m inc_join_9 200 -1 radio 37

perl -pi -e 's/uint16_t join_ue = 9/uint16_t join_ue = 12/g' scratch/lte-ues-m.cc
./lte-ues-m inc_join_12 200 -1 radio 49

perl -pi -e 's/uint16_t join_ue = 12/uint16_t join_ue = 15/g' scratch/lte-ues-m.cc
./lte-ues-m inc_join_15 200 -1 radio 41

perl -pi -e 's/uint16_t join_ue = 15/uint16_t join_ue = 18/g' scratch/lte-ues-m.cc
./lte-ues-m inc_join_18 200 -1 radio 73

perl -pi -e 's/uint16_t join_ue = 18/uint16_t join_ue = 21/g' scratch/lte-ues-m.cc
./lte-ues-m inc_join_21 200 -1 radio 85

 perl -pi -e 's/uint16_t join_ue = 21/uint16_t join_ue = 30/g' scratch/lte-ues-m.cc
./lte-ues-m inc_join_30 300 -1 radio 121


