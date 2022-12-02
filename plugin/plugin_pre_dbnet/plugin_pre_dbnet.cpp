#include "plugin_pre_dbnet.hpp"

#define DEBUG 0

#if DEBUG//debug
#include <opencv2/opencv.hpp>
#endif

PreDbnetProcesser::PreDbnetProcesser(int thread_id, int dst_w, int dst_h, int device_id, int infer_id, int cap)
{
    infer_id_ = infer_id;
    mlu_mem_cap_ = cap;
    mlu_mem_idx_ = 0;
    batch_ = 1;
    device_id_ = device_id;
    thread_id_ = thread_id;
    // src_w_ = src_w;
    // src_h_ = src_h;
    dst_w_ = dst_w;
    dst_h_ = dst_h;
}


string PreDbnetProcesser::name()
{
    return "PreDbnetProcesser";
}

bool PreDbnetProcesser::init_in_main_thread()
{
    return true;
}

bool PreDbnetProcesser::init_in_sub_thread()
{
    init_device();
    init_context();
    init_imageDesc(src_w_, src_h_, dst_w_, dst_h_);
    init_mlu_mem();
    init_workspace();
    // cur_offline_ = pCtxt->offline[infer_id_][0];
    return true;
}

bool PreDbnetProcesser::callback(TData *&pdata_in, vector<TData *>&pdatas_out)
{    
    if (pdata_in == nullptr)
    {
        pdatas_out.push_back(pdata_in);
        return true;
    }
#if DEBUG//debug
    std::cout << "Debug : ====================================" << std::endl;
    cout << "pdata_in->s_w=" << pdata_in->s_w << endl;
    cout << "pdata_in->s_h=" << pdata_in->s_h << endl;
#endif
    resize_convert(&pdata_in->mlu_input[0], &pdata_in->mlu_input[1], pdata_in->s_w, pdata_in->s_h, pdata_in->mlu_origin_image);
    mean_std_split(pdata_in->mlu_input);   
    mlu_mem_idx_ = (mlu_mem_idx_ + 1) % mlu_mem_cap_; 
    // mean_std_split_(pdata_in->mlu_origin_image);
    pdatas_out.push_back(pdata_in);
    return true;
}

// ================================= below is private function =========================
bool PreDbnetProcesser::init_device()
{
    // cnrtInit(0);
    // cnrtDev_t dev;
    // cnrtGetDeviceHandle(&dev, device_id_);
    // cnrtSetCurrentDevice(dev);
    // cnrtSetCurrentChannel((cnrtChannelType_t)(thread_id_ % 4));
    CNPEAK_CHECK_CNRT(cnrtSetDevice(device_id_));
    return true;
}

bool PreDbnetProcesser::init_context()
{
    CNPEAK_CHECK_CNRT(cnrtQueueCreate(&queue_));
    CNPEAK_CHECK_CNCV(cncvCreate(&cncv_handle_));
    CNPEAK_CHECK_CNCV(cncvSetQueue(cncv_handle_, queue_));
    return true;
}

bool PreDbnetProcesser::init_imageDesc(int src_width, int src_height, int dst_width, int dst_height)
{
    // cncvDepth_t dst_depth = CNCV_DEPTH_8U;
    // cncvPixelFormat dst_fmt = CNCV_PIX_FMT_RGB;
    src_desc_ = new cncvImageDescriptor();
    src_desc_->width = src_width;
    src_desc_->height = src_height;
    src_desc_->pixel_fmt = CNCV_PIX_FMT_NV12;
    src_desc_->stride[0] =  src_width;
    src_desc_->stride[1] = src_width;
    src_desc_->depth = CNCV_DEPTH_8U;
    src_desc_->color_space = CNCV_COLOR_SPACE_BT_601;

    dst_desc_ = new cncvImageDescriptor();
    dst_desc_->width = dst_width;
    dst_desc_->height = dst_height;
    dst_desc_->pixel_fmt = CNCV_PIX_FMT_BGR;
    dst_desc_->stride[0] = dst_width * 3 * sizeof(char);
    dst_desc_->depth = CNCV_DEPTH_8U;
    dst_desc_->color_space = CNCV_COLOR_SPACE_BT_601;

    mean_std_desc_ = new cncvImageDescriptor();
    mean_std_desc_->width = dst_width;
    mean_std_desc_->height = dst_height;
    mean_std_desc_->pixel_fmt = CNCV_PIX_FMT_BGR;
    mean_std_desc_->stride[0] = dst_width * 3 * sizeof(float);
    mean_std_desc_->depth = CNCV_DEPTH_32F;
    mean_std_desc_->color_space = CNCV_COLOR_SPACE_BT_601;

    split_desc_ = new cncvImageDescriptor();
    split_desc_->width = dst_width;
    split_desc_->height = dst_height;
    split_desc_->pixel_fmt = CNCV_PIX_FMT_GRAY;
    split_desc_->stride[0] = dst_width * sizeof(float);
    split_desc_->stride[1] = dst_width * sizeof(float);
    split_desc_->stride[2] = dst_width * sizeof(float);
    split_desc_->depth = CNCV_DEPTH_32F;
    split_desc_->color_space = CNCV_COLOR_SPACE_BT_601;

    src_roi_ = new cncvRect();
    src_roi_->x = 0;
    src_roi_->y = 0;
    src_roi_->w = src_width;
    src_roi_->h = src_height;

    dst_roi_ = new cncvRect();
    dst_roi_->x = 0;
    dst_roi_->w = dst_width;
    dst_roi_->h = dst_height;
    dst_roi_->y = 0;
    
    mean_roi_ = new cncvRect();
    mean_roi_->x = 0;
    mean_roi_->y = 0;
    mean_roi_->w = dst_width;
    mean_roi_->h = dst_height;
    return true;
}

