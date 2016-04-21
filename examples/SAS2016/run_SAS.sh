GLUE=../../../Glue/cbmc-repair/src/cbmc/./cbmc
MEMORAX=../../../memorax-master/src/./memorax
TARA=../.././tara

echo "TARA\n\n\n\nPSO..\n\n."
echo "peterson"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso tara/peterson.ctrc
echo "dekker\n"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso tara/dekker.ctrc
echo "dekker-2\n"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso tara/dekker-2.ctrc
echo "lamport\n"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso tara/lamport.ctrc
echo "dijkstra\n"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso tara/dijkstra.ctrc
echo "burns\n"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso tara/burns.ctrc
echo "burns-3\n"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso tara/burns-3.ctrc
echo "scheduler"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso tara/scheduler.ctrc
echo "double-mp"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso tara/wmm-cycles-2.ctrc
echo "five-mp"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso tara/wmm-cycle-conv-3.ctrc


echo "MEMORAX\n\n\n\nPSO..\n\n."
echo "peterson"
$MEMORAX -a hsb -v fencins --rff memorax/peterson.rmm
echo "dekker\n"
$MEMORAX -a hsb -v fencins --rff memorax/dekker-tara.rmm
echo "dekker-2\n"
$MEMORAX  -a hsb -v fencins --rff memorax/dekker-2-tara.rmm
echo "lamport\n"
$MEMORAX -a hsb -v fencins --rff memorax/lamport-tara.rmm
echo "dijkstra\n"
$MEMORAX -a hsb -v fencins --rff memorax/dijkstra-tara.rmm
echo "burns\n"
$MEMORAX -a hsb -v fencins --rff memorax/burns-tara.rmm
echo "burns-3"
$MEMORAX -a hsb -v fencins --rff memorax/burns-3.rmm
echo "scheduler"
$MEMORAX -a hsb -v fencins --rff memorax/scheduler.rmm
echo "double-mp"
$MEMORAX -a hsb -v fencins --rff memorax/wmm-cycle-2.rmm
echo "five-mp"
$MEMORAX -a hsb -v fencins --rff memorax/wmm-cycle-conv-3.rmm


echo "GLUE\n\n\n\nPSO..\n\n."
echo "peterson"
$GLUE --repair pso:sc glue/peterson.c
echo "dekker\n"
$GLUE --repair pso:sc glue/dekker-2-tara.c
echo "dekker-2\n"
$GLUE --repair pso:sc glue/dekker-tara.c
echo "lamport\n"
$GLUE --repair pso:sc glue/lamport.c
echo "dijkstra\n"
$GLUE --repair pso:sc glue/dijkstra.c
echo "burns\n"
$GLUE --repair pso:sc glue/burns-tara.c
echo "burns-3\n"
$GLUE --repair pso:sc glue/burns-3.c
echo "scheduler"
$GLUE --repair pso:sc glue/scheduler.c
echo "double-mp"
$GLUE --repair pso:sc glue/wmm-cycle-2.c
echo "five-mp"
$GLUE --repair pso:sc glue/wmm-cycle-conv-3.c


echo "*****THE END*****"
