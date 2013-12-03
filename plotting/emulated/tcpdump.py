#!/usr/bin/python

import os
import sys
import FileHandle

if __name__ == "__main__":
	INPUT_FILE =  "sequence_send.raw" 
	OUTPUT_FILE = "sequence_send.dat"
	MOD = 1000
	TIME_DIFF = 0.5 ##diff between simulation time and tcpdump timestamp

	file = open (INPUT_FILE)
	if (os.path.isfile(OUTPUT_FILE)):      ##if output file not exist
	    open(OUTPUT_FILE,'w').close()
	outfile = open (OUTPUT_FILE,'w+')

	line = file.readline()
	tokens = {}
	last_timestamp = 0

	while (line):
		tokens = line.split()
		timestamp_bl = tokens[0].split(":")
		seq_bl = tokens[8]	#sending seq
		seq_from = seq_bl.split(":")[0]
		seq_to = seq_bl.split(":")[1].split(",")[0]
		hr = timestamp_bl[0]
		min = timestamp_bl[1]
		seconds = timestamp_bl[2]
		timestamp_sec = float(hr)*60*60 + float(min)*60 + float(seconds) + TIME_DIFF*2
		inter_ack = timestamp_sec - last_timestamp
		last_timestamp = timestamp_sec
		
		s_len = tokens[14]
		seq_from = long (seq_from)/ long (s_len)
		seq_to = long (seq_to)/ long (s_len)
		seq_from = seq_from % MOD
		seq_to = seq_to % MOD
		delta = seq_to - seq_from
		if delta < 0: 
			delta = 1
		outfile.write(str (timestamp_sec)+"\t"+ str (seq_from)+"\t"+str (delta)+ "\t" + str(inter_ack) + "\n")
		line = file.readline()

	INPUT_FILE =  "sequence_ack.raw" 
	OUTPUT_FILE = "sequence_ack.dat"

	file = open (INPUT_FILE)
	if (os.path.isfile(OUTPUT_FILE)):      ##if output file not exist
	    open(OUTPUT_FILE,'w').close()
	outfile = open (OUTPUT_FILE,'w+')


	line = file.readline()
	tokens = {}
	last_timestamp = 0
	while (line):
		tokens = line.split()
		len = tokens[12]
		#if len is not "0":
		#	print "Not a ack packet in tcpdump file %s\n" % len
		#	continue
		timestamp_bl = tokens[0].split(":")
		seq_bl = tokens[8]	#next seq
		seq = seq_bl.split(",")[0]
		hr = timestamp_bl[0]
		min = timestamp_bl[1]
		seconds = timestamp_bl[2]
		timestamp_sec = float (hr)*60*60 + float (min)*60 + float (seconds) + TIME_DIFF*2
		inter_ack = timestamp_sec - last_timestamp
		last_timestamp = timestamp_sec
		
		seq = long (seq)/ long (s_len)
		seq = seq % MOD
		outfile.write(str (timestamp_sec)+"\t"+ str (seq)+ "\t" + str (inter_ack) + "\n")
		line = file.readline()


