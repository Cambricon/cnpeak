//
// Created by cambricon on 18-12-13.
//
#include "plugin_infer.hpp"
#include "core/tools.hpp"

Inferencer::Inferencer(ModelType type, 
                       string cali_file, 
                       string cali_config_file,
                       string config_file,
                       string arg1, 
                       string arg2, 
                       string modelname, 
                       string output_model_file,
                       bool fake, 
                       int infer_id, 
                       int device_id,
                       vector<int> cluster_ids) {
    mlu_mem_idx_    = 0;
    mlu_mem_cap_    = 16;
    infer_id_       = infer_id;
    fake_mlu_input  = fake;

    mm_infer_ = new MMInfer(type,
                            cali_file, 
                            cali_config_file, 
                            config_file, 
                            arg1, 
                            arg2, 
                            output_model_file, 
                            modelname, 
                            device_id, 
                            cluster_ids);
}

Inferencer::~Inferencer() {

    for (int i = 0; i < mlu_mem_cap_; i++) {
        int num = mm_infer_->get_inputs_num();
        for(int j=0; j<num; j++){
            CNPEAK_CHECK_CNRT(cnrtFree(mlu_output_[i][j]));
        }
        free(mlu_output_[i]);
    }
    mlu_output_.clear();

    if (fake_mlu_input) {
        for(size_t i = 0; i < mm_infer_->get_outputs_num(); i++) {
            CNPEAK_CHECK_CNRT(cnrtFree(mlu_input_[i]));
        }
        free(mlu_input_);
    }
    delete mm_infer_;

}

string Inferencer::name() {
    return "Inferencer";
}

bool Inferencer::init_in_main_thread() {
    cur_offline_ = new cnOfflineInfo();
    vector<cnOfflineInfo *> v;
    v.push_back(cur_offline_);

    pCtxt->offline.push_back(v);
 
    malloc_mlu_output_mem();
    if(fake_mlu_input){
        malloc_mlu_input_mem();
    }
    parse_model_info();
    
    return true;
}

bool Inferencer::init_in_sub_thread() {
    return true;
}

bool Inferencer ::callback(TData *&pdata_in, vector<TData *>&pdatas_out) {
    if (pdata_in == nullptr) {
        pdatas_out.push_back(pdata_in);
        return true;
    }
    start_infer(pdata_in);
    pdatas_out.push_back(pdata_in);
    return true;
}

bool Inferencer::start_infer(TData* pdata_in) {
    for(size_t i = 0; i < mm_infer_->get_inputs_num(); i++){
        mm_infer_->set_input_data(pdata_in->mlu_input[i], i);
    }


    void** output = mlu_output_[mlu_mem_idx_];
    mlu_mem_idx_ = (mlu_mem_idx_ + 1) % mlu_mem_cap_;

    for(size_t i = 0; i < mm_infer_->get_outputs_num(); i++){
        mm_infer_->set_output_data(output[i], i);
    }

    mm_infer_->infer();
    pdata_in->mlu_output = output;
    
    return true;
}

void Inferencer::malloc_mlu_output_mem() {
    for (int i = 0; i < mlu_mem_cap_; i++) {
        int num = mm_infer_->get_outputs_num();
        void** mlu = (void**)(malloc(sizeof(void*)*num));
        for(int j=0; j<num; j++){
            int s = mm_infer_->get_output_size(j);
            CNPEAK_CHECK_CNRT(cnrtMalloc(&mlu[j], s));
        }
        mlu_output_.push_back(mlu);
    }
}

void Inferencer::malloc_mlu_input_mem() {
    mlu_input_ = reinterpret_cast<void**>(malloc(sizeof(void*) * cur_offline_->inputNum));
    for(size_t i = 0; i < mm_infer_->get_inputs_num(); i++) {
        int size = mm_infer_->get_input_size(i);
        CNPEAK_CHECK_CNRT(cnrtMalloc(&mlu_input_[i], size));
    }
}

void Inferencer::parse_model_info(){
    cur_offline_->inputNum = mm_infer_->get_inputs_num();
    cur_offline_->outputNum = mm_infer_->get_outputs_num();
    for(size_t i=0; i< cur_offline_->inputNum; i++){
        vector<int64_t> d = mm_infer_->get_input_dim(i);
        cur_offline_->input_n[i] = (unsigned int)d[0];
        cur_offline_->input_c[i] = (unsigned int)d[1];
        cur_offline_->input_h[i] = (unsigned int)d[2];
        cur_offline_->input_w[i] = (unsigned int)d[3];
        cur_offline_->in_count[i] = mm_infer_->get_input_count(i);
        cur_offline_->input_datatype[i] = mm_infer_->get_input_datatypesize(i);
    }
    for(size_t i=0; i<cur_offline_->outputNum; i++){
        vector<int64_t> d = mm_infer_->get_output_dim(i);
        int n = (unsigned int)d[0];
        int c = d.size() > 1 ? (unsigned int)d[1] : 1;
        int h = d.size() > 2 ? (unsigned int)d[2] : 1;
        int w = d.size() > 3 ? (unsigned int)d[3] : 1;
        cur_offline_->output_n[i] = n;
        cur_offline_->output_c[i] = c;
        cur_offline_->output_h[i] = h;
        cur_offline_->output_w[i] = w;
        cur_offline_->out_count[i] = mm_infer_->get_output_count(i);
        cur_offline_->output_datatype[i] = mm_infer_->get_output_datatypesize(i);
    }
}