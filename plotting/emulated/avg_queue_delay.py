#!/usr/bin/python

import os
import sys
import FileHandle

INPUT_FILE =  "/Users/binh/Documents/workspace/lena/results/tcp/data-scripts/emulated/tcp-queue.txt" 
OUTPUT_FILE = "/Users/binh/Documents/workspace/lena/results/tcp/data-scripts/emulated/tcp-queue-averaged.txt" 
INPUT_FILE_DL =  "/Users/binh/Documents/workspace/lena/results/tcp/data-scripts/emulated/tcp-queue-dl.txt" 
OUTPUT_FILE_DL = "/Users/binh/Documents/workspace/lena/results/tcp/data-scripts/emulated/tcp-queue-averaged-dl.txt" 



INTERVAL = 50

if __name__ == "__main__":
    file = open (INPUT_FILE)
    line = file.readline()
    tokens = {}
    ctr = 0
    i_sum = 0
    q_sum = 0
    
    if (os.path.isfile(OUTPUT_FILE)):      ##if output file not exist
        open(OUTPUT_FILE,'w').close()
    outfile = open (OUTPUT_FILE,'w+')
    
    while (line):
        tokens = line.split()
	#print ("delay = "+tokens[2])
	if len(tokens) != 6:	#skip invalid line
		line = file.readline()
		continue
	try:
		float (tokens[5])	#queuing delay
	except ValueError:
		line = file.readline()
		continue
	i_sum += float (tokens[5])	#queuing delay
 	q_sum += float (tokens[3])	#queue size	
	ctr += 1
	if (ctr == INTERVAL):
 		outfile.write(tokens[0]+"\t"+str (i_sum/ctr)+ "\t"+ str (q_sum/ctr)+"\n") #writing: time_stamp  averaged_queuing_delay 	queue_size
		ctr = 0
		i_sum = 0
		q_sum = 0
	line = file.readline()



#downlink 
    file = open (INPUT_FILE_DL)
    line = file.readline()
    tokens = {}
    ctr = 0
    i_sum = 0
    q_sum = 0
    
    if (os.path.isfile(OUTPUT_FILE_DL)):      ##if output file not exist
        open(OUTPUT_FILE_DL,'w').close()
    outfile = open (OUTPUT_FILE_DL,'w+')
    
    while (line):
        tokens = line.split()
	#print ("delay = "+tokens[2])
	if len(tokens) != 6:	#skip invalid line
		line = file.readline()
		continue
		
	try:
		float (tokens[5])	#queuing delay
	except ValueError:
		line = file.readline()
		continue
	i_sum += float (tokens[5])
	q_sum += float (tokens[3])
	ctr += 1
	if (ctr == INTERVAL):
 		outfile.write(tokens[0]+"\t"+str (i_sum/ctr)+"\t"+ str (q_sum/ctr) + "\n") #writing: time_stamp  averaged_queuing_delay
		ctr = 0
		i_sum = 0
		q_sum = 0
	line = file.readline()

    file.close()
    outfile.close()


        
			

