#!/usr/bin/python
#-------------------------------------------------------------------------------
#todo:
# - make update fully automated
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
import argparse


known_files=[ '../locks.ctrc',
              '../wmm-simple.ctrc',
              '../wmm-simple-fence.ctrc',
              '../wmm-litmus-mp.ctrc',
              '../wmm-litmus-mp-s.ctrc',
              '../wmm-litmus-mp-10-var.ctrc',
              '../wmm-litmus-wrc.ctrc',
              '../wmm-litmus-wwc.ctrc',
              '../wmm-litmus-rwc.ctrc',
              '../wmm-litmus-isa2.ctrc',
              '../wmm-litmus-sb.ctrc',
              '../wmm-litmus-sb-2+2w.ctrc',
              '../wmm-litmus-sb-3-thread.ctrc',
              '../wmm-litmus-sb-r.ctrc',
              '../wmm-litmus-iriw.ctrc',
              '../wmm-litmus-sb-redundant.ctrc',
              '../wmm-litmus-sb-3-thread.ctrc',
              '../wmm-litmus-corr0.ctrc',
              '../wmm-litmus-corr2.ctrc',
              '../wmm-litmus-corw1.ctrc',
              '../wmm-litmus-coww.ctrc',
              '../wmm-litmus-corr1.ctrc',
              '../wmm-litmus-corw.ctrc',
              '../wmm-litmus-cowr.ctrc',
              '../wmm-litmus-cow3r.ctrc',
              '../wmm-litmus-lb-ctrl.ctrc',
              '../wmm-litmus-lb-data.ctrc',
              '../wmm-litmus-lb.ctrc',
              '../wmm-rmo-reduction.ctrc',
              '../wmm-fib-1.ctrc',
              '../wmm-fib-2.ctrc',
              '../wmm-fib-3.ctrc',
              '../wmm-fib-4.ctrc',
              '../wmm-fib-5.ctrc',
              '../wmm-litmus-irrwiw.ctrc',
	      '../wmm-litmus-wrw+wr.ctrc',
	      '../wmm-litmus-wrw+2w.ctrc',
	      '../wmm-litmus-wrr+2w.ctrc',
	      '../wmm-litmus-irwiw.ctrc',
	      '../wmm-litmus-rfi-pso.ctrc',
	      '../wmm-litmus-thin.ctrc'
            ]

#wmm-litmus-sb-self-read.ctrc

#------------------------------------------------------------------
# parsing cmd options

parser = argparse.ArgumentParser(description='Auto testing for TARA')
parser.add_argument("-v","--verbose", action='store_true', help = "verbose")
parser.add_argument("-u","--update", action='store_true', help = "update")
parser.add_argument("-c","--compare", nargs=2, help = "compare the memory models", type = str)
parser.add_argument('files', nargs=argparse.REMAINDER, help='files')
args = parser.parse_args()

if( args.update and len(args.files) == 0 ):
    raise ValueError('update needs list of files!!')

#------------------------------------------------------------------
# choosing list of files

if len(args.files) > 0:
    files = args.files
else:
    files=known_files
        
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

def execute_example(fname,o):
    ops =o.split(' ')
    ops = [o for o in ops if o != '']
    ops.append(fname)
    return check_output([bin_name]+ops)


class example:
    def populate_runs( self, options, output ):
        for o in options:
            self.count = self.count + 1
            printf( "[%d..", self.count )
            sys.stdout.flush()
            result = execute_example(self.filename, o)
            if result == output:
                printf( "pass] " )
            else:
                printf( "fail]\n" )
                printf( "call: %s %s %s\n", bin_name, o, self.filename)
                if( args.verbose ):
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
            printf( "\n" )

    def __init__( self, fname ):
        self.filename = fname
        self.count = 0
        self.load_running_cases()


#------------------------------------------------------------------

