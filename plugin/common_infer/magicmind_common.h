#ifndef COMMON_INFER_MAGICMIND_COMMON_H
#define COMMON_INFER_MAGICMIND_COMMON_H

#include <mm_parser.h>
#include <mm_builder.h>
#include <mm_runtime.h>

#include <cnrt.h>
// #include <cndrv_params.h>

#include "core/blockqueue.h"
#include "core/tools.hpp"
#include "cvImageCalibData.h"

// #define CNPEAK_CHECK_MM(status)                                  \
//   do {                                                    \
//     if ((status) != magicmind::Status::OK()) {            \
//       std::cout << "mm failure: " << status << std::endl; \
//       abort();                                            \
//     }                                                     \
//   } while (0)

#define CNPEAK_CHECK_MM(mmFunc, ...)                                                 \
  { magicmind::Status ret_code = (mmFunc);                                                   \
    if ( magicmind::Status::OK() != ret_code ) {                                              \
      std::cerr << "mm function " << #mmFunc << " call failed. code:"                 \
                << ret_code << ". (" << __FILE__ << ":"<< __LINE__ << ")." #__VA_ARGS__;  \
      exit(EXIT_FAILURE);                                                                 \
    }                                                                                     \
  }

typedef struct inferbatchmsg{
    void** msg_in;
    void** msg_out;
    int frame_id;
    BlockQueue<int> *qout;
}InferBatchMsg;

bool AppendYolov3Detection(magicmind::INetwork* network, const std::vector<int>& order);
bool AppendYolov5Detection(magicmind::INetwork* network, const float conf, const float iou, const int64_t max_det);
uint64_t GenBindBitmap(int dev_id, int thread_id, uint64_t bitmap);
void BindCluster(int dev_id, uint64_t bind_bitmap);

// void BindCluster(int dev_id, int thread_id) {
//   // Set device id and get max cluster num
//   CNPEAK_CHECK_CNRT(cnrtSetDevice(dev_id));
//   int cluster_num = 0;
//   CNPEAK_CHECK_CNRT(cnrtDeviceGetAttribute(&cluster_num, cnrtAttrClusterCount, dev_id));

//   CNctxConfigParam param;
//   param.unionLimit = CN_KERNEL_CLASS_UNION;

//   // Set ctx
//   CNcontext ctx;
//   assert(cnCtxGetCurrent(&ctx) == CNresult::CN_SUCCESS);
//   assert(cnSetCtxConfigParam(ctx, CN_CTX_CONFIG_UNION_LIMIT, &param) == CNresult::CN_SUCCESS);
//   param.visibleCluster = 0x01 << (thread_id % cluster_num);
//   assert(cnSetCtxConfigParam(ctx, CN_CTX_CONFIG_VISIBLE_CLUSTER, &param) == CNresult::CN_SUCCESS);
// }

const int batch_out_num = 20;

#endif