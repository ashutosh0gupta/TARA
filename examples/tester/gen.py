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

l = len(sys.argv)

fname = sys.argv[l-1]

call_options =sys.argv[1:]

bin_name = "../../tara"
output = check_output([bin_name]+call_options)
outs = output.split('\n')

opt_string = ' '.join(call_options[:-1])
fname = call_options[-1]
with open( fname, "a") as myfile:
    myfile.write( "\n################################################\n" )
    myfile.write( "#!" )
    myfile.write( opt_string )
    myfile.write( "\n" )
    myfile.write("################################################\n")
    myfile.write("#~\n")
    for s in outs[:-1]:
        myfile.write( "##" )
        myfile.write( s )
        myfile.write( "\n" )
    myfile.write( "#~\n\n" )
