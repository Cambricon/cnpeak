//
// Created by cambricon on 19-7-9.
//

#ifndef CNPIPE_PLUGIN_SRC_IMAGE_CNCODEC_HPP
#define CNPIPE_PLUGIN_SRC_IMAGE_CNCODEC_HPP


// #include "cn_codec_common.h"
// #include "cn_jpeg_dec.h"
#include "cncodec_v3_common.h"
#include "cncodec_v3_dec.h"
#include "core/cnPipe.hpp"
#include "core/tools.hpp"

#define ALIGN_UP(x, a) ((x + a - 1) & (~(a - 1)))

class CncodecImageProvider: public Plugin{
public:
    CncodecImageProvider(int pipe_id, 
                         string file_path, 
                         bool out_mlu,
                         bool out_cpu,
                         int pipe_num = 1,
                         int thread_num = 1, 
                         int device_id = 0,
                         int cap = 16);
    ~CncodecImageProvider();
    virtual bool init_in_main_thread();
    virtual bool init_in_sub_thread();
    virtual bool callback(TData *&pdata_in, vector<TData *>&pdatas_out);
    virtual string name();

public:
    // int newFrameCallback(cnjpegDecOutput *pDecOuput);
    int newFrameCallback(cncodecFrame_t *pDecOutput);

private:
    // bool enqueue_frame(cnjpegDecOutput *pimage);
    bool enqueue_frame(cncodecFrame_t *pimage);
    bool create_image_read_process(int channel_id);
    bool image_read_thread_start(int channel_id);
    bool parse_image_path();
    bool init_cncodec(int channel_id);
    bool init_cnrt();
    // bool config_attr(cnjpegDecCreateInfo *attr);
    bool config_attr(cncodecDecCreateInfo_t *attr);
    bool config_params(cncodecDecParams_t *params);
    // bool create_decoder(cnjpegDecCreateInfo *attr, int channel_id);
    bool create_decoder(cncodecDecCreateInfo_t *attr, int channel_id);
    bool set_decoder_params(cncodecDecParams_t *params, int channel_id);
    // bool dequeue_frame(cnjpegDecOutput* result);
    bool dequeue_frame(cncodecFrame_t* result);
    void malloc_mlu_buffer();
    // bool output_to_mlu(TData*& pdata_in, cnjpegDecOutput* frame);
    bool output_to_mlu(TData*& pdata_in, cncodecFrame_t* frame);
    // bool output_to_cpu(TData*& pdata, cnjpegDecOutput* frame);
    bool output_to_cpu(TData*& pdata, cncodecFrame_t* frame);
    // bool output_rgb_img(TData*& pdata, cv::Mat &img, cnjpegDecOutput* frame);
    bool output_rgb_img(TData*& pdata, cv::Mat &img, cncodecFrame_t* frame);

private:
    bool is_dump_image_;
    string file_path_;
    int frame_id;
    vector<string> img_list_;
    map<int,vector<string>> img_dict;
    int image_num_ = 0;
    int image_parsed_num_ = 0;

    int pipe_id_;
    int pipe_num_;

    // BlockQueue<cnjpegDecOutput*> *queue_;
    BlockQueue<cncodecFrame_t*> *queue_; 
    int batch_;

    int thread_num_;
    int thread_idx_;
    // vector<cnjpegDecoder>  hdecoder_;
    vector<cncodecHandle_t> hdecoder_;
    int device_id_;
    cnOfflineInfo *cur_offline_ = NULL;

    bool out_mlu_;
    bool out_cpu_;
    int mlu_mem_cap_;
    int mlu_mem_idx_;
    vector<void**> mlu_inputs_;
};


#endif //CNPIPE_PLUGIN_SRC_IMAGE_CNCODEC_HPP
