#!/bin/bash

for (( i = 1; i < 32; i++ )); do
    if [[ $i -lt 10 ]]; then
        sed -i -e "s/<num_partitions>2/<num_partitions>4/g"  wpan_config_split_row_parts4_t0${i}.xml
    else
        sed -i -e "s/<num_partitions>2/<num_partitions>4/g"  wpan_config_split_row_parts4_t${i}.xml
    fi
done
