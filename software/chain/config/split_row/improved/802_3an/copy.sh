#!/bin/bash

for (( i = 1; i < 32; i++ )); do
    if [[ $i -lt 10 ]]; then
        cp wpan_config.xml wpan_config_802_3an_split_row_theshold_improved_t0${i}.xml
    else
        cp wpan_config.xml wpan_config_802_3an_split_row_theshold_improved_t${i}.xml
    fi
done

