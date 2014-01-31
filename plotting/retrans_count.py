#!/usr/bin/python

import os
import sys
import FileHandle

if __name__ == "__main__":
	if (len (sys.argv) < 2):
	 	print ("Usage retrans_count.py <number of Ues>")
	 	exit(1)
	UE = int (str(sys.argv[1]))
	for i in range(0,UE):
		INPUT_FILE =  "cubic-"+str(i+7002)+".dat" 
		OUTPUT_FILE = "retrans-"+str(i+7002)+".dat"
		
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
				#print (tokens[-1]+"\n")
				try:
					float (tokens[0])
				except ValueError:
					line = file.readline()
					continue

				if (float (tokens[0]) < last_rt_time + CUMMULATIVE_INTERVAL): ##cummulatively count retrans
					cum_retrans += 1
				else:
				#print (tokens[-1]+"\n")
					outfile.write(str (last_rt_time)+"\t"+ str (cum_retrans+1)+"\n")
					cum_retrans = 0  #reset cum_retrans		
					last_rt_time = float (tokens[0])
			line = file.readline()


