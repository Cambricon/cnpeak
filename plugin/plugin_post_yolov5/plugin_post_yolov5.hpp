//
// Created by cambricon on 18-12-13.
//

#ifndef PLUGIN_POST_YOLOV5_HPP
#define PLUGIN_POST_YOLOV5_HPP


#include "core/cnPipe.hpp"

typedef struct rect{
    int x1;
    int y1;
    int x2;
    int y2;
    int label;
    float score;
}Rect;

class Yolov5PostProcesser: public Plugin{
public:
    Yolov5PostProcesser(string label_filename,
                             int min_side,
                             int infer_id=0,
                             bool is_draw=true,
                             bool is_show=true);
    virtual bool init_in_main_thread();
    virtual bool init_in_sub_thread();
    virtual bool callback(TData *&pdata_in, vector<TData *>&pdatas_out);
    virtual string name();

private:
    bool postprocess(TData*& pdata);
    vector<Rect> adapt_box(vector<vector<float>> result, int src_h, int src_w);
    vector<vector<float>> getResults(float* bbox_data, int* bbox_num_data);
    void draw_rect(TData*& pdata_in, vector<Rect> rects);
    void load_label_name();

private:
    vector<vector<float>> postProcess(float* rois, float* bbox,float* score);
    cnOfflineInfo *cur_offline_;
    int infer_id_;
    float min_side_;

    bool is_draw_;
    bool is_show_;
    string label_filename_;
    std::map<int, std::string> label_name_map;
    std::vector<std::string> labels_;
    std::vector<cv::Scalar> colors_;
    float threshold_;
};

#endif //PLUGIN_POST_YOLOV3_HPP
