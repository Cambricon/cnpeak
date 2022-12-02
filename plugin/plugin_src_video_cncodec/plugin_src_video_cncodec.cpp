//
// Created by cambricon on 19-3-30.
//

#include "plugin_src_video_cncodec.hpp"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

CncodecVideoProvider::CncodecVideoProvider(int channel_id,
                                           string file_path, 
                                           int format,
                                           bool out_mlu, 
                                           bool out_cpu, 
                                           bool loop, 
                                           int tar_w, 
                                           int tar_h, 
                                           int fps,
                                           int device_id) {
    // channel_id_ = channel_id;
    file_path_ = file_path;
    frame_id = 0;
    format_ = format;
    out_mlu_ = out_mlu;
    out_cpu_ = out_cpu;
    buf_count_ = 16;
    tar_w_ = tar_w;
    tar_h_ = tar_h;
    loop_play_ = loop;
    device_id_ = device_id;

    if (fps > 0) {
        frame_interval_ms_ = 1000. / (double)fps;
    }

    mlu_mem_cap_ = 20;
    mlu_mem_idx_ = 0;

    input_buffer_num_ = 4;
    output_buffer_num_ = mlu_mem_cap_;
    cache_buffer_num_ = 12;
}


string CncodecVideoProvider :: name(){
    return "CncodecVideoProvider";
}

bool CncodecVideoProvider :: init_in_main_thread() {
    queue_ = new BlockQueue<cncodecFrame_t*>(2);
    return true;
}

bool CncodecVideoProvider :: init_in_sub_thread() {
    batch_ = 1;
    for(int i=0; i<mlu_mem_cap_; i++){
        auto mlu_input = reinterpret_cast<void**>(malloc(sizeof(void*)*2));
        mlu_inputs_.push_back(mlu_input);
    }
    init_cnrt();
    create_ffmpeg_process();

    timeval ts;
    gettimeofday(&ts, 0);
    timestamp_ = (double)(ts.tv_sec * 1000 + (ts.tv_usec / 1000));

    return true;
}

bool CncodecVideoProvider::callback(TData *&pdata_in, vector<TData *>&pdatas_out) {
    if (pdata_in == nullptr) {
        pdatas_out.push_back(pdata_in);
        return true;
    }
    timeval ts;
    gettimeofday(&ts, 0);
    double timestamp_cur = (double)(ts.tv_sec * 1000 + (ts.tv_usec / 1000));
    double sleep_time = frame_interval_ms_ - (timestamp_cur - timestamp_);
    timestamp_ = timestamp_cur;
    std::cout << sleep_time << std::endl;
    if (sleep_time > 0) {
        usleep(sleep_time * 1000);
    }
    cncodecFrame_t result;
    bool ret = dequeue_frame(&result);
    if(!ret){
        pdata_in = nullptr;
        pdatas_out.push_back(pdata_in);
        return true;
    }
    pdata_in->s_w = result.width;
    pdata_in->s_h = result.height;
    release_mlu_buffer(&result);

    if(out_mlu_){
        // set mlu ptr only when first frame
        output_to_mlu(pdata_in, &result);
    }

    if(out_cpu_){
        output_to_cpu(pdata_in, &result);
    }

    pdatas_out.push_back(pdata_in);
    usleep(30 * 1000);
    return true;
}

bool CncodecVideoProvider::add_ref(cncodecFrame_t *pimage){
    cncodecDecFrameRef(decoder_, pimage);
    return true;
}

bool CncodecVideoProvider::del_ref(cncodecFrame_t *pimage){
    cncodecDecFrameUnref(decoder_, pimage);
    return true;
}

bool CncodecVideoProvider::enqueue_frame(cncodecFrame_t *pimage){
    if(pimage == nullptr){
        queue_->push(pimage);
        return true;
    }
    cncodecFrame_t *info = new cncodecFrame_t();
    *info = *pimage;
    queue_->push(info);
    return true;
}

