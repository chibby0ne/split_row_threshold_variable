#!/bin/bash

for (( i = 1; i < 32; i++ )); do
    if [[ $i -lt 10 ]]; then
        cp wpan_config_split_row_t0${i}.xml wpan_config_split_row_parts4_t0${i}.xml
    else
        cp wpan_config_split_row_t${i}.xml wpan_config_split_row_parts4_t${i}.xml
    fi
done
