#!/bin/bash

for (( i = 1; i < 32; i++ )); do
    if [[ $i -lt 10 ]]; then
        sed -i -e "s/MIN_SUM/SPLIT_ROW_IMPROVED/g"  wpan_config_802_3an_split_row_threshold_improved_t0${i}.xml
    else
        sed -i -e "s/MIN_SUM/SPLIT_ROW_IMPROVED/g"  wpan_config_802_3an_split_row_threshold_improved_t${i}.xml
    fi
done