bool PreDbnetProcesser::init_mlu_mem()
{
    CNPEAK_CHECK_CNRT(cnrtMalloc((void **)(&src_y_mlu_), sizeof(void*)));
    CNPEAK_CHECK_CNRT(cnrtMalloc((void **)(&src_uv_mlu_), sizeof(void*)));
    
    CNPEAK_CHECK_CNRT(cnrtMalloc((void **)(&mean_std_mlu_), sizeof(void*)));  

    for(int i=0; i<mlu_mem_cap_; i++)
    {  
        CNPEAK_CHECK_CNRT(cnrtMalloc((void **)(&rgb_mlu_), sizeof(void*)));
        rgb_mlu_cpu_ = (void **)malloc(sizeof(void*));
        mean_std_mlu_cpu_ = (void **)malloc(sizeof(void*));    
        CNPEAK_CHECK_CNRT(cnrtMalloc(&(rgb_mlu_cpu_[0]), dst_w_*dst_h_*3*sizeof(char)));
        CNPEAK_CHECK_CNRT(cnrtMemset(rgb_mlu_cpu_[0], 0, dst_w_*dst_h_*3*sizeof(char)));
        CNPEAK_CHECK_CNRT(cnrtMalloc(&(mean_std_mlu_cpu_[0]), dst_w_*dst_h_*3*sizeof(float)));    

        CNPEAK_CHECK_CNRT(cnrtMemcpy(rgb_mlu_, rgb_mlu_cpu_, sizeof(void*), CNRT_MEM_TRANS_DIR_HOST2DEV));
        CNPEAK_CHECK_CNRT(cnrtMemcpy(mean_std_mlu_, mean_std_mlu_cpu_, sizeof(void*), CNRT_MEM_TRANS_DIR_HOST2DEV));

        dst_rgb_mlu_.push_back(rgb_mlu_);
        dst_rgb_cpu_.push_back(rgb_mlu_cpu_);

    
        void** split_mlu;
        CNPEAK_CHECK_CNRT(cnrtMalloc((void **)(&split_mlu), 3*sizeof(void *)));
        
        void* split_mlu_data;
        CNPEAK_CHECK_CNRT(cnrtMalloc((&split_mlu_data), dst_w_*dst_h_*3*sizeof(float)));
        
        void** split_mlu_cpu = (void**)malloc(3*sizeof(void *));
        int size = dst_w_*dst_h_*sizeof(float);
        split_mlu_cpu[0] = split_mlu_data;
        split_mlu_cpu[1] = (char*)split_mlu_data + size;
        split_mlu_cpu[2] = (char*)split_mlu_data + size*2;
        CNPEAK_CHECK_CNRT(cnrtMemcpy(split_mlu, split_mlu_cpu, 3*sizeof(void*), CNRT_MEM_TRANS_DIR_HOST2DEV));

        dst_split_mlu_.push_back(split_mlu);
        dst_split_cpu_.push_back(split_mlu_cpu);
    }
    return true;
}

