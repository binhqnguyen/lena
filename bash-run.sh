#!/bin/bash

./x2-auto-ho-a3 a3_newlossless 200 400 0 radio

perl -pi -e 's/EpsBearerToRlcMapping \"RlcAmAlways\"/EpsBearerToRlcMapping \"RlcUmAlways\"/g' lte.in
./x2-auto-ho-a3 a3_oldseamless 200 400 0 radio


##a2a4 with ctr-error=0
#echo "running a2a4 rlcam newlossless no ctr-error..."
perl -pi -e 's/EpsBearerToRlcMapping \"RlcUmAlways\"/EpsBearerToRlcMapping \"RlcAmAlways\"/g' lte.in
perl -pi -e 's/:CtrlErrorModelEnabled \"false\"/:CtrlErrorModelEnabled \"true\"/g' lte.in
./x2-auto-ho-a2a4 a2a4_newlossless_ctrerror 200 400 0 radio
##a3 with ctr-error=0
#echo "running a3 rlcam newlossless ctr-error..."


perl -pi -e 's/:CtrlErrorModelEnabled \"true\"/:CtrlErrorModelEnabled \"false\"/g' lte.in
perl -pi -e 's/a2_servingcell_threshold = 34/a2_servingcell_threshold = 30/g' scratch/lena-x2-auto-handover.cc
./x2-auto-ho-a2a4 a2a4_newlossless_a2_30 200 400 0 radio
#perl -pi -e 's/EpsBearerToRlcMapping \"RlcAmAlways\"/EpsBearerToRlcMapping \"RlcUmAlways\"/g' lte.in
perl -pi -e 's/a2_servingcell_threshold = 30/a2_servingcell_threshold = 28/g' scratch/lena-x2-auto-handover.cc
./x2-auto-ho-a2a4 a2a4_newlossless_a2_28 200 400 0 radio

perl -pi -e 's/a2_servingcell_threshold = 28/a2_servingcell_threshold = 26/g' scratch/lena-x2-auto-handover.cc
./x2-auto-ho-a2a4 a2a4_newlossless_a2_26 200 400 0 radio

perl -pi -e 's/a2_servingcell_threshold = 26/a2_servingcell_threshold = 24/g' scratch/lena-x2-auto-handover.cc
./x2-auto-ho-a2a4 a2a4_newlossless_a2_24 200 400 0 radio

perl -pi -e 's/a2_servingcell_threshold = 24/a2_servingcell_threshold = 22/g' scratch/lena-x2-auto-handover.cc
./x2-auto-ho-a2a4 a2a4_newlossless_a2_22 200 400 0 radio

perl -pi -e 's/a2_servingcell_threshold = 20/a2_servingcell_threshold = 20/g' scratch/lena-x2-auto-handover.cc
./x2-auto-ho-a2a4 a2a4_newlossless_a2_20 200 400 0 radio

perl -pi -e 's/a2_servingcell_threshold = 20/a2_servingcell_threshold = 18/g' scratch/lena-x2-auto-handover.cc
./x2-auto-ho-a2a4 a2a4_newlossless_a2_18 200 400 0 radio

perl -pi -e 's/a2_servingcell_threshold = 18/a2_servingcell_threshold = 16/g' scratch/lena-x2-auto-handover.cc
./x2-auto-ho-a2a4 a2a4_newlossless_a2_16 200 400 0 radio

perl -pi -e 's/a2_servingcell_threshold = 16/a2_servingcell_threshold = 14/g' scratch/lena-x2-auto-handover.cc
./x2-auto-ho-a2a4 a2a4_newlossless_a2_14 200 400 0 radio

perl -pi -e 's/a2_servingcell_threshold = 14/a2_servingcell_threshold = 12/g' scratch/lena-x2-auto-handover.cc
./x2-auto-ho-a2a4 a2a4_newlossless_a2_12 200 400 0 radio

perl -pi -e 's/a2_servingcell_threshold = 12/a2_servingcell_threshold = 10/g' scratch/lena-x2-auto-handover.cc
./x2-auto-ho-a2a4 a2a4_newlossless_a2_10 200 400 0 radio


##a2a4 without ctr-error=0
#echo "running a2a4 rlcum newlossless ctr-error..."
#./x2-auto-ho-a2a4 a2a4_rlcum_newlossless_no_ctrerror 200 400 0 radio

##a3 with ctr-error=0
#echo "running a3 rlcum newlossless  ctr-error..."
#./x2-auto-ho-a3 a3_rlcum_newlossless_no_ctrerror 200 400 0 radio

#perl -pi -e 's/EpsBearerToRlcMapping \"RlcUmAlways\"/EpsBearerToRlcMapping \"RlcAmAlways\"/g' lte.in
#./bash-run-rlcam-windowssize.sh 25

#./x2-manual-ho tmp 200 150 -1 radio

#./x2-auto-ho-a2a4 a2a4_newlossless 200 400 0 radio

#perl -pi -e 's/EpsBearerToRlcMapping \"RlcAmAlways\"/EpsBearerToRlcMapping \"RlcUmAlways\"/g' lte.in
#./x2-auto-ho-a2a4 a2a4_oldseamless 200 400 0 radio
#perl -pi -e 's/EpsBearerToRlcMapping \"RlcUmAlways\"/EpsBearerToRlcMapping \"RlcAmAlways\"/g' lte.in
