#!/usr/bin/env bash


TIMEFORMAT="%2R"
TARA=~/research/shared/tara/tara


# model="pso"
model="tso"
# model="alpha"
# model="rmo"

echo "$model..\n\n."

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
double-mp.cpp
fivemp.cpp
order3.cpp
order5.cpp
order7.cpp
nested-if-test.cpp
triple-sb.cpp
double-rwc.cpp"


#RUNNING TARA
for i in $files
do
    printf "${i%.cpp}  &  "
    time ($TARA -M $model -o fsynth $i | grep thread | wc -l | tr -d "\n" && printf "  &  ")
done
