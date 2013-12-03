#!/usr/bin/python

import os
import sys
import FileHandle

if __name__ == "__main__":

	##########RLC UM Queue drop event#############
	INPUT_FILE =  "enb_rlcum_txqueue_drop_tmp.tmp"
	OUTPUT_FILE = "enb_rlcum_txqueue_drop.dat"

	file = open (INPUT_FILE)
	line = file.readline()
	tokens = {}

	if (os.path.isfile(OUTPUT_FILE)):      ##if output file not exist
	    open(OUTPUT_FILE,'w').close()
	outfile = open (OUTPUT_FILE,'w+')


	while (line):
		tokens = line.split()
		outfile.write(str ( tokens[0][:-1]+"\n"))
		line = file.readline()


	##########RLC UM Queue size#############
	INPUT_FILE =  "enb_rlcum_txqueue_size_tmp.tmp"
	OUTPUT_FILE = "enb_rlcum_txqueue_size.dat"

	file = open (INPUT_FILE)
	line = file.readline()
	tokens = {}

	if (os.path.isfile(OUTPUT_FILE)):      ##if output file not exist
	    open(OUTPUT_FILE,'w').close()
	outfile = open (OUTPUT_FILE,'w+')


	while (line):
		tokens = line.split()
		outfile.write(str ( tokens[0][:-1]+"\t"+tokens[5]+"\n"))
		line = file.readline()
