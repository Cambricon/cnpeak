//
// Created by jiapeiyuan on 4/24/20.
//

#include "plugin_pre_cncv.hpp"

#if 0//debug
#include <opencv2/opencv.hpp>
#endif

CncvPreprocess::CncvPreprocess(std::vector<int> resize_size,
               bool keep_aspect_ratio,
               bool need_bgr2rgb,
               bool need_tranpose,
               std::vector<float> mean,
               std::vector<float> std,
               int align_stride,
               int device_id,
               int infer_id,
               int cap,
               vector<int> cluster_ids) {

    resize_size_ = resize_size;
    if (!(mean.empty() || std.empty())) {
        mean_ = mean;
        std_ = std;
        need_meanstd_ = true;
    } else {
        need_meanstd_ = false;
    }

    keep_aspect_ratio_ = keep_aspect_ratio;
    need_bgr2rgb_ = need_bgr2rgb;
    need_tranpose_ = need_tranpose;
   
    infer_id_ = infer_id;
    mlu_mem_cap_ = cap;
    mlu_mem_idx_ = 0;
    align_stride_ = align_stride;
   
    batch_ = 1;
    device_id_ = device_id;

    if ( !cluster_ids.empty() ) {
        for (size_t i = 0; i < cluster_ids.size(); i++) {
            bind_bitmap_ = GenBindBitmap(device_id_, cluster_ids[i], bind_bitmap_);
        }
    }
}


string CncvPreprocess :: name(){
    return "CncvPreprocess";
}

bool CncvPreprocess :: init_in_main_thread() {
    return true;
}

bool CncvPreprocess :: init_in_sub_thread() {
    init_device();
    init_context();
    init_imageDesc();
    init_mlu_mem();
    if (bind_bitmap_ > 0) {
        BindCluster(device_id_, bind_bitmap_);
    }
    // cur_offline_ = pCtxt->offline[infer_id_][0];
    return true;
}

bool CncvPreprocess ::callback(TData *&pdata_in, vector<TData *>&pdatas_out) {
    if (pdata_in == nullptr) {
        pdatas_out.push_back(pdata_in);
        return true;
    }
    resize_convert(&pdata_in->mlu_input[0], &pdata_in->mlu_input[1], pdata_in->s_w, pdata_in->s_h, pdata_in->mlu_input);
    if (need_meanstd_) {
        mean_std(pdata_in->mlu_input);
    }
    if (need_tranpose_) {
        split(pdata_in->mlu_input);
    }
    pdatas_out.push_back(pdata_in);
    mlu_mem_idx_ = (mlu_mem_idx_ + 1) % mlu_mem_cap_;
    return true;
}

// ================================= below is private function =========================
bool CncvPreprocess ::init_device() {
    CNPEAK_CHECK_CNRT(cnrtSetDevice(device_id_));
    return true;
}

bool CncvPreprocess ::init_context() {
    CNPEAK_CHECK_CNRT(cnrtQueueCreate(&queue_));
    CNPEAK_CHECK_CNCV(cncvCreate(&cncv_handle_));
    CNPEAK_CHECK_CNCV(cncvSetQueue(cncv_handle_, queue_));
    return true;
}

