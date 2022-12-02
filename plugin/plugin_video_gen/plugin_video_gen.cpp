//
// Created by cambricon on 19-7-16.
//


#include "plugin_video_gen.hpp"

VideoGener::VideoGener(string outputname, int h, int w, int frame_rate) {
    h_ = h;
    w_ = w;
    frame_rate_ = frame_rate;
    output_name_ = outputname;
}


string VideoGener :: name(){
    return "VideoGener";
}

bool VideoGener :: init_in_main_thread() {
    server_ = VideoGenServer::create_video_gen_server(output_name_, h_, w_, frame_rate_);
    buf_ = server_->create_queue();

    return true;
}

bool VideoGener :: init_in_sub_thread() {
    server_->start();
    return true;
}

bool VideoGener ::callback(TData *&pdata_in, vector<TData *>&pdatas_out) {
    for(auto img: pdata_in->display_images){
        buf_->push_non_block(img);
    }
    return true;
}
