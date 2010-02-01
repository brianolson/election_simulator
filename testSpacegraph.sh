#!/bin/sh -x
THREADS='--threads 2'
VIEWER=open
#VIEWER=display
start=`date`
./spacegraph ${THREADS} -px 100 -py 100 -n 1 -minx -1 -miny -1 -maxx 1 -maxy 1 -Z 1.0 -v 1000 -c -0.20,0.14 -c -0.68,0.08 -c -0.90,0.24 -c 0.82,0.40 --method Condorcet,IRNR -o /tmp/4c_%m.png ${EXTRA_OPTS}
./spacegraph ${THREADS} -px 100 -py 100 -n 1 -minx -1 -miny -1 -maxx 1 -maxy 1 -Z 1.0 -v 1000 -c -0.20,0.14 -c -0.68,0.08 -c -0.90,0.24 -c 0.82,0.40 --seats=2 --method STV,IRNRP -o /tmp/4c2_%m.png --planes=/tmp/4c2_%m_%p.png ${EXTRA_OPTS}
./spacegraph ${THREADS} -px 100 -py 100 -n 1 -minx -1 -miny -1 -maxx 1 -maxy 1 -Z 1.0 -v 1000 -tg /tmp/pop.png ${EXTRA_OPTS}
finish=`date`
echo 'start ' ${start}
echo finish ${finish}
ls -lat /tmp/4c*png /tmp/pop.png
${VIEWER} /tmp/4c*png /tmp/pop.png
