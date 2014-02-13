#!/bin/bash

for (( i = 1; i < 32; i++ )); do
    if [[ $i -lt 10 ]]; then
        cp ../wpan_config_split_row_t0${i}.xml wpan_config_802_3an_split_row_theshold_improved_t0${i}.xml
        sed -i -e 's/<module>\n.*Encoder.*\n.*\n.*<\/module>//g' wpan_config_802_3an_split_row_theshold_improved_t0${i}.xml
        sed -i -e 's/num_info_bits">336/num_info_bits">384/g'  wpan_config_802_3an_split_row_theshold_improved_t0${i}.xml
        sed -i -e 's/num_other_bits">336/num_other_bits">384/g' wpan_config_802_3an_split_row_theshold_improved_t0${i}.xml
        sed -i -e 's/IEEE_802_11ad_P42_N672_R050/IEEE_802_3AN_P1_N2048_R084/g' wpan_config_802_3an_split_row_theshold_improved_t0${i}.xml
        sed -i -e 's/Random/All_zero/g' wpan_config_802_3an_split_row_theshold_improved_t0${i}.xml
        sed -i -e 's/Decoder_LDPC_IEEE_802_11ad/Decoder_LDPC_IEEE_802_3an/g' wpan_config_802_3an_split_row_theshold_improved_t0${i}.xml
        sed -i -e 's/500/80/g' wpan_config_802_3an_split_row_theshold_improved_t0${i}.xml
    else
        cp ../wpan_config_split_row_t${i}.xml wpan_config_802_3an_split_row_theshold_improved_t${i}.xml
        sed -i -e 's/<module>\n.*Encoder.*\n.*\n.*<\/module>//g' wpan_config_802_3an_split_row_theshold_improved_t${i}.xml
        sed -i -e 's/num_info_bits">336/num_info_bits">384/g'  wpan_config_802_3an_split_row_theshold_improved_t${i}.xml
        sed -i -e 's/num_info_bits">336/num_info_bits">384/g' wpan_config_802_3an_split_row_theshold_improved_t${i}.xml
        sed -i -e 's/IEEE_802_11ad_P42_N672_R050/IEEE_802_3AN_P1_N2048_R084/g' wpan_config_802_3an_split_row_theshold_improved_t${i}.xml
        sed -i -e 's/Random/All_zero/g' wpan_config_802_3an_split_row_theshold_improved_t${i}.xml
        sed -i -e 's/Decoder_LDPC_IEEE_802_11ad/Decoder_LDPC_IEEE_802_3an/g' wpan_config_802_3an_split_row_theshold_improved_t${i}.xml
        sed -i -e 's/500/80/g' wpan_config_802_3an_split_row_theshold_improved_t${i}.xml
    fi
done
