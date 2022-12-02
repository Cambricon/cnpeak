//
// Created by cambricon on 19-1-5.
//

#ifndef PLUGIN_INFER_MULTI_COPYOUT_HPP
#define PLUGIN_INFER_MULTI_COPYOUT_HPP


#include "core/cnPipe.hpp"
#include "core/tools.hpp"
#include "cnrt.h"  //NOLINT

#define MAX_BATCH 100

class InferMultiMemCopyoutOperator: public Plugin{
public:
    InferMultiMemCopyoutOperator(int thread_id, int infer_id=0, int device_id = 0, int cpu_mem_cap = 8);
    ~InferMultiMemCopyoutOperator();
    virtual bool init_in_main_thread();
    virtual bool init_in_sub_thread();
    virtual bool callback(TData *&pdata_in, vector<TData *>&pdatas_out);
    virtual string name();

private:
    bool init_device();
    bool malloc_cpu_output();

private:
    vector<vector<void**>> cpu_output_;
    int cpu_mem_cap_;
    int cpu_mem_idx_;

    int thread_id_;
    int infer_id_;
    int device_id_;
    cnOfflineInfo *cur_offline_;
    void** cpu_output_trans_;
};



#endif //PLUGIN_INFER_COPYOUT_HPP
