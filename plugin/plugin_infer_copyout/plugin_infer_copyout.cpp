//
// Created by cambricon on 19-1-5.
//

#include "plugin_infer_copyout.hpp"

InferMemCopyoutOperator::InferMemCopyoutOperator(int thread_id, int infer_id, int device_id, int cpu_mem_cap) {
    cpu_mem_cap_ = cpu_mem_cap;
    cpu_mem_idx_ = 0;
    thread_id_ = thread_id;
    infer_id_ = infer_id;
    device_id_ = device_id;
}

InferMemCopyoutOperator::~InferMemCopyoutOperator() {
    for (auto cpu_output : cpu_outputs_) {
        for (int i = 0; i < cur_offline_->outputNum; i++) {
            free(cpu_output[i]);
        }
        free(cpu_output);
    }
    cpu_outputs_.clear();
}

string InferMemCopyoutOperator::name(){
    return "InferMemCopyoutOperator";
}

bool InferMemCopyoutOperator::init_in_main_thread() {
    return true;
}

bool InferMemCopyoutOperator::init_in_sub_thread() {
    cur_offline_ = pCtxt->offline[infer_id_][0];
    init_device();
    malloc_cpu_output();
    return true;
}

bool InferMemCopyoutOperator::callback(TData *&pdata_in, vector<TData *>&pdatas_out) {
    if (pdata_in == nullptr) {
        pdatas_out.push_back(pdata_in);
        return true;
    }
    pdata_in->cpu_output = cpu_outputs_[cpu_mem_idx_];

    for (int i = 0; i < cur_offline_->outputNum; i++) {
        int size = cur_offline_->out_count[i] * cur_offline_->output_datatype[i];
        CNPEAK_CHECK_CNRT(cnrtMemcpy(pdata_in->cpu_output[i], pdata_in->mlu_output[i], size, CNRT_MEM_TRANS_DIR_DEV2HOST));
    }
    
    cpu_mem_idx_ = (cpu_mem_idx_ + 1) % cpu_mem_cap_;
    pdatas_out.push_back(pdata_in);
    return true;
}

// ==================================================
bool InferMemCopyoutOperator::init_device() {
    unsigned dev_num;
    cnrtGetDeviceCount(&dev_num);
    if (dev_num == 0) {
        std::cout << "no device found" << std::endl;
        exit(-1);
    }
    CNPEAK_CHECK_CNRT(cnrtSetDevice(device_id_));

    return true;
}

bool InferMemCopyoutOperator::malloc_cpu_output(){
    for(int k=0; k<cpu_mem_cap_; k++)
    {
        void** cpu_output = reinterpret_cast<void**>(malloc(sizeof(void*) * cur_offline_->outputNum));
        for (int i = 0; i < cur_offline_->outputNum; i++) {
            cpu_output[i] = reinterpret_cast<void *>(malloc(cur_offline_->out_count[i] * cur_offline_->output_datatype[i]));
        }
        cpu_outputs_.push_back(cpu_output);
    }

    return true;
}

