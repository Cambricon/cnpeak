#include <opencv2/opencv.hpp>
#include "plugin_pre_crnn.hpp"

#define DEBUG 0

#if 0//debug
#include <opencv2/opencv.hpp>
#endif

using namespace cv;

PreCrnnProcesser::PreCrnnProcesser(int thread_id, int dst_w, int dst_h, int device_id, int infer_id, int cap)
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
      
    dst_pts.resize(5);
    dst_pts[0].x = 0;
    dst_pts[0].y = dst_h;
    dst_pts[1].x = 0;
    dst_pts[1].y = 0;
    dst_pts[2].x = dst_w;
    dst_pts[2].y = 0;
    dst_pts[3].x = dst_w;
    dst_pts[3].y = dst_h;
}

string PreCrnnProcesser::name()
{
    return "PreCrnnProcesser";
}

bool PreCrnnProcesser::init_in_main_thread()
{
    return true;
}

bool PreCrnnProcesser::init_in_sub_thread()
{
    init_device();
    init_context();
    init_imageDesc(src_w_, src_h_, dst_w_, dst_h_);
    malloc_mlu();
    init_mlu_mem();
    init_workspace();
    // cur_offline_ = pCtxt->offline[infer_id_][0];
    return true;
}

bool PreCrnnProcesser::callback(TData *&pdata_in, vector<TData *>&pdatas_out)
{        
    if (pdata_in == nullptr) {
        pdatas_out.push_back(pdata_in);
        return true;
    }
    pdata_in->mlu_multi_inputs.clear();
    if (pdata_in->boxes.size() == 0) {        
        pdatas_out.push_back(pdata_in);
        return true;
    // } else if (pdata_in->boxes.size() > MAX_BOXES) {
    //     pdatas_out.push_back(pdata_in);
    //     return false;
    }

#if DEBUG//debug
    std::cout << "Debug : ====================================" << std::endl;
    cout << "pdata_in->s_w=" << pdata_in->s_w << endl;
    cout << "pdata_in->s_h=" << pdata_in->s_h << endl;
#endif
    preProcess(pdata_in->boxes, pdata_in->mlu_origin_image[0], pdata_in->s_w, pdata_in->s_h, pdata_in->mlu_multi_inputs);
    mlu_mem_idx_ = (mlu_mem_idx_ + 1) % mlu_mem_cap_;
    pdatas_out.push_back(pdata_in);
    return true;
}

// ================================= below is private function =========================
bool PreCrnnProcesser::init_device()
{
    // cnrtInit(0);
    // cnrtDev_t dev;
    // cnrtGetDeviceHandle(&dev, device_id_);
    // cnrtSetCurrentDevice(dev);
    // cnrtSetCurrentChannel((cnrtChannelType_t)(thread_id_ % 4));
    CNPEAK_CHECK_CNRT(cnrtSetDevice(device_id_));
    return true;
}

bool PreCrnnProcesser::init_context()
{
    CNPEAK_CHECK_CNRT(cnrtQueueCreate(&queue_));
    CNPEAK_CHECK_CNCV(cncvCreate(&cncv_handle_));
    CNPEAK_CHECK_CNCV(cncvSetQueue(cncv_handle_, queue_));
    return true;
}

