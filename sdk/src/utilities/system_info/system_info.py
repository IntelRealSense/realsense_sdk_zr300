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
parser.add_argument('--disable_file_output', action='store_false', help="disable output to file")
parser.add_argument('--file', default="system_info.txt", help="file for output. Default is system_info.txt")
parser.add_argument('--groups', default="spc", help="Enable output for some group. Default: spc")    


#Initialize commands for getting infrmation
commands = [

#System information
    {"group":"s", "command":"lscpu", "description":"CPU information"},
    {"group":"s", "command":"lsb_release -a", "description":"OS information"},
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
    {"group":"c", "command":"dmesg | tail -n 50", "description":"dmseg information"},
    {"group":"c", "command":"ldconfig -p | grep librealsense", "description":"Accesability of librealsense.so"},

]

#Class for writing into files and stdout
class Writer(object):
    def __init__(self, file_name):
        if file_name is not None:
            self.file = open(file_name, 'w')
        else:
            self.file = None
    
    def output (self, str):
        if self.file  is not None:
            self.file.write(str + "\n")
        print(str.rstrip('\n'))


#Class for run command with timeout
class Command(object):
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
                    self.out.output( spaces + t )

    def write(self):
        self.out.output( "\n" + self.desc + ":" )
        self.out.output( "\tCommand: " + str(self.cmd) )
        self.out.output( "\tReturn code: " + str(self.process.returncode) )
        self.out.output( "\tStandard output:"  )
        self.print_output(self.std_out, "\t\t")
        self.out.output( "\tStandard error:" )
        self.print_output(self.std_err, "\t\t")   


if __name__ == '__main__':
    args = parser.parse_args(sys.argv[1:])
    
    if(args.disable_file_output):
        out = Writer(args.file)
    else:
        out = Writer(None)
    for command in commands:
        if(args.groups.find(command["group"]) != -1):
            rez_command = Command(command["command"], command["description"], out)
            rez_command.run(timeout=5)
            rez_command.write()
    
    #command = Command("echo 'Process started'; sleep 2; echo 'Process finished'")
    #command.run(timeout=3)
    #command.run(timeout=1)
    sys.exit(0)

