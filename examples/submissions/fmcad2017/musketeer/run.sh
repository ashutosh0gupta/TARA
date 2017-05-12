#!/usr/bin/env bash

# musketeer downloaded from the following location
#http://www.cprover.org/wmm/fence13/musketeer_170414.tar.gz

TIMEFORMAT="%2R"
MUSKETEER1=~/tmp/musket/goto-cc
MUSKETEER2=~/tmp/musket/musketeer
TARA=~/research/shared/tara/tara


# model="pso"
# model="tso"
# model="alpha"
model="rmo"

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

#Running MUSKETEER
for i in $files
do
    printf "${i%.cpp}\t&  "
    $MUSKETEER1 -o o.gb $i
    time ($MUSKETEER2 --mm $model o.gb | grep ": fence\|: dp" | wc -l | tr -d "\n" && printf "  &  ")
done

