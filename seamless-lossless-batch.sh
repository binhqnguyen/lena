#!/bin/bash

./bash-run-lossless.sh

perl -pi -e 's/EpsBearerToRlcMapping \"RlcAmAlways\"/EpsBearerToRlcMapping \"RlcUmAlways\"/g' lte-handover.in
perl -pi -e 's/MaxTxBuffer \"4096000\"/EpsBearerToRlcMapping \"64000\"/g' lte-handover.in

./bash-run-seamless.sh
