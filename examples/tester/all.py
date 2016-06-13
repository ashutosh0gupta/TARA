#!/usr/bin/python
from itertools import islice
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
	      '../wmm-litmus-irwiw.ctrc'
            ]

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
    'return set(list1) == set(list2) and len(list1) == len(list2)'
    if list1 == list2:
	print("Same Output\n")
    else:
	print("Different output\n")

models = ["sc","tso","pso","rmo"]
lists=[]
for files in known_files:
    for model1 in models:
	for model2 in models:
	    print (files,model1,model2)
	    if model1 != model2:
	    	compare(files,model1,model2)
		'lists.append(compare(files,model1,model2))'
		
