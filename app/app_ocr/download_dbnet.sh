#!/bin/bash
set -e

echo "export model begin..."

cd "$( dirname "${BASH_SOURCE[0]}" )"

MODEL_PATH=${PWD}/../../models/dbnet_pytorch

if [ ! -f ${MODEL_PATH}/dbnet_traced.pt ];
then
    if [ ! -d "DB" ];
    then
      git clone https://github.com/MhLiao/DB.git
    fi

    pip install -r tools/requirement.txt

    cp -r tools/cpu_dcn DB/assets/ops/
    cp tools/vision_deform_conv.py DB/backbones/
    
    pushd DB/assets/ops/cpu_dcn
    python setup.py build develop
    popd
    
    # share link https://drive.google.com/drive/folders/1T9n0HTP3X3Y_nJ0D1ekMhCQRHntORLJG
    if [ ! -f DB/totaltext_resnet18 ];
    then
        gdown https://drive.google.com/uc?id=1bVBYXSnFpwZxu8wHROAqJ4ddhLfZE7MY -O DB/totaltext_resnet18
    fi

    cp -r tools/datasets/ DB/

    # 3.patch-retinaface
    if grep -q "traced_model = torch.jit.trace(model, torch.rand(1, 3, 736, 896))" DB/demo.py
    then
      echo "modifying file:demo.py has been already done"
    else
      echo "modifying file: demo.py ..."
      patch -u DB/demo.py < tools/patch/demo.diff
      patch -u DB/structure/model.py < tools/patch/model.diff
      patch -u DB/backbones/resnet.py < tools/patch/resnet.diff
    fi
    # 4.trace model
    echo "export model begin..."
    mkdir -p ${MODEL_PATH}
    pushd DB
    python demo.py experiments/seg_detector/totaltext_resnet18_deform_thre.yaml \
                  --image_path img10.jpg \
                  --resume totaltext_resnet18 \
                  --polygon --box_thresh 0.7 --visualize \
                  --traced_pt ${MODEL_PATH}/dbnet_traced.pt
    echo "export model end..."
    popd
fi
