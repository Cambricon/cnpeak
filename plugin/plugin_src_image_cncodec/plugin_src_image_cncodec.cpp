//
// Created by cambricon on 19-7-9.
//


#include "plugin_src_image_cncodec.hpp"

CncodecImageProvider::CncodecImageProvider(int pipe_id, 
                                           string file_path, 
                                           bool out_mlu,
                                           bool out_cpu,
                                           int pipe_num,
                                           int thread_num, 
                                           int device_id,
                                           int cap) {
    file_path_      = file_path;
    pipe_id_        = pipe_id;
    pipe_num_       = pipe_num;
    device_id_      = device_id;
    mlu_mem_cap_    = cap;
    thread_num_     = thread_num;
    out_mlu_        = out_mlu;
    out_cpu_        = out_cpu;

    frame_id        = 0;
    thread_idx_     = 0;
    is_dump_image_  = false;
    mlu_mem_idx_    = 0;
}

CncodecImageProvider::~CncodecImageProvider() {
    for (auto mlu_input : mlu_inputs_) {
        delete mlu_input;
    }
    mlu_inputs_.clear();
}


string CncodecImageProvider::name(){
    return "CncodecImageProvider";
}

bool CncodecImageProvider::init_in_main_thread() {
    queue_ = new BlockQueue<cncodecFrame_t*>(mlu_mem_cap_);
    return true;
}

bool CncodecImageProvider::init_in_sub_thread() {
    // if (pCtxt->offline.size() > 0) {
    //     cur_offline_ = pCtxt->offline[0][0];
    //     batch_ = cur_offline_->input_n[0];
    // } else {
    //     batch_ = 1;
    // }
    // init_cnrt();
    parse_image_path();
    malloc_mlu_buffer();
    for (int i = 0; i < thread_num_; i++) {
        cncodecHandle_t handle;
        hdecoder_.push_back(handle);
    }

    for (int i = 0; i < thread_num_; i++){
        create_image_read_process(i);
        usleep(1000);
    }
    return true;
}

bool CncodecImageProvider::callback(TData *&pdata_in, vector<TData *>&pdatas_out) {
    if (pdata_in == nullptr) {
        pdatas_out.push_back(pdata_in);
        return true;
    }
    if (image_parsed_num_ >= image_num_) {
        pdata_in = nullptr;
        return true;
    }

    string ori_img_name = "";

    // for (int i = 0; i < batch_; i++) {
        cncodecFrame_t frame;
        dequeue_frame(&frame);

        //TODO EOS
        // if(frame. == CNJPEGDEC_FLAG_EOS) {
        //     return false;
        // }
    if (out_mlu_) {
        output_to_mlu(pdata_in, &frame);
    }
    if (out_cpu_) {
        output_to_cpu(pdata_in, &frame);
    }

        i32_t ret = cncodecDecFrameUnref(hdecoder_[thread_idx_], &frame);
        assert(ret == CNCODEC_SUCCESS);

        if(image_parsed_num_ == image_num_) {
            ori_img_name = img_list_[0];
        }
        else {
            // ori_img_name=std::to_string(image_parsed_num_);
            ori_img_name = img_list_[image_parsed_num_];
        }
        // std::cout << ori_img_name << std::endl;
        pdata_in->origin_image_names.push_back(ori_img_name);
        image_parsed_num_++;

    // }

    pdatas_out.push_back(pdata_in);
    return true;
}

// bool CncodecImageProvider::enqueue_frame(cnjpegDecOutput *pimage){
bool CncodecImageProvider::enqueue_frame(cncodecFrame_t *pimage){
    // cnjpegDecOutput *info = new cnjpegDecOutput();
    cncodecFrame_t *info = new cncodecFrame_t();
    *info = *pimage;
    queue_->push(info);
    return true;
}

// ================================= below is private function =========================
void CncodecImageProvider::malloc_mlu_buffer(){
    for(int i=0; i<mlu_mem_cap_; i++){
        auto mlu_input = reinterpret_cast<void**>(malloc(sizeof(void*)*2));
        mlu_inputs_.push_back(mlu_input);
    }
}