bool CncvPreprocess ::init_imageDesc() {

    CNPEAK_CHECK_PARAM(resize_size_.size() >= 2);
    int dst_width = resize_size_[0];
    int dst_height = resize_size_[1];
    
    // init image descriptors
    resizeconvert_src_desc_ = new cncvImageDescriptor();
    // init when process

    resizeconvert_dst_desc_ = new cncvImageDescriptor();
    resizeconvert_dst_desc_->width = dst_width;
    resizeconvert_dst_desc_->height = dst_height;
    if (need_bgr2rgb_) {
        resizeconvert_dst_desc_->pixel_fmt = CNCV_PIX_FMT_RGB;
    } else {
        resizeconvert_dst_desc_->pixel_fmt = CNCV_PIX_FMT_BGR;
    }
    resizeconvert_dst_desc_->stride[0] = dst_width * 3 * sizeof(char);
    resizeconvert_dst_desc_->depth = CNCV_DEPTH_8U;
    resizeconvert_dst_desc_->color_space = CNCV_COLOR_SPACE_BT_601;

    mean_std_desc_ = new cncvImageDescriptor();
    mean_std_desc_->width = dst_width;
    mean_std_desc_->height = dst_height;
    if (need_bgr2rgb_) {
        mean_std_desc_->pixel_fmt = CNCV_PIX_FMT_RGB;
    } else {
        mean_std_desc_->pixel_fmt = CNCV_PIX_FMT_BGR;
    }
    if (need_meanstd_) {
        mean_std_desc_->stride[0] = dst_width * 3 * sizeof(float);
        mean_std_desc_->depth = CNCV_DEPTH_32F;
    } else {
        mean_std_desc_->stride[0] = dst_width * 3;
        mean_std_desc_->depth = CNCV_DEPTH_8U;
    }
    mean_std_desc_->color_space = CNCV_COLOR_SPACE_BT_601;

    split_desc_ = new cncvImageDescriptor();
    split_desc_->width = dst_width;
    split_desc_->height = dst_height;
    split_desc_->pixel_fmt = CNCV_PIX_FMT_GRAY;
    if (need_meanstd_) {
        split_desc_->stride[0] = dst_width * sizeof(float);
        split_desc_->stride[1] = dst_width * sizeof(float);
        split_desc_->stride[2] = dst_width * sizeof(float);
        split_desc_->depth = CNCV_DEPTH_32F;
    } else {
        split_desc_->stride[0] = dst_width;
        split_desc_->stride[1] = dst_width;
        split_desc_->stride[2] = dst_width;
        split_desc_->depth = CNCV_DEPTH_8U;
    }
    split_desc_->color_space = CNCV_COLOR_SPACE_BT_601;

    // init ROIs
    src_roi_ = new cncvRect();
    dst_roi_ = new cncvRect();
    // init when process

    if (!keep_aspect_ratio_) {
        dst_roi_->x = 0;
        dst_roi_->y = 0;
        dst_roi_->w = dst_width;
        dst_roi_->h = dst_height;
    }
    
    mean_roi_ = new cncvRect();
    mean_roi_->x = 0;
    mean_roi_->y = 0;
    mean_roi_->w = dst_width;
    mean_roi_->h = dst_height;
    return true;
}

bool CncvPreprocess ::init_mlu_mem(){
    CNPEAK_CHECK_PARAM(resize_size_.size() >= 2);
    int dst_width = resize_size_[0];
    int dst_height = resize_size_[1];

    src_mlu_cpu_ = (void **)malloc(2 * sizeof(void*));
    CNPEAK_CHECK_CNRT(cnrtMalloc((void **)(&src_mlu_), 2 * sizeof(void*)));

    for (int i = 0; i < mlu_mem_cap_; i++) {
        // resizeconvert result points
        void** dst_resizeconvert_cpu = (void**)malloc(sizeof(void*));
        void** dst_resizeconvert_mlu;
        CNPEAK_CHECK_CNRT(cnrtMalloc((void**)&dst_resizeconvert_mlu, sizeof(void*)));

        CNPEAK_CHECK_CNRT(cnrtMalloc(&(dst_resizeconvert_cpu[0]), dst_width * dst_height * 3 * sizeof(char)));
        CNPEAK_CHECK_CNRT(cnrtMemcpy(dst_resizeconvert_mlu, dst_resizeconvert_cpu, sizeof(void*), CNRT_MEM_TRANS_DIR_HOST2DEV));
        
        dst_resizeconvert_mlu_.push_back(dst_resizeconvert_mlu);
        dst_resizeconvert_cpu_.push_back(dst_resizeconvert_cpu);

        // meanstd
        if (need_meanstd_) {
            void** dst_meanstd_cpu = (void**)malloc(sizeof(void*));
            void** dst_meanstd_mlu;
            CNPEAK_CHECK_CNRT(cnrtMalloc((void**)&dst_meanstd_mlu, sizeof(void*)));

            CNPEAK_CHECK_CNRT(cnrtMalloc(&(dst_meanstd_cpu[0]), dst_width * dst_height * 3 * sizeof(float)));
            CNPEAK_CHECK_CNRT(cnrtMemcpy(dst_meanstd_mlu, dst_meanstd_cpu, sizeof(void*), CNRT_MEM_TRANS_DIR_HOST2DEV));
        
            dst_meanstd_mlu_.push_back(dst_meanstd_mlu);
            dst_meanstd_cpu_.push_back(dst_meanstd_cpu);
        }
        
        // split
        if (need_tranpose_) {
            void** dst_split_cpu = (void**)malloc(3*sizeof(void*));
            void** dst_split_mlu;
            CNPEAK_CHECK_CNRT(cnrtMalloc((void**)&dst_split_mlu, 3*sizeof(void*)));

            void* dst_split_mlu_data;
            if (need_meanstd_) {
                CNPEAK_CHECK_CNRT(cnrtMalloc(&dst_split_mlu_data, dst_width * dst_height * 3 * sizeof(float)));
                dst_split_cpu[0] = dst_split_mlu_data;
                dst_split_cpu[1] = dst_split_mlu_data + dst_width * dst_height * sizeof(float);
                dst_split_cpu[2] = dst_split_mlu_data + dst_width * dst_height * sizeof(float) * 2;
            } else {
                CNPEAK_CHECK_CNRT(cnrtMalloc(&dst_split_mlu_data, dst_width * dst_height * 3 * sizeof(char)));
                dst_split_cpu[0] = dst_split_mlu_data;
                dst_split_cpu[1] = dst_split_mlu_data + dst_width * dst_height;
                dst_split_cpu[2] = dst_split_mlu_data + dst_width * dst_height * 2;
            }
            CNPEAK_CHECK_CNRT(cnrtMemcpy(dst_split_mlu, dst_split_cpu, 3*sizeof(void*), CNRT_MEM_TRANS_DIR_HOST2DEV));
        
            dst_split_mlu_.push_back(dst_split_mlu);
            dst_split_cpu_.push_back(dst_split_cpu);
            dst_split_mlu_data_.push_back(dst_split_mlu_data);
        }
    }

    if (need_meanstd_) {
        CNPEAK_CHECK_CNCV(cncvGetMeanStdWorkspaceSize(3, &meanstd_workspace_size_))
        CNPEAK_CHECK_CNRT(cnrtMalloc(&meanstd_workspace_, meanstd_workspace_size_));
    }

    return true;
}

