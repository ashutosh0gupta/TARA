#!/usr/bin/python

from __future__ import print_function
import xml.etree.ElementTree as ET
from datetime import datetime
from subprocess import call,check_output
import os.path
import sys
import io
import difflib

def printf(str, *args):
    print(str % args, end='')

if len(sys.argv) > 1:
    files=sys.argv[1:]
else:
    exit

for filename in files:
    options = []
    with open(filename, 'r') as f:
        output = ''
        started = False
        for line in f:
            if line[:2] == '#!':
                options.append( line[2:-1] )
    
    with open( filename, "a") as myfile:
        myfile.write( "\n######## ERASE TESTS ABOVE #######\n" )

    for o in options:
        printf( "%s\n", o )
        ops =o.split(' ')
        ops = [o for o in ops if o != '']
        ops.append(filename)
        check_output(['./gen.py']+ops)
