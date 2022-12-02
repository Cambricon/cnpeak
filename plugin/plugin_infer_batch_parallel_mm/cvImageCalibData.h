#ifndef CVIMAGE_CALIBDATA_H
#define CVIMAGE_CALIBDATA_H

#include <mm_calibrator.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

class CVImageCalibData : public magicmind::CalibDataInterface{
public:
    CVImageCalibData(const magicmind::Dims &shape,
                     const magicmind::DataType &data_type,
                     int max_samples,
                     std::vector<std::string> &data_paths) {
        shape_ = shape;
        data_type_ = data_type;
        batch_size_ = shape.GetDimValue(0);
        max_samples_ = max_samples;
        data_paths_ = data_paths;
        current_sample_ = 0;
        buffer_.resize(batch_size_ * shape.GetElementCount());
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
        int sz[] = {1, 3, min_side, min_side};
        for (auto data : temp_paths) {
            cv::Mat img_src = cv::imread(data);
            cv::Mat img_data(4, sz, CV_32FC4, ptr);
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

    int min_side = 416;

private:
    int process_img(cv::Mat src_img, cv::Mat &dst_img) {
        if (src_img.empty()) {
            std::cout << "no image data!" << std::endl;
            return -1;
        }
        cv::resize(src_img, src_img, cv::Size(min_side, min_side));

        // normlize
        src_img.convertTo(dst_img, CV_32F);
        cv::Scalar std(0.00392, 0.00392, 0.00392);
        cv::multiply(src_img, std, src_img);

        // swapBR
        cv::cvtColor(src_img, src_img, cv::COLOR_BGR2RGB);

        if (src_img.depth() != CV_32F)
        {
            src_img.convertTo(src_img, CV_32F);
        }

        // transpose
        cv::Mat ch[3];
        for (int j = 0; j < 3; j++)
        {
            ch[j] = cv::Mat(src_img.rows, src_img.cols, CV_32F, dst_img.ptr(0, j));
        }
        cv::split(src_img, ch);

        return 0;
    }
};

#endif
