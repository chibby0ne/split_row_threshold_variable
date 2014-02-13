#!/bin/bash

for (( i = 1; i < 32; i++ )); do
    if [[ $i -lt 10 ]]; then
        sed -i -e "s/<edges_threshold>7<\/edges_threshold>//g" wpan_config_split_row_t0${i}.xml
    else
        sed -i -e "s/<edges_threshold>7<\/edges_threshold>//g" wpan_config_split_row_t${i}.xml
    fi
done
