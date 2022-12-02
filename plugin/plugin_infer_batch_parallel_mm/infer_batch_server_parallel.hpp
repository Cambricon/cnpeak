//
// Created by cambricon on 19-11-28.
//

#ifndef CNPIPE_INFER_BATCH_SERVER_PARALLEL_HPP
#define CNPIPE_INFER_BATCH_SERVER_PARALLEL_HPP

#include "core/cnPipe.hpp"
#include "plugin/common_infer/magicmind_common.h"
#include "plugin/common_infer/magicmind_infer.h"

#include "mm_runtime.h"
#include "cnrt.h"  //NOLINT

using namespace magicmind;


class InferBatchParallelServer {
public:
    static InferBatchParallelServer* create_infer_batch_server(int server_num,
                                                    int thread_id, MMInfer* model, cnOfflineInfo* offineInfo);
    InferBatchParallelServer(int server_id, MMInfer* model, cnOfflineInfo* offineInfo);
    ~InferBatchParallelServer();

    BlockQueue<InferBatchMsg*>* get_server_mailbox();

private:
    void start_work();
    void init_server();
    // bool init_device();
    // bool init_ctxt();
    // void parse_input_args();
    // void parse_output_args();
    void malloc_mlu_output_cache();
    void malloc_mlu_input_cache();
    bool init_function();
    bool start_infer();
    bool batch_collect(InferBatchMsg* msg);
    void send_result_back();
    void batch_d2d_copy();
    // void parse_model_info();

private:
    static vector<InferBatchParallelServer*> insts_;
    int server_id_;
    int batch_size_;
    BlockQueue<InferBatchMsg*>* mail_box_;
    vector<InferBatchMsg*> msg_buffer_;
    int msg_idx_;

    void** mlu_input_cache_;
    void** mlu_output_cache_;

    vector<void**> mlu_output_;
    void** mlu_input_;
    bool fake_mlu_input;

    cnOfflineInfo *cur_offline_;

    string model_file_;
    bool is_stop_;

    bool is_yuv;
    int device_id_;
    // cnrtRuntimeContext_t rt_ctx_;
    // cnrtQueue_t queue_;

    // cnrtModel_t model;
    // cnrtFunction_t function;
    // cnrtFunction_t function2;

    // int64_t* inputSizeArray_;
    // int64_t* outputSizeArray_;

    int* input_dims[4];
    int input_dims_num[4];
    int* output_dims[batch_out_num];
    int output_dims_num[batch_out_num];
    cnrtNotifier_t noti_start,noti_end;

    int inputNum, outputNum; //outputnum is output dim
    unsigned int input_n, input_c, input_h, input_w;
    unsigned int output_n[batch_out_num], output_c[batch_out_num], output_h[batch_out_num], output_w[batch_out_num];
    int out_count[batch_out_num]; // equal n*c*h*w
    float inferencer_time;

private:
    MMInfer* model_;
    // IEngine* engine_;
    // cnrtQueue_t queue_;
    // IContext* context_;

    // vector<IRTTensor *> inputs_;
    // vector<IRTTensor *> outputs_;

    int mlu_mem_cap_;
    int mlu_mem_idx_;
};

#endif //CNPIPE_INFER_BATCH_SERVER_HPP
