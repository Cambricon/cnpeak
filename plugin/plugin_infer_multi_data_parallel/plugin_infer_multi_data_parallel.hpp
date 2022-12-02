//
// Created by cambricon on 19-4-15.
//

#ifndef CNPIPE_PLUGIN_INFER_MULTI_DATA_PARALLEL_HPP
#define CNPIPE_PLUGIN_INFER_MULTI_DATA_PARALLEL_HPP

#include <memory>
#include "core/cnPipe.hpp"
#include "cnrt.h"  //NOLINT
#include "infer_multidata_worker.hpp"

#define MAX_BATCH 100

typedef struct workinfo{
    int id;
    BlockQueue<std::pair<void*, void**>*> *qin;
    BlockQueue<int> *qout;
    unique_ptr<MultiDataInferWorker> server;
}WorkInfo;

class InferencerMultiDataParallel: public Plugin{
public:
    InferencerMultiDataParallel(int server_num,
                                ModelType type,
                                string cali_file,
                                string cali_config_file,
                                string config_file,
                                string arg1,
                                string arg2,
                                string modelname,
                                string output_model_file,
                                bool fake = false,
                                int infer_id = 0, 
                                int device_id = 0,
                                vector<int> cluster_ids = {});
    ~InferencerMultiDataParallel();
    virtual bool init_in_main_thread();
    virtual bool init_in_sub_thread();
    virtual bool callback(TData *&pdata_in, vector<TData *>&pdatas_out);
    virtual string name();

private:
    // bool init_device();
    void malloc_mlu_output_cache();
    // bool parse_input_para();
    // bool parse_output_para();
    // bool malloc_cpu_output();
    // bool extend_output_cpu_mem(int mem_idx);
    bool inference_multi(TData*& pdata, int num, int buf_idx);
    void CreateWorker(int i);

    
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
    void parse_model_info();

private:
    // IBuilder* builder_;
    // INetwork* network_;
    // IBuilderConfig* config_;
    // IParser<ModelKind::kCaffe, std::string, std::string>* caffe_parser_;
    // IParser<ModelKind::kPytorch,std::string>* pytorch_parser_;
    // IParser<ModelKind::kOnnx,std::string>* onnx_parser_;
    // IParser<ModelKind::kTensorflow,std::string>* tf_parser_;

    // IModel* model_;

    // MultiDataInferWorker* server_;
    int server_num_;
    int infer_id_;
    // int device_id_;
    // int bind_bitmap_ = 0x00;
    // vector<int> cluster_ids_;

    vector<void**> mlu_input_cache_;
    vector<vector<void**>> mlu_output_cache_;
    
    int mlu_input_cap_;
    int mlu_output_cap_;
    int mlu_output_idx_;

    // string arg1_;
    // string arg2_;
    // string output_model_file_;
    // string cali_file_;
    // string cali_config_file_;
    // string config_file_;
    // string modelname_;
    // ModelType modeltype_;
    
    bool fake_mlu_input_;
    cnOfflineInfo *cur_offline_;

    // string offlinemodel_;
    string symbol_;
    MMInfer *mm_infer_;

    vector<vector<void**>> cpu_output_;
    vector<WorkInfo*> workers_;
    // int worker_num_;
    int output_idx_;
    int input_idx_;
};



#endif //CNPIPE_PLUGIN_INFER_MULTI_DATA_PARALLEL_HPP
