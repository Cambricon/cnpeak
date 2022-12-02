#ifndef CNPEAK_PLUGIN_PRE_CRNN_HPP
#define CNPEAK_PLUGIN_PRE_CRNN_HPP

#include <cncv.h>
#include <cn_api.h>
#include <opencv2/opencv.hpp>

#include "core/cnPipe.hpp"
#include "core/tools.hpp"
#include "cncv.h"
#include "cnrt.h"

#define MAX_BOXES 100
#define ALIGN_UP(x, a) ((x + a - 1) & (~(a - 1)))

using namespace cv;

class PreCrnnProcesser: public Plugin{
public:    
    PreCrnnProcesser(int thread_id, int dst_w, int dst_h, int device_id=0, int infer_id=0, int cap=8);
    virtual bool init_in_main_thread();
    virtual bool init_in_sub_thread();
    virtual bool callback(TData *&pdata_in, vector<TData *>&pdatas_out);
    virtual string name();

private:
    // bool malloc_cpu();
    void malloc_mlu();

    // bool malloc_mlu_buf();
    bool init_device();
    bool init_context();
    bool init_imageDesc(int src_width, int src_height, int dst_width, int dst_height);
    bool init_mlu_mem();
    bool init_workspace();
    bool resize_convert(void** src_y_mlu, void** src_uv_mlu, size_t src_w, size_t src_h);
    bool mean_std_split(void** & output_ptr);
    bool preProcess(vector<vector<Point2f>> &boxes, void* mlu_input, size_t src_w, size_t src_h, vector<void*> &output_ptr);
    // bool preProcess_(vector<cv::Mat> &box_imgs, vector<void*> &output_ptr);
    
private:
    cnOfflineInfo *cur_offline_;
    int device_id_;
    int thread_id_;
    int infer_id_;
    int mlu_mem_cap_;
    int mlu_mem_idx_;
    int batch_;
    int channel_num = 3;

    //===================
    vector<cncvPoint> dst_pts;
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

    vector<void*> dst_warp_data_;
    vector<void**> dst_warp_mlu_;
    vector<void**> dst_warp_cpu_;
    vector<void**> gray_warp_mlu_;
    vector<void**> gray_warp_cpu_;

    cncvImageDescriptor *src_desc_;
    cncvImageDescriptor *dst_desc_;
    cncvImageDescriptor *mean_std_desc_;
    cncvImageDescriptor *split_desc_;
    cncvImageDescriptor *gray_desc_;
    cncvRect *src_roi_;
    cncvRect *dst_roi_;
    cncvRect *mean_roi_;
    cncvRect *gray_roi_;
    
    void** src_y_mlu_;
    void** src_uv_mlu_;
    void** rgb_mlu_;
    void **rgb_mlu_cpu_;

    void** mean_std_mlu_;
    void **mean_std_mlu_cpu_;
    vector<void**> dst_split_mlu_;
    vector<void**> dst_split_cpu_;

    void **src_mlu_;
    void **src_mlu_cpu_;
    void **mat_mlu_;
    void **mat_mlu_cpu_;
};

#endif