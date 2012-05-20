#!/bin/bash

STEP=256
HEIGHT=16384
let H_LIM=$HEIGHT-$STEP

machines=(evia
kefalonia
kerkyra
kythnos
lemnos
lesvos
leykada
naxos
patmos
paxoi
poros
serifos
skopelos
skyros
zakynthos
)
nr=${#machines[@]}
for i in `seq 0 $STEP $H_LIM`
do
    let j=i+$STEP
    let mindex=i%nr
    for k in $(seq $mindex $nr)
    do
        m=${machines[$(($k))]}
        ping -W 1 -q $m -c 1 > /dev/null
        if [ "$?" == "0" ]; then
            break
        fi
    done
    (ssh $m /home/oslab/oslabb03/Repos/OS/ask_3/stage_1.2_bitmap/mandel $i $j) &
done

sleep 2

for i in `seq 0 $STEP $H_LIM`
do 
    while( true )
    do 
        lines=$(wc -l $i | cut -d' ' -f 1)
        if [ $lines -ge $STEP ]; then
            echo $i Done
            break 
        else
            sleep 1
        fi
    done 
done
echo All done!

rm mandel_parallel.ppm
for i in `seq 0 $STEP $H_LIM`
do
    cat ~/$i >> mandel_parallel.ppm
    rm ~/$i
done
