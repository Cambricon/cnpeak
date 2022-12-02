//
// Created by cambricon on 18-12-13.
//
#include "plugin_post_yolov5.hpp"
#include "core/tools.hpp"

const string input_path = "testcase/input/yolov3/";
const string output_path = "testcase/output/yolov3/";

Yolov5PostProcesser::Yolov5PostProcesser(string label_filename,
                            int min_side,
                            int infer_id,
                            bool is_draw,
                            bool is_show) {
    label_filename_ = label_filename;
    infer_id_ = infer_id;
    min_side_ = min_side;
    is_draw_ = is_draw;
    is_show_ = is_show;
}


string Yolov5PostProcesser::name(){
    return "Yolov5PostProcesser";
}

bool Yolov5PostProcesser::init_in_main_thread() {
    load_label_name();
    return true;
}

bool Yolov5PostProcesser::init_in_sub_thread() {
    cur_offline_ = pCtxt->offline[infer_id_][0];
    return true;
}

bool Yolov5PostProcesser ::callback(TData *&pdata_in, vector<TData *>&pdatas_out) {
    if (pdata_in == nullptr) {
        pdatas_out.push_back(pdata_in);
        return true;
    }
    postprocess(pdata_in);
    pdatas_out.push_back(pdata_in);
    return true;
}

// =============================================
bool Yolov5PostProcesser ::postprocess(TData*& pdata_in) {
    auto detections = getResults((float*)pdata_in->cpu_output[0], (int*)pdata_in->cpu_output[1]);
    vector<Rect> rects = adapt_box(detections, pdata_in->s_h, pdata_in->s_w);
    draw_rect(pdata_in, rects);

    // for(auto r : rects){
    //     printf("%d\t%d\t%d\t%d\n", r.x1, r.x2, r.y1, r.y2);
    // }


    return true;
}

void Yolov5PostProcesser::draw_rect(TData*& pdata_in, vector<Rect> rects){
//     // mark the object in the picture
    char buffer[50];
    cv::Mat img = pdata_in->origin_images[0];
    for (auto it : rects) {
        const uint32_t label = it.label;
        const float score = std::round(it.score * 1000) / 1000.0;
//        if (score < threshold_) {
//            continue;
//        }
        // std::string label_name = "Unknown";
        // if(label < labels_.size()) {
        //     label_name = labels_[label];
        // }
        float xmin, xmax, ymin, ymax;
        xmin = it.x1;
        ymin = it.y1;
        xmax = it.x2;
        ymax = it.y2;

        if (is_draw_){
            cv::Point top_left(xmin, ymin);
            cv::Point bottom_right(xmax, ymax);
            // CHECK_LT(label, colors_.size());
            // auto color = colors_[label];
            cv::Scalar color = 0;

            // rectangle
            rectangle(img, top_left, bottom_right, color, 1);
            cv::Point bottom_left(xmin, ymax);
            snprintf(buffer, sizeof(buffer), "%s: %.2f", label_name_map[label].c_str(), score);
            std::string text = buffer;
    //        double scale = 0.9 - (0.6 * channel_cnt / 32);
            double scale = 0.5;
            auto text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, scale, 1, nullptr);
            cv::rectangle(
                    img, bottom_left + cv::Point(0, 0),
                    bottom_left + cv::Point(text_size.width, -text_size.height),
                    color, CV_FILLED);
            cv::putText(img, text, bottom_left - cv::Point(0, 0),
                        cv::FONT_HERSHEY_SIMPLEX, scale, CV_RGB(255, 255, 255), 1, 8, false);
        }
    }

#if 0
    static int i=0;
    cv::imwrite(output_path+"/out"+std::to_string(i)+".jpg", img);
    i++;
#endif

    if(is_show_){
        cv::Mat img = pdata_in->origin_images[0];
        pdata_in->display_images.push_back(img);
    }
}

