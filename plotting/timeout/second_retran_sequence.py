#!/usr/bin/python

import os
import sys
import FileHandle
import re



if __name__ == "__main__":
	if (len (sys.argv) < 2):
	 	print ("Usage retrans_count.py <number of Ues>")
	 	exit(1)
	UE = int (str(sys.argv[1]))
	for i in range(0,UE):
		##filter retrans from send sequence.
		grep_cmd = 'grep "TCP Retransmission" sequence_send-700'+ str(i+2)+'.all > sequence_retrans-700'+ str(i+2)+'.dat'
		os.system(grep_cmd)

		INPUT_FILE =  'sequence_retrans-700'+ str(i+2)+'.dat' 
		OUTPUT_FILE_1 = "sequence_second_retrans-700"+str(i+2)+".dat"
		OUTPUT_FILE_2 = "sequence_first_retrans-700"+str(i+2)+".dat"
		
		if (not os.path.exists(INPUT_FILE)):
			continue
		file = open (INPUT_FILE)
		line = file.readline()
		tokens = {}
		MOD = 1000

		if re.search('TSval',line) and flag == 0:
			SEG_SIZE = 1448
    		else:
    			SEG_SIZE = 1460

		if (os.path.isfile(OUTPUT_FILE_1)):      ##if output file not exist
			open(OUTPUT_FILE_1,'w').close()
		outfile_1 = open (OUTPUT_FILE_1,'w+')

		if (os.path.isfile(OUTPUT_FILE_2)):      ##if output file not exist
			open(OUTPUT_FILE_2,'w').close()
		outfile_2 = open (OUTPUT_FILE_2,'w+')

		retran_seq = {}
		second_retran_seq = {}
		while (line):
			tokens = line.split()
			time_stamp = tokens[1]
			seq_i = 0
			for j in range(0,len(tokens)):
				if re.match(r'Seq=',tokens[j]):
					seq_i = j
					break
			retran_seq[time_stamp] = int(tokens[seq_i].split("=")[1]) #assuming time_stamp is unique for each retran.
			line = file.readline()

		print len(retran_seq)
		dup_retran = {}
		for key in retran_seq:
			for key_1 in retran_seq:
				if retran_seq[key] == retran_seq[key_1] and key < key_1:  
					dup_retran[key_1] = retran_seq[key_1]
		print len(dup_retran)
		for key in sorted(retran_seq):
			outfile_2.write(str (key)+"\t"+ str (retran_seq[key]/SEG_SIZE % MOD)+"\n")
		for key in sorted(dup_retran):
			outfile_1.write(str (key)+"\t"+ str (retran_seq[key]/SEG_SIZE % MOD)+"\n")







