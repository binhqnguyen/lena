#!/usr/bin/python

import os
import sys
import FileHandle
import re

if __name__ == "__main__":
  if len(sys.argv) < 2:
    print "<Number of UE>"
    exit(1)
  UE = int (str(sys.argv[1]))
  for i in range(0,UE):
    INPUT_FILE =  "sequence_send-700"+str(i+2)+".all" 
    OUTPUT_FILE = "sequence_send-700"+str(i+2)+".dat"
    MOD = 1000

    file = open (INPUT_FILE)
    if (os.path.isfile(OUTPUT_FILE)):      ##if output file not exist
        open(OUTPUT_FILE,'w').close()
    outfile = open (OUTPUT_FILE,'w+')

    line = file.readline()
    tokens = {}

    if re.search('TSval',line) and flag == 0:
      SEG_SIZE = 1448
    else SEG_SIZE = 1460

    while (line):

      tokens = line.split()
      timestamp = tokens[1]
      
      seq_i = 0
      for j in range(0,len(tokens)):
        if re.match(r'Seq=',tokens[j]):
          seq_i = j
          break
      seq = int(tokens[seq_i].split("=")[1])/SEG_SIZE % MOD
      outfile.write(str (timestamp)+"\t"+ str (seq)+"\n")
      line = file.readline()

    INPUT_FILE =  "sequence_ack-700"+str(i+2)+".all" 
    OUTPUT_FILE_1 = "sequence_ack-700"+str(i+2)+".dat" 
    OUTPUT_FILE_2 = "sequence_dupack-700"+str(i+2)+".dat" 

    file = open (INPUT_FILE)
    if (os.path.isfile(OUTPUT_FILE_1)):      ##if output file not exist
        open(OUTPUT_FILE_1,'w').close()
    outfile_1 = open (OUTPUT_FILE_1,'w+')

    if (os.path.isfile(OUTPUT_FILE_2)):      ##if output file not exist
        open(OUTPUT_FILE_2,'w').close()
    outfile_2 = open (OUTPUT_FILE_2,'w+')

    line = file.readline()
    tokens = {}

    while (line):
      tokens = line.split()
      timestamp = tokens[1]
      #dupack and ack filtering
      if re.search('Dup ACK',line):
        outfile = outfile_2
      else: outfile = outfile_1
      
      ack_i = 0
      for j in range(0,len(tokens)):
        if re.match(r'Ack=',tokens[j]):
          ack_i = j
          break
      ack = int(tokens[ack_i].split("=")[1])/SEG_SIZE
      outfile.write(str (timestamp)+"\t"+ str (ack % MOD)+"\n")
      line = file.readline()


