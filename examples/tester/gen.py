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

if fname[-5:] == ".ctrc": 
    comment_string = "#"
elif fname[-4:] == ".cpp" or fname[-3:] == ".cc" or fname[-2:] == ".C":
    comment_string = "//"
else:
    raise ValueError('unrecognized file extension!!')

clen = len( comment_string ) + 1


call_options =sys.argv[1:]

bin_name = "../../tara"
output = check_output([bin_name]+call_options)
outs = output.split('\n')

line = "###############################################\n"

opt_string = ' '.join(call_options[:-1])
fname = call_options[-1]
with open( fname, "a") as myfile:
    myfile.write( "\n" + comment_string + line )
    myfile.write( comment_string + "!" + opt_string + "\n" )
    myfile.write( comment_string + line )
    myfile.write(comment_string + "~\n")
    for s in outs[:-1]:
        myfile.write( comment_string + "#" +  s + "\n" )
    myfile.write( comment_string + "~\n\n" )