void PreCrnnProcesser::malloc_mlu() {
    CNPEAK_CHECK_CNRT(cnrtMalloc((void **)(&src_mlu_), MAX_BOXES * sizeof(void*)));    
    CNPEAK_CHECK_CNRT(cnrtMalloc((void **)(&mat_mlu_), MAX_BOXES * sizeof(void*)));


    src_mlu_cpu_ = (void**)malloc(MAX_BOXES * sizeof(void*));    
    mat_mlu_cpu_ = (void**)malloc(MAX_BOXES * sizeof(void*));


    for (int i = 0; i < MAX_BOXES; i++) {
        CNPEAK_CHECK_CNRT(cnrtMalloc((void **)(&mat_mlu_cpu_[i]), 9 * sizeof(float)));
    }
    CNPEAK_CHECK_CNRT(cnrtMemcpy(mat_mlu_, mat_mlu_cpu_, MAX_BOXES * sizeof(void *), CNRT_MEM_TRANS_DIR_HOST2DEV));

    for (int i = 0; i < mlu_mem_cap_; i++) {
        size_t size = dst_w_ * dst_h_ * 3 * sizeof(char);
        size_t size_gray = dst_w_ * dst_h_ * sizeof(char);        
        void** warp_mlu;
        CNPEAK_CHECK_CNRT(cnrtMalloc((void**)&warp_mlu, MAX_BOXES * sizeof(void*)));

        void** warp_mlu_cpu;
        warp_mlu_cpu = (void**)malloc(MAX_BOXES * sizeof(void*));

        void* dst_mlu_data;
        CNPEAK_CHECK_CNRT(cnrtMalloc((&dst_mlu_data), MAX_BOXES * size));

        for (int j = 0; j < MAX_BOXES; j++) {
            warp_mlu_cpu[j] = (char*)dst_mlu_data + j * size;
        }

        CNPEAK_CHECK_CNRT(cnrtMemcpy(warp_mlu, warp_mlu_cpu, MAX_BOXES * sizeof(void*), CNRT_MEM_TRANS_DIR_HOST2DEV));


        void** gray_mlu;        
        CNPEAK_CHECK_CNRT(cnrtMalloc((void**)&gray_mlu, MAX_BOXES * sizeof(void*)));

        void** gray_mlu_cpu;
        gray_mlu_cpu = (void**)malloc(MAX_BOXES * sizeof(void*));

        void* gray_mlu_data;
        CNPEAK_CHECK_CNRT(cnrtMalloc((&gray_mlu_data), MAX_BOXES * size_gray));

        for (int j = 0; j < MAX_BOXES; j++) {
            gray_mlu_cpu[j] = (char*)gray_mlu_data + j * size_gray;
        }

        CNPEAK_CHECK_CNRT(cnrtMemcpy(gray_mlu, gray_mlu_cpu, MAX_BOXES * sizeof(void*), CNRT_MEM_TRANS_DIR_HOST2DEV));
        
        dst_warp_data_.push_back(dst_mlu_data);
        dst_warp_mlu_.push_back(warp_mlu);
        dst_warp_cpu_.push_back(warp_mlu_cpu);
        gray_warp_mlu_.push_back(gray_mlu);
        gray_warp_cpu_.push_back(gray_mlu_cpu);
    }
}

