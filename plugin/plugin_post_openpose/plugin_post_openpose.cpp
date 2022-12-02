//
// Created by cambricon on 18-12-13.
//
#include "plugin_post_openpose.hpp"
#include "core/tools.hpp"

OpenposePostProcess :: OpenposePostProcess(int infer_id, bool is_show, vector<int> baseSize){
    infer_id_ = infer_id;
    is_show_ = is_show;
    baseSize_ = baseSize;
}


string OpenposePostProcess :: name(){
    return "OpenposePostProcesser";
}

bool OpenposePostProcess :: init_in_main_thread() {
    // load_label_name();
    input_ = createBlob_local(1, 57, baseSize_[1], baseSize_[0]);
    nms_out_ = createBlob_local(1, 56, POSE_MAX_PEOPLE + 1, 3);
    return true;
}

bool OpenposePostProcess :: init_in_sub_thread() {
    cur_offline_ = pCtxt->offline[infer_id_][0];
    return true;
}

bool OpenposePostProcess ::callback(TData *&pdata_in, vector<TData *>&pdatas_out) {
    if (pdata_in == nullptr) {
        pdatas_out.push_back(pdata_in);
        return true;
    }
    postprocess(pdata_in);
    pdatas_out.push_back(pdata_in);
    return true;
}

// =============================================
bool OpenposePostProcess ::postprocess(TData*& pdata_in) {
    vector<float> keypoints;
    vector<int> shape;
    float scale = 0;
    int pad_x = 0, pad_y = 0;

    cv::Mat raw_image = pdata_in->origin_images[0].clone();
    getScale(raw_image, cv::Size(baseSize_[0], baseSize_[1]), &scale, &pad_x, &pad_y);
    //把heatmap给resize到约定大小
    for (int i = 0; i < cur_offline_->output_c[0]; ++i) {
        cv::Mat um(baseSize_[0], baseSize_[1], CV_32F, input_->list + baseSize_[0] * baseSize_[1] * i);

        //featuremap的resize插值方法很有关系
        resize(cv::Mat(cur_offline_->output_w[0], cur_offline_->output_h[0], CV_32F, 
            (float*)pdata_in->cpu_output[0] + cur_offline_->output_w[0] * cur_offline_->output_h[0] * i), 
            um, cv::Size(baseSize_[0], baseSize_[1]), 0, 0, CV_INTER_CUBIC);
    }

    //获取每个feature map的局部极大值
    nms(input_, nms_out_, 0.05);

    //得到局部极大值后，根据PAFs、points做部件连接
    connectBodyPartsCpu(keypoints, input_->list, nms_out_->list, cv::Size(baseSize_[0], baseSize_[1]), 
        POSE_MAX_PEOPLE, 9, 0.05, 3, 0.4, 1, shape);

    //printf("render to image.\n");
    //绘图，显示
    renderPoseKeypointsCpu(raw_image, keypoints, shape, 0.05, scale, pad_x, pad_y);

    pdata_in->display_images.push_back(raw_image);

    return true;
}