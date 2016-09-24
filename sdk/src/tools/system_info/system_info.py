import re
import os
import argparse
import subprocess
import threading
import logging
import sys


#Setup arguments
parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, 
epilog='''Avaliable groups:\n s - System information \n p - Package information \n c - Camera information''')
parser.add_argument('--disable_file_output', action='store_true', help="disable output to file")
parser.add_argument('--file', default="system_info.txt", help="file for output. Default is system_info.txt")
parser.add_argument('--error_stop', action='store_true', help="stop output in case of error")
parser.add_argument('--groups', default="spc", help="Enable output for some group. Default: spc")    


#Initialize commands for getting infrmation
commands = [

#System information
    {"group":"s", "command":"lscpu", "description":"CPU information"},
    {"group":"s", "command":"lsb_release -a", "description":"OS information", "req":"Ubuntu 16", "error":"Ubuntu 16 is required"},
    {"group":"s", "command":"uname -a", "description":"Kernel information"},
    {"group":"s", "command":"lspci", "description":"PCI devices information"},
    {"group":"s", "command":"g++ --version", "description":"g++ version"},   
    {"group":"s", "command":"echo $PATH", "description":"PATH environment variable"},
    {"group":"s", "command":"echo $LD_LIBRARY_PATH", "description":"LD_LIBRARYPATH environment variable"},
    {"group":"s", "command":"echo $INCLUDE", "description":"INCLUDE environment variable"},
    
#Package information
    {"group":"p", "command":"dpkg-query -l | grep log4cxx", "description":"Version of log4cxx package"},
    {"group":"p", "command":"dpkg-query -l | grep libjpeg", "description":"Version of libjpeg package"},
    {"group":"p", "command":"dpkg-query -l | grep opencv-dev", "description":"Version of OpenCV package"},
    
#Camera information
    {"group":"c", "command":"lsusb", "description":"USB devises information"},
    {"group":"c", "command":"lsusb -v | grep -E '\<(Bus|iProduct|bDeviceClass|bDeviceProtocol)' 2>/dev/null", "description":"USB devices information"},
    {"group":"c", "command":"dmesg | tail -n 50", "description":"dmseg information", "req":"ZR300", "error":"ZR300 camera is absent"},
    {"group":"c", "command":"ldconfig -p | grep librealsense", "description":"Accesability of librealsense.so", "req":"librealsense.so", "error":"librealsense.so is absent"},

]

#Class for writing into files and stdout
class output_handler(object):
    def __init__(self, file_name):
        if file_name is not None:
            self.file = open(file_name, 'w')
        else:
            self.file = None
    
    def print_line (self, str):
        if self.file  is not None:
            self.file.write(str + "\n")
        print(str.rstrip('\n'))


#Class for run command with timeout
class run_command(object):
    def __init__(self, cmd, desc, out):
        self.cmd = cmd
        self.desc = desc
        self.out = out
        self.process = None
        self.std_out = None
        self.std_err = None

    def run(self, timeout):

        def target():
            #print 'Thread started. Command: ' + self.cmd
            self.process = subprocess.Popen(self.cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            self.std_out, self.std_err = self.process.communicate()
            #print 'Thread finished. Command: ' + self.cmd

        thread = threading.Thread(target=target)
        thread.start()

        thread.join(timeout)
        if thread.is_alive():
            print 'Terminating process. Command: ' + self.cmd
            self.process.terminate()
            thread.join()

    def print_output(self, output, spaces):
        if output is not None:
            # get rid of empty lines in output.
            #output = '\n'.join([s for s in output.split('\n') if s.strip(' \t\n\r') != ""])
            for t in output.split('\n'):
                if t.strip() is not "":
                    self.out.print_line( spaces + t )

    def write(self):
        self.out.print_line( "\n" + self.desc + ":" )
        self.out.print_line( "\tCommand: " + str(self.cmd) )
        self.out.print_line( "\tReturn code: " + str(self.process.returncode) )
        self.out.print_line( "\tStandard output:"  )
        self.print_output(self.std_out, "\t\t")
        self.out.print_line( "\tStandard error:" )
        self.print_output(self.std_err, "\t\t")   
        
    def filter(self, req, error):
        if(self.std_out.find(req) == -1):
            self.out.print_line("\tERROR: " + error)
            return 1
        return 0


if __name__ == '__main__':
    args = parser.parse_args(sys.argv[1:])
    
    if(args.disable_file_output):
        out = output_handler(None)
    else:
        out = output_handler(args.file)
        
    error_numbers = 0
    for command in commands:
        if(args.groups.find(command["group"]) != -1):
            res_command = run_command(command["command"], command["description"], out)
            res_command.run(timeout=5)
            res_command.write()
            if(command.has_key("req")):
                error_numbers += res_command.filter(command["req"], command["error"])
                if(args.error_stop and error_numbers > 0):
                    sys.exit(1)
                    
    if(error_numbers > 0):
        sys.exit(1)
    else:
        sys.exit(0)