bool PreCrnnProcesser::init_imageDesc(int src_width, int src_height, int dst_width, int dst_height)
{
    src_desc_ = new cncvImageDescriptor();
    src_desc_->width = src_width;
    src_desc_->height = src_height;
    src_desc_->pixel_fmt = CNCV_PIX_FMT_BGR;
    src_desc_->stride[0] =  src_width * 3;
    // src_desc_->stride[1] = src_width;
    src_desc_->depth = CNCV_DEPTH_8U;
    src_desc_->color_space = CNCV_COLOR_SPACE_BT_601;

    dst_desc_ = new cncvImageDescriptor();
    dst_desc_->width = dst_width;
    dst_desc_->height = dst_height;
    dst_desc_->pixel_fmt = CNCV_PIX_FMT_BGR;
    dst_desc_->stride[0] = dst_width * channel_num * sizeof(char);
    dst_desc_->depth = CNCV_DEPTH_8U;
    dst_desc_->color_space = CNCV_COLOR_SPACE_BT_601;

    mean_std_desc_ = new cncvImageDescriptor();
    mean_std_desc_->width = dst_width;
    mean_std_desc_->height = dst_height;
    mean_std_desc_->pixel_fmt = CNCV_PIX_FMT_BGR;
    mean_std_desc_->stride[0] = dst_width * channel_num * sizeof(float);
    mean_std_desc_->depth = CNCV_DEPTH_32F;
    mean_std_desc_->color_space = CNCV_COLOR_SPACE_BT_601;

    gray_desc_ = new cncvImageDescriptor();
    gray_desc_->width = dst_width;
    gray_desc_->height = dst_height;
    gray_desc_->pixel_fmt = CNCV_PIX_FMT_GRAY;
    gray_desc_->stride[0] = dst_width * sizeof(char);
    // gray_desc_->stride[1] = dst_width * sizeof(char);
    // gray_desc_->stride[2] = dst_width * sizeof(char);
    gray_desc_->depth = CNCV_DEPTH_8U;
    gray_desc_->color_space = CNCV_COLOR_SPACE_BT_601;

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

bool PreCrnnProcesser::init_mlu_mem()
{
    CNPEAK_CHECK_CNRT(cnrtMalloc((void **)(&src_y_mlu_), sizeof(void*)));
    CNPEAK_CHECK_CNRT(cnrtMalloc((void **)(&src_uv_mlu_), sizeof(void*)));
    CNPEAK_CHECK_CNRT(cnrtMalloc((void **)(&rgb_mlu_), sizeof(void*)));
    CNPEAK_CHECK_CNRT(cnrtMalloc((void **)(&mean_std_mlu_), sizeof(void*)));
    
    rgb_mlu_cpu_ = (void **)malloc(sizeof(void*));
    mean_std_mlu_cpu_ = (void **)malloc(sizeof(void*));
    CNPEAK_CHECK_CNRT(cnrtMalloc(&(rgb_mlu_cpu_[0]), dst_w_*dst_h_*channel_num*sizeof(char)));
    CNPEAK_CHECK_CNRT(cnrtMemset(rgb_mlu_cpu_[0], 0, dst_w_*dst_h_*channel_num*sizeof(char)));
    CNPEAK_CHECK_CNRT(cnrtMalloc(&(mean_std_mlu_cpu_[0]), dst_w_*dst_h_*channel_num*sizeof(float)));

    CNPEAK_CHECK_CNRT(cnrtMemcpy(rgb_mlu_, rgb_mlu_cpu_, sizeof(void*), CNRT_MEM_TRANS_DIR_HOST2DEV));
    CNPEAK_CHECK_CNRT(cnrtMemcpy(mean_std_mlu_, mean_std_mlu_cpu_, sizeof(void*), CNRT_MEM_TRANS_DIR_HOST2DEV));

    for(int i=0; i<mlu_mem_cap_; i++)
    {
        void** split_mlu;
        CNPEAK_CHECK_CNRT(cnrtMalloc((void **)(&split_mlu), channel_num*sizeof(void *)));
        
        void* split_mlu_data;
        CNPEAK_CHECK_CNRT(cnrtMalloc((&split_mlu_data), dst_w_*dst_h_*channel_num*sizeof(float)));
        
        void** split_mlu_cpu = (void**)malloc(channel_num*sizeof(void *));
        int size = dst_w_*dst_h_*sizeof(float);
        split_mlu_cpu[0] = split_mlu_data;
        split_mlu_cpu[1] = (char*)split_mlu_data + size;
        split_mlu_cpu[2] = (char*)split_mlu_data + size*2;
        CNPEAK_CHECK_CNRT(cnrtMemcpy(split_mlu, split_mlu_cpu, channel_num*sizeof(void*), CNRT_MEM_TRANS_DIR_HOST2DEV));

        dst_split_mlu_.push_back(split_mlu);
        dst_split_cpu_.push_back(split_mlu_cpu);
    }
    return true;
}

bool PreCrnnProcesser::init_workspace()
{
    // cncvGetResizeConvertWorkspaceSize(1,
    //                                 src_desc_,
    //                                 src_roi_,
    //                                 dst_desc_,
    //                                 dst_roi_,
    //                                 &rc_workspace_size_);
    // cnrtMalloc(&rc_workspace_, rc_workspace_size_);
    
    CNPEAK_CHECK_CNCV(cncvGetMeanStdWorkspaceSize(channel_num, &mean_std_workspace_size_));
    CNPEAK_CHECK_CNRT(cnrtMalloc(&mean_std_workspace_, mean_std_workspace_size_));
    return true;
}

bool PreCrnnProcesser::resize_convert(void** src_y_mlu, void** src_uv_mlu, size_t src_w, size_t src_h)
{    
    src_desc_->width = src_w;
    src_desc_->height = src_h;    
    src_desc_->stride[0] = ALIGN_UP(src_w, 64);
    src_desc_->stride[1] = ALIGN_UP(src_w, 64);    
    src_roi_->w = src_w;
    src_roi_->h = src_h;
    cnrtMemcpy(src_y_mlu_, src_y_mlu, sizeof(void *), CNRT_MEM_TRANS_DIR_HOST2DEV);
    cnrtMemcpy(src_uv_mlu_, src_uv_mlu, sizeof(void *), CNRT_MEM_TRANS_DIR_HOST2DEV);
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
                      rgb_mlu_,
                      rc_workspace_size_,
                      rc_workspace_,
                      CNCV_INTER_BILINEAR));
    
    CNPEAK_CHECK_CNRT(cnrtQueueSync(queue_));

#if 0// debug
    int size = dst_w_*dst_h_*channel_num*sizeof(char);
    void* bgr_cpu = malloc(size);
    cnrtMemcpy(bgr_cpu, rgb_mlu_cpu_[0], size, CNRT_MEM_TRANS_DIR_DEV2HOST);
    cv::Mat img(cv::Size(dst_w_, dst_h_), CV_8UC3, bgr_cpu);
    cv::imwrite("outputs/out-111.jpg", img);
    // FILE* fp = fopen("outputs/out-111.rgb", "w");
    // fwrite(bgr_cpu, 1, size, fp);
    // fclose(fp);
#endif
    return true;
}

