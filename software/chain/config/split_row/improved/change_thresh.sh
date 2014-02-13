#!/bin/bash

for (( i = 10; i < 32; i++ )); do
    echo "changing threshold of $i"
        sed -i -e "s/<threshold>[0-2]/<threshold>${i}/g" wpan_config_split_row_t${i}.xml
#        sed -i -e "s/<threshold>[0-32]/<threshold>${i}/g" wpan_config_split_row_t${i}_edges.xml
done
