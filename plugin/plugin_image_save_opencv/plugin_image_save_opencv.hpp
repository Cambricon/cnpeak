//
// Created by cambricon on 19-4-3.
//

#ifndef PLUGIN_IMAGE_SAVE_OPENCV_HPP
#define PLUGIN_IMAGE_SAVE_OPENCV_HPP


#include <vector>
#include <opencv2/opencv.hpp>
#include "core/cnPipe.hpp"


class CvImageSaver: public Plugin {
public:
    CvImageSaver(string winname = "video");
    virtual bool init_in_main_thread();
    virtual bool callback(TData *&pdata_in, vector<TData *>&pdatas_out);
    virtual string name();

//private:
//    inline void set_frame_rate(int frame_rate) {
//        fr_ = frame_rate;
//    }
//    inline int frame_rate() const {return fr_;}
//    inline void set_window_w(int w) {ww_ = w;}
//    inline int window_w() const {return ww_;}
//    inline void set_window_h(int h) {wh_ = h;}
//    inline int window_h() const {return wh_;}
//    inline void set_data_factory(const std::function<std::vector<UpdateData>()> &factory) {
//        data_factory_ = factory;
//    }
//    inline std::function<std::vector<UpdateData>()> data_factory() const {
//        return data_factory_;
//    }
    inline bool running() const {return running_;}
//    inline SDL_Window* window() const {return window_;}

private:
    string save_path_;
    int save_count_{0};
    bool running_ = false;
};




#endif //PLUGIN_VIDEO_PLAY_HPP
