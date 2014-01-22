#!/usr/bin/python

import os
import sys
import FileHandle

if __name__ == "__main__":

	# DIR = str (sys.argv[0])
	INPUT_FILE =  "endhost_dev_dequeue_tmp.txt" 
	OUTPUT_FILE = "endhost_dev_dequeue.txt"

	file = open (INPUT_FILE)
	line = file.readline()
	tokens = {}

	if (os.path.isfile(OUTPUT_FILE)):      ##if output file not exist
	    open(OUTPUT_FILE,'w').close()
	outfile = open (OUTPUT_FILE,'w+')

	while (line):
		tokens = line.split()
		outfile.write(tokens[0][0:-1]+"\n")
		line = file.readline()

	INPUT_FILE =  "enb_dev_enqueue_tmp.txt" 
	OUTPUT_FILE = "enb_dev_enqueue.txt"


	file = open (INPUT_FILE)
	line = file.readline()

	if (os.path.isfile(OUTPUT_FILE)):      ##if output file not exist
	    open(OUTPUT_FILE,'w').close()
	outfile = open (OUTPUT_FILE,'w+')

	while (line):
	    tokens = line.split()
	    outfile.write(tokens[0][:-1]+"\n")
	    line = file.readline()


	INPUT_FILE =  "enb_dev_dequeue_tmp.txt" 
	OUTPUT_FILE = "enb_dev_dequeue.txt"


	file = open (INPUT_FILE)
	line = file.readline()
	tokens = {}

	if (os.path.isfile(OUTPUT_FILE)):      ##if output file not exist
	    open(OUTPUT_FILE,'w').close()
	outfile = open (OUTPUT_FILE,'w+')

	while (line):
	    tokens = line.split()
	    outfile.write(tokens[0][:-1]+"\n")
	    line = file.readline()


	
	INPUT_FILE =  "enb_dev_queue_drop_tmp.txt" 
	OUTPUT_FILE = "enb_dev_queue_drop.txt"


	file = open (INPUT_FILE)
	line = file.readline()

	if (os.path.isfile(OUTPUT_FILE)):      ##if output file not exist
	    open(OUTPUT_FILE,'w').close()
	outfile = open (OUTPUT_FILE,'w+')

	while (line):
	    tokens = line.split()
	    outfile.write(tokens[0][:-1]+"\n")
	    line = file.readline()


