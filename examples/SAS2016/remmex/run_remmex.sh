REMMEX=../../../../Remmex/jar/remmex/
cd $REMMEX
echo "errorCorrection TSO maximal permissive"
echo "natural_long"
java -Xss2048k -Xmx4096m -jar remmex.jar -f ../../../TARA/examples/SAS2016/remmex/natural_long.txt -P safety -MM TSO -Mode errorCorrection -e 2 10 12 -maximalPermissive
echo "natural_long_2"
java -Xss2048k -Xmx4096m -jar remmex.jar -f ../../../TARA/examples/SAS2016/remmex/natural_long_2.txt -P safety -MM TSO -Mode errorCorrection -e 2 7 9 -maximalPermissive
echo "natural_self"
java -Xss2048k -Xmx4096m -jar remmex.jar -f ../../../TARA/examples/SAS2016/remmex/natural_self.txt -P safety -MM TSO -Mode errorCorrection -e 2 5 10 -maximalPermissive
echo "scheduler"
java -Xss2048k -Xmx4096m -jar remmex.jar -f ../../../TARA/examples/SAS2016/remmex/scheduler.txt -P safety -MM TSO -Mode errorCorrection -e 2 3 14 -maximalPermissive
echo "wmm-cycles-tso-2"
java -Xss2048k -Xmx4096m -jar remmex.jar -f ../../../TARA/examples/SAS2016/remmex/wmm-cycles-tso-2.txt -P safety -MM TSO -Mode errorCorrection -e 2 6 6 -maximalPermissive
echo "wmm-cycle-2"
java -Xss2048k -Xmx4096m -jar remmex.jar -f ../../../TARA/examples/SAS2016/remmex/wmm-cycle-2.txt -P safety -MM TSO -Mode errorCorrection -e 2 4 6 -maximalPermissive
echo "wmm-litmus-sb"
java -Xss2048k -Xmx4096m -jar remmex.jar -f ../../../TARA/examples/SAS2016/remmex/wmm-litmus-sb.txt -P safety -MM TSO -Mode errorCorrection -e 2 2 2 -maximalPermissive


echo "errorCorrection PSO maximal permissive"
echo "natural_long"
java -Xss2048k -Xmx4096m -jar remmex.jar -f ../../../TARA/examples/SAS2016/remmex/natural_long.txt -P safety -MM PSO -Mode errorCorrection -e 2 10 12 -maximalPermissive
echo "natural_long_2"
java -Xss2048k -Xmx4096m -jar remmex.jar -f ../../../TARA/examples/SAS2016/remmex/natural_long_2.txt -P safety -MM PSO -Mode errorCorrection -e 2 7 9 -maximalPermissive
echo "natural_self"
java -Xss2048k -Xmx4096m -jar remmex.jar -f ../../../TARA/examples/SAS2016/remmex/natural_self.txt -P safety -MM PSO -Mode errorCorrection -e 2 5 10 -maximalPermissive
echo "scheduler"
java -Xss2048k -Xmx4096m -jar remmex.jar -f ../../../TARA/examples/SAS2016/remmex/scheduler.txt -P safety -MM PSO -Mode errorCorrection -e 2 3 14 -maximalPermissive
echo "wmm-cycles-tso-2"
java -Xss2048k -Xmx4096m -jar remmex.jar -f ../../../TARA/examples/SAS2016/remmex/wmm-cycles-tso-2.txt -P safety -MM PSO -Mode errorCorrection -e 2 6 6 -maximalPermissive
echo "wmm-cycle-2"
java -Xss2048k -Xmx4096m -jar remmex.jar -f ../../../TARA/examples/SAS2016/remmex/wmm-cycle-2.txt -P safety -MM PSO -Mode errorCorrection -e 2 4 6 -maximalPermissive
echo "wmm-litmus-sb"
java -Xss2048k -Xmx4096m -jar remmex.jar -f ../../../TARA/examples/SAS2016/remmex/wmm-litmus-sb.txt -P safety -MM PSO -Mode errorCorrection -e 2 2 2 -maximalPermissive