bool CncodecImageProvider::create_image_read_process(int channel_id){
    new thread(&CncodecImageProvider::image_read_thread_start, this, channel_id);
    return true;
}

bool CncodecImageProvider::parse_image_path() {
    std::string line;

    int count = 0;
    CNPEAK_CHECK_PARAM(check_file_exist(file_path_));
    std::ifstream files(file_path_.c_str(), std::ios::in);
    while (getline(files, line)) {
        if (!check_file_exist(line)) {
            cout << "cannot find file: " << line << endl;
            cout << "skip..." << endl;
            continue;
        }
        if (pipe_id_ != -1 && thread_num_ != -1) {
            if (count % thread_num_ == pipe_id_) {
                img_list_.push_back(line);
                image_num_++;
            }
            count++;
        } else {
            img_list_.push_back(line);
            image_num_++;
        }
    }
    files.close();

    // 补充到mem_cap张图片? why?
    // int i = 0;
    // while (image_num_ % mlu_mem_cap_ != 0) {
    //     img_list_.push_back(img_list_[i++]);
    //     image_num_++;
    // }

    int count1 = 0;
    for (auto img_file : img_list_) {
        img_dict[count1 % thread_num_].push_back(img_file);
        count1++;
    }

    return true;
}

bool CncodecImageProvider::image_read_thread_start(int thread_id){
    init_cnrt();
    init_cncodec(thread_id);

    // cnjpegDecInput    decinput;
    cncodecStream_t decinput;
    memset(&decinput, 0, sizeof(decinput));
    char data_buffer[2 * 1000 * 1000];

    for (auto img_file : img_dict[thread_id]) {
        string image;
        string img_name;
        FILE *fid;

        auto index = img_file.find(" ");
        if (index != string::npos) {
            image = img_file.substr(0, index);
        } else {
            image = img_file;
        }

        std::cout << image << std::endl;
        fid = fopen(image.c_str(), "rb");
        int image_data_len = fread(data_buffer, 1, sizeof(data_buffer), fid);
        fclose(fid);

        if (img_file.find(" ") != string::npos) {
            img_file = img_file.substr(img_file.find(" "));
        }
//        u64_t label_id = atoi(img_file.c_str());

//        int k1 = image.find("00");
//        int k2 = image.find_first_of("0123456789");
//        int end = image.find_last_of("0123456789");
//        if (k1!=string::npos)
//            img_name=image.substr(k1,end-k1+1);
//        else
//            img_name=image.substr(k2,end-k2+1);
//        u64_t img_num = atoi(img_name.c_str());
//
//        u64_t info=0x0;
//        info|= channel_id;
//        info|= label_id << 8;
//        info|= img_num << 24;

        decinput.data_len = image_data_len;
        decinput.mem_addr = (u64_t)data_buffer;
        decinput.mem_type = CNCODEC_MEM_TYPE_HOST;
        decinput.pts = 0;

        // decinput.streamLength = image_data_len;
        // decinput.streamBuffer = (u8_t*)data_buffer;
        // decinput.flags           = 0;
        // decinput.pts             = 0;  // demo use chunk mode, so set pts info to 0

        // i32_t ret = cnjpegDecFeedData(hdecoder_[channel_idx_], &decinput, 4000);
        i32_t ret = cncodecDecSendStream(hdecoder_[thread_idx_], &decinput, 4000);
        assert(ret == CNCODEC_SUCCESS);
        usleep(1000);
    }

    // decinput.streamLength = 0;
    // decinput.streamBuffer = (u8_t*)data_buffer;
    // decinput.flags        = CNJPEGDEC_FLAG_EOS;
    // decinput.pts          = 0;
    // cnjpegDecFeedData(hdecoder_[channel_idx_], &decinput, 4000);

    cncodecDecSetEos(hdecoder_[thread_idx_]);

    return true;
}


