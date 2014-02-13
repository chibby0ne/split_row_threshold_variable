#!/bin/bash

for i in {2..10..1}
do
    cp wpan_config_split_row_t01_r081.xml wpan_config_split_row_t0${i}_r081.xml
#    cp wpan_config_split_row_t01_r081.xml wpan_config_split_row_t${i}_r081.xml
done
