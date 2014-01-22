#!/bin/bash

pollPdu=4
last_poll=$pollPdu
while [ $pollPdu -le 8 ]; do
				perl -pi -e "s/m_pollPdu = $last_poll;/m_pollPdu = $pollPdu;/g" src/lte/model/lte-rlc-am.cc
				echo "running lte rlcam poll = $pollPdu..."
				./lte-nsc rlcam_poll_$pollPdu 25 300 -1 radio_dev
				let last_poll=pollPdu
				let pollPdu=pollPdu*2
				echo "$last_poll"
done


