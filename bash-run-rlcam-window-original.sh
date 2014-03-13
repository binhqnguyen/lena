#!/bin/bash
radio="radio_dev"


cp src/lte/model/lte-rlc-am-original.cc src/lte/model/lte-rlc-am.cc

wsize=10
last_wsize=$wsize
##clean the log file
touch BASH_LOG
for ((wsize=10;wsize <= 512; wsize=wsize+20))
do
				perl -pi -e "s/m_windowSize = $last_wsize;/m_windowSize = $wsize;/g" src/lte/model/lte-rlc-am.cc
				echo "$last_wsize, $wsize"
				#./rlcam emptybuffer-window-put 40 1 $radio 1
				let last_wsize=$wsize
done



