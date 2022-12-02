//
// Created by cambricon on 19-7-16.
//

#include "video_gen_server.hpp"

VideoGenServer* VideoGenServer::inst_ = NULL;
VideoGenServer* VideoGenServer::create_video_gen_server(string output_name, int h, int w, int frame_rate){
    if(!inst_){
        inst_ = new VideoGenServer(output_name, h, w, frame_rate);
    }
    return inst_;
}

VideoGenServer::VideoGenServer(string output_name, int h, int w, int frame_rate) {
    h_ = h;
    w_ = w;
    frame_rate_ = frame_rate;
    output_name_ = output_name;
}


bool VideoGenServer::start() {
    init_lock_.lock();
    if(!inited_){
        init();
        calc_layout();
        new std::thread(&VideoGenServer::main_loop, this);
        inited_ = true;
    }
    init_lock_.unlock();
    return true;
}


bool VideoGenServer::calc_layout() {
    int channel_count = channel_bufs_.size();
    int square = std::ceil(std::sqrt(channel_count));
    row_ = col_ = square;
    while (static_cast<uint32_t>(row_ * col_) >= channel_count) {
        row_--;
    }
    row_++;

    channel_w_ = w_ / col_;
    channel_h_ = h_ / row_;

    last_times_.resize(channel_count);
    return true;
}

bool VideoGenServer::main_loop() {
    cv::Mat bg(h_, w_, CV_8UC3);
    while(true){
        if(is_stop_){
            break;
        }
        for(int i = 0, size = channel_bufs_.size();i < size; ++i) {
            cv::Mat display_image;
            cv::Mat resize_image;

            bool ret = channel_bufs_[i]->pop_non_block(display_image);
            if (!ret){
                std::cout << "skip one frame ==================" << std::endl;
                continue;
            }

            cv::resize(display_image, resize_image, cv::Size(channel_w_, channel_h_));

            // calculate fps
            auto now = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::micro> diff;
            diff = now - last_times_[i];
            last_times_[i] = now;
            float real_frame_rate = 1e6 / diff.count();
            string fps_str = "fps: " + std::to_string(real_frame_rate);
            double scale = 1 - (0.6 * size / 32);
            auto fps_size = cv::getTextSize(fps_str, cv::FONT_HERSHEY_SIMPLEX,
                                            scale, 1, nullptr);

            // 计算图片填充坐标
            const int x = channel_w_ * (i % col_);
            const int y = channel_h_ * (i / col_);

            // 填充图片
            cv::putText(resize_image, "fps: " + std::to_string(real_frame_rate),
                        cv::Point(resize_image.cols - fps_size.width, resize_image.rows - fps_size.height),
                        cv::FONT_HERSHEY_SIMPLEX, scale, cv::Scalar(0, 0, 255),
                        1, scale, false);

            cv::Rect rect(x, y, resize_image.cols, resize_image.rows);
            cv::Mat roi = bg(rect);

            resize_image.copyTo(roi);
        }
        usleep(100 * 1000);
        video_ << bg;
    }
    return true;
}
BlockQueue<cv::Mat> *VideoGenServer::create_queue(int cap){
    BlockQueue<cv::Mat> *q = new BlockQueue<cv::Mat>(cap);
    channel_bufs_.push_back(q);
    return q;
}

bool VideoGenServer :: init(){
    cv::Size size(w_, h_);
    video_ = cv::VideoWriter(output_name_, CV_FOURCC('D', 'I','V','X'), frame_rate_, size);
    return true;
}
