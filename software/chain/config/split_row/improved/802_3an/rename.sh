#!/bin/bash

for (( i = 1; i < 32; i++ )); do
    if [[ $i -lt 10 ]]; then
        mv wpan_config_802_3an_split_row_theshold_improved_t0${i}.xml wpan_config_802_3an_split_row_threshold_improved_t0${i}.xml
    else
        mv wpan_config_802_3an_split_row_theshold_improved_t${i}.xml wpan_config_802_3an_split_row_threshold_improved_t${i}.xml 
    fi
done
