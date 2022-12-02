//
// Created by cambricon on 19-4-3.
//
#include "plugin_image_save_opencv.hpp"

CvImageSaver::CvImageSaver(string path) {
    save_path_ = path;
}


string CvImageSaver::name(){
    return "CvImageSaver";
}

bool CvImageSaver::init_in_main_thread() {
//    cv::namedWindow("video", CV_WINDOW_AUTOSIZE);
    return true;
}

bool CvImageSaver::callback(TData *&pdata_in, vector<TData *>&pdatas_out) {
    if (pdata_in == nullptr) {
        pdatas_out.push_back(pdata_in);
        return true;
    }
    // std::cout << "saver" << std::endl;
    if(pdata_in->display_images.size() > 0){
        stringstream ss;
        ss << save_path_ << "/" << "result_" << save_count_++ << ".jpg";
        // std::cout << ss.str() << std::endl;
        cv::imwrite(ss.str(), pdata_in->display_images[0]);
    }
    if (save_count_ == 99) {
        sleep(1);
    }
    pdatas_out.push_back(pdata_in);
    return true;
}


// ================================= below is private function =========================
