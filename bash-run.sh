#!/bin/bash
cd ~/ln_/

./rp-emulated-nsc.sh des_reno_300kbps

#no.1: no error#
#no.2: 46 noise, with error, no harq##
perl -pi -e 's/TCP_VERSION=\"reno\"/TCP_VERSION=\"vegas\"/g' scratch/emulated_nsc.cc
./rp-emulated-nsc.sh des_vegas_300kbps


perl -pi -e 's/TCP_VERSION=\"vegas\"/TCP_VERSION=\"westwood\"/g' scratch/emulated_nsc.cc
./rp-emulated-nsc.sh des_westwood_300kbps


##=============20pkts buffer increasing===========
perl -pi -e 's/::MaxPackets \"100\"/::MaxPackets \"20\"/g' emulated-nsc.in
perl -pi -e 's/TCP_VERSION=\"westwood\"/TCP_VERSION=\"cubic\"/g' scratch/emulated_nsc.cc
perl -pi -e 's/rate_slope = -0.1/rate_slope = 0.1/g' scratch/emulated_nsc.cc
perl -pi -e 's/init_radio_bandwidth = \"20Mb\/s\"/init_radio_bandwidth = \"1Mb\/s\"/g' scratch/emulated_nsc.cc
./rp-emulated-nsc.sh inc_cubic_20pkts


perl -pi -e 's/TCP_VERSION=\"cubic\"/TCP_VERSION=\"vegas\"/g' scratch/emulated_nsc.cc
./rp-emulated-nsc.sh inc_vegas_20pkts


perl -pi -e 's/TCP_VERSION=\"vegas\"/TCP_VERSION=\"westwood\"/g' scratch/emulated_nsc.cc
./rp-emulated-nsc.sh inc_westwood_20pkts

####==========20pkts buffer decreasing=======
perl -pi -e 's/TCP_VERSION=\"westwood\"/TCP_VERSION=\"cubic\"/g' scratch/emulated_nsc.cc
perl -pi -e 's/init_radio_bandwidth = \"1Mb\/s\"/init_radio_bandwidth = \"20Mb\/s\"/g' scratch/emulated_nsc.cc
perl -pi -e 's/rate_slope = 0.1/rate_slope = -0.1/g' scratch/emulated_nsc.cc
./rp-emulated-nsc.sh des_cubic_20pkts

perl -pi -e 's/TCP_VERSION=\"cubic\"/TCP_VERSION=\"vegas\"/g' scratch/emulated_nsc.cc
./rp-emulated-nsc.sh des_vegas_20pkts


perl -pi -e 's/TCP_VERSION=\"vegas\"/TCP_VERSION=\"westwood\"/g' scratch/emulated_nsc.cc
./rp-emulated-nsc.sh des_westwood_20pkts





