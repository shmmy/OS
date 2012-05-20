#!/bin/bash
for i in `seq 0 128 4064`
do 
    while( true )
    do 
        lines=$(wc -l $i | cut -d' ' -f 1)
        if [ $lines -ge 128 ]; then
            echo $i Done
            break 
        else
            sleep 1
        fi
    done 
done
echo All done!
