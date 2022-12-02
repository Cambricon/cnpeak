//
// Created by jiapeiyuan on 4/24/20.
//

#ifndef CNPIPE2_PLUGIN_PRE_CNCV_HPP
#define CNPIPE2_PLUGIN_PRE_CNCV_HPP


#include "core/cnPipe.hpp"
#include "core/tools.hpp"
#include "cncv.h"
#include "cnrt.h"
#include "cn_api.h"

#define ALIGN_UP(x, a) ((x + a - 1) & (~(a - 1)))

class CncvPreprocess: public Plugin{
public:
    CncvPreprocess(std::vector<int> resize_size,
           bool keep_aspect_ratio = false,
           bool need_bgr2rgb = false,
           bool need_tranpose = false,
           std::vector<float> mean = {},
           std::vector<float> std = {},
           int align_stride = 1,
           int device_id = 0,
           int infer_id = 0,
           int cap = 8,
           vector<int> cluster_ids = {});
    virtual bool init_in_main_thread();
    virtual bool init_in_sub_thread();
    virtual bool callback(TData *&pdata_in, vector<TData *>&pdatas_out);
    virtual string name();

private:
    bool malloc_mlu_buf();
    bool init_device();
    bool init_context();
    bool init_imageDesc();
    bool init_mlu_mem();
    // bool init_workspace();
    bool resize_convert(void** src_y_mlu, void** src_uv_mlu, int src_w, int src_h, void** & output_ptr);
    bool mean_std(void** & output_ptr);
    bool split(void** &output_ptr);
    void BindCluster(int dev_id, uint64_t bitmap);
    uint64_t GenBindBitmap(int dev_id, int thread_id, uint64_t bitmap);

private:
    cnOfflineInfo *cur_offline_;
    int device_id_;
    int infer_id_;
    int mlu_mem_cap_;
    int mlu_mem_idx_;
    int batch_;

    //===================
    // int src_h_;
    // int src_w_;
    // int dst_h_;
    // int dst_w_;
    // cnrtDev_t dev_;
    cncvHandle_t cncv_handle_ = nullptr;
    cnrtQueue_t queue_ = nullptr;

    std::vector<int> resize_size_;
    std::vector<float> mean_;
    std::vector<float> std_;

    uint64_t bind_bitmap_ = 0x00;
    int align_stride_;

    bool keep_aspect_ratio_;
    bool need_bgr2rgb_;
    bool need_tranpose_;
    bool need_meanstd_;
    
    void *resizeconvert_workspace_          = nullptr;
    size_t resizeconvert_workspace_size_    = 0;
    
    size_t meanstd_workspace_size_          = 0;
    void *meanstd_workspace_                = nullptr;

    cncvImageDescriptor *resizeconvert_src_desc_;
    cncvImageDescriptor *resizeconvert_dst_desc_;
    cncvImageDescriptor *mean_std_desc_;
    cncvImageDescriptor *split_desc_;
    cncvRect *src_roi_;
    cncvRect *dst_roi_;
    cncvRect *mean_roi_;
    
    void** src_mlu_;
    void** src_mlu_cpu_;

    vector<void**> dst_resizeconvert_mlu_;
    vector<void**> dst_resizeconvert_cpu_;
    vector<void**> dst_meanstd_mlu_;
    vector<void**> dst_meanstd_cpu_;
    vector<void**> dst_split_mlu_;
    vector<void**> dst_split_cpu_;
    vector<void*>  dst_split_mlu_data_;
};


#endif //CNPIPE2_PLUGIN_PRE_CNCV_RC_HPP
