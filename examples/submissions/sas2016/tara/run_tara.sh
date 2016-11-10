TARA=../../.././tara

echo "TSO..\n\n."
echo "peterson"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M tso peterson.ctrc
echo "dekker\n"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M tso dekker.ctrc
echo "dekker-2\n"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M tso dekker-2.ctrc
echo "lamport\n"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M tso lamport.ctrc
echo "dijkstra\n"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M tso dijkstra.ctrc
echo "burns\n"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M tso burns.ctrc
echo "burns-3\n"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M tso burns-3.ctrc
echo "scheduler"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M tso scheduler.ctrc
echo "double-mp"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M tso wmm-cycles-2.ctrc
echo "five-mp"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M tso wmm-cycle-conv-3.ctrc
echo "natural-long"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M tso natural_long.ctrc
echo "syzmanski"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M tso szymanski.ctrc

echo "PSO..."
echo "peterson"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso peterson.ctrc
echo "dekker\n"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso dekker.ctrc
echo "dekker-2\n"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso dekker-2.ctrc
echo "lamport\n"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso lamport.ctrc
echo "dijkstra\n"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso dijkstra.ctrc
echo "burns\n"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso burns.ctrc
echo "burns-3\n"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso burns-3.ctrc
echo "scheduler"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso scheduler.ctrc
echo "double-mp"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso wmm-cycles-2.ctrc
echo "five-mp"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso wmm-cycle-conv-3.ctrc
echo "natural-long"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso natural_long.ctrc
echo "syzmanski"
$TARA -o wmm_synthesis -r diffvar,unsat_core,remove_implied -M pso szymanski.ctrc

