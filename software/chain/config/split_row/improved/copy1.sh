#!/bin/bash

for (( i = 1; i < 32; i++ )); do
    echo "copying $i"
    if [[ $i -lt 10 ]]; then
        cp wpan_config_split_row_edges_threshold_t0${i}.xml wpan_config_split_row_t0${i}.xml
    else
        cp wpan_config_split_row_edges_threshold_t${i}.xml wpan_config_split_row_t${i}.xml
    fi
done
