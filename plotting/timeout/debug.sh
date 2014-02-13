#!/bin/bash

grep "SEQ" TCP_LOG | grep "RecvHandoverRequest" > 1
grep "PDCP_FORWARDING" TCP_LOG > 2


grep "UE DATA FORWARD" TCP_LOG | grep "Recv" > 3

grep "PDCPPDU SEQ" TCP_LOG | grep "4 Lte"  > 4

grep "rlcAmTxedSduBuffer SEQ" TCP_LOG > 5
