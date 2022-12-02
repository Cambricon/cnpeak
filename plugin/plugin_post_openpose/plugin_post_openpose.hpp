//
// Created by cambricon on 18-12-13.
//

#ifndef PLUGIN_POST_OPENPOSE_HPP
#define PLUGIN_POST_OPENPOSE_HPP

#include "core/cnPipe.hpp"
#include "openpose_tools.h"

class OpenposePostProcess: public Plugin{
public:
    OpenposePostProcess(int infer_id, bool is_show = false, vector<int> baseSize = {656,368});
    virtual bool init_in_main_thread();
    virtual bool init_in_sub_thread();
    virtual bool callback(TData *&pdata_in, vector<TData *>&pdatas_out);
    virtual string name();

private:
    bool postprocess(TData*& pdata);

private:
    vector<vector<float>> postProcess(float* rois, float* bbox,float* score);
    cnOfflineInfo *cur_offline_;
    int infer_id_;

    bool is_draw_;
    bool is_show_;
    string label_filename_;
    std::map<int, std::string> label_name_map;
    std::vector<std::string> labels_;
    std::vector<cv::Scalar> colors_;
    float threshold_;

    BlobData* input_;
    BlobData* nms_out_;
    vector<int> baseSize_;
};

#endif //PLUGIN_POST_YOLOV3_HPP
