#ifndef CNPEAK_PLUGIN_POST_DBNET_HPP
#define CNPEAK_PLUGIN_POST_DBNET_HPP

#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <math.h>

#include "core/cnPipe.hpp"

#define MAX_BOXES 100

using namespace cv;
using namespace std;

class PostDbnetProcesser: public Plugin{
public:
    PostDbnetProcesser(int infer_id=0);
    virtual bool init_in_main_thread();
    virtual bool init_in_sub_thread();
    virtual bool callback(TData *&pdata_in, vector<TData *>&pdatas_out);
    virtual string name();

private:
    // bool init_device();
    // bool init_context();
    // bool init_imageDesc(int src_width, int src_height, int dst_width, int dst_height);
    // bool init_mlu_mem();    
    bool post_processer(TData *&pdata_in);
    void get_results(Mat pred, vector<vector<Point2f>> &boxes, vector<float> &confidences, int width, int height, float threshold_);
    float contourScore(const Mat& pred, const vector<Point>& contour);
    void unclip(const vector<Point2f>& inPoly, vector<Point2f> &outPoly, float unclipRatio);
    void drawboxes(vector<vector<Point2f>> boxes, Mat img_show, float scaleHeight, float scaleWidth);
    void cropboxes(vector<vector<Point2f>> boxes, Mat img_show, float scaleHeight, float scaleWidth);
    // void cropboxes_mlu(vector<vector<Point2f>> boxes, TData *&pdata_in);

private:
    int infer_id_;
    cnOfflineInfo *cur_offline_;
};



#endif //CNPEAK_PLUGIN_POST_DBNET_HPP
