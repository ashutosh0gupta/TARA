#!/usr/bin/env bash

#todo: add instructions for downloading and compiling glue 

#the script was run a virtual machine; yet to be adapted for reusability

TIMEFORMAT="%2R"
GLUE=~/cbmc/src/cbmc/cbmc # in virtual machine

models="tso
pso
"
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
mp-2.c
mp-5.c
order-3.c
order-5.c
order-7.c
nested-if-test.c
sb-2.c
rwc-2.c"

for model in $models
do
    echo "======================================================"
    echo "$model..\n\n."
    for i in $files
    do
        printf "%10.10s" ${i%.c}
        printf "&  "
        time ($GLUE --repair $model:sc $i 2> /dev/null | grep Fence | wc -l | tr -d "\n" && printf "  &  ")
    done
done
