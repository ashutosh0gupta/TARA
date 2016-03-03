from __future__ import print_function
import xml.etree.ElementTree as ET
from datetime import datetime
from subprocess import call,check_output
import os.path
import sys
import io
import difflib

#------------------------------------------------------------------
# General utilities

def get_time_stamp( date, time ):
    dt = datetime.strptime( date+"T"+ time, "%Y-%m-%dT%H" )
    return dt

def printf(str, *args):
    print(str % args, end='')

def process_line_diff(a,b,l):
    for i,s in enumerate(difflib.ndiff(a, b)):
        if s[0]==' ': continue
        printf( "(%d,%d) : %s\n", l, i, s )
        # elif s[0]=='-':
        #     print(u'Delete "{}" from position ({},{})'.format(s[-1],l,i))
        # elif s[0]=='+':
        #     print(u'Add "{}" to position ({},{})'.format(s[-1],l,i))

def process_diff( a, b):
    alist = a.split("\n");
    blist = b.split("\n");
    l = 0
    for aline in alist:
        bline = blist[l]
        l = l + 1
        process_line_diff( aline, bline, l )

#------------------------------------------------------------------

bin_name = "../../tara"

class example:
    def execute_example(self,o):
        ops =o.split(' ')
        ops = [o for o in ops if o != '']
        ops.append(self.filename)
        return check_output([bin_name]+ops)

    def populate_runs( self, options, output ):
        for o in options:
            printf( "testing: %s ...", self.filename )
            result = self.execute_example(o)
            if result == output:
                printf( "[passed]\n" )
            else:
                printf( "[failed]\n" )
                printf( "options were: %s \n", o)
                process_diff( output, result )
                # printf( "Expected output: \n %s \n", output)
                # printf( "Observed output: \n %s \n", result)
                


    def load_running_cases(self):
        with open(self.filename, 'r') as f:
            options = []
            output = ''
            started = False
            for line in f:
                if started:
                    if line[:2] == '#~' :
                        self.populate_runs( options, output)
                        options = []
                        output = ''
                        started = False
                    elif line[:2] == '##' :
                        output += line[2:]
                else:
                    if line[:2] == '#!':
                        options.append( line[2:-1] )
                    if line[:2] == '#~' :
                        started = True

    def __init__( self, fname ):
        self.filename = fname
        self.load_running_cases()
    

benchmakr = example('../wmm-simple.ctrc')
benchmakr = example('../wmm-simple-fence.ctrc')
