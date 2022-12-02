//
// Created by cambricon on 19-3-30.
//

#ifndef CNPIPE_PLUGIN_SRC_VIDEO_CNCODEC_HPP
#define CNPIPE_PLUGIN_SRC_VIDEO_CNCODEC_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
}
#endif

#include "cncodec_v3_common.h"
#include "cncodec_v3_dec.h"
#include "core/cnPipe.hpp"
#include "core/tools.hpp"

#define DEFAULT_STRIDE_ALIGN         64
#define DEFAULT_VIDEO_HEIGHT_ALIGN    1
typedef enum {
    video_end  = 0,
    video_not_ready = 1,
    video_ready  = 2,
}VIDEO_STATUS;


class CncodecVideoProvider: public Plugin{
public:
    CncodecVideoProvider(int channel_id, 
                         string file_path, 
                         int format,
                         bool out_mlu, 
                         bool out_cpu, 
                         bool loop, 
                         int tar_w, 
                         int tar_h, 
                         int fps = 0,
                         int device_id = 0);
    virtual bool init_in_main_thread();
    virtual bool init_in_sub_thread();
    virtual bool callback(TData *&pdata_in, vector<TData *>&pdatas_out);
    virtual string name();

public:
    bool enqueue_frame(cncodecFrame_t *pimage);
    bool start_decoder(cncodecDecSequenceInfo_t *pInfo);
    bool add_ref(cncodecFrame_t *pimage);
    bool del_ref(cncodecFrame_t *pimage);

private:
    bool create_ffmpeg_process();
    bool ffmpeg_thread_start();

    bool init_cncodec();
    bool init_ffmpeg();
    bool init_cnrt();

    bool config_attr(cncodecDecCreateInfo_t *attr);
    bool create_decoder(cncodecDecCreateInfo_t *attr);
    void release_mlu_buffer(cncodecFrame_t *frame);
    void release_ffmpeg_resource(cncodecStream_t* para);
    bool dequeue_frame(cncodecFrame_t* result);
    VIDEO_STATUS ffmpeg_read_frame(cncodecStream_t *pic);
    bool output_to_mlu(TData*& pdata, cncodecFrame_t* result);
    bool output_to_cpu(TData*& pdata, cncodecFrame_t* result);
    int get_released_buffer(unsigned long u32BufIndex);
    bool output_rgb_img(cv::Mat &img, cncodecFrame_t* result);
    bool output_yuv_img(cv::Mat &img, cncodecFrame_t* result);
    bool set_last_pic_flag(cncodecStream_t *pic);
    u32_t get_align_size(u32_t width);


private:
    string file_path_;
    int frame_id;
    double frame_interval_ms_ = 0.0;
    double timestamp_;
    // int channel_id_;

    AVFormatContext* av_ctxt_ = nullptr;
    AVBitStreamFilterContext* p_bsfc_ = nullptr;
    AVPacket packet_;
    int video_index_ = -1;
    bool first_frame_ = true;

    cv::Size resolution_ = cv::Size(0, 0);

    BlockQueue<cncodecFrame_t*> *queue_;
    int batch_;

    int buf_count_;
    int format_; /*0-yuv;  1-rgb*/
    bool out_mlu_;
    bool out_cpu_;

    u32_t tar_w_;
    u32_t tar_h_;
    bool loop_play_;
    int device_id_;

    vector<void**> mlu_inputs_;
    int mlu_mem_cap_;
    int mlu_mem_idx_;

    cnOfflineInfo *cur_offline_ = NULL;

    cncodecHandle_t decoder_;
    cncodecDecCreateInfo_t st_create_attr_;
    int input_buffer_num_;
    int output_buffer_num_;
    vector<cncodecFrame_t> cache_buffer_;
    int cache_buffer_num_;
};



#endif //CNPIPE_PLUGIN_SRC_VIDEO_CNCODEC_HPP