if args.update:
    for filename in files:
        options = []
        with open(filename, 'r') as f:
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
elif args.compare != None:
    m1 = args.compare[0]
    m2 = args.compare[1]
    printf( "Comparing %s %s:\n", m1, m2)
    for filename in files:
	out1 = execute_example(filename, '-r diffvar,unsat_core,remove_implied -M ' + m1)
	out2 = execute_example(filename, '-r diffvar,unsat_core,remove_implied -M ' + m2)
	if out1 == out2:
            pass
	else:
	    printf( "Different output %s", filename)
	    printf( "%s",out1)
	    printf( "%s",out2)
else:
    for f in files:
        printf( "%s\n", f )
        benchmakr = example(f)

#----------------------------------------------------------------
# code added by Barke

# all.py

# def memory_model(filename,model):
#     n = 0
#     list = []
#     with open(filename, 'r') as f:
# 	lines = f.readlines()
# 	for line in lines:
# 	    n = n + 1
# 	    if "#!-r diffvar,unsat_core,remove_implied -M " + model + "\n" in line:
# 	        'print (line,n)'
# 	    	num = n
# 		'print num'
# 		break
# 	for line in islice(lines,num,num+100):
# 	    'print (line)'
# 	    if "#!" not in line:
# 		list.append(line.strip())
# 	    else:
# 		break
#     return list

# def compare(filename,model1,model2):
#     list1 = memory_model(filename,model1)
#     list2 = memory_model(filename,model2)
#     'print list1'
#     'print list2'
#     'return set(list1) == set(list2) and len(list1) == len(list2)'
#     if list1 == list2:
# 	print("Same Output\n")
#     else:
# 	print("Different output\n")

# models = ["sc","tso","pso","rmo"]
# lists=[]
# for files in known_files:
#     for model1 in models:
# 	for model2 in models:
# 	    print (files,model1,model2)
# 	    if model1 != model2:
# 	    	compare(files,model1,model2)
# 		'lists.append(compare(files,model1,model2))'


#compare.py

# parser = argparse.ArgumentParser(description='Compare 2 memory models')
# parser.add_argument("-c1","--compare1", choices=["sc","tso","pso","alpha","rmo"],help = "compare two memory models",type = str)
# parser.add_argument("-c2","--compare2", choices=["sc","tso","pso","alpha","rmo"],help = "compare two memory models",type = str)
# parser.add_argument("-file","--file", help = "filename",type = str)
# args = parser.parse_args()
# first_model = args.compare1
# second_model = args.compare2
# filename = args.file
# print (first_model)
# print (second_model)
# print (filename)

# def memory_model(filename,model):
#     n = 0
#     list = []
#     with open(filename, 'r') as f:
# 	lines = f.readlines()
# 	for line in lines:
# 	    n = n + 1
# 	    if "#!-r diffvar,unsat_core,remove_implied -M " + model + "\n" in line:
# 	        'print (line,n)'
# 	    	num = n
# 		'print num'
# 		break
# 	for line in islice(lines,num,num+100):
# 	    'print (line)'
# 	    if "#!" not in line:
# 		list.append(line.strip())
# 	    else:
# 		break
#     return list
	    
# def compare(filename,model1,model2):
#     list1 = memory_model(filename,model1)
#     list2 = memory_model(filename,model2)
#     'print list1'
#     'print list2'
#     if list1 == list2:
# 	print("Same Output")
#     else:
# 	print("Different output")

# compare(filename,first_model,second_model)



# new.py

# def execute_example(o,filename):
#     ops =o.split(' ')
#     ops = [o for o in ops if o != '']
#     ops.append(filename)
#     return check_output([bin_name]+ops)

# def compare(o1,o2,filename):
#     rmo = execute_example(o1,filename)
#     'print rmo'
#     alpha = execute_example(o2,filename)
#     'print alpha'
#     print filename
#     if rmo == alpha:
# 	print "Same"
#     else:
# 	print "Different"

# for files in known_files:
#     compare('-r diffvar,unsat_core,remove_implied -M rmo','-r diffvar,unsat_core,remove_implied -M alpha',files)
