#ifndef CNPEAK_PLUGIN_PRE_DBNET_HPP
#define CNPEAK_PLUGIN_PRE_DBNET_HPP



#include "core/cnPipe.hpp"
#include "core/tools.hpp"
#include "cncv.h"
#include "cnrt.h"

#define ALIGN_UP(x, a) ((x + a - 1) & (~(a - 1)))

class PreDbnetProcesser: public Plugin{
public:    
    PreDbnetProcesser(int thread_id, int dst_w, int dst_h, int device_id=0, int infer_id=0, int cap=8);
    virtual bool init_in_main_thread();
    virtual bool init_in_sub_thread();
    virtual bool callback(TData *&pdata_in, vector<TData *>&pdatas_out);
    virtual string name();

private:
    bool malloc_mlu_buf();
    bool init_device();
    bool init_context();
    bool init_imageDesc(int src_width, int src_height, int dst_width, int dst_height);
    bool init_mlu_mem();
    bool init_workspace();
    bool resize_convert(void** src_y_mlu, void** src_uv_mlu, size_t src_w, size_t src_h, void** &output_ptr);
    bool mean_std_split(void** & output_ptr);    

private:
    cnOfflineInfo *cur_offline_;
    int device_id_;
    int thread_id_;
    int infer_id_;
    int mlu_mem_cap_;
    int mlu_mem_idx_;
    int batch_;

    //===================
    int src_h_;
    int src_w_;
    int dst_h_;
    int dst_w_;
    // cnrtDev_t dev_;
    cncvHandle_t cncv_handle_ = nullptr;
    cnrtQueue_t queue_ = nullptr;
    
    void *rc_workspace_ = nullptr;
    size_t rc_workspace_size_ = 0;
    
    size_t mean_std_workspace_size_ = 0;
    void *mean_std_workspace_ = nullptr;

    cncvImageDescriptor *src_desc_;
    cncvImageDescriptor *dst_desc_;
    cncvImageDescriptor *mean_std_desc_;
    cncvImageDescriptor *split_desc_;
    cncvRect *src_roi_;
    cncvRect *dst_roi_;
    cncvRect *mean_roi_;
    
    void** src_y_mlu_;
    void** src_uv_mlu_;
    void** rgb_mlu_;
    void **rgb_mlu_cpu_;
    void** mean_std_mlu_;
    void **mean_std_mlu_cpu_;
    // void** mean_std_mlu__;    
    // void **mean_std_mlu_cpu__;
    vector<void**> dst_split_mlu_;
    vector<void**> dst_split_cpu_;
    vector<void**> dst_rgb_mlu_;
    vector<void**> dst_rgb_cpu_;
    // vector<void**> dst_split_mlu__;
    // vector<void**> dst_split_cpu__;
};


#endif //CNPEAK_PLUGIN_PRE_DBNET_HPP
