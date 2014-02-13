#!/bin/bash

for (( i = 1; i < 32; i++ )); do
    if [[ $i -lt 10 ]]; then
        cp wpan_config_split_row_t0${i}.xml 802_3an/wpan_config_split_row_t0${i}.xml
    else
        cp wpan_config_split_row_t${i}.xml 802_3an/wpan_config_split_row_t${i}.xml
    fi
done
