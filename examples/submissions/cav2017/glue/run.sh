#!/usr/bin/env bash

TIMEFORMAT="%2R"
GLUE=~/cbmc/src/cbmc/cbmc # in virtual machine


# model="pso"
model="tso"
# model="alpha"
# model="rmo"

echo "$model..\n\n."

files="peterson.c
dekker.c
dekker-2.c
lamport.c
bakery.c
szymanski.c
dijkstra.c
burns.c
burns-3.c
scheduler.c
double-mp.c
fivemp.c
order3.c
order5.c
order7.c
nested-if-test.c
triple-sb.c
double-rwc.c"


for i in $files
do
    printf "${i%.c}  &  "
    # $GLUE $i
    # time $GLUE --repair tso:sc $i
    time ($GLUE --repair $model:sc $i 2> /dev/null | grep Fence | wc -l | tr -d "\n" && printf "  &  ")
done
