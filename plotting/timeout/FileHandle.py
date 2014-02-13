#!/usr/bin/python

import os

###This class is used to handle (read/write) file/folder.
class FileHandle(object):
    '''
    classdocs
    '''
    input_dir = ""
    output_file = ""
    old_file_path_list = []

    #constructor
    def __init__(self,input_dir, output_file = ""):
        if input_dir is "":
            print("Warning: No input directory explicit! quit now.")
            exit()
        self.input_dir = input_dir
        self.output_file = output_file
        
    #read a file, return the Y-axis values (line 3) of the file
    def read_single_file(self, file_name = ""):
        print("***reading file ... " + file_name)
        return_value = []
        values_str = ""
        file = open(file_name)
        line = file.readline()
        i = 0
        while line:
            i += 1
            if i == 4:
                values_str += line
                break
        file.close()
        print("return = " + values_str)
        
        return return_value
    
    #write a string to OUTPUT_FILE
    def write_output(self, write_content):
        file = open(self.output_file,"a")
        file.write(write_content)
        file.close()
   
    #read all files of a specified directory, return a list of names of files in the directory.
    def read_dir(self, extension = ""):
        print ("input dir = " + self.input_dir)
        f_list = os.listdir(self.input_dir)
        if not f_list:
            return self.old_file_path_list
        for file in f_list:
            ###if not match the extension, skip the file
            if (os.path.splitext(file)[1] != extension):
                continue
            file = self.input_dir + "/"+file
            self.old_file_path_list.append(file)
        return self.old_file_path_list
            
    def clear_output_file(self):
        open(self.output_file,"w").close()
        
    ##################