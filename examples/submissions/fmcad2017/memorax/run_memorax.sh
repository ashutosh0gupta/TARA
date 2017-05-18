# non tested code... experiments were executed manually
#todo : add instruction for downloading and compiling memorax

# MEMORAX=../../../../memorax-master/src/./memorax

TIMEFORMAT="%2R"
ulimit -t 180
ulimit -Sv 1000000
MEMORAX=~/memorax/src/memorax

models="sb
hsb
"

# files="peterson.rmm
# dekker-tara.rmm
# dekker-2-tara.rmm
# lamport-tara.rmm
# bakery.rmm
# szymanski.rmm
# dijkstra.rmm
# burns-tara.rmm
# burns-3.rmm
# scheduler.rmm
# mp-2.rmm
# mp-5.rmm
# order-3.rmm
# order-5.rmm
# order-7.rmm
# nested-if.rmm
# sb-3.rmm
# rwc-2.rmm"

files="
order-3.rmm
"

for model in $models
do
    echo "======================================================"
    echo "$model..\n\n."
    for i in $files
    do
        printf "%10.10s" ${i%.cpp}
        printf "&  "
        # time ($MEMORAX -a $model -v fencins --rff $i | grep ": fence\|: dp" | wc -l | tr -d "\n" && printf "  &  ")
        timeout 180s time $MEMORAX -a $model -v fencins --rff $i
    echo "======================================================"
    done
done


# echo "TSO..\n\n."
# echo "peterson"
# $MEMORAX -a sb -v fencins --rff peterson.rmm
# echo "dekker\n"
# $MEMORAX -a sb -v fencins --rff dekker-tara.rmm
# echo "dekker-2\n"
# $MEMORAX -a sb -v fencins --rff dekker-2-tara.rmm
# echo "lamport\n"
# $MEMORAX -a sb -v fencins --rff lamport-tara.rmm
# echo "dijkstra\n"
# $MEMORAX -a sb -v fencins --rff dijkstra-tara.rmm
# echo "burns\n"
# $MEMORAX -a sb -v fencins --rff burns-tara.rmm
# echo "burns-3"
# $MEMORAX -a sb -v fencins --rff burns-3.rmm
# echo "scheduler"
# $MEMORAX -a sb -v fencins --rff scheduler.rmm
# echo "double-mp"
# $MEMORAX -a sb -v fencins --rff wmm-cycle-2.rmm
# echo "five-mp"
# $MEMORAX -a sb -v fencins --rff wmm-cycle-conv-3.rmm
# echo "syzmanski"
# $MEMORAX -a sb -v fencins --rff syzmanski.rmm
# echo "bakery"
# $MEMORAX -a sb -v fencins --rff bakery.rmm
# echo "wmm-litmus-sb"
# $MEMORAX -a sb -v fencins --rff wmm-litmus-sb.rmm
# echo "natural-long"
# $MEMORAX -a sb -v fencins --rff natural_long.rmm

# echo "PSO..\n\n."
# echo "peterson"
# $MEMORAX -a hsb -v fencins --rff peterson.rmm
# echo "dekker\n"
# $MEMORAX -a hsb -v fencins --rff dekker-tara.rmm
# echo "dekker-2\n"
# $MEMORAX  -a hsb -v fencins --rff dekker-2-tara.rmm
# echo "lamport\n"
# $MEMORAX -a hsb -v fencins --rff lamport-tara.rmm
# echo "dijkstra\n"
# $MEMORAX -a hsb -v fencins --rff dijkstra-tara.rmm
# echo "burns\n"
# $MEMORAX -a hsb -v fencins --rff burns-tara.rmm
# echo "burns-3"
# $MEMORAX -a hsb -v fencins --rff burns-3.rmm
# echo "scheduler"
# $MEMORAX -a hsb -v fencins --rff scheduler.rmm
# echo "double-mp"
# $MEMORAX -a hsb -v fencins --rff wmm-cycle-2.rmm
# echo "five-mp"
# $MEMORAX -a hsb -v fencins --rff wmm-cycle-conv-3.rmm
# echo "natural-long"
# $MEMORAX -a hsb -v fencins --rff natural_long.rmm
# echo "syzmanski"
# $MEMORAX -a hsb -v fencins --rff syzmanski-tara.rmm
# echo "bakery"
# $MEMORAX -a hsb -v fencins --rff bakery.rmm
# echo "wmm-litmus-sb"
# $MEMORAX -a hsb -v fencins --rff wmm-litmus-sb.rmm