vector<vector<float>> Yolov5PostProcesser::getResults(float* bbox_data, int* bbox_num_data) {
    const size_t max_bbox = 1000;  // the maximum number of bboxes per image
    const size_t per_bbox_info_count = 7;  // batchid, classid, score, x, y, w, h
    size_t count = max_bbox * per_bbox_info_count;
    size_t num_boxes = bbox_num_data[0];
    vector<vector<float>> batch_box;
    if (num_boxes > max_bbox) return batch_box;
    for (size_t k = 0; k < num_boxes; ++k) {
        auto index = 0 * count + k * per_bbox_info_count;
        // if (bbox_data[index + 2] < threshold_) {
        //     continue;
        // }
        vector<float> single_box;
        auto bl = max(
                float(0), min(min_side_, bbox_data[index + 3]));  // x1
        auto br = max(
                float(0), min(min_side_, bbox_data[index + 5]));  // x2
        auto bt = max(
                float(0), min(min_side_, bbox_data[index + 4]));  // y1
        auto bb = max(
                float(0), min(min_side_, bbox_data[index + 6]));  // y2
        single_box.push_back(bl);
        single_box.push_back(bt);
        single_box.push_back(br);
        single_box.push_back(bb);
        single_box.push_back(bbox_data[index + 2]);
        single_box.push_back(bbox_data[index + 1]);
        if ((br - bl) > 0 && (bb - bt) > 0) {
            batch_box.push_back(single_box);
        }
    }
    return batch_box;
}

vector<Rect> Yolov5PostProcesser::adapt_box(vector<vector<float>> result, int src_h, int src_w) {
    auto scaling_factors = min(
            static_cast<float>(min_side_) / static_cast<float>(src_w),
            static_cast<float>(min_side_) / static_cast<float>(src_h));
    // std::cout << scaling_factors << "," << src_w << "," << src_h << std::endl;
    for (size_t j = 0; j < result.size(); j++) {
        result[j][0] =
        result[j][0] -
            static_cast<float>(min_side_ - scaling_factors * src_w) / 2.0;
        result[j][2] =
        result[j][2] -
            static_cast<float>(min_side_ - scaling_factors * src_w) / 2.0;
        result[j][1] =
        result[j][1] -
            static_cast<float>(min_side_ - scaling_factors * src_h) / 2.0;
        result[j][3] =
        result[j][3] -
            static_cast<float>(min_side_ - scaling_factors * src_h) / 2.0;

        for (int k = 0; k < 4; k++) {
            result[j][k] = result[j][k] / scaling_factors;
        }
    }

    for (size_t j = 0; j < result.size(); j++) {
        result[j][0] = result[j][0] < 0 ? 0 : result[j][0];
        result[j][2] = result[j][2] < 0 ? 0 : result[j][2];
        result[j][1] = result[j][1] < 0 ? 0 : result[j][1];
        result[j][3] = result[j][3] < 0 ? 0 : result[j][3];
        result[j][0] = result[j][0] > src_w ? src_w : result[j][0];
        result[j][2] = result[j][2] > src_w ? src_w : result[j][2];
        result[j][1] = result[j][1] > src_h ? src_h : result[j][1];
        result[j][3] = result[j][3] > src_h ? src_h : result[j][3];
    }
    vector<Rect> rects;
    for (size_t j = 0; j < result.size(); j++) {
        Rect r;
        r.x1 = static_cast<int>(result[j][0]);
        r.y1 = static_cast<int>(result[j][1]);
        r.x2 = static_cast<int>(result[j][2]);
        r.y2 = static_cast<int>(result[j][3]);
        r.score = result[j][4];
        r.label = static_cast<int>(result[j][5]);
        rects.push_back(r);
    }
    return rects;
}

void Yolov5PostProcesser::load_label_name() 
{
    if(!check_file_exist(label_filename_)){
        std::cout<<"coco_name file: " + label_filename_ + " does not exist.\n";
        exit(0);
    }
    std::ifstream in(label_filename_);
    if (!in){
        std::cout<<"failed to load coco_name file: " + label_filename_ + ".\n";
        exit(0);
    }
    std::string line;
    int index = 0;
    while (getline(in, line))
    {
        label_name_map[index] = line;
        index += 1;
    }
}


