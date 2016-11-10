MUSKETEER1=../../../../musketeer_root/./goto-cc
MUSKETEER2=../../../../musketeer_root/./musketeer

echo "PSO..\n\n."
echo "natural_long"
$MUSKETEER1 -o natural_long.gb natural_long.c
$MUSKETEER2 --mm pso natural_long.gb
rm natural_long.gb
echo "scheduler"
$MUSKETEER1 -o scheduler.gb scheduler.c
$MUSKETEER2 --mm pso scheduler.gb
rm scheduler.gb
echo "wmm-cycle-2"
$MUSKETEER1 -o wmm-cycle-2.gb wmm-cycle-2.c
$MUSKETEER2 --mm pso wmm-cycle-2.gb
rm wmm-cycle-2.gb
echo "wmm-simple-sb"
$MUSKETEER1 -o wmm-simple-sb.gb wmm-simple-sb.c
$MUSKETEER2 --mm pso wmm-simple-sb.gb
rm wmm-simple-sb.gb

echo "TSO..\n\n."
echo "natural_long"
$MUSKETEER1 -o natural_long.gb natural_long.c
$MUSKETEER2 --mm tso natural_long.gb
rm natural_long.gb
echo "scheduler"
$MUSKETEER1 -o scheduler.gb scheduler.c
$MUSKETEER2 --mm tso scheduler.gb
rm scheduler.gb
echo "wmm-cycle-2"
$MUSKETEER1 -o wmm-cycle-2.gb wmm-cycle-2.c
$MUSKETEER2 --mm tso wmm-cycle-2.gb
rm wmm-cycle-2.gb
echo "wmm-simple-sb"
$MUSKETEER1 -o wmm-simple-sb.gb wmm-simple-sb.c
$MUSKETEER2 --mm tso wmm-simple-sb.gb
rm wmm-simple-sb.gb
