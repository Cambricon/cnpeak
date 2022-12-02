//
// Created by cambricon on 19-4-3.
//
#include "plugin_video_http.hpp"

HttpVideoPlayer::HttpVideoPlayer(int port, int timeout) {
    port_ = port;
    timeout_ = timeout;
}


string HttpVideoPlayer :: name(){
    return "HttpVideoPlayer";
}

bool HttpVideoPlayer :: init_in_main_thread() {
//    cv::namedWindow("video", CV_WINDOW_AUTOSIZE);
    wri_ = std::make_shared<Http_Server>(port_, timeout_);
    return true;
}

bool HttpVideoPlayer ::callback(TData *&pdata_in, vector<TData *>&pdatas_out) {
    if (pdata_in == nullptr) {
        pdatas_out.push_back(pdata_in);
        return true;
    }
    if(pdata_in->display_images.size() > 0){
        // cv::imshow(winname_, pdata_in->display_images[0]);
        // cvWaitKey(50);
        std::vector<int> param = std::vector<int>(2);
        param[0] = CV_IMWRITE_JPEG_QUALITY;
        param[1] = 60;
        cv::imencode(".jpg", pdata_in->display_images[0], buffer_, param);
        wri_.get()->send_image(reinterpret_cast<char*>(buffer_.data()), 
                               buffer_.size());
    }
    pdatas_out.push_back(pdata_in);

    return true;
}


// ================================= below is private function =========================
