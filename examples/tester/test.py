#!/usr/bin/python
#-------------------------------------------------------------------------------
#todo:
# - add option parsing (-v option)
# - add more example
# - report if unexpected input in example files
# - annotate more files and add them here
# - add concurrent launches
#-------------------------------------------------------------------------------

from __future__ import print_function
import xml.etree.ElementTree as ET
from datetime import datetime
from subprocess import call,check_output
import os.path
import sys
import io
import difflib


known_files=['../wmm-simple.ctrc',
             '../wmm-simple-fence.ctrc',
             '../wmm-litmus-tso.ctrc',
             '../wmm-litmus-tso-redundant.ctrc',
             '../wmm-litmus-tso-3-thread.ctrc',
             '../locks.ctrc',
             '../wmm-rmo-reduction.ctrc'
             ]

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
        printf( "%d,%d\t: %s\n", l, i, s )

def process_diff( expected, observed ):
    printf( "Expected - and Observed + \n" )    
    ex_list = expected.split("\n");
    ob_list = observed.split("\n");
    minlen = min( len(ex_list),len(ob_list) )
    for l in range(0,minlen-1):
        process_line_diff( ex_list[l], ob_list[l], l+1 )
    if minlen < len(ex_list):
        for l in range( minlen-1, len( ex_list )-1 ):
            printf( "%d,0 \t: - %s\n", l+1, ex_list[l])
    if minlen < len( ob_list ):
        for l in range( minlen-1, len( ob_list )-1 ):
            printf( "%d,0 \t: + %s\n", l+1, ob_list[l])

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
            self.count = self.count + 1
            printf( "[test %d]...", self.count )
            result = self.execute_example(o)
            if result == output:
                printf( "[passed]\n" )
            else:
                printf( "[failed]\n" )
                printf( "options were: %s \n", o)
                process_diff( output, result )
                
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
        self.count = 0
        self.load_running_cases()


if len(sys.argv) > 1:
    files=sys.argv[1:]
else:
    files=known_files

for f in files:
        printf( "%s\n", f )
        benchmakr = example(f)
