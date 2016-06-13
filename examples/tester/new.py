#!/usr/bin/python
from subprocess import call,check_output
import os.path
import sys
import io

bin_name = "../../tara"
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

def execute_example(o,filename):
    ops =o.split(' ')
    ops = [o for o in ops if o != '']
    ops.append(filename)
    return check_output([bin_name]+ops)

def compare(o1,o2,filename):
    rmo = execute_example(o1,filename)
    'print rmo'
    alpha = execute_example(o2,filename)
    'print alpha'
    print filename
    if rmo == alpha:
	print "Same"
    else:
	print "Different"

for files in known_files:
    compare('-r diffvar,unsat_core,remove_implied -M rmo','-r diffvar,unsat_core,remove_implied -M alpha',files)


    