bool CncvPreprocess ::resize_convert(void** src_y_mlu, void** src_uv_mlu, int src_w, int src_h, void** & output_ptr){
    CNPEAK_CHECK_PARAM(resize_size_.size() >= 2);
    int dst_width = resize_size_[0];
    int dst_height = resize_size_[1];
    
    src_mlu_cpu_[0] = src_y_mlu[0];
    src_mlu_cpu_[1] = src_uv_mlu[0];
    CNPEAK_CHECK_CNRT(cnrtMemcpy(src_mlu_, src_mlu_cpu_, 2 * sizeof(void *), CNRT_MEM_TRANS_DIR_HOST2DEV));

    // assign descriptor & roi
    resizeconvert_src_desc_->width = src_w;
    resizeconvert_src_desc_->height = src_h;
    resizeconvert_src_desc_->pixel_fmt = CNCV_PIX_FMT_NV12;
    resizeconvert_src_desc_->stride[0] = ALIGN_UP(src_w, align_stride_);
    resizeconvert_src_desc_->stride[1] = ALIGN_UP(src_w, align_stride_);
    resizeconvert_src_desc_->depth = CNCV_DEPTH_8U;
    resizeconvert_src_desc_->color_space = CNCV_COLOR_SPACE_BT_601;

    src_roi_->x = 0;
    src_roi_->y = 0;
    src_roi_->w = src_w;
    src_roi_->h = src_h;

    if (keep_aspect_ratio_) {
        float scale_w = (float)dst_width / (float)src_w;
        float scale_h = (float)dst_height / (float)src_h;

        if (scale_w < scale_h) {
            dst_roi_->x = 0;
            dst_roi_->w = dst_width;
            dst_roi_->h = src_h * scale_w;
            dst_roi_->y = (dst_height - dst_roi_->h) / 2;
        } else {
            dst_roi_->y = 0;
            dst_roi_->h = dst_height;
            dst_roi_->w = src_w * scale_h;
            dst_roi_->x = (dst_width - dst_roi_->w) / 2; 
        }
    }

    size_t resizeconvert_workspace_size_tmp;
    cncvGetResizeConvertWorkspaceSize(1,
                                      const_cast<cncvImageDescriptor*>(resizeconvert_src_desc_),
                                      const_cast<cncvRect*>(src_roi_),
                                      const_cast<cncvImageDescriptor*>(resizeconvert_dst_desc_),
                                      const_cast<cncvRect*>(dst_roi_),
                                      &resizeconvert_workspace_size_tmp);
    if (resizeconvert_workspace_size_ < resizeconvert_workspace_size_tmp) {
        if (resizeconvert_workspace_) {
            CNPEAK_CHECK_CNRT(cnrtFree(resizeconvert_workspace_));
        }
        resizeconvert_workspace_size_ = resizeconvert_workspace_size_tmp;
        CNPEAK_CHECK_CNRT(cnrtMalloc((void**)&resizeconvert_workspace_, resizeconvert_workspace_size_));
    }

    CNPEAK_CHECK_CNCV(cncvResizeConvert_AdvancedROI(cncv_handle_,
                         1,
                         resizeconvert_src_desc_,
                         src_roi_,
                         src_mlu_,
                         resizeconvert_dst_desc_,
                         dst_roi_,
                         dst_resizeconvert_mlu_[mlu_mem_idx_],
                         resizeconvert_workspace_size_,
                         resizeconvert_workspace_,
                         CNCV_INTER_BILINEAR));
    
    CNPEAK_CHECK_CNRT(cnrtQueueSync(queue_));
    output_ptr = dst_resizeconvert_cpu_[mlu_mem_idx_];

#if 0 // debug
    int size = dst_width*dst_height*3*sizeof(char);
    void* bgr_cpu = malloc(size);
    cnrtMemcpy(bgr_cpu, dst_resizeconvert_cpu_[mlu_mem_idx_][0], size, CNRT_MEM_TRANS_DIR_DEV2HOST);
    cv::Mat img(cv::Size(dst_width, dst_height), CV_8UC3, bgr_cpu);
    cv::imwrite("outputs/out-111.jpg", img);
    // FILE* fp = fopen("outputs/out-111.rgb", "w");
    // fwrite(bgr_cpu, 1, size, fp);
    // fclose(fp);
#endif
    return true;
}

