#!/usr/bin/env bash


TIMEFORMAT="%2R"
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


#RUNNING TARA
for model in $models
do
    echo "======================================================"
    echo "$model..\n\n."
    for i in $files
    do
        printf "%10.10s" ${i%.cpp}
        printf "&  "
        time ($TARA -M $model -o fsynth $i | grep thread | wc -l | tr -d "\n" && printf "  &  ")
    done
done
