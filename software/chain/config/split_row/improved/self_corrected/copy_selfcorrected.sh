#!/bin/bash

for (( i = 1; i < 32; i++ )); do
    if [[ $i -lt 10 ]]; then
        cp ../wpan_config_split_row_t0${i}.xml wpan_config_split_row_self_corrected_t0${i}.xml
        sed -i -e "s/SPLIT_ROW_IMPROVED/SPLIT_ROW_SELF_CORRECTING/g" wpan_config_split_row_self_corrected_t0${i}.xml
    else
        cp ../wpan_config_split_row_t${i}.xml wpan_config_split_row_self_corrected_t${i}.xml
        sed -i -e "s/SPLIT_ROW_IMPROVED/SPLIT_ROW_SELF_CORRECTING/g" wpan_config_split_row_self_corrected_t${i}.xml
    fi
done
