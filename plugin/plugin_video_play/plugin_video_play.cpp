//
// Created by cambricon on 19-4-3.
//
#include "plugin_video_play.hpp"

CvVideoPlayer::CvVideoPlayer(string winname) {
    winname_ = winname;
}


string CvVideoPlayer :: name(){
    return "CvVideoPlayer";
}

bool CvVideoPlayer :: init_in_main_thread() {
//    cv::namedWindow("video", CV_WINDOW_AUTOSIZE);
    return true;
}

bool CvVideoPlayer ::callback(TData *&pdata_in, vector<TData *>&pdatas_out) {
    if(pdata_in->display_images.size() > 0){
        cv::imshow(winname_, pdata_in->display_images[0]);
        cvWaitKey(50);
    }
    pdatas_out.push_back(pdata_in);
    return true;
}


// ================================= below is private function =========================