// ================================= below is private function =========================y
bool CncodecVideoProvider::create_ffmpeg_process(){
    thread *th = new thread(&CncodecVideoProvider::ffmpeg_thread_start, this);
    return true;
}

bool CncodecVideoProvider::ffmpeg_thread_start(){
    init_ffmpeg();
    init_cnrt();
    init_cncodec();

    cncodecStream_t pic;
    while(true){
        VIDEO_STATUS s = ffmpeg_read_frame(&pic);
        if(s == video_end){
            // set_last_pic_flag(&pic);
            // cncodecDecSendStream(decoder_, &pic, -1);
            cncodecDecSetEos(decoder_);
            return true;
        }
        if(s == video_ready){
            cncodecDecSendStream(decoder_, &pic, -1);
            release_ffmpeg_resource(&pic);
        }
    }

    return true;
}

void CncodecVideoProvider::release_ffmpeg_resource(cncodecStream_t* stream) {
    if (p_bsfc_) {
        av_free((void*)stream->mem_addr);
    }
    av_packet_unref(&packet_);
}

VIDEO_STATUS CncodecVideoProvider::ffmpeg_read_frame(cncodecStream_t *stream){
    int ret = av_read_frame(av_ctxt_, &packet_);
    if(ret >= 0) {
        if (packet_.stream_index == video_index_) {
            auto vstream = av_ctxt_->streams[video_index_];
            if (first_frame_) {
                if (packet_.flags & AV_PKT_FLAG_KEY) first_frame_ = false;
            }
            if (!first_frame_) {
                if (p_bsfc_) {
                    av_bitstream_filter_filter(p_bsfc_, vstream->codec,
                                               NULL, &packet_.data, &packet_.size,
                                               packet_.data, packet_.size, 0);
                    stream->mem_addr = reinterpret_cast<u64_t>(packet_.data);
                    stream->data_len = (unsigned long)packet_.size;
                } else {
                    stream->mem_addr = reinterpret_cast<u64_t>(packet_.data);
                    stream->data_len = (unsigned long)packet_.size;
                }
                stream->pts = (uint64_t)frame_id;
                frame_id ++;
                return video_ready;
            }
        }
        av_packet_unref(&packet_);
    } else if(AVERROR_EOF == ret && loop_play_) {
        av_seek_frame(av_ctxt_, video_index_, av_ctxt_->streams[video_index_]->start_time, AVSEEK_FLAG_BACKWARD);
        frame_id = 0;
    } else if(AVERROR_EOF == ret && !loop_play_){
        return video_end;
    }
    return video_not_ready;
}


bool CncodecVideoProvider::start_decoder(cncodecDecSequenceInfo_t *pInfo) {
    
    cncodecDecParams_t params;
    memset(&params, 0, sizeof(params));
    params.max_width = pInfo->coded_width;
    params.max_height = pInfo->coded_height;
    params.output_buf_num = mlu_mem_cap_;//pInfo->min_output_buf_num*3;
    params.pixel_format = CNCODEC_PIX_FMT_NV12;
    params.color_space = CNCODEC_COLOR_SPACE_BT_601;
    params.stride_align = DEFAULT_STRIDE_ALIGN;
    params.dec_mode = CNCODEC_DEC_MODE_IPB;
    params.output_order = CNCODEC_DEC_OUTPUT_ORDER_DISPLAY;
    params.output_buf_source = CNCODEC_BUF_SOURCE_LIB;
    // params.pp_attr.crop = user_data->options.crop;
    // params.pp_attr.scale = user_data->options.scale;
    cncodecDecSetParams(decoder_, &params);

    return true;
}

