#ifndef COMMON_INFER_CVIMAGE_CALIBDATA_H
#define COMMON_INFER_CVIMAGE_CALIBDATA_H

#include <mm_calibrator.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

class CVImageCalibData : public magicmind::CalibDataInterface{
public:
    CVImageCalibData(const magicmind::Dims &shape,
                     const magicmind::DataType &data_type,
                     int max_samples,
                     const std::vector<std::string> &data_paths,
                     const std::string &config_file) {
        shape_ = shape;
        data_type_ = data_type;
        batch_size_ = shape.GetDimValue(0);
        max_samples_ = max_samples;
        data_paths_ = data_paths;
        current_sample_ = 0;
        buffer_.resize(batch_size_ * shape.GetElementCount());

        parse_json(config_file);
    }

    void *GetSample() { return buffer_.data(); }
    magicmind::Dims GetShape() const { return shape_; }
    magicmind::DataType GetDataType() const { return data_type_; }

    magicmind::Status Next() {
        if (current_sample_ + batch_size_ > max_samples_) {
            std::string msg = "sample number is bigger than max sample number!\n";
            magicmind::Status status_(magicmind::error::Code::OUT_OF_RANGE, msg);
            return status_;
        }

        auto data_size = sizeof(float) * shape_.GetElementCount();
        std::vector<std::string> temp_paths(data_paths_.begin() + current_sample_,
                                            data_paths_.begin() + current_sample_ + batch_size_);
        char* ptr = (char*)buffer_.data();
        int sz[] = {1, channel_, center_crop_size_[1], center_crop_size_[0]};
        for (auto data : temp_paths) {
            cv::Mat img_src = cv::imread(data);
            cv::Mat img_data(4, sz, CV_32F, ptr);
            process_img(img_src, img_data);
            ptr += data_size;
        }
        

        current_sample_ += batch_size_;
        return magicmind::Status::OK();
    }

    magicmind::Status Reset() {
        current_sample_ = 0;
        return magicmind::Status::OK();
    }

private:
    std::vector<cv::Mat> imgs_;
    magicmind::Dims shape_;
    magicmind::DataType data_type_;
    int batch_size_;
    int max_samples_;
    int current_sample_;
    std::vector<std::string> data_paths_;
    std::vector<float> buffer_;

    std::vector<int> resize_size_;
    std::vector<int> center_crop_size_;
    bool keep_aspect_ratio_     = false;
    bool need_bgr2rgb_          = false;
    bool need_transpose_        = false;
    std::string color_;
    std::vector<float> mean_;
    std::vector<float> std_;

    int channel_;

private:
    int process_img(cv::Mat src_img, cv::Mat &dst_img) {
        if (src_img.empty()) {
            std::cout << "no image data!" << std::endl;
            return -1;
        }

        // resize
        if (keep_aspect_ratio_) {
            float scale = std::min(float(resize_size_[0]) / src_img.cols, float(resize_size_[1]) / src_img.rows);
            int new_w = int(std::floor(float(src_img.cols) * scale));
            int new_h = int(std::floor(float(src_img.rows) * scale));

            cv::resize(src_img, src_img, cv::Size(new_w, new_h));

            int top, bottom, left, right;
            if (new_w % 2 != 0 && new_h % 2 == 0) {
                top = (resize_size_[1] - new_h) / 2;
                bottom = (resize_size_[1] - new_h) / 2;
                left = (resize_size_[0] - new_w) / 2 + 1;
                right = (resize_size_[0] - new_w) / 2;
            } else if (new_w % 2 == 0 && new_h % 2 != 0) {
                top = (resize_size_[1] - new_h) / 2 + 1;
                bottom = (resize_size_[1] - new_h) / 2;
                left = (resize_size_[0] - new_w) / 2;
                right = (resize_size_[0] - new_w) / 2;
            } else if (new_w % 2 == 0 && new_h % 2 == 0) {
                top = (resize_size_[1] - new_h) / 2;
                bottom = (resize_size_[1] - new_h) / 2;
                left = (resize_size_[0] - new_w) / 2;
                right = (resize_size_[0] - new_w) / 2;
            } else {
                top = (resize_size_[1] - new_h) / 2 + 1;
                bottom = (resize_size_[1] - new_h) / 2;
                left = (resize_size_[0] - new_w) / 2 + 1;
                right = (resize_size_[0] - new_w) / 2;
            }

            cv::copyMakeBorder(src_img, src_img, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(128, 128, 128));
        } else {
            cv::resize(src_img, src_img, cv::Size(resize_size_[0], resize_size_[1]));
        }

        // crop
        cv::Rect center_crop_rect(int(std::round((src_img.cols - center_crop_size_[0]) / 2.)), 
                                  int(std::round((src_img.rows - center_crop_size_[1]) / 2.)),
                                  center_crop_size_[0],
                                  center_crop_size_[1]);
        src_img = src_img(center_crop_rect);

        

        // bgr2rgb
        // if (need_bgr2rgb_) {
        //     cv::cvtColor(src_img, src_img, cv::COLOR_BGR2RGB);
        // }

        if (color_ == "rgb") {
            cv::cvtColor(src_img, src_img, cv::COLOR_BGR2RGB);
        } else if (color_ == "gray") {
            cv::cvtColor(src_img, src_img, cv::COLOR_BGR2GRAY);
        }

        if (!mean_.empty() || !std_.empty()) {
            src_img.convertTo(src_img, CV_32F);
        }

        // mean
        if (!mean_.empty()) {
            cv::Scalar s_mean;
            if (src_img.channels() == 1) {
                s_mean = cv::Scalar(mean_[0]);
            } else if (src_img.channels() == 3){
                s_mean = cv::Scalar(mean_[0], mean_[1], mean_[2]);
            }
            cv::subtract(src_img, s_mean, src_img);
        }

        // std
        if (!std_.empty()) {
            cv::Scalar s_std;
            if (src_img.channels() == 1) {
                s_std = cv::Scalar(1. / std_[0]);
            } else if (src_img.channels() == 3){
                s_std = cv::Scalar(1. / std_[0], 1. / std_[1], 1. / std_[2]);
            }
            cv::multiply(src_img, s_std, src_img);
        }

        // transpose
        if (need_transpose_) {
            cv::Mat ch[3];
            for (int j = 0; j < 3; j++)
            {
                ch[j] = cv::Mat(src_img.rows, src_img.cols, CV_32F, dst_img.ptr(0, j));
            }
            cv::split(src_img, ch);
        } else {
            dst_img = src_img.clone();
        }

        return 0;
    }

