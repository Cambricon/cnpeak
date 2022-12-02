//
// Created by cambricon on 19-10-23.
//

#ifndef CNPIPE_INFER_MULTI_DATA_WORKER_HPP
#define CNPIPE_INFER_MULTI_DATA_WORKER_HPP

#include <vector>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <cnrt.h>

#include "core/blockqueue.h"
#include "core/cnPublicContext.h"
#include "plugin/common_infer/magicmind_common.h"
#include "plugin/common_infer/magicmind_infer.h"

using namespace magicmind;
using namespace std;

const int out_num = 20;

class MultiDataInferWorker {
public:
    MultiDataInferWorker(MMInfer* model,
                         cnOfflineInfo* offlineInfo,
                         BlockQueue<std::pair<void*, void**> *>* in,
                         BlockQueue<int>* out);
    ~MultiDataInferWorker() {};

private:
    void start_work();
    void init_worker();
    bool forward(void* in, void** out);
    // bool init_device();
    // void parse_input_args();
    // void parse_output_args();
    bool start_infer();

private:
    BlockQueue<std::pair<void*, void**>*>* queue_in_;
    BlockQueue<int>* queue_out_;
    const char* model_file_;
    bool is_stop_;

private:
    // uint64_t bind_bitmap_;
    // int device_id_;

    // IModel* model_;
    // IEngine* engine_;
    // cnrtQueue_t queue_;
    // IContext* context_;

    // vector<IRTTensor *> inputs_;
    // vector<IRTTensor *> outputs_;
    MMInfer *model_;

    cnOfflineInfo *cur_offline_;
};



#endif //CNPIPE_INFER_MULTI_DATA_WORKER_HPP
