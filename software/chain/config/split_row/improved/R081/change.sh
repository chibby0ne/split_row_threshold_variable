#!/bin/bash

for (( i = 1; i < 32; i++ )); do
    if [[ $i -lt 10 ]]; then
        sed -i -e 's/"num_info_bits">336/"num_info_bits">546/g' wpan_config_split_row_t0${i}_r081.xml
        sed -i -e 's/"num_other_bits">336/"num_other_bits">126/g' wpan_config_split_row_t0${i}_r081.xml
    else
        sed -i -e 's/"num_info_bits">336/"num_info_bits">546/g' wpan_config_split_row_t${i}_r081.xml
        sed -i -e 's/"num_other_bits">336/"num_other_bits">126/g' wpan_config_split_row_t${i}_r081.xml
    fi
done