    bool parse_json(const std::string &JSON_file) {
        std::ifstream ifs(JSON_file);
        if (!ifs.is_open()) {
            std::cout << "Could not open file for reading!\n";
            return false;
        }
        rapidjson::IStreamWrapper isw(ifs);
        rapidjson::Document doc;
        doc.ParseStream(isw);

        if (doc.HasParseError()) {
            std::cout << "Error :" << doc.GetParseError() << '\n'
                      << "Offset :" << doc.GetErrorOffset() << '\n';
            return false;
        }

        if (doc.HasMember("resize_size") && doc["resize_size"].IsArray()) {
            const rapidjson::Value& array = doc["resize_size"];
            resize_size_.resize(2);
            resize_size_[0] = array[0].GetInt();
            resize_size_[1] = array[1].GetInt();
        }

        if (doc.HasMember("center_crop_size") && doc["center_crop_size"].IsArray()) {
            const rapidjson::Value& array = doc["center_crop_size"];
            center_crop_size_.resize(2);
            center_crop_size_[0] = array[0].GetInt();
            center_crop_size_[1] = array[1].GetInt();
        }

        if (doc.HasMember("mean") && doc["mean"].IsArray()) {
            const rapidjson::Value& array = doc["mean"];
            mean_.resize(array.Size());
            for (auto i = 0; i < array.Size(); ++i) {
                mean_[i] = array[i].GetFloat();
            }
        }

        if (doc.HasMember("std") && doc["std"].IsArray()) {
            const rapidjson::Value& array = doc["std"];
            std_.resize(array.Size());
            for (auto i = 0; i < array.Size(); ++i) {
                std_[i] = array[i].GetFloat();
            }
        }

        if (doc.HasMember("need_bgr2rgb") && doc["need_bgr2rgb"].IsBool()) {
            const rapidjson::Value& value = doc["need_bgr2rgb"];
            need_bgr2rgb_ = value.GetBool();
        }

        if (doc.HasMember("color") && doc["color"].IsString()) {
            const rapidjson::Value& value = doc["color"];
            color_ = value.GetString();

            if(color_ == "rgb" || color_ == "bgr") {
                channel_ = 3;
            } else if (color_ == "gray") {
                channel_ = 1;
            }
        }

        if (doc.HasMember("need_transpose") && doc["need_transpose"].IsBool()) {
            const rapidjson::Value& value = doc["need_transpose"];
            need_transpose_ = value.GetBool();
        }

        if (doc.HasMember("keep_aspect_ratio") && doc["keep_aspect_ratio"].IsBool()) {
            const rapidjson::Value& value = doc["keep_aspect_ratio"];
            keep_aspect_ratio_ = value.GetBool();
        }
        return true;
    }
};

#endif
