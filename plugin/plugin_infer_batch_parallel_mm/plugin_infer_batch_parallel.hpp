//
// Created by cambricon on 18-12-13.
//

#ifndef PLUGIN_INFER_BATCH_PARALLEL_HPP
#define PLUGIN_INFER_BATCH_PARALLEL_HPP


#include "core/cnPipe.hpp"
#include "core/tools.hpp"

#include "mm_builder.h"
#include "mm_calibrator.h"
#include "mm_parser.h"
#include "cnrt.h"
#include "infer_batch_server_parallel.hpp"

using namespace magicmind;

class InferencerBatchParallel: public Plugin{
public:
    InferencerBatchParallel(int server_num, 
                    ModelType type,
                    string cali_file,
                    string cali_config_file,
                    string config_file,
                    string arg1,
                    string arg2,
                    string modelname,
                    string output_model_file,
                    bool fake=false,
                    int infer_id=0, 
                    int device_id=0);

    virtual bool init_in_main_thread();
    virtual bool init_in_sub_thread();
    virtual bool callback(TData *&pdata_in, vector<TData *>&pdatas_out);
    virtual string name();

private:
    // bool init_device();
    void malloc_mlu_output_cache();
    void infer_cur_data(TData*& pdata);
    bool get_finished_data(vector<TData*>& pdatas);

    // void load_magicmind_model();
    // void init_magicmind_parser();
    // void init_magicmind_parser_caffe();
    // void init_magicmind_parser_pytorch();
    // void init_magicmind_offline();
    // void init_magicmind_calibration();
    // void init_magicmind_build();
    // void init_magicmind_engine();
    // void init_magicmind_tensor();
    // void init_magicmind_model();
    // void parse_model_info();

private:
    // IBuilder* builder_;
    // INetwork* network_;
    // IBuilderConfig* config_;
    // IParser<ModelKind::kCaffe, std::string, std::string>* caffe_parser_;
    // IParser<ModelKind::kPytorch,std::string>* pytorch_parser_;
    // IParser<ModelKind::kOnnx,std::string>* onnx_parser_;
    // IParser<ModelKind::kTensorflow,std::string>* tf_parser_;

    // IModel* model_;

    BlockQueue<int>* mail_box_;
    InferBatchParallelServer* server_;
    int server_num_;
    vector<void**> mlu_input_cache_;
    vector<void**> mlu_output_cache_;
    int mlu_input_cap_;
    int mlu_output_cap_;
    int mlu_output_idx_;

    vector<TData*> data_cache_;

    int infer_id_;
    // int device_id_;
    bool fake_mlu_input;
    // string symbol_;
    cnOfflineInfo *cur_offline_;

    // string arg1_;
    // string arg2_;
    // string output_model_file_;
    // string cali_file_;
    // string cali_config_file_;
    // string config_file_;
    // string modelname_;
    // ModelType modeltype_;

    string offlinemodel_;
    // cnrtQueue_t queue_;
    MMInfer *mm_infer_;
};

#endif //PLUGIN_INFER_BATCH_HPP
