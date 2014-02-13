#!/bin/bash

for (( i = 1; i < 32; i++ )); do
    if [[ $i -lt 10 ]]; then
        sed -i -e "s/>10</>15</g" wpan_config_split_row_t0${i}.xml
        sed -i -e "s/:10</:15</g" wpan_config_split_row_t0${i}.xml
        sed -i -e "s/IEEE_802_11ad_P42_N672_R050/IEEE_802_3an_P1_N2048_R084/g" wpan_config_split_row_t0${i}.xml
        sed -i -e "s/Encoder_LDPC_IEEE_802_11ad/Encoder_IEEE_802_3an/g" wpan_config_split_row_t0${i}.xml
        sed -i -e "s/Decoder_LDPC_IEEE_802_11ad/Decoder_IEEE_802_3an/g" wpan_config_split_row_t0${i}.xml
        sed -i -e "s/100000/163480/g" wpan_config_split_row_t0${i}.xml
    else
        sed -i -e "s/>10</>15</g" wpan_config_split_row_t${i}.xml
        sed -i -e "s/:10</:15</g" wpan_config_split_row_t${i}.xml
        sed -i -e "s/IEEE_802_11ad_P42_N672_R050/IEEE_802_3an_P1_N2048_R084/g" wpan_config_split_row_t${i}.xml
        sed -i -e "s/Encoder_LDPC_IEEE_802_11ad/Encoder_IEEE_802_3an/g" wpan_config_split_row_t${i}.xml
        sed -i -e "s/Decoder_LDPC_IEEE_802_11ad/Decoder_IEEE_802_3an/g" wpan_config_split_row_t${i}.xml
        sed -i -e "s/100000/163480/g" wpan_config_split_row_t${i}.xml
    fi
done
