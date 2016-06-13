#!/usr/bin/python
from itertools import islice
import argparse


parser = argparse.ArgumentParser(description='Compare 2 memory models')
parser.add_argument("-c1","--compare1", choices=["sc","tso","pso","alpha","rmo"],help = "compare two memory models",type = str)
parser.add_argument("-c2","--compare2", choices=["sc","tso","pso","alpha","rmo"],help = "compare two memory models",type = str)
parser.add_argument("-file","--file", help = "filename",type = str)
args = parser.parse_args()
first_model = args.compare1
second_model = args.compare2
filename = args.file
print (first_model)
print (second_model)
print (filename)

def memory_model(filename,model):
    n = 0
    list = []
    with open(filename, 'r') as f:
	lines = f.readlines()
	for line in lines:
	    n = n + 1
	    if "#!-r diffvar,unsat_core,remove_implied -M " + model + "\n" in line:
	        'print (line,n)'
	    	num = n
		'print num'
		break
	for line in islice(lines,num,num+100):
	    'print (line)'
	    if "#!" not in line:
		list.append(line.strip())
	    else:
		break
    return list
	    
def compare(filename,model1,model2):
    list1 = memory_model(filename,model1)
    list2 = memory_model(filename,model2)
    'print list1'
    'print list2'
    if list1 == list2:
	print("Same Output")
    else:
	print("Different output")


compare(filename,first_model,second_model)
		    