int event_callback(cncodecEventType_t EventType, void *pContext, void *pOut) {
    auto inst = (CncodecVideoProvider *)pContext;
    switch (EventType) {
        case CNCODEC_EVENT_NEW_FRAME:
            inst->add_ref((cncodecFrame_t *)pOut);
            inst->enqueue_frame((cncodecFrame_t *)pOut);
            break;
        case CNCODEC_EVENT_SEQUENCE:
            inst->start_decoder((cncodecDecSequenceInfo_t *)pOut);
            break;
        case CNCODEC_EVENT_EOS:
            inst->enqueue_frame(nullptr);
            break;
        default:
            break;
    }
    return 0;
}

bool CncodecVideoProvider::dequeue_frame(cncodecFrame_t* result){
    cncodecFrame_t* frame;
    queue_->pop(frame);
    if(frame == nullptr) return false;
    *result = *frame;
    delete frame;
    return true;
}

void CncodecVideoProvider::release_mlu_buffer(cncodecFrame_t *frame){
    if((int)(cache_buffer_.size()) >= cache_buffer_num_){
        auto rel_frame = cache_buffer_[0];
        del_ref(&rel_frame);
        cache_buffer_.erase(cache_buffer_.begin());
    }
    cache_buffer_.push_back(*frame);
}

bool CncodecVideoProvider::config_attr(cncodecDecCreateInfo_t *attr){
    memset(attr, 0, sizeof(cncodecDecCreateInfo_t));

    attr->device_id = 0;
    attr->codec = CNCODEC_H264;
    attr->send_mode = CNCODEC_DEC_SEND_MODE_FRAME;
    attr->run_mode = CNCODEC_RUN_MODE_ASYNC;
    attr->stream_buf_size = 12 << 20;
    attr->user_context = reinterpret_cast<void*>(this);
    return true;
}

u32_t CncodecVideoProvider::get_align_size(u32_t width){
    if(width % 128 == 0) return 128;
    if(width % 64 == 0) return 64;
    if(width % 32 == 0) return 32;
    return 16;
}
bool CncodecVideoProvider::create_decoder(cncodecDecCreateInfo_t *attr){
    int r = cncodecDecCreate(&decoder_, event_callback, attr);
    if(r == CNCODEC_SUCCESS){
        return true;
    }
    return false;
}

bool CncodecVideoProvider::init_cncodec(){
    config_attr(&st_create_attr_);
    create_decoder(&st_create_attr_);
    return true;
}

bool CncodecVideoProvider::init_cnrt(){
    CNPEAK_CHECK_CNRT(cnrtSetDevice(device_id_));
    return true;
}


bool CncodecVideoProvider::init_ffmpeg(){
    avcodec_register_all();
    av_register_all();
    avformat_network_init();
    av_ctxt_ = avformat_alloc_context();
    // options
    AVDictionary *options = NULL;
    av_dict_set(&options, "buffer_size", "1024000", 0);
    av_dict_set(&options, "stimeout", "200000", 0);

    int ret_code = avformat_open_input(&av_ctxt_, file_path_.c_str(), NULL, &options);
    if (0 != ret_code) {
        printf("couldn't open input stream.\n");
        return false;
    }
    // find video stream information
    ret_code = avformat_find_stream_info(av_ctxt_, NULL);
    if (ret_code < 0) {
        printf("couldn't find stream information.\n");
        return false;
    }
    video_index_ = -1;
    AVStream* vstream = nullptr;
    for (uint32_t iloop = 0; iloop < av_ctxt_->nb_streams; iloop++) {
        vstream = av_ctxt_->streams[iloop];
        if (vstream->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index_ = iloop;
            break;
        }
    }
    if (video_index_ == -1) {
        printf("didn't find a video stream.\n");
        return false;
    }
    auto codec_id = vstream->codecpar->codec_id;

    p_bsfc_ = nullptr;
    if (strstr(av_ctxt_->iformat->name, "mp4") ||
        strstr(av_ctxt_->iformat->name, "flv") ||
        strstr(av_ctxt_->iformat->name, "matroska") ||
        strstr(av_ctxt_->iformat->name, "rtsp")) {
        if (AV_CODEC_ID_H264 == codec_id) {
            p_bsfc_ = av_bitstream_filter_init("h264_mp4toannexb");
        } else if (AV_CODEC_ID_HEVC == codec_id) {
            p_bsfc_ = av_bitstream_filter_init("hevc_mp4toannexb");
        }
    }
    // get resolution
    auto width = av_ctxt_->streams[video_index_]->codecpar->width;
    auto height = av_ctxt_->streams[video_index_]->codecpar->height;
    resolution_.width = width;
    resolution_.height = height;
    return true;
}

