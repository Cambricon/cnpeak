#!/bin/bash
set -e

echo "export model begin..."

cd "$( dirname "${BASH_SOURCE[0]}" )"

MODEL_PATH=${PWD}/../../models/crnn_pytorch

if [ ! -f ${MODEL_PATH}/crnn_traced.pt ];
then
    # 1. clone code 
    if [ ! -d "crnn-pytorch" ];
    then
        echo "git clone crnn-pytorch..."
        git clone https://github.com/GitYCC/crnn-pytorch.git
    fi
    
    # 2.trace model
    mkdir -p ${MODEL_PATH}
    python tools/jit_crnn.py \
        --model_weight crnn-pytorch/checkpoints/crnn_synth90k.pt \
    	--input_width 100 \
    	--input_height 32 \
        --traced_pt ${MODEL_PATH}/crnn_traced.pt
    
fi

echo "export crnn success, model save to pytorch_crnn/crnn_traced.pt "
