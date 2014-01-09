#!/bin/bash
radio="radio"
if [ $# -lt 1 ]; then
	echo "Usage: <this-script> <Number of radio RBs>"
	exit
fi
poll=1
last_poll=$poll
wsize=8
last_wsize=$wsize
##clean the log file
touch BASH_LOG
perl -pi -e "s/m_windowSize = 512;/m_windowSize = $wsize;/g" src/lte/model/lte-rlc-am.cc
while [ $wsize -le 1024 ]; do
				perl -pi -e "s/m_windowSize = $last_wsize;/m_windowSize = $wsize;/g" src/lte/model/lte-rlc-am.cc
				let poll=1
				let last_poll=$poll
				###poll has to be smaller than window size.
				while [ $poll -le $wsize ]; do
								perl -pi -e "s/m_pollPdu = $last_poll;/m_pollPdu = $poll;/g" src/lte/model/lte-rlc-am.cc
								./lte-nsc rlcam-windowsize-$wsize-poll-$poll-$1-RBs 15 400 -1 $radio
								echo "(poll, wsize) = $poll $wsize" >> BASH_LOG
								let last_poll=poll
								let poll=poll*2
				done
				let last_wsize=wsize
				let wsize=wsize*2
done



