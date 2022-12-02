# cnpeak

- [cnpeak](#cnpeak)
  - [1 总体介绍](#1-总体介绍)
  - [2 前提条件](#2-前提条件)
  - [3 环境准备](#3-环境准备)
  - [4 使用方法](#4-使用方法)
  - [5 流程介绍](#5-流程介绍)
    - [5.1 总体结构](#51-总体结构)
    - [5.2 组件说明](#52-组件说明)
    - [5.3 目录结构](#53-目录结构)
    - [5.4 支持的插件列表](#54-支持的插件列表)
  - [6 二次开发FAQ](#6-二次开发faq)
    - [6.1 如何替换模型权值](#61-如何替换模型权值)
    - [6.2 如何更换插件](#62-如何更换插件)
    - [6.3 如何更换模型](#63-如何更换模型)

## 1 总体介绍

cnpeak是一个基于流水线的简易框架，通过在流水线上安装各种插件，可以实现不同的业务流程。

----------
已有的应用示例：
- yolov5
- openpose
- ocr

## 2 前提条件

- Linux 常见操作系统版本(如 Ubuntu16.04，Ubuntu18.04，CentOS7.x 等)，安装 docker(>=v18.00.0)应用程序；
- 服务器装配好寒武纪 300 系列及以上的智能加速卡，并安装好驱动(>=v4.20.6)；
- 若不具备以上软硬件条件，可前往[寒武纪开发者社区](https://developer.cambricon.com/)申请试用;

## 3 环境准备

若基于寒武纪云平台环境可跳过该环节。否则需运行以下步骤：

1.请前往[寒武纪开发者社区](https://developer.cambricon.com/)，下载 MagicMind(version >= 0.13.0)镜像和cncv的deb包，名字如下：

```
magicmind_version_os.tar.gz  例如: magicmind_0.13.1-1_ubuntu18.04.tar.gz  
cncv_version.os.deb          例如: cncv_0.12.0-1.ubuntu18.04_amd64.deb  
```

2.加载：

```bash
docker load -i magicmind_version_os.tar.gz
```

3.运行：

```bash
docker run -it --name=dockername \
           --network=host --cap-add=sys_ptrace \
           -v /your/host/path/cnpeak:/cnpeak \
           -v /usr/bin/cnmon:/usr/bin/cnmon \
           --device=/dev/cambricon_dev0:/dev/cambricon_dev0 --device=/dev/cambricon_ctl \
           -w /cnpeak/ magicmind_version_image_name:tag_name /bin/bash
```

4.安装依赖

```bash
# 安装cncodec3 
apt update && apt install -y cncodec3
# 安装cncv
apt install ./cncv_<version>.<os_arch>.deb
```

## 4 使用方法

1. 下载测试数据
```bash
git clone https://gitee.com/mldata/data.git
```

2. 编译cnpeak代码

```
./build.sh
```

3. 执行demo

yolov5, 正常启动后，可在浏览器输入\<ip>:8088,查看结果

```bash
# 下载yolov5模型
./app/app_yolov5/download_yolov5.sh
# 执行demo
./build/app/app_yolov5/app_yolov5
```

openpose, 正常启动后，可在浏览器输入\<ip>:8088,查看结果

```bash
# 下载openpose模型
./app/app_openpose/download_openpose.sh
# 执行demo
./build/app/app_openpose/app_openpose
```

ocr, 正常启动后，可在终端查看文字识别内容

```bash
# 下载dbnet文字检测模型，和crnn文子识别模型
./app/app_ocr/download_dbnet.sh
./app/app_ocr/download_crnn.sh
# 执行demo
./build/app/app_ocr/app_ocr
```

## 5 流程介绍

### 5.1 总体结构

cnpeak总体可分为三个部分：底层框架、插件库、上层应用。
上层应用（app），通过组装各种所需的插件，实现不同的业务功能，就像搭积木一样。

    +------------------------------------------------+
    |                  application                   |
    +------------------------------------------------+
    |                plugin-repository               |
    +------------------------------------------------+
    |                pipeline-framework              |
    +------------------------------------------------+

### 5.2 组件说明

核心框架是流水线的基础设施，职责是负责插件的生命周期管理、调度，以及插件之间的消息传递。另外负责一些公共的数据管理。
各个插件在流水线上传递的数据是TData结构体，通过阻塞队列实现的，阻塞队列的size默认为2。
做的慢的插件会导致上游队列阻塞，从而反压处在上游的插件的效率，以此类推，最终会反压到数据源头。
流水线的好处是可以提高系统整体的吞吐量（fps），但不能降低一个数据（TData）的处理时延。
整个流水线的吞吐量由耗时最长的那个插件来决定。如果想优化吞吐量，可以尝试把最慢的插件所作的工作（前提是任务是可拆解的）拆成多个插件并行来做。

插件是安装在流水线上的实例。每个插件继承于Plugin基类，实现包括构造函数在内的5个虚接口。
其中callback接口是最关键的处理逻辑的接口，参数有2个，分别是输入的TData和输出的TData，它的作用是完成该插件的业务逻辑，把加工过的数据放到输出的TData参数中。
另外有2个初始化虚接口，用来控制在不同的时机做对应的初始化操作（在主线程或子线程两个时机）。

应用是通过插件来组装成的一条流水线，一条流水线可认为是一路业务。可通过创建多条流水线实现多路的处理场景。
创建好的插件实例，要严格按照业务逻辑的顺序add到流水线上，具体可参考app目录下的代码。

下图是以yolov5业务流程示例表示整体业务在运行时刻的简易原理图：

       +------------------+      +------------------+      +------------------+      +------------------+
       |      plugin1     |      |      plugin2     |      |      plugin3     |      |      plugin4     |
       |                  |      |                  |      |                  |      |                  |
       | +--------------+ |      | +--------------+ |      | +--------------+ |      | +--------------+ |
       | | image + pre  | |      | |    infer     | |      | | yolov3-post  | |      | | video-player | |
       | +--------------+ |      | +--------------+ |      | +--------------+ |      | +--------------+ |
       |                  |      |                  |      |                  |      |                  |
       +---------+--------+      +---------+--------+      +--------+---------+      +---------+--------+
                 |                         |                        |                          |
                 v                         v                        v                          v
    +---------------------------------------------------------------------------------------------------------------+
    |       +--------+                +--------+                +--------+                 +--------+               >
    |       |  data3 |   +------>     |  data2 |     +------>   |  data1 |    +------>     |  data0 |     +------>  >
    |       +--------+                +--------+                +--------+                 +--------+               >
    +---------------------------------------------------------------------------------------------------------------+

### 5.3 目录结构

- 3rdparty：　　所有的第三方依赖所在目录，包括ffmpeg库等
- app：　　应用所在的目录，里面每一个应用单独一个文件夹
- core：　　底层framework目录，包含pipe管理、插件管理、以及消息队列管理代码
- plugin：　　插件所在目录，每一个插件单独一个文件夹
- data：　　存放视频和图片

### 5.4 支持的插件列表

- common_infer  # 通用推理模块
- plugin_id_gen # id生成
- plugin_image_save_opencv # opencv图片保存插件
- plugin_infer  # 推理插件
- plugin_infer_batch # 多batch推理
- plugin_infer_batch_parallel_mm # 多batch推理
- plugin_infer_copyout # 拷出
- plugin_infer_multi_copyout # 拷出
- plugin_infer_multi_data_parallel # 拷出
- plugin_post_crnn  # crnn后处理
- plugin_post_dbnet # dbnet后处理
- plugin_post_openpose # openpose后处理
- plugin_post_yolov5   # yolov5后处理
- plugin_pre_cncv_common # cncv预处理
- plugin_pre_crnn         # crnn预处理
- plugin_pre_dbnet        # dbnet预处理
- plugin_src_image_cncodec # 图片解码
- plugin_src_video_cncodec # 视频解码
- plugin_video_gen  # 视频生成
- plugin_video_http # 视频http传输
- plugin_video_play # 视频播放

## 6 二次开发FAQ

### 6.1 如何替换模型权值

以yolov5为例，当前工程中下载的是官方提供的权值。如果想更换自己训练的yolov5权值，可以这样做：
在执行完./app/app_yolov5/download_yolov5.sh之后，把models/yolov5_pytorch/yolov5m.pt替换成自己的权值，再执行一遍./app/app_yolov5/download_yolov5.sh脚本即可

### 6.2 如何更换插件

比如app_yolov5中，输入数据的插件是视频流（CncodecVideoProvider），想更换成图片流，可以改成CncodecImageProvider这个插件，然后按照此插件所需的参数设置，即可轻松更换数据源头

### 6.3 如何更换模型

比如如果想把yolov5升级成fasterrcnn的模型，需要新增加一个app。先确保fasterrcnn模型在寒武纪软件栈上能够正确运行（可参考modelzoo）。
然后在app文件中重新组织各个插件和参数，推理插件（Inferencer）的输入参数（模型路径）要换成fasterrcnn的路径，
前处理插件（CncvPreprocess）的参数需要按需修改，后处理插件（Yolov5PostProcesser）则需要全部换掉，新增一个fasterrcnn后处理的插件，可参考Yolov5PostProcesser插件的流程和接口形式。