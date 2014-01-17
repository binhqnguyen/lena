#!/bin/bash

##a2a4 with ctr-error=0
#echo "running a2a4 rlcam newlossless no ctr-error..."
#./x2-auto-ho-a2a4 a2a4_rlcam_newlossless_no_ctrerror 200 400 0 radio

##a3 with ctr-error=0
#echo "running a3 rlcam newlossless ctr-error..."
#./x2-auto-ho-a3 a3_rlcam_newlossless_no_ctrerror 200 400 0 radio


#perl -pi -e 's/EpsBearerToRlcMapping \"RlcAmAlways\"/EpsBearerToRlcMapping \"RlcUmAlways\"/g' lte.in

##a2a4 without ctr-error=0
#echo "running a2a4 rlcum newlossless ctr-error..."
#./x2-auto-ho-a2a4 a2a4_rlcum_newlossless_no_ctrerror 200 400 0 radio

##a3 with ctr-error=0
#echo "running a3 rlcum newlossless  ctr-error..."
#./x2-auto-ho-a3 a3_rlcum_newlossless_no_ctrerror 200 400 0 radio

#perl -pi -e 's/EpsBearerToRlcMapping \"RlcUmAlways\"/EpsBearerToRlcMapping \"RlcAmAlways\"/g' lte.in
#./bash-run-rlcam-windowssize.sh 25

#./x2-manual-ho tmp 200 150 -1 radio

./x2-auto-ho-a2a4 a2a4_newlossless 200 400 0 radio

perl -pi -e 's/EpsBearerToRlcMapping \"RlcAmAlways\"/EpsBearerToRlcMapping \"RlcUmAlways\"/g' lte.in
./x2-auto-ho-a2a4 a2a4_newseamless 200 400 0 radio
#perl -pi -e 's/EpsBearerToRlcMapping \"RlcUmAlways\"/EpsBearerToRlcMapping \"RlcAmAlways\"/g' lte.in
