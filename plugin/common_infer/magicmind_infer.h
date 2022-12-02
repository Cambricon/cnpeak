#ifndef COMMON_INFER_MAGICMIND_INFER_H
#define COMMON_INFER_MAGICMIND_INFER_H

#include "plugin/common_infer/magicmind_common.h"

#include "mm_builder.h"
#include "mm_calibrator.h"
#include "mm_runtime.h"
#include "mm_parser.h"
#include "cnrt.h"  //NOLINT

using namespace magicmind;
using namespace std;

typedef enum {
    mCaffe = 0,
    mPytorch,
    mTensorflow,
    mOnnx,
    mOffline,
}ModelType;

class MMInfer
{
private:
    IBuilder* builder_;
    INetwork* network_;
    IBuilderConfig* config_;
    IParser<ModelKind::kCaffe, std::string, std::string>* caffe_parser_;
    IParser<ModelKind::kPytorch,std::string>* pytorch_parser_;
    IParser<ModelKind::kOnnx,std::string>* onnx_parser_;
    IParser<ModelKind::kTensorflow,std::string>* tf_parser_;

    IModel* model_      = nullptr;
    IEngine* engine_    = nullptr;
    IContext* context_  = nullptr;
    cnrtQueue_t queue_;

    vector<IRTTensor *> inputs_;
    vector<IRTTensor *> outputs_;

    string arg1_;
    string arg2_;
    string output_model_file_;
    string cali_file_;
    string cali_config_file_;
    string config_file_;
    string modelname_;
    ModelType modeltype_;
    uint64_t bind_bitmap_ = 0x00;
    int device_id_;

private:
    void load_magicmind_model();
    void init_magicmind_parser();
    void init_magicmind_parser_caffe();
    void init_magicmind_parser_pytorch();
    void init_magicmind_offline();
    void init_magicmind_calibration();
    void init_magicmind_build();
    void init_magicmind_engine();
    void init_magicmind_tensor();
    void init_magicmind_model();
    void parse_model_info();

public:
    MMInfer() = delete;
    MMInfer(ModelType type,
            string cali_file,
            string cali_config_file,
            string config_file,
            string arg1, 
            string arg2,
            string output_model_file,
            string modelname,
            int device_id = 0,
            vector<int> cluster_ids = {});
    ~MMInfer();
    int get_inputs_num() const { return inputs_.size(); }
    int get_outputs_num() const { return outputs_.size(); }
    vector<int64_t> get_input_dim(int idx) const { 
        CNPEAK_CHECK_PARAM(idx < get_inputs_num())
        return inputs_[idx]->GetDimensions().GetDims();
    }
    vector<int64_t> get_output_dim(int idx) const { 
        CNPEAK_CHECK_PARAM(idx < get_outputs_num())
        return outputs_[idx]->GetDimensions().GetDims();
    }
    int64_t get_input_count(int idx) const {
        CNPEAK_CHECK_PARAM(idx < get_inputs_num())
        return inputs_[idx]->GetDimensions().GetElementCount();
    }
    int64_t get_output_count(int idx) const {
        CNPEAK_CHECK_PARAM(idx < get_outputs_num())
        return outputs_[idx]->GetDimensions().GetElementCount();
    }
    int64_t get_input_size(int idx) const {
        CNPEAK_CHECK_PARAM(idx < get_inputs_num())
        return inputs_[idx]->GetSize();
    }
    int64_t get_output_size(int idx) const {
        CNPEAK_CHECK_PARAM(idx < get_outputs_num())
        return outputs_[idx]->GetSize();
    }
    int get_input_datatypesize(int idx) const {
        CNPEAK_CHECK_PARAM(idx < get_outputs_num())
        return DataTypeSize(inputs_[idx]->GetDataType());
    }
    int get_output_datatypesize(int idx) const {
        CNPEAK_CHECK_PARAM(idx < get_outputs_num())
        return DataTypeSize(outputs_[idx]->GetDataType());
    }
    void set_input_data(void* ptr, int idx = 0);
    void set_output_data(void* ptr, int idx = 0);
    void infer();
};

#endif