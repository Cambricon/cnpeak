//
// Created by cambricon on 19-1-5.
//

#include "plugin_infer_multi_copyout.hpp"

InferMultiMemCopyoutOperator::InferMultiMemCopyoutOperator(int thread_id, int infer_id, int device_id, int cpu_mem_cap) {
    cpu_mem_cap_ = cpu_mem_cap;
    cpu_mem_idx_ = 0;
    thread_id_ = thread_id;
    infer_id_ = infer_id;
    device_id_ = device_id;
}

InferMultiMemCopyoutOperator::~InferMultiMemCopyoutOperator() {
    for (int i = 0; i < cpu_mem_cap_; i++) {
        for (int j = 0; j < MAX_BATCH; j++) {
            for (int o = 0; o < cur_offline_->outputNum; o++) {
                free(cpu_output_[i][j][o]);
            }
            cpu_output_[i][j] = nullptr;
        }
        cpu_output_[i].clear();
    }
    cpu_output_.clear();
}

string InferMultiMemCopyoutOperator::name() {
    return "InferMultiMemCopyoutOperator";
}

bool InferMultiMemCopyoutOperator::init_in_main_thread() {
    return true;
}

bool InferMultiMemCopyoutOperator::init_in_sub_thread() {
    cur_offline_ = pCtxt->offline[infer_id_][0];
    init_device();
    malloc_cpu_output();
    return true;
}

bool InferMultiMemCopyoutOperator::callback(TData *&pdata_in, vector<TData *>&pdatas_out) {
    if (pdata_in == nullptr) {
        pdatas_out.push_back(pdata_in);
        return true;
    }
    pdata_in->cpu_multi_outputs.clear();
    if (pdata_in->mlu_multi_outputs.empty()) {
        pdatas_out.push_back(pdata_in);
        return true;
    } else if (pdata_in->mlu_multi_outputs.size() > MAX_BATCH) {
        pdatas_out.push_back(pdata_in);
        return false;
    }

    cpu_mem_idx_ = (cpu_mem_idx_ + 1) % cpu_mem_cap_;
    // pdata_in->cpu_multi_outputs = cpu_output_[cpu_mem_idx_];

    for (size_t n = 0; n < pdata_in->mlu_multi_outputs.size(); n++) {
        void **output_ptr = cpu_output_[cpu_mem_idx_][n];
        for (int i = 0; i < cur_offline_->outputNum; i++) {
            int size = cur_offline_->out_count[i] * cur_offline_->output_datatype[i];
            CNPEAK_CHECK_CNRT(cnrtMemcpy(output_ptr[i], pdata_in->mlu_multi_outputs[n][i], size, CNRT_MEM_TRANS_DIR_DEV2HOST));
        }
        pdata_in->cpu_multi_outputs.push_back(output_ptr);
    }
    
    pdatas_out.push_back(pdata_in);
    return true;
}

// ==================================================
bool InferMultiMemCopyoutOperator::init_device() {
    unsigned dev_num;
    cnrtGetDeviceCount(&dev_num);
    if (dev_num == 0) {
        std::cout << "no device found" << std::endl;
        exit(-1);
    }
    CNPEAK_CHECK_CNRT(cnrtSetDevice(device_id_));

    return true;
}

bool InferMultiMemCopyoutOperator::malloc_cpu_output() {
    for (int i = 0; i < cpu_mem_cap_; i++) {
        vector<void**> cpu_outputs_tmp;
        for (int j = 0; j < MAX_BATCH; j++) {
            void** cpu_output = reinterpret_cast<void**>(malloc(sizeof(void*) * cur_offline_->outputNum));
            for (int o = 0; o < cur_offline_->outputNum; o++) {
                cpu_output[o] = reinterpret_cast<void *>(malloc(cur_offline_->out_count[o] * cur_offline_->output_datatype[o]));
            }
            cpu_outputs_tmp.push_back(cpu_output);
        }
        cpu_output_.push_back(cpu_outputs_tmp);
    }

    return true;
    
}

