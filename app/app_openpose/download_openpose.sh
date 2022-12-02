#!/bin/bash
set -e

cd "$( dirname "${BASH_SOURCE[0]}" )"

MODEL_PATH=${PWD}/../../models/openpose_caffe

mkdir -p ${MODEL_PATH}

wget -c -O ${MODEL_PATH}/pose_deploy_linevec.prototxt \
    https://raw.githubusercontent.com/spmallick/learnopencv/master/OpenPose/pose/coco/pose_deploy_linevec.prototxt

wget -c -O ${MODEL_PATH}/pose_iter_440000.caffemodel \
    http://posefs1.perception.cs.cmu.edu/OpenPose/models/pose/coco/pose_iter_440000.caffemodel

echo "replace width and height from 1 to 224"
sed -i "4s/1/224/" ${MODEL_PATH}/pose_deploy_linevec.prototxt
sed -i "5s/1/224/" ${MODEL_PATH}/pose_deploy_linevec.prototxt
