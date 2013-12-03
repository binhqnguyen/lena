#!/usr/bin/python

import os
import sys
import FileHandle

INPUT_FILE =  "/Users/binh/Documents/workspace/lena/results/tcp/data-scripts/emulated/tcp-700m-put.txt" 
OUTPUT_FILE = "/Users/binh/Documents/workspace/lena/results/tcp/emulated/tcp-700m-put-averaged.txt" 
INTERVAL = 1

if __name__ == "__main__":
    file = open (INPUT_FILE)
    line = file.readline()
    tokens = {}
    ctr = 0
    i_sum = 0
    
    if (os.path.isfile(OUTPUT_FILE)):      ##if output file not exist
        open(OUTPUT_FILE,'w').close()
    outfile = open (OUTPUT_FILE,'w+')
    
    while (line):
        tokens = line.split()
	#print ("delay = "+tokens[2])
	if len(tokens) != 10 or float (tokens[10]) > 4000 or float(tokens[0]) < 500: #skip invalid data 
		line = file.readline()
		continue
	try:
		float (tokens[10])
	except ValueError:
		line = file.readline()
		continue
	i_sum += float (tokens[10])
	ctr += 1
	if (ctr == INTERVAL):
 		outfile.write(tokens[0]+"\t"+str (i_sum/ctr)+"\n")
		ctr = 0
		i_sum = 0
	line = file.readline()
    file.close()
    outfile.close()


        