bool PreCrnnProcesser::mean_std_split(void** & output_ptr)
{
    float mean[] = {127.5, 127.5, 127.5};
    float std[] = {127.5, 127.5, 127.5};
    CNPEAK_CHECK_CNCV(cncvMeanStd(cncv_handle_, 1, *dst_desc_, rgb_mlu_, mean,
                        std, *mean_std_desc_, mean_std_mlu_, 
                        mean_std_workspace_size_,
                        mean_std_workspace_));
    CNPEAK_CHECK_CNRT(cnrtQueueSync(queue_));
    CNPEAK_CHECK_CNCV(cncvSplit_BasicROI(cncv_handle_, *mean_std_desc_, *mean_roi_, mean_std_mlu_cpu_[0], *split_desc_, dst_split_mlu_[mlu_mem_idx_]));
    output_ptr = dst_split_cpu_[mlu_mem_idx_];
    mlu_mem_idx_ = (mlu_mem_idx_ + 1) % mlu_mem_cap_;
    CNPEAK_CHECK_CNRT(cnrtQueueSync(queue_));
    return true;
}


bool PreCrnnProcesser::preProcess(vector<vector<Point2f>> &boxes, void* mlu_input, size_t src_w, size_t src_h, vector<void*> &output_ptr) {
    
    for (size_t i = 0; i < boxes.size(); ++i) {
        vector<cncvPoint> src_pts;
        src_pts.resize(4);
        for (int j = 0; j < 4; ++j) {
            src_pts[j].x = boxes[i][j].x;
            src_pts[j].y = boxes[i][j].y;
        }
        float matrixes[9] = {0};
        cncvGetPerspectiveTransform(src_pts.data(), dst_pts.data(), matrixes);
        CNPEAK_CHECK_CNRT(cnrtMemcpy(mat_mlu_cpu_[i], matrixes, 9 * sizeof(float), CNRT_MEM_TRANS_DIR_HOST2DEV));

        src_mlu_cpu_[i] = mlu_input;
    }
    CNPEAK_CHECK_CNRT(cnrtMemcpy(src_mlu_, src_mlu_cpu_, boxes.size() * sizeof(void*), CNRT_MEM_TRANS_DIR_HOST2DEV));
    
    src_desc_->width = 896;//src_w;
    src_desc_->height = 736;//src_h;
    src_desc_->stride[0] = 896 * 3;//src_w;
    cncvWarpPerspective(cncv_handle_, boxes.size(), WARP_FORWARD, CNCV_PAD_CONSTANT, mat_mlu_, *src_desc_, src_mlu_, *dst_desc_, dst_warp_mlu_[mlu_mem_idx_]);
    CNPEAK_CHECK_CNRT(cnrtQueueSync(queue_));

    cncvRgbxToGray_ROI(cncv_handle_, boxes.size(), *dst_desc_, *dst_roi_, dst_warp_mlu_[mlu_mem_idx_], *gray_desc_, *dst_roi_, gray_warp_mlu_[mlu_mem_idx_], nullptr);
    CNPEAK_CHECK_CNRT(cnrtQueueSync(queue_));
    
    for (size_t i = 0; i < boxes.size(); i++) {
        output_ptr.push_back(gray_warp_cpu_[mlu_mem_idx_][i]);        
    }

#if 0 // debug
    int size = 896 * 736 * 3 * sizeof(char);
    void* bgr_cpu = malloc(size);
    cnrtMemcpy(bgr_cpu, mlu_input, size, CNRT_MEM_TRANS_DIR_DEV2HOST);
    // cnrtMemcpy(bgr_cpu, gray_mlu_[0], size, CNRT_MEM_TRANS_DIR_DEV2HOST);
    cv::Mat img(cv::Size(896, 736), CV_8UC3, bgr_cpu);
    cv::imwrite("outputs/out-111" + std::to_string(mlu_mem_idx_) + ".jpg", img);
    // cv::waitKey();
    // FILE* fp = fopen("outputs/out-111.rgb", "w");
    // fwrite(bgr_cpu, 1, size, fp);
    // fclose(fp);
#endif

#if 0 // debug
    int size = dst_w_ * dst_h_ * 3 * sizeof(char);
    void* bgr_cpu = malloc(size);
    cnrtMemcpy(bgr_cpu, dst_warp_cpu_[mlu_mem_idx_][0], size, CNRT_MEM_TRANS_DIR_DEV2HOST);
    // cnrtMemcpy(bgr_cpu, gray_mlu_[0], size, CNRT_MEM_TRANS_DIR_DEV2HOST);
    cv::Mat img(cv::Size(dst_w_, dst_h_), CV_8UC3, bgr_cpu);
    cv::imwrite("outputs/out-111" + std::to_string(mlu_mem_idx_) + ".jpg", img);
    // cv::waitKey();
    // FILE* fp = fopen("outputs/out-111.rgb", "w");
    // fwrite(bgr_cpu, 1, size, fp);
    // fclose(fp);
#endif

    return true;
}