// int CncodecImageProvider::newFrameCallback(cnjpegDecOutput *pDecOuput)
int CncodecImageProvider::newFrameCallback(cncodecFrame_t *pDecOuput)
{
    // int ret = cnjpegDecAddReference(hdecoder_[channel_idx_], &pDecOuput->frame);
    int ret = cncodecDecFrameRef(hdecoder_[thread_idx_], pDecOuput);
    assert(ret == CNCODEC_SUCCESS);

    enqueue_frame(pDecOuput);
    return 0;
}

// int decode_image_cb(cncodecCbEventType eventType, void *pUserData, void *cbInfo)
int decode_image_cb(cncodecEventType_t eventType, void *pUserData, void *cbInfo)
{
    auto *pContext = (CncodecImageProvider *)pUserData;
    switch (eventType) {
        case CNCODEC_EVENT_NEW_FRAME: {
            auto frame = reinterpret_cast<cncodecFrame_t *>(cbInfo);
            pContext->newFrameCallback(frame);
            break;
        }
        case CNCODEC_EVENT_EOS: {
            printf("CNCODEC_EVENT_EOS\n");
            break;
        }
        case CNCODEC_EVENT_STREAM_CORRUPT: {
            auto corrupt_info = reinterpret_cast<cncodecDecStreamCorruptInfo_t *>(cbInfo);
            printf("Error pts: %ld, Total num: %ld\n", corrupt_info->pts, corrupt_info->total_num);
            break;
        }
        case CNCODEC_EVENT_STREAM_NOT_SUPPORTED: {
            printf("Current stream is not supported \n");
            break;
        }
        default: {
            printf("invalid Event: %d \n", eventType);
            break;
        }
    }
    return CNCODEC_SUCCESS;
}

// bool CncodecImageProvider::dequeue_frame(cnjpegDecOutput* result){
bool CncodecImageProvider::dequeue_frame(cncodecFrame_t* result){
    cncodecFrame_t* frame;
    queue_->pop(frame);
    *result = *frame;
    delete frame;
    return true;
}

// bool CncodecImageProvider::config_attr(cnjpegDecCreateInfo *attr){
bool CncodecImageProvider::config_attr(cncodecDecCreateInfo_t *attr){
// -----------------配置decoder worker参数------------------ //
    memset(attr, 0, sizeof(cncodecDecCreateInfo_t));
    // attr->width               = 1920;
    // attr->height              = 1080;
    // attr->inputBufNum         = 3;
    // attr->outputBufNum        = 16;
    // attr->pixelFmt            = CNCODEC_PIX_FMT_NV12;
    // attr->bitDepthMinus8      = 0;
    // attr->userContext         = reinterpret_cast<void*>(this);
    // attr->instance            = CNJPEGDEC_INSTANCE_AUTO;
    // attr->allocType           = CNCODEC_BUF_ALLOC_LIB;

    attr->codec             = CNCODEC_JPEG;
    attr->device_id         = device_id_;
    attr->send_mode         = CNCODEC_DEC_SEND_MODE_FRAME;
    attr->stream_buf_size   = (8 << 20); // default JPEG max size
    attr->user_context      = this;
    attr->run_mode          = CNCODEC_RUN_MODE_ASYNC;

    return true;
}

bool CncodecImageProvider::config_params(cncodecDecParams_t *params){
    memset(params, 0, sizeof(cncodecDecParams_t));

    cncodecCropAttr_t     crop;
    cncodecScaleAttr_t    scale;
    memset(&crop, 0, sizeof(cncodecCropAttr_t));
    memset(&scale, 0, sizeof(cncodecScaleAttr_t));

    params->output_buf_num      = 30;
    params->output_buf_source   = CNCODEC_BUF_SOURCE_LIB;
    params->stride_align        = 64;
    params->color_space         = CNCODEC_COLOR_SPACE_BT_601;
    params->max_width           = 8192;
    params->max_height          = 4096;
    params->pixel_format        = CNCODEC_PIX_FMT_NV12;
    params->pp_attr.crop        = crop;
    params->pp_attr.scale       = scale;

    return true;
}

