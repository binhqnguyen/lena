#!/bin/bash

#./lte-ues-load-increase inc_join_1_from_1_nosack_frto_timestamp_dp 170 -1 radio 10

cp src/lte/model/epc-sgw-pgw-application-delay.cc src/lte/model/epc-sgw-pgw-application.cc
cp src/lte/model/epc-sgw-pgw-application-delay.h src/lte/model/epc-sgw-pgw-application.h


./lte-ues-load-increase inc_join_1_from_1_nosack_nofrto_timestamp_dp 100 -1 radio 2

perl -pi -e 's/FRTO=\"0\"/FRTO=\"1\"/g' scratch/lte-ues.h
./lte-ues-load-increase inc_join_1_from_1_nosack_frto_timestamp_dp 100 -1 radio 2

 << 'END'
###CUBIC###
#./lte-ues-load-increase inc_join_3_from_3_nosack_nofrto_notimestamp_dp 170 -1 radio 21

#perl -pi -e 's/FRTO=\"0\"/FRTO=\"1\"/g' scratch/lte-ues.h
#./lte-ues-load-increase inc_join_3_from_3_nosack_frto_notimestamp_dp 170 -1 radio 21

#perl -pi -e 's/SACK=\"0\"/SACK=\"1\"/g' scratch/lte-ues.h
#./lte-ues-load-increase inc_join_3_from_3_sack_frto_notimestamp_dp 170 -1 radio 21


#perl -pi -e 's/FRTO=\"1\"/FRTO=\"0\"/g' scratch/lte-ues.h
#./lte-ues-load-increase inc_join_3_from_3_sack_nofrto_notimestamp_dp 170 -1 radio 21
### with timestamp###

#perl -pi -e 's/SACK=\"1\"/SACK=\"0\"/g' scratch/lte-ues.h
#perl -pi -e 's/TIME_STAMP=\"0\"/TIME_STAMP=\"1\"/g' scratch/lte-ues.h
./lte-ues-load-increase inc_join_3_from_3_nosack_nofrto_timestamp_dp 170 -1 radio 21

perl -pi -e 's/FRTO=\"0\"/FRTO=\"1\"/g' scratch/lte-ues.h
./lte-ues-load-increase inc_join_3_from_3_nosack_frto_timestamp_dp 170 -1 radio 21

perl -pi -e 's/SACK=\"0\"/SACK=\"1\"/g' scratch/lte-ues.h
./lte-ues-load-increase inc_join_3_from_3_sack_frto_timestamp_dp 170 -1 radio 21

perl -pi -e 's/FRTO=\"1\"/FRTO=\"0\"/g' scratch/lte-ues.h
./lte-ues-load-increase inc_join_3_from_3_sack_nofrto_timestamp_dp 170 -1 radio 21
END




