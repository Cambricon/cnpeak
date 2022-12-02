//
// Created by cambricon on 19-7-16.
//

#ifndef CNPIPE_PLUGIN_VIDEO_GEN_HPP
#define CNPIPE_PLUGIN_VIDEO_GEN_HPP


#include "core/cnPipe.hpp"
#include "video_gen_server.hpp"

class VideoGener: public Plugin {
public:
    VideoGener(string outputname="output", int h=1080, int w=1920, int rate=20);
    virtual bool init_in_main_thread();
    virtual bool init_in_sub_thread();
    virtual bool callback(TData *&pdata_in, vector<TData *>&pdatas_out);
    virtual string name();

private:
    void Init();
    void Destroy();
    void Stop();
    void EventLoop(const std::function<void()> &quit_callback);


private:
    VideoGenServer* server_;
    BlockQueue<cv::Mat> *buf_;
    bool running_ = false;
    int h_;
    int w_;
    int frame_rate_;
    string output_name_;
};

#endif //CNPIPE_PLUGIN_VIDEO_GEN_HPP
