#!/bin/bash

###CUBIC###
./lte-ues-m inc_join_3_from_3_nosack_nofrto_notimestamp 170 -1 radio 12

perl -pi -e 's/FRTO=\"0\"/FRTO=\"1\"/g' scratch/lte-ues.h
./lte-ues-m inc_join_3_from_3_nosack_frto_notimestamp 170 -1 radio 12

perl -pi -e 's/SACK=\"0\"/SACK=\"1\"/g' scratch/lte-ues.h
./lte-ues-m inc_join_3_from_3_sack_frto_notimestamp 170 -1 radio 12


perl -pi -e 's/FRTO=\"1\"/FRTO=\"0\"/g' scratch/lte-ues.h
./lte-ues-m inc_join_3_from_3_sack_nofrto_notimestamp 170 -1 radio 12

### with timestamp###

perl -pi -e 's/SACK=\"1\"/SACK=\"0\"/g' scratch/lte-ues.h
perl -pi -e 's/TIME_STAMP=\"0\"/TIME_STAMP=\"1\"/g' scratch/lte-ues.h
./lte-ues-m inc_join_3_from_3_nosack_nofrto_timestamp 170 -1 radio 12

perl -pi -e 's/FRTO=\"0\"/FRTO=\"1\"/g' scratch/lte-ues.h
./lte-ues-m inc_join_3_from_3_nosack_frto_timestamp 170 -1 radio 12

perl -pi -e 's/SACK=\"0\"/SACK=\"1\"/g' scratch/lte-ues.h
./lte-ues-m inc_join_3_from_3_sack_frto_timestamp 170 -1 radio 12

perl -pi -e 's/FRTO=\"1\"/FRTO=\"0\"/g' scratch/lte-ues.h
./lte-ues-m inc_join_3_from_3_sack_nofrto_timestamp 170 -1 radio 12





