#!/bin/bash

##a2a4 with ctr-error=0
echo "running a2a4 no ctr-error..."
./x2-auto-ho-a2a4 a2a4_auto_ho_no_ctr_error 200 300 0

##a3 with ctr-error=0
echo "running a3 no ctr-error..."
./x2-auto-ho-a3 a3_auto_ho_no_ctr_error 200 300 0

##a2a4 with ctr-error=1
echo "running a2a4 with ctr-error..."
perl -pi -e 's/CtrlErrorModelEnabled \"false\"/CtrlErrorModelEnabled \"true\"/g' 
./x2-auto-ho-a2a4 a2a4_auto_ho_with_ctr_error 200 300 0

##a3 with ctr-error=1
echo "running a3 with ctr-error..."
./x2-auto-ho-a3 a3_auto_ho_with_ctr_error 200 300 0
