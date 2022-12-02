//
// Created by jiapeiyuan on 12/27/19.
//

#ifndef CNPIPE_APP_YOLOV5
#define CNPIPE_APP_YOLOV5


#include "core/cnPipe.hpp"
#include <glog/logging.h>

#include "plugin/plugin_src_video_cncodec/plugin_src_video_cncodec.hpp"
#include "plugin/plugin_src_image_cncodec/plugin_src_image_cncodec.hpp"
#include "plugin/plugin_id_gen/plugin_id_gen.hpp"
#include "plugin/plugin_pre_cncv_common/plugin_pre_cncv.hpp"
#include "plugin/plugin_infer/plugin_infer.hpp"
#include "plugin/plugin_infer_copyout/plugin_infer_copyout.hpp"
#include "plugin/plugin_post_yolov5/plugin_post_yolov5.hpp"
#include "plugin/plugin_image_save_opencv/plugin_image_save_opencv.hpp"
#include "plugin/plugin_video_http/plugin_video_http.hpp"


#include "core/blockqueue.h"
#include "core/compare.hpp"


#endif //CNPIPE_APP_YOLOV3
