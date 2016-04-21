GLUE=../../../../Glue/cbmc-repair/src/cbmc/./cbmc
\\
echo "TSO..."
echo "peterson"
$GLUE --repair tso:sc peterson.c
echo "dekker"
$GLUE --repair tso:sc dekker-2-tara.c
echo "dekker-2"
$GLUE --repair tso:sc dekker-tara.c
echo "lamport"
$GLUE --repair tso:sc lamport.c
echo "dijkstra"
$GLUE --repair tso:sc dijkstra.c
echo "burns"
$GLUE --repair tso:sc burns-tara.c
echo "burns-3\n"
$GLUE --repair tso:sc burns-3.c
echo "scheduler"
$GLUE --repair tso:sc scheduler.c
echo "double-mp"
$GLUE --repair tso:sc wmm-cycle-2.c
echo "five-mp"
$GLUE --repair tso:sc wmm-cycle-conv-3.c
echo "natural-long_1"
$GLUE --repair tso:sc natural_long_1.c
echo "natural-long_3"
$GLUE --repair tso:sc natural_long_3.c
echo "syzmanski"
$GLUE --repair tso:sc syzmanski.c

echo "PSO..\n\n."
echo "peterson"
$GLUE --repair pso:sc peterson.c
echo "dekker\n"
$GLUE --repair pso:sc dekker-2-tara.c
echo "dekker-2\n"
$GLUE --repair pso:sc dekker-tara.c
echo "lamport\n"
$GLUE --repair pso:sc lamport.c
echo "dijkstra\n"
$GLUE --repair pso:sc dijkstra.c
echo "burns\n"
$GLUE --repair pso:sc burns-tara.c
echo "burns-3\n"
$GLUE --repair pso:sc burns-3.c
echo "scheduler"
$GLUE --repair pso:sc scheduler.c
echo "double-mp"
$GLUE --repair pso:sc wmm-cycle-2.c
echo "five-mp"
$GLUE --repair pso:sc wmm-cycle-conv-3.c
echo "natural-long"
$GLUE --repair pso:sc natural_long_1.c
echo "syzmanski"
$GLUE --repair pso:sc syzmanski.c

