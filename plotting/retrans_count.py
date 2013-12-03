#!/usr/bin/python

import os
import sys
import FileHandle

if __name__ == "__main__":
	# if (len (sys.argv) == 0):
	# 	print ("Usage retrans_count.py <dir of cubic.dat file>")
	# 	exit(1)
	# DIR = str (sys.argv[0])
	INPUT_FILE =  "cubic.dat" 
	OUTPUT_FILE = "retrans.dat"

	file = open (INPUT_FILE)
	line = file.readline()
	tokens = {}

	CUMMULATIVE_INTERVAL = 0.01 #within 10ms, retrans are cummulatively counted.

	if (os.path.isfile(OUTPUT_FILE)):      ##if output file not exist
	    open(OUTPUT_FILE,'w').close()
	outfile = open (OUTPUT_FILE,'w+')
	last_rt_cnt = 0
	last_rt_time = 0.0
	cum_retrans = 0
	while (line):
		tokens = line.split()
		#print (tokens[-1]+"\n")
		if (int (tokens[-1]) > last_rt_cnt):	#retrans detected
			last_rt_cnt = int (tokens[-1])
			try:
				float (tokens[0])
			except ValueError:
				line = file.readline()
				continue
			if (float (tokens[0]) < last_rt_time + CUMMULATIVE_INTERVAL): ##cummulatively count retrans
				cum_retrans += 1
			else:
				outfile.write(str (last_rt_time)+"\t"+ str (cum_retrans+1)+"\n")
				cum_retrans = 0  #reset cum_retrans		
				last_rt_time = float (tokens[0])
		line = file.readline()


