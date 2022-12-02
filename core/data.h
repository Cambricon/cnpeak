//
// Created by cambricon on 19-1-2.
//

#ifndef CNPIPE_DATA_H
#define CNPIPE_DATA_H

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using std::vector;
using std::string;

struct TData{
    int id;
    struct timeval start_time;
    struct timeval end_time;

    vector<string> origin_image_names;
    vector<cv::Mat> origin_images;
    vector<cv::Mat> parsed_images;
    vector<vector<cv::Mat>> split_images;
    vector<cv::Mat> display_images;
    vector<cv::Mat> display_faces;
    vector<cv::Mat> encode_images;

    void** cpu_input;
    void** cpu_output;
    void** mlu_input;
    void** mlu_output;

    vector<void*> mlu_multi_inputs;
    vector<void**> mlu_multi_outputs;
    vector<void**> cpu_multi_outputs;

    void** mlu_origin_image;
    void** mlu_crop_faces;

    int s_h, s_w;

    vector<void**> cpu_input_multi_model;
    vector<void**> cpu_output_multi_model;
    vector<void**> cpu_input_multi_data;
    vector<void**> cpu_output_multi_data;

    /* yolov3 postprocess */
    vector<vector<vector<float>>> detections;

    /* openpose pose postprocess */
    void* openpose_data;

    /* dbnet text boxes */
    vector<vector<cv::Point2f>> boxes;
    vector<cv::Mat> box_imgs;
};

#endif //CNPIPE_DATA_H
