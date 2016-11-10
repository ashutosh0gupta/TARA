MEMORAX=../../../../memorax-master/src/./memorax


echo "TSO..\n\n."
echo "peterson"
$MEMORAX -a sb -v fencins --rff peterson.rmm
echo "dekker\n"
$MEMORAX -a sb -v fencins --rff dekker-tara.rmm
echo "dekker-2\n"
$MEMORAX -a sb -v fencins --rff dekker-2-tara.rmm
echo "lamport\n"
$MEMORAX -a sb -v fencins --rff lamport-tara.rmm
echo "dijkstra\n"
$MEMORAX -a sb -v fencins --rff dijkstra-tara.rmm
echo "burns\n"
$MEMORAX -a sb -v fencins --rff burns-tara.rmm
echo "burns-3"
$MEMORAX -a sb -v fencins --rff burns-3.rmm
echo "scheduler"
$MEMORAX -a sb -v fencins --rff scheduler.rmm
echo "double-mp"
$MEMORAX -a sb -v fencins --rff wmm-cycle-2.rmm
echo "five-mp"
$MEMORAX -a sb -v fencins --rff wmm-cycle-conv-3.rmm
echo "syzmanski"
$MEMORAX -a sb -v fencins --rff syzmanski.rmm
echo "bakery"
$MEMORAX -a sb -v fencins --rff bakery.rmm
echo "wmm-litmus-sb"
$MEMORAX -a sb -v fencins --rff wmm-litmus-sb.rmm
echo "natural-long"
$MEMORAX -a sb -v fencins --rff natural_long.rmm

echo "PSO..\n\n."
echo "peterson"
$MEMORAX -a hsb -v fencins --rff peterson.rmm
echo "dekker\n"
$MEMORAX -a hsb -v fencins --rff dekker-tara.rmm
echo "dekker-2\n"
$MEMORAX  -a hsb -v fencins --rff dekker-2-tara.rmm
echo "lamport\n"
$MEMORAX -a hsb -v fencins --rff lamport-tara.rmm
echo "dijkstra\n"
$MEMORAX -a hsb -v fencins --rff dijkstra-tara.rmm
echo "burns\n"
$MEMORAX -a hsb -v fencins --rff burns-tara.rmm
echo "burns-3"
$MEMORAX -a hsb -v fencins --rff burns-3.rmm
echo "scheduler"
$MEMORAX -a hsb -v fencins --rff scheduler.rmm
echo "double-mp"
$MEMORAX -a hsb -v fencins --rff wmm-cycle-2.rmm
echo "five-mp"
$MEMORAX -a hsb -v fencins --rff wmm-cycle-conv-3.rmm
echo "natural-long"
$MEMORAX -a hsb -v fencins --rff natural_long.rmm
echo "syzmanski"
$MEMORAX -a hsb -v fencins --rff syzmanski-tara.rmm
echo "bakery"
$MEMORAX -a hsb -v fencins --rff bakery.rmm
echo "wmm-litmus-sb"
$MEMORAX -a hsb -v fencins --rff wmm-litmus-sb.rmm



