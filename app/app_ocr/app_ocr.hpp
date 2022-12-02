#ifndef CNPEAK_APP_DBNET
#define CNPEAK_APP_DBNET


#include <glog/logging.h>

#include "core/cnPipe.hpp"
#include "core/blockqueue.h"
#include "core/compare.hpp"

#include "plugin/plugin_src_video_cncodec/plugin_src_video_cncodec.hpp"
#include "plugin/plugin_src_image_cncodec/plugin_src_image_cncodec.hpp"
#include "plugin/plugin_id_gen/plugin_id_gen.hpp"
#include "plugin/plugin_pre_dbnet/plugin_pre_dbnet.hpp"
#include "plugin/plugin_infer/plugin_infer.hpp"
#include "plugin/plugin_infer_copyout/plugin_infer_copyout.hpp"
#include "plugin/plugin_post_dbnet/plugin_post_dbnet.hpp"
// #include "plugin/plugin_video_play/plugin_video_play.hpp"
#include "plugin/plugin_image_save_opencv/plugin_image_save_opencv.hpp"
#include "plugin/plugin_video_http/plugin_video_http.hpp"
// #include "plugin/plugin_video_sdl/plugin_video_sdl.hpp"

#include "plugin/plugin_pre_crnn/plugin_pre_crnn.hpp"
#include "plugin/plugin_post_crnn/plugin_post_crnn.hpp"

// #include "plugin/plugin_infer_parallel/plugin_infer_parallel.hpp"
// #include "plugin/plugin_infer_batch/plugin_infer_batch.hpp"
// #include "plugin/plugin_infer_batch_parallel_mm/plugin_infer_batch_parallel.hpp"
//#include "plugin/plugin_infer_copyin/plugin_infer_copyin.hpp"
#include "plugin/plugin_infer_multi_copyout/plugin_infer_multi_copyout.hpp"
#include "plugin/plugin_infer_multi_data_parallel/plugin_infer_multi_data_parallel.hpp"




#endif //CNPEAK_APP_DBNET
