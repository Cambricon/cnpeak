//
// Created by cambricon on 18-12-13.
//

#ifndef PLUGIN_INFER_HPP
#define PLUGIN_INFER_HPP


#include "core/cnPipe.hpp"
#include "plugin/common_infer/magicmind_common.h"
#include "plugin/common_infer/magicmind_infer.h"

#include "mm_builder.h"
#include "mm_calibrator.h"
#include "mm_runtime.h"
#include "mm_parser.h"
#include "cnrt.h"  //NOLINT

using namespace magicmind;

#define RES_SIZE 1

class Inferencer: public Plugin{
public:
    Inferencer(ModelType type, 
               string cali_file, 
               string cali_config_file,
               string config_file,
               string arg1, 
               string arg2, 
               string output_model_file,
               string modelname, 
               bool fake = false,
               int infer_id = 0, 
               int device_id = 0,
               std::vector<int> cluster_ids = {});
    ~Inferencer();
    virtual bool init_in_main_thread();
    virtual bool init_in_sub_thread();
    virtual bool callback(TData *&pdata_in, vector<TData *>&pdatas_out);
    virtual string name();

private:
    bool start_infer(TData* pdata);
    void malloc_mlu_output_mem();
    void malloc_mlu_input_mem();
    void parse_model_info();

private:
    MMInfer *mm_infer_;

    int infer_id_;
    bool fake_mlu_input;
    cnOfflineInfo *cur_offline_;
    void** mlu_input_;
    vector<void**> mlu_output_;
    int mlu_mem_cap_;
    int mlu_mem_idx_;

    string arg1_;
    string arg2_;
    string output_model_file_;
    string cali_file_;
    string cali_config_file_;
    string config_file_;
    string modelname_;
    ModelType modeltype_;

    float Inferencer2_time_ = 0;
    cnrtNotifier_t noti_start,noti_end;
};

#endif //PLUGIN_INFER_HPP
