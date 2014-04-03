#!/bin/bash

poll=1
last_poll=$poll
wsize=1
last_wsize=$wsize
#perl -pi -e "s/m_pollByte= 50;/m_pollByte = 150000;/g" src/lte/model/lte-rlc-am.cc
#while [ $poll -le 512 ]; do
				#perl -pi -e "s/m_pollPdu = $last_poll;/m_pollPdu = $poll;/g" src/lte/model/lte-rlc-am.cc
				while [ $wsize -le 1024 ]; do
								perl -pi -e "s/m_windowSize = $last_wsize;/m_windowSize = $wsize;/g" src/lte/model/lte-rlc-am.cc
								echo "running lte rlcam poll = , window-size= $wsize ..."
								./lte-nsc rlcam-windowsize-$wsize-poll-$poll-largePollByte 30 300 -1 radio_dev
								let last_wsize=wsize
								let wsize=wsize*2
								echo "(poll, wsize) = $last_poll $last_wsize"
				done
#				let last_poll=poll
#				let poll=poll*2
#				echo "(poll, wsize) = $last_poll $last_wsize"
#done


