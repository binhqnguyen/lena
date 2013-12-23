#!/bin/bash

##a2a4 with ctr-error=0
echo "running a2a4 rlcam lossless no ctr-error..."
./x2-auto-ho-a2a4 a2a4_rlcam_lossless_no_ctrerror 200 300 0 radio_dev

##a3 with ctr-error=0
echo "running a3 rlcam lossless ctr-error..."
./x2-auto-ho-a3 a3_rlcam_lossless_no_ctrerror 200 300 0 radio_dev


perl -pi -e 's/EpsBearerToRlcMapping \"RlcAmAlways\"/EpsBearerToRlcMapping \"RlcUmAlways\"/g' lte.in

##a2a4 without ctr-error=0
echo "running a2a4 rlcum lossless ctr-error..."
./x2-auto-ho-a2a4 a2a4_rlcum_lossless_no_ctrerror 200 300 0 radio_dev

##a3 with ctr-error=0
echo "running a3 rlcum lossless  ctr-error..."
./x2-auto-ho-a3 a3_rlcum_lossless_no_ctrerror 200 300 0 radio_dev

perl -pi -e 's/EpsBearerToRlcMapping \"RlcUmAlways\"/EpsBearerToRlcMapping \"RlcAmAlways\"/g' lte.in
./bash-run-rlcam-windowssize.sh 


perl -pi -e 's/EpsBearerToRlcMapping \"RlcUmAlways\"/EpsBearerToRlcMapping \"RlcUmAlways\"/g' lte.in