bool PreDbnetProcesser::init_workspace()
{
    // cncvGetResizeConvertWorkspaceSize(1,
    //                                 src_desc_,
    //                                 src_roi_,
    //                                 dst_desc_,
    //                                 dst_roi_,
    //                                 &rc_workspace_size_);
    // cnrtMalloc(&rc_workspace_, rc_workspace_size_);
    
    CNPEAK_CHECK_CNCV(cncvGetMeanStdWorkspaceSize(3, &mean_std_workspace_size_));
    CNPEAK_CHECK_CNRT(cnrtMalloc(&mean_std_workspace_, mean_std_workspace_size_));
    return true;
}
bool PreDbnetProcesser::resize_convert(void** src_y_mlu, void** src_uv_mlu, size_t src_w, size_t src_h, void** &output_ptr)
{    
    src_desc_->width = src_w;
    src_desc_->height = src_h;    
    src_desc_->stride[0] = ALIGN_UP(src_w, 64);
    src_desc_->stride[1] = ALIGN_UP(src_w, 64);    
    src_roi_->w = src_w;
    src_roi_->h = src_h;
    CNPEAK_CHECK_CNRT(cnrtMemcpy(src_y_mlu_, src_y_mlu, sizeof(void *), CNRT_MEM_TRANS_DIR_HOST2DEV));
    CNPEAK_CHECK_CNRT(cnrtMemcpy(src_uv_mlu_, src_uv_mlu, sizeof(void *), CNRT_MEM_TRANS_DIR_HOST2DEV));
    size_t workspace_tmp;
    CNPEAK_CHECK_CNCV(cncvGetResizeConvertWorkspaceSize(1,
                                    src_desc_,
                                    src_roi_,
                                    dst_desc_,
                                    dst_roi_,
                                    &workspace_tmp));
    if (rc_workspace_size_ < workspace_tmp)
    {
        rc_workspace_size_ = workspace_tmp;
        if (rc_workspace_) {
            CNPEAK_CHECK_CNRT(cnrtFree(rc_workspace_));
        }
        CNPEAK_CHECK_CNRT(cnrtMalloc(&rc_workspace_, rc_workspace_size_));
    }

    CNPEAK_CHECK_CNCV(cncvResizeConvert(cncv_handle_,
                      1,
                      src_desc_,
                      src_roi_,
                      src_y_mlu_,
                      src_uv_mlu_,
                      dst_desc_,
                      dst_roi_,
                      dst_rgb_mlu_[mlu_mem_idx_],
                      rc_workspace_size_,
                      rc_workspace_,
                      CNCV_INTER_BILINEAR));
    
    // cnrtQueueSync(queue_);
    CNPEAK_CHECK_CNRT(cnrtQueueSync(queue_));
    output_ptr = dst_rgb_cpu_[mlu_mem_idx_];
#if 0 // debug    
    int size = dst_w_*dst_h_*3*sizeof(char);
    void* bgr_cpu = malloc(size);
    CNPEAK_CHECK_CNRT(cnrtMemcpy(bgr_cpu, dst_rgb_cpu_[mlu_mem_idx_][0], size, CNRT_MEM_TRANS_DIR_DEV2HOST));
    cv::Mat img(cv::Size(dst_w_, dst_h_), CV_8UC3, bgr_cpu);
    cv::imwrite("outputs/out-110.jpg", img);
    // FILE* fp = fopen("outputs/out-111.rgb", "w");
    // fwrite(bgr_cpu, 1, size, fp);
    // fclose(fp);
#endif
    return true;
}

bool PreDbnetProcesser::mean_std_split(void** & output_ptr)
{
    float mean[3] = {122.67891434, 116.66876762, 104.00698793};
    // float std[3] = {0.00392, 0.00392, 0.00392};
    float std[3] = {255, 255, 255};
    CNPEAK_CHECK_CNCV(cncvMeanStd(cncv_handle_, 1, *dst_desc_, dst_rgb_mlu_[mlu_mem_idx_], mean,
                        std, *mean_std_desc_, mean_std_mlu_, 
                        mean_std_workspace_size_,
                        mean_std_workspace_));
    CNPEAK_CHECK_CNRT(cnrtQueueSync(queue_));
    // cncvSplit(cncv_handle_, *mean_std_desc_, *mean_roi_, mean_std_mlu_cpu_[0], *split_desc_, dst_split_mlu_[mlu_mem_idx_]);
    CNPEAK_CHECK_CNCV(cncvSplit_BasicROI(cncv_handle_, *mean_std_desc_, *mean_roi_, mean_std_mlu_cpu_[0], *split_desc_, dst_split_mlu_[mlu_mem_idx_]));
    output_ptr = dst_split_cpu_[mlu_mem_idx_];
    CNPEAK_CHECK_CNRT(cnrtQueueSync(queue_));
    return true;
}
