//
// Created by cambricon on 19-9-24.
//

#ifndef CNPIPE_APP_OPENPOSE_HPP
#define CNPIPE_APP_OPENPOSE_HPP



#include "core/cnPipe.hpp"
#include "core/blockqueue.h"
#include "core/compare.hpp"

#include "plugin/plugin_src_video_cncodec/plugin_src_video_cncodec.hpp"
#include "plugin/plugin_src_image_cncodec/plugin_src_image_cncodec.hpp"
#include "plugin/plugin_pre_cncv_common/plugin_pre_cncv.hpp"
#include "plugin/plugin_infer/plugin_infer.hpp"
#include "plugin/plugin_infer_batch/plugin_infer_batch.hpp"
#include "plugin/plugin_infer_batch_parallel_mm/plugin_infer_batch_parallel.hpp"
#include "plugin/plugin_infer_copyout/plugin_infer_copyout.hpp"
#include "plugin/plugin_infer_multi_copyout/plugin_infer_multi_copyout.hpp"
#include "plugin/plugin_post_openpose/plugin_post_openpose.hpp"
#include "plugin/plugin_id_gen/plugin_id_gen.hpp"
#include "plugin/plugin_infer_multi_data_parallel/plugin_infer_multi_data_parallel.hpp"
#include "plugin/plugin_video_play/plugin_video_play.hpp"
#include "plugin/plugin_video_gen/plugin_video_gen.hpp"
#include "plugin/plugin_image_save_opencv/plugin_image_save_opencv.hpp"
#include "plugin/plugin_video_http/plugin_video_http.hpp"



#endif //CNPIPE_APP_CNFACE_HPP
