#!/usr/bin/env bash

# musketeer downloaded from the following location
#http://www.cprover.org/wmm/fence13/musketeer_170414.tar.gz

TIMEFORMAT="%2R"
MUSKETEER1=~/tmp/musket/goto-cc
MUSKETEER2=~/tmp/musket/musketeer
TARA=~/research/shared/tara/tara

models="tso
pso
rmo
alpha
"

files="peterson.cpp
dekker.cpp
dekker-2.cpp
lamport.cpp
bakery.cpp
szymanski.cpp
dijkstra.cpp
burns.cpp
burns-3.cpp
scheduler.cpp
mp-2.cpp
mp-5.cpp
order-3.cpp
order-5.cpp
order-7.cpp
nested-if-test.cpp
sb-3.cpp
rwc-2.cpp"

#Running MUSKETEER
for model in $models
do
    echo "======================================================"
    echo "$model..\n\n."
    for i in $files
    do
        printf "%10.10s" ${i%.cpp}
        printf "&  "
        $MUSKETEER1 -o o.gb $i
        time ($MUSKETEER2 --mm $model o.gb | grep ": fence\|: dp" | wc -l | tr -d "\n" && printf "  &  ")
        rm o.gb
    done
done
