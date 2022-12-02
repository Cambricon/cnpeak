#!/bin/bash
set -e

cd "$( dirname "${BASH_SOURCE[0]}" )"

MODEL_PATH="${PWD}/../../models/yolov5_pytorch"

if [ -d $MODEL_PATH ];
then
    echo "folder $MODEL_PATH already exist!!!"
else
    mkdir -p "$MODEL_PATH"
fi

# 1.下载数据集和模型
pushd $MODEL_PATH
if [ -f "yolov5m.pt" ]; 
then
  echo "yolov5m.pt already exists."
else
  echo "Downloading yolov5m.pt file"
  wget -c https://github.com/ultralytics/yolov5/releases/download/v6.1/yolov5m.pt
fi
popd

# 2.下载yolov5实现源码，切换到v6.1分支
if [ -d "yolov5" ];
then
  echo "yolov5 already exists."
else
  echo "git clone yolov5..."
  git clone https://github.com/ultralytics/yolov5.git
  pushd yolov5
  git checkout -b v6.1 v6.1
  popd
fi

# 3.patch-yolov5
if grep -q "${MODELPATH}/yolov5m_traced.pt" yolov5/export.py;
then 
  echo "modifying the yolov5m has been already done"
else
  echo "modifying the yolov5m..."
  pushd yolov5
  git apply ../tool/yolov5_v6_1_pytorch.patch
  popd
fi

# 4.patch-torch-cocodataset
if grep -q "SiLU" /usr/lib/python3.7/site-packages/torch/nn/modules/__init__.py;
then
  echo "SiLU activation operator already exists.";
else
  echo "add SiLU op in '/usr/lib/python3.7/site-packages/torch/nn/modules/__init__.py and activation.py'"
  patch -p0 /usr/lib/python3.7/site-packages/torch/nn/modules/__init__.py < tool/init.patch
  patch -p0 /usr/lib/python3.7/site-packages/torch/nn/modules/activation.py < tool/activation.patch
fi

# 5.trace model
echo "export model begin..."
pip install -r tool/requirements.txt
python yolov5/export.py --weights $MODEL_PATH/yolov5m.pt --imgsz 640 640 --include torchscript --batch-size 1
echo "export model end..."
