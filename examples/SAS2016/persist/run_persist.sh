PERSIST=../../../../persistence-master/build/./PERSIST

echo "TSO..\n\n."
echo "dekker\n"
$PERSIST -b -f dekker.txt
echo "burns\n"
$PERSIST -b -f burns.txt
echo "dekker-2\n"
$PERSIST -b -f dekker-2.txt
echo "dijkstra\n"
$PERSIST -b -f dijakstra.txt
echo "lamport\n"
$PERSIST -b -f lamport.txt
echo "peterson"
$PERSIST -b -f peterson.txt
echo "syzmanski"
$PERSIST -b -f szymanski.txt
echo "wmm-cycle-tso-1"
$PERSIST -b -f wmm-cycle-tso-1.txt
echo "wmm-cycle-conv-2"
$PERSIST -b -f wmm-cycle-conv-2.txt
echo "burns-3"
$PERSIST -b -f burns-3.txt
echo "bakery"
$PERSIST -b -f bakery.txt
echo "wmm-cycle-tso-2"
$PERSIST -b -f wmm-cycle-tso-2.txt

