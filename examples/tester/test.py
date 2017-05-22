#!/usr/bin/python
#-------------------------------------------------------------------------------
#todo:
# - make update fully automated
# - report if unexpected input in example files
# - annotate more files and add them here
# - add concurrent launches
# - merge suppress and verbose options
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

litmus_files=[
# cpp litmus
              '../c/iriw-ra.cpp',
              '../c/sbmp-ra.cpp',
              '../c/mp-ra.cpp',
              '../c/sb-ra.cpp',
              '../c/mp-rlx.cpp',
              '../c/double-mp-rlx.cpp',
              '../c/screads.cpp',
              '../c/screads-mini.cpp',
              '../c/lb.cpp',
              '../c/lb-ra.cpp',
              '../c/lb-rlx.cpp',
              '../c/lb-dep.cpp',
              '../c/lb-cond.cpp',
              '../c/lb-cond-3.cpp',
              '../c/lb-combo.cpp',
# original tara
              '../locks.ctrc',
# wmm test examples
              '../wmm-simple.ctrc',
              '../wmm-simple-fence.ctrc',
              '../wmm-litmus/mp.ctrc',
              '../wmm-litmus/mp-s.ctrc',
              '../wmm-litmus/mp-10-var.ctrc',
              '../wmm-litmus/double-mp.ctrc',
              '../wmm-litmus/wrc.ctrc',
              '../wmm-litmus/wwc.ctrc',
              '../wmm-litmus/rwc.ctrc',
              '../wmm-litmus/isa2.ctrc',
              '../wmm-litmus/sb.ctrc',
              '../wmm-litmus/sb-2+2w.ctrc',
              '../wmm-litmus/sb-3-thread.ctrc',
              '../wmm-litmus/sb-r.ctrc',
              '../wmm-litmus/iriw.ctrc',
              '../wmm-litmus/sb-redundant.ctrc',
              '../wmm-litmus/sb-3-thread.ctrc',
              '../wmm-litmus/corr0.ctrc',
              '../wmm-litmus/corr2.ctrc',
              '../wmm-litmus/corw1.ctrc',
              '../wmm-litmus/coww.ctrc',
              '../wmm-litmus/corr1.ctrc',
              '../wmm-litmus/corw.ctrc',
              '../wmm-litmus/cowr.ctrc',
              '../wmm-litmus/cow3r.ctrc',
              '../wmm-litmus/lb-ctrl.ctrc',
              '../wmm-litmus/lb-data.ctrc',
              '../wmm-litmus/lb.ctrc',
              '../wmm-litmus/irrwiw.ctrc',
	      '../wmm-litmus/wrw+wr.ctrc',
	      '../wmm-litmus/wrw+2w.ctrc',
	      '../wmm-litmus/wrr+2w.ctrc',
	      '../wmm-litmus/irwiw.ctrc',
	      '../wmm-litmus/rfi-pso.ctrc',
	      '../wmm-litmus/thin.ctrc',
              '../wmm-rmo-reduction.ctrc',
    ]

hard_files=[
# pyn examples
              '../c/syn/abp.cc',
              '../c/syn/d-prcu-v1.cc',
              '../c/syn/d-prcu-v2.cc',
              '../c/syn/bakery_s0.cc',
              '../c/syn/dekker.cc',
              '../c/syn/kessel.cc',
              '../c/syn/peterson.cc',
              '../c/syn/ticket.cc',
              # '../c/syn/treiber-stack_s0.cc', # pipe dream
# combi hard instances
              '../wmm-fib-1.ctrc',
              '../wmm-fib-2.ctrc',
              '../wmm-fib-3.ctrc',
              '../wmm-fib-4.ctrc',
              '../wmm-fib-5.ctrc'
#Benchmark examples
            ]

known_files = litmus_files + hard_files

#wmm-litmus-sb-self-read.ctrc

#------------------------------------------------------------------
# parsing cmd options

parser = argparse.ArgumentParser(description='Auto testing for TARA')
parser.add_argument("-v","--verbose", action='store_true', help = "verbose")
parser.add_argument("-u","--update", action='store_true', help = "update")
parser.add_argument("-s","--suppress", action='store_true', help = "only report errors")
parser.add_argument("-c","--compare", nargs=2, help = "compare the memory models", type = str)
parser.add_argument("-f","--suite", help = "choose test suite", type = str)
parser.add_argument('files', nargs=argparse.REMAINDER, help='files')
args = parser.parse_args()

if( args.update and len(args.files) == 0 ):
    raise ValueError('update needs list of files!!')

#------------------------------------------------------------------
# choosing list of files

if len(args.files) > 0:
    files = args.files
elif args.suite != None:
    sname = args.suite
    if sname == "all":
        files = known_files
    elif sname == "litmus":
        files = litmus_files
    elif sname == "hard":
        files = hard_files
    else:
        raise ValueError('wrong suite name!!')        
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

comment_string = "#"
clen = len( comment_string ) + 1

def set_comment_string( fname ):
    global comment_string
    global clen
    if fname[-5:] == ".ctrc": 
        comment_string = "#"
    elif fname[-2:] == ".c" or fname[-4:] == ".cpp" or fname[-3:] == ".cc":
        comment_string = "//"
    else:
        raise ValueError('unrecognized file type: ' + fname)
    clen = len( comment_string ) + 1

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
            if( not args.suppress ):
                printf( "[%d..", self.count )
            sys.stdout.flush()
            result = execute_example(self.filename, o)
            if result == output:
                if( not args.suppress ):
                    printf( "pass] " )
            else:
                if( not args.suppress ):
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
                    if line[:clen] == comment_string +'~' :
                        self.populate_runs( options, output)
                        options = []
                        output = ''
                        started = False
                    elif line[:clen] == comment_string + '#' :
                        output += line[clen:]
                else:
                    if line[:clen] == comment_string + '!':
                        options.append( line[clen:-1] )
                    if line[:clen] == comment_string + '~' :
                        started = True
            if( not args.suppress ):
                printf( "\n" )

    def __init__( self, fname ):
        set_comment_string( fname )
        self.filename = fname
        self.count = 0
        self.load_running_cases()


#------------------------------------------------------------------

if args.update:
    for filename in files:
        set_comment_string( filename )
        options = []
        with open(filename, 'r') as f:
            for line in f:
                if line[:clen] == comment_string + '!':
                    options.append( line[clen:-1] )
    
        with open( filename, "a") as myfile:
            myfile.write( "\n" + comment_string + "####### ERASE TESTS ABOVE #######\n" )
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
        if( not args.suppress ):
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