bool CncodecVideoProvider::output_to_mlu(TData*& pdata_in, cncodecFrame_t* pFrame){
    assert(tar_h_ <= pFrame->height);
    assert(tar_w_ <= pFrame->width);

    void* mlu_y_ptr = reinterpret_cast<void*>(pFrame->plane[0].dev_addr);
    void* mlu_uv_ptr = reinterpret_cast<void*>(pFrame->plane[1].dev_addr);
    pdata_in->mlu_input = mlu_inputs_[mlu_mem_idx_];
    mlu_mem_idx_ = (mlu_mem_idx_ + 1) % mlu_mem_cap_;
    pdata_in->mlu_input[0] = mlu_y_ptr;
    pdata_in->mlu_input[1] = mlu_uv_ptr;
    return true;
}

bool CncodecVideoProvider::output_to_cpu(TData*& pdata_in, cncodecFrame_t* frame){
    cv::Mat img;
    if(format_ == 0){
        output_yuv_img(img, frame);
    }
    else if(format_ == 1){
        output_rgb_img(img, frame);
    }

    pdata_in->origin_images.push_back(img);
    pdata_in->origin_image_names.push_back(file_path_ + "-" + std::to_string(frame_id));
//    pdata_in->display_images.push_back(img);
    return true;
}

bool CncodecVideoProvider::output_rgb_img(cv::Mat &img, cncodecFrame_t* result){
//    img.create(tar_h_, tar_w_, CV_8UC3);
//
//    void* mlu_ptr = reinterpret_cast<void*>(frame->u64VirAddr);
//    cnrtMemcpy(img.data, mlu_ptr, frame->u32FrameSize, CNRT_MEM_TRANS_DIR_DEV2HOST);
    return true;
}

bool CncodecVideoProvider::output_yuv_img(cv::Mat &img, cncodecFrame_t* pFrame){
    int h = tar_h_ / 2 * 3;
    int w = tar_w_;
    img.create(tar_h_, tar_w_, CV_8UC3);
    cv::Mat yuvimg(h, w, CV_8UC1);

    int size_y, size_uv;
    size_y = pFrame->plane[0].stride*tar_h_;
    cnrtMemcpy(yuvimg.data, reinterpret_cast<void*>(pFrame->plane[0].dev_addr), size_y, CNRT_MEM_TRANS_DIR_DEV2HOST);

    size_uv = pFrame->plane[1].stride*tar_h_/2;
    cnrtMemcpy(yuvimg.data+size_y, reinterpret_cast<void*>(pFrame->plane[1].dev_addr), size_uv, CNRT_MEM_TRANS_DIR_DEV2HOST);

    cv::cvtColor(yuvimg, img, cv::COLOR_YUV2BGR_NV12);

#if 0
    static int i=0;
    cv::imwrite("testcase/output/decode/out"+std::to_string(i)+".jpg", img);
    i++;
#endif

    return true;
}

int CncodecVideoProvider::get_released_buffer(unsigned long u32BufIndex){
    return (u32BufIndex + 5 * batch_) %  (batch_ * buf_count_);
}

bool CncodecVideoProvider::set_last_pic_flag(cncodecStream_t *pic) {
    // pic->streamLength = 0;
    // pic->flags = CNVIDEODEC_FLAG_EOS;
    return true;
}
