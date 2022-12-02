//
// Created by cambricon on 19-7-16.
//

#ifndef CNPIPE_VIDEO_GEN_SERVER_HPP
#define CNPIPE_VIDEO_GEN_SERVER_HPP


#include <vector>
#include "core/cnPipe.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

class VideoGenServer {
public:
    VideoGenServer(string output, int h, int w, int frame_rate);
    static VideoGenServer* create_video_gen_server(string output, int h, int w, int frame_rate);
    bool init();
    bool start();
    bool destroy();
    bool stop();
    bool main_loop();
    bool calc_layout();
    BlockQueue<cv::Mat> *create_queue(int cap = 4);

private:
    bool inited_ = false;
    std::mutex init_lock_;
    static VideoGenServer* inst_;
//    int channel_num_;
    vector<BlockQueue<cv::Mat> *> channel_bufs_;

    cv::VideoWriter video_;
    int h_;
    int w_;
    int frame_rate_;

    int row_;
    int col_;
    int channel_w_;
    int channel_h_;
    bool is_stop_ = false;
    string output_name_;

    std::vector<std::chrono::time_point
            <std::chrono::high_resolution_clock>> last_times_;
};  // class SDLVideoPlayer



#endif //CNPIPE_VIDEO_GEN_SERVER_HPP
