#!/bin/bash

##a2a4 with ctr-error=0
echo "running a2a4 rlcam seamless no ctr-error..."
./x2-auto-ho-a2a4 a2a4_rlcam_seamless_no_ctrerror 200 300 0

##a3 with ctr-error=0
echo "running a3 rlcam seamless ctr-error..."
./x2-auto-ho-a3 a3_rlcam_seamless_no_ctrerror 200 300 0


perl -pi -e 's/EpsBearerToRlcMapping \"RlcAmAlways\"/EpsBearerToRlcMapping \"RlcAmAlways\"/g' lte.in

##a2a4 without ctr-error=0
echo "running a2a4 rlcum lossless ctr-error..."
./x2-auto-ho-a2a4 a2a4_rlcum_lossless_no_ctrerror 200 300 0

##a3 with ctr-error=0
echo "running a3 rlcum lossless  ctr-error..."
./x2-auto-ho-a3 a3_rlcum_lossless_no_ctrerror 200 300 0