bool CncvPreprocess ::mean_std(void** & output_ptr){


    CNPEAK_CHECK_CNCV(cncvMeanStd(cncv_handle_, 
            1, 
            *resizeconvert_dst_desc_, 
            dst_resizeconvert_mlu_[mlu_mem_idx_], 
            mean_.data(),
            std_.data(),
            *mean_std_desc_, 
            dst_meanstd_mlu_[mlu_mem_idx_], 
            meanstd_workspace_size_,
            meanstd_workspace_));
    CNPEAK_CHECK_CNRT(cnrtQueueSync(queue_));
    output_ptr = dst_meanstd_cpu_[mlu_mem_idx_];

    return true;
}

bool CncvPreprocess::split(void** &output_ptr) {
    if (need_meanstd_) {
        CNPEAK_CHECK_CNCV(cncvSplit_BasicROI(cncv_handle_, *mean_std_desc_, *mean_roi_, dst_meanstd_cpu_[mlu_mem_idx_][0], *split_desc_, dst_split_mlu_[mlu_mem_idx_]));
    } else {
        CNPEAK_CHECK_CNCV(cncvSplit_BasicROI(cncv_handle_, *mean_std_desc_, *mean_roi_, dst_resizeconvert_cpu_[mlu_mem_idx_][0], *split_desc_, dst_split_mlu_[mlu_mem_idx_]));
    }
    CNPEAK_CHECK_CNRT(cnrtQueueSync(queue_));
    output_ptr = dst_split_cpu_[mlu_mem_idx_];
    
    return true;
}

void CncvPreprocess::BindCluster(int dev_id, uint64_t bitmap) {
  // Set device id and get max cluster num
  CNPEAK_CHECK_CNRT(cnrtSetDevice(dev_id));
  int cluster_num = 0;
  CNPEAK_CHECK_CNRT(cnrtDeviceGetAttribute(&cluster_num, cnrtAttrClusterCount, dev_id));

  CNctxConfigParam param;
  param.unionLimit = CN_KERNEL_CLASS_UNION;

  // Set ctx
  CNcontext ctx;
  assert(cnCtxGetCurrent(&ctx) == CNresult::CN_SUCCESS);
  assert(cnSetCtxConfigParam(ctx, CN_CTX_CONFIG_UNION_LIMIT, &param) == CNresult::CN_SUCCESS);
  param.visibleCluster = bitmap;
  assert(cnSetCtxConfigParam(ctx, CN_CTX_CONFIG_VISIBLE_CLUSTER, &param) == CNresult::CN_SUCCESS);
}

uint64_t CncvPreprocess::GenBindBitmap(int dev_id, int thread_id, uint64_t bitmap) {
    int cluster_num = 0;
    CNPEAK_CHECK_CNRT(cnrtDeviceGetAttribute(&cluster_num, cnrtAttrClusterCount, dev_id));
    uint64_t bitmap_cur = 0x01;
    return bitmap | (bitmap_cur << (thread_id % cluster_num));
}