// bool CncodecImageProvider::create_decoder(cnjpegDecCreateInfo *attr, int channel_id){
bool CncodecImageProvider::create_decoder(cncodecDecCreateInfo_t *attr, int thread_id){
    i32_t ret = cncodecDecCreate(&(hdecoder_[thread_id]), decode_image_cb, attr);
    assert(ret == CNCODEC_SUCCESS);
    return true;
}

bool CncodecImageProvider::set_decoder_params(cncodecDecParams_t *params, int thread_id){
    i32_t ret = cncodecDecSetParams(hdecoder_[thread_id], params);
    assert(ret == CNCODEC_SUCCESS);
    return true;
}

bool CncodecImageProvider::init_cncodec(int thread_id){
    // cnjpegDecCreateInfo st_create_attr;
    cncodecDecCreateInfo_t st_create_attr;
    config_attr(&st_create_attr);
    create_decoder(&st_create_attr, thread_id);

    cncodecDecParams_t st_decode_params;
    config_params(&st_decode_params);
    set_decoder_params(&st_decode_params, thread_id);
    return true;
}

bool CncodecImageProvider::init_cnrt(){
    CNPEAK_CHECK_CNRT(cnrtSetDevice(device_id_));
    return true;
}


bool CncodecImageProvider::output_to_mlu(TData*& pdata_in, cncodecFrame_t* frame){
    void* mlu_y_ptr = reinterpret_cast<void*>(frame->plane[0].dev_addr);
    void* mlu_uv_ptr = reinterpret_cast<void*>(frame->plane[1].dev_addr);
    pdata_in->mlu_input = mlu_inputs_[mlu_mem_idx_];
    mlu_mem_idx_ = (mlu_mem_idx_ + 1) % mlu_mem_cap_;
    pdata_in->mlu_input[0] = mlu_y_ptr;
    pdata_in->mlu_input[1] = mlu_uv_ptr;
    pdata_in->s_w = ALIGN_UP(frame->width, 2);
    pdata_in->s_h = ALIGN_UP(frame->height, 2);
    return true;
}

bool CncodecImageProvider::output_to_cpu(TData*& pdata, cncodecFrame_t* frame){
    cv::Mat img;
    output_rgb_img(pdata, img, frame);
    pdata->origin_images.push_back(img);
    frame_id++;
    return true;
}

bool CncodecImageProvider::output_rgb_img(TData*& pdata, cv::Mat &img, cncodecFrame_t* frame){
    int h = ALIGN_UP(frame->height, 2);
    int w = ALIGN_UP(frame->width, 64);
    int w_origin = frame->width;
    // size_t stride[2];
    // stride[0] = frame->plane[0].stride;
    // stride[1] = frame->plane[1].stride;
    cv::Mat yuvimg(h / 2 * 3, w, CV_8UC1);
    img.create(h, w, CV_8UC3);

    void* y_ptr = reinterpret_cast<void*>(frame->plane[0].dev_addr);
    void* uv_ptr = reinterpret_cast<void*>(frame->plane[1].dev_addr);
    int size_y = frame->plane[0].stride * h;
    int size_uv = frame->plane[1].stride * h / 2;
    CNPEAK_CHECK_CNRT(cnrtMemcpy(yuvimg.data, y_ptr, size_y, CNRT_MEM_TRANS_DIR_DEV2HOST));
    CNPEAK_CHECK_CNRT(cnrtMemcpy((char*)yuvimg.data + size_y, uv_ptr, size_uv, CNRT_MEM_TRANS_DIR_DEV2HOST));

    cv::cvtColor(yuvimg, img, cv::COLOR_YUV2BGRA_NV12);
    img = img(cv::Rect(0,0,w_origin,h));

    if (is_dump_image_) {
        static int idx = 0;
        string image_name = "testcase/output/decode/" + (std::to_string)(idx++) + ".jpg";
        imwrite(image_name, img);
    }
    return true;
}


