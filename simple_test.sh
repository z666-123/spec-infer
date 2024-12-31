#! /usr/bin/env bash
set -e
set -x

# Cd into directory holding this script
cd "${BASH_SOURCE[0]%/*}"

# Export UCX_DIR and update PATH and LD_LIBRARY_PATH (keep as is)
# export UCX_DIR="/media/data1/zhurh/ucx"
# export PATH=$UCX_DIR/bin:$PATH
# export LD_LIBRARY_PATH=$UCX_DIR/lib:$LD_LIBRARY_PATH

# Download the simplest dataset and model for testing
./download_dataset.sh  # Assuming this downloads a simple dataset by default
# python ./FlexFlow/inference/utils/download_hf_model.py --half-precision-only JackFram/llama-68m
# /media/data1/zhurh/download/JackFramllama-68m

# Prepare output directory
mkdir -p ./FlexFlow/inference/output

start_time=$(date +%s)

# # Single node, single GPU configuration with batch size 1
# ncpus=8
# ngpus=1
# fsize=21890
# zsize=80000
# max_sequence_length=128

ncpus=8
ngpus=1
fsize=22000  # 设为约22MB，稍微高于您当前的21890
zsize=80000  # 设为约78MB，保持不变
max_sequence_length=128


# llm_model_name="JackFram/llama-68m"
llm_model_name="/media/data1/zhurh/download/JackFramllama-68m"

# Only test incremental decoding with batch size 1
bs=1
prompt_file="./FlexFlow/inference/prompt/chatgpt_1.json"  # Adjust path if necessary

# Incremental decoding
./FlexFlow/build/inference/incr_decoding/incr_decoding \
    -ll:cpu $ncpus -ll:util $ncpus -ll:gpu $ngpus -ll:fsize $fsize -ll:zsize $zsize \
    -llm-model $llm_model_name \
    -prompt $prompt_file \
    --max-requests-per-batch $bs \
    --max-sequence-length $max_sequence_length \
    -tensor-parallelism-degree $ngpus \
    --fusion \
    -output-file ./FlexFlow/inference/output/server_small-${bs}_batchsize-incr_dec.txt \
    > ./FlexFlow/inference/output/server_small-${bs}_batchsize-incr_dec.out

end_time=$(date +%s)
execution_time=$((end_time - start_time))
echo "Total server gpu test time: $execution_time seconds"