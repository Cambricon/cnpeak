#define _CRT_SECURE_NO_WARNINGS
#include "plugin_post_dbnet.hpp"

using namespace cv;
using namespace std;

PostDbnetProcesser::PostDbnetProcesser(int infer_id) {
    infer_id_ = infer_id;
}


string PostDbnetProcesser::name(){
    return "PostDbnetProcesser";
}


bool PostDbnetProcesser::init_in_main_thread() {
    return true;
}

bool PostDbnetProcesser::init_in_sub_thread() {
    // init_device();
    // init_context();
    // init_imageDesc(src_w_, src_h_, dst_w_, dst_h_);
    // init_mlu_mem();
    // init_workspace();
    cur_offline_ = pCtxt->offline[infer_id_][0];
    return true;
}

// bool PreDbnetProcesser::init_device()
// {
//     cnrtInit(0);
//     cnrtDev_t dev;
//     cnrtGetDeviceHandle(&dev, device_id_);
//     cnrtSetCurrentDevice(dev);
//     cnrtSetCurrentChannel((cnrtChannelType_t)(thread_id_ % 4));
//     return true;
// }

// bool PreDbnetProcesser::init_context()
// {
//     cnrtCreateQueue(&queue_);
//     cncvCreate(&cncv_handle_);
//     cncvSetQueue(cncv_handle_, queue_);
//     return true;
// }

// bool PreDbnetProcesser::init_imageDesc(int src_width, int src_height, int dst_width, int dst_height)
// {
//     // cncvDepth_t dst_depth = CNCV_DEPTH_8U;
//     // cncvPixelFormat dst_fmt = CNCV_PIX_FMT_RGB;
//     src_desc_ = new cncvImageDescriptor();
//     src_desc_->width = src_width;
//     src_desc_->height = src_height;
//     src_desc_->pixel_fmt = CNCV_PIX_FMT_NV12;
//     src_desc_->stride[0] =  src_width;
//     src_desc_->stride[1] = src_width;
//     src_desc_->depth = CNCV_DEPTH_8U;
//     src_desc_->color_space = CNCV_COLOR_SPACE_BT_601;

//     dst_desc_ = new cncvImageDescriptor();
//     dst_desc_->width = dst_width;
//     dst_desc_->height = dst_height;
//     dst_desc_->pixel_fmt = CNCV_PIX_FMT_BGR;
//     dst_desc_->stride[0] = dst_width * 3 * sizeof(char);
//     dst_desc_->depth = CNCV_DEPTH_8U;
//     dst_desc_->color_space = CNCV_COLOR_SPACE_BT_601;

//     mean_std_desc_ = new cncvImageDescriptor();
//     mean_std_desc_->width = dst_width;
//     mean_std_desc_->height = dst_height;
//     mean_std_desc_->pixel_fmt = CNCV_PIX_FMT_BGR;
//     mean_std_desc_->stride[0] = dst_width * 3 * sizeof(float);
//     mean_std_desc_->depth = CNCV_DEPTH_32F;
//     mean_std_desc_->color_space = CNCV_COLOR_SPACE_BT_601;

//     split_desc_ = new cncvImageDescriptor();
//     split_desc_->width = dst_width;
//     split_desc_->height = dst_height;
//     split_desc_->pixel_fmt = CNCV_PIX_FMT_GRAY;
//     split_desc_->stride[0] = dst_width * sizeof(float);
//     split_desc_->stride[1] = dst_width * sizeof(float);
//     split_desc_->stride[2] = dst_width * sizeof(float);
//     split_desc_->depth = CNCV_DEPTH_32F;
//     split_desc_->color_space = CNCV_COLOR_SPACE_BT_601;

//     src_roi_ = new cncvRect();
//     src_roi_->x = 0;
//     src_roi_->y = 0;
//     src_roi_->w = src_width;
//     src_roi_->h = src_height;

//     dst_roi_ = new cncvRect();
//     dst_roi_->x = 0;
//     dst_roi_->w = dst_width;
//     dst_roi_->h = dst_height;
//     dst_roi_->y = 0;
    
//     mean_roi_ = new cncvRect();
//     mean_roi_->x = 0;
//     mean_roi_->y = 0;
//     mean_roi_->w = dst_width;
//     mean_roi_->h = dst_height;
//     return true;
// }

// bool PreDbnetProcesser::init_mlu_mem()
// {
//     cnrtMalloc((void **)(&src_y_mlu_), sizeof(void*));
//     cnrtMalloc((void **)(&src_uv_mlu_), sizeof(void*));
//     cnrtMalloc((void **)(&rgb_mlu_), sizeof(void*));
//     cnrtMalloc((void **)(&mean_std_mlu_), sizeof(void*));
    
//     rgb_mlu_cpu_ = (void **)malloc(sizeof(void*));
//     mean_std_mlu_cpu_ = (void **)malloc(sizeof(void*));
//     cnrtMalloc(&(rgb_mlu_cpu_[0]), dst_w_*dst_h_*3*sizeof(char));
//     cnrtMemset(rgb_mlu_cpu_[0], 0, dst_w_*dst_h_*3*sizeof(char));
//     cnrtMalloc(&(mean_std_mlu_cpu_[0]), dst_w_*dst_h_*3*sizeof(float));

//     cnrtMemcpy(rgb_mlu_, rgb_mlu_cpu_, sizeof(void*), CNRT_MEM_TRANS_DIR_HOST2DEV);
//     cnrtMemcpy(mean_std_mlu_, mean_std_mlu_cpu_, sizeof(void*), CNRT_MEM_TRANS_DIR_HOST2DEV);

//     for(int i=0; i<mlu_mem_cap_; i++)
//     {
//         void** split_mlu;
//         cnrtMalloc((void **)(&split_mlu), 3*sizeof(void *));
        
//         void* split_mlu_data;
//         cnrtMalloc((&split_mlu_data), dst_w_*dst_h_*3*sizeof(float));
        
//         void** split_mlu_cpu = (void**)malloc(3*sizeof(void *));
//         int size = dst_w_*dst_h_*sizeof(float);
//         split_mlu_cpu[0] = split_mlu_data;
//         split_mlu_cpu[1] = (char*)split_mlu_data + size;
//         split_mlu_cpu[2] = (char*)split_mlu_data + size*2;
//         cnrtMemcpy(split_mlu, split_mlu_cpu, 3*sizeof(void*), CNRT_MEM_TRANS_DIR_HOST2DEV);

//         dst_split_mlu_.push_back(split_mlu);
//         dst_split_cpu_.push_back(split_mlu_cpu);
//     }
//     return true;
// }


void debug_dump_cpu_output2(TData*& pdata_in){
    float *a = (float *)(pdata_in->cpu_output_multi_data[0][0]);
    for(int i=0; i<20; i++){
        cout << *(a+i) << endl;
    }
    cout << "----------------------------" << endl;
}

bool PostDbnetProcesser::callback(TData *&pdata_in, vector<TData *>&pdatas_out) {
    if (pdata_in == nullptr) {
        pdatas_out.push_back(pdata_in);
        return true;
    }
    post_processer(pdata_in);
    pdatas_out.push_back(pdata_in);
    return true;
}

// =============================================
bool PostDbnetProcesser::post_processer(TData *&pdata_in) 
{
    int height = 736;
    int width = 896;
    Mat pred = Mat(height, width, CV_32F, (float*)pdata_in->cpu_output[0]);        
    // vector<vector<Point2f>> boxes;    
    vector<float> confidences;
    get_results(pred, pdata_in->boxes, confidences, width, height, 0.7);        
    // Scale ratio
    float scaleHeight = (float)(pdata_in->origin_images[0].size[0]) / (float)(height);
    float scaleWidth = (float)(pdata_in->origin_images[0].size[1]) / (float)(width);
    // cropboxes_mlu(boxes, pdata_in);
    cropboxes(pdata_in->boxes, pdata_in->origin_images[0], scaleHeight, scaleWidth);
    // drawboxes(pdata_in->boxes, pdata_in->origin_images[0], scaleHeight, scaleWidth);    
    // cout << "********boxes.size() =" << pdata_in->boxes.size() << endl;
    return true;
}

void PostDbnetProcesser::drawboxes(vector<vector<Point2f>> boxes, Mat img_show, float scaleHeight, float scaleWidth)
{    
    // drawContours(img_show, boxes, -1, Scalar(0, 255, 0), 1);
    for(int i=0; i<boxes.size(); ++i)
    {        
        line(img_show, Point((int)(boxes[i][0].x*scaleWidth), (int)(boxes[i][0].y*scaleHeight)), Point((int)(boxes[i][1].x*scaleWidth), (int)(boxes[i][1].y*scaleHeight)), Scalar(255, 0, 0), 1);
        line(img_show, Point((int)(boxes[i][1].x*scaleWidth), (int)(boxes[i][1].y*scaleHeight)), Point((int)(boxes[i][2].x*scaleWidth), (int)(boxes[i][2].y*scaleHeight)), Scalar(0, 255, 0), 1);
        line(img_show, Point((int)(boxes[i][2].x*scaleWidth), (int)(boxes[i][2].y*scaleHeight)), Point((int)(boxes[i][3].x*scaleWidth), (int)(boxes[i][3].y*scaleHeight)), Scalar(0, 0, 255), 1);
        line(img_show, Point((int)(boxes[i][3].x*scaleWidth), (int)(boxes[i][3].y*scaleHeight)), Point((int)(boxes[i][0].x*scaleWidth), (int)(boxes[i][0].y*scaleHeight)), Scalar(255, 255, 0), 1);
    }
    imwrite("img_show_boxes.jpg", img_show);
}

// void PostDbnetProcesser::cropboxes(TData *&pdata_in, vector<vector<Point2f>> boxes, Mat img_show, float scaleHeight, float scaleWidth)
void PostDbnetProcesser::cropboxes(vector<vector<Point2f>> boxes, Mat img_show, float scaleHeight, float scaleWidth)
{   
    int dst_w = 100;
    int dst_h = 32;
    Point2f dst_pts[4] = {
        Point2f(0, dst_h), 
        Point2f(0, 0), 
        Point2f(dst_w, 0), 
        Point2f(dst_w, dst_h)
        };
    string path = "testcase/input/image/quant_crnn/";
    ofstream file_list;
    file_list.open(path + "file_list", ios::out | ios::trunc );
    for(int i=0; i<boxes.size(); ++i)
    {        
        Point2f src_pts[4] = {
            Point2f(boxes[i][0].x*scaleWidth, boxes[i][0].y*scaleHeight),
            Point2f(boxes[i][1].x*scaleWidth, boxes[i][1].y*scaleHeight),
            Point2f(boxes[i][2].x*scaleWidth, boxes[i][2].y*scaleHeight),
            Point2f(boxes[i][3].x*scaleWidth, boxes[i][3].y*scaleHeight)
            };
        Mat M = getPerspectiveTransform(src_pts, dst_pts);
        Mat warped;                
        warpPerspective(img_show, warped, M, Size(dst_w, dst_h));        
        stringstream img_names;
        img_names << "warped_boxes_" << i << ".jpg";        
        file_list << path << img_names.str() << endl;
        imwrite(path + img_names.str(), warped);
    }    
}

// void PostDbnetProcesser::cropboxes_mlu(vector<vector<Point2f>> boxes, TData *&pdata_in)
// {   
//     int dst_w = 100;
//     int dst_h = 32;
//     cncvPoint dst_pts[] = {
//         cncvPoint(0, dst_h), 
//         cncvPoint(0, 0), 
//         cncvPoint(dst_w, 0), 
//         cncvPoint(dst_w, dst_h)
//         };
//     // string path = "testcase/input/image/quant_crnn/";
//     // ofstream file_list;
//     // file_list.open(path + "file_list", ios::out | ios::trunc );
//     for(int i=0; i<boxes.size(); ++i)
//     {        
//         cncvPoint src_pts[] = {
//             cncvPoint(boxes[i][0].x*scaleWidth, boxes[i][0].y*scaleHeight),
//             cncvPoint(boxes[i][1].x*scaleWidth, boxes[i][1].y*scaleHeight),
//             cncvPoint(boxes[i][2].x*scaleWidth, boxes[i][2].y*scaleHeight),
//             cncvPoint(boxes[i][3].x*scaleWidth, boxes[i][3].y*scaleHeight)
//             };
//         float M[9];
//         cncvGetPerspectiveTransform(src_pts, dst_pts, M);
//         Mat warped;      

//         cncvWarpPerspective(handle, 1, WARP_FORWARD, CNCV_PAD_CONSTANT, M);        
//     }    
// }

float PostDbnetProcesser::contourScore(const Mat& pred, const vector<Point>& contour)
{
    Rect rect = boundingRect(contour);
    int xmin = max(rect.x, 0);
    int xmax = min(rect.x + rect.width, pred.cols - 1);
    int ymin = max(rect.y, 0);
    int ymax = min(rect.y + rect.height, pred.rows - 1);

    Mat binROI = pred(Rect(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1));

    Mat mask = Mat::zeros(ymax - ymin + 1, xmax - xmin + 1, CV_8U);
    vector<Point> roiContour;
    for (size_t i = 0; i < contour.size(); i++) {
        Point pt = Point(contour[i].x - xmin, contour[i].y - ymin);
        roiContour.push_back(pt);
    }
    vector<vector<Point>> roiContours = {roiContour};
    fillPoly(mask, roiContours, Scalar(1));
    float score = mean(binROI, mask).val[0];
    return score;
}

void PostDbnetProcesser::unclip(const vector<Point2f>& inPoly, vector<Point2f> &outPoly, float unclipRatio)
{
    float area = contourArea(inPoly);
    float length = arcLength(inPoly, true);
    float distance = area * unclipRatio / length;

    size_t numPoints = inPoly.size();
    vector<vector<Point2f>> newLines;
    for (size_t i = 0; i < numPoints; i++)
    {
        vector<Point2f> newLine;
        Point pt1 = inPoly[i];
        Point pt2 = inPoly[(i - 1) % numPoints];
        Point vec = pt1 - pt2;
        float unclipDis = (float)(distance / norm(vec));
        Point2f rotateVec = Point2f(vec.y * unclipDis, -vec.x * unclipDis);
        newLine.push_back(Point2f(pt1.x + rotateVec.x, pt1.y + rotateVec.y));
        newLine.push_back(Point2f(pt2.x + rotateVec.x, pt2.y + rotateVec.y));
        newLines.push_back(newLine);
    }

    size_t numLines = newLines.size();
    for (size_t i = 0; i < numLines; i++)
    {
        Point2f a = newLines[i][0];
        Point2f b = newLines[i][1];
        Point2f c = newLines[(i + 1) % numLines][0];
        Point2f d = newLines[(i + 1) % numLines][1];
        Point2f pt;
        Point2f v1 = b - a;
        Point2f v2 = d - c;
        float cosAngle = (v1.x * v2.x + v1.y * v2.y) / (norm(v1) * norm(v2));

        if( fabs(cosAngle) > 0.7 )
        {
            pt.x = (b.x + c.x) * 0.5;
            pt.y = (b.y + c.y) * 0.5;
        }
        else
        {
            float denom = a.x * (float)(d.y - c.y) + b.x * (float)(c.y - d.y) +
                          d.x * (float)(b.y - a.y) + c.x * (float)(a.y - b.y);
            float num = a.x * (float)(d.y - c.y) + c.x * (float)(a.y - d.y) + d.x * (float)(c.y - a.y);
            float s = num / denom;

            pt.x = a.x + s*(b.x - a.x);
            pt.y = a.y + s*(b.y - a.y);
        }
        outPoly.push_back(pt);
    }
}

void PostDbnetProcesser::get_results(Mat pred, vector<vector<Point2f>> &boxes, vector<float> &confidences, int width, int height, float threshold_)
{    
    // Threshold
    Mat bitmap;
    threshold(pred, bitmap, threshold_, 255, THRESH_BINARY);
    // Scale ratio
    float scaleHeight = (float)(height) / (float)(pred.size[0]);
    float scaleWidth = (float)(width) / (float)(pred.size[1]);
    // Find contours
    vector< vector<Point> > contours;
    bitmap.convertTo(bitmap, CV_8U);
    // imwrite("bitmap.jpg", bitmap);
    findContours(bitmap, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);

    reverse(contours.begin(),contours.end());

    // Candidate number limitation
    size_t numCandidate = min((int)contours.size(), MAX_BOXES);
    for (size_t i = 0; i < numCandidate; i++)
    {
        vector<Point>& contour = contours[i];

        // Calculate text contour score
        confidences.emplace_back(contourScore(pred, contour));
        // if (contourScore(pred, contour) < polygonThreshold)
        //     continue;

        // Rescale
        vector<Point> contourScaled; contourScaled.reserve(contour.size());
        for (size_t j = 0; j < contour.size(); j++)
        {
            contourScaled.push_back(Point(int(contour[j].x * scaleWidth),
                                          int(contour[j].y * scaleHeight)));
        }
        
        // minArea() rect is not normalized, it may return rectangles with angle=-90 or height < width
        RotatedRect bounding_box = minAreaRect(contourScaled);        
        const float angle_threshold = 60;  // do not expect vertical text, TODO detection algo property
        bool swap_size = false;
        if (bounding_box.size.width < bounding_box.size.height)  // horizontal-wide text area is expected
            swap_size = true;
        else if (fabs(bounding_box.angle) >= angle_threshold)  // don't work with vertical rectangles
            swap_size = true;
        if (swap_size)
        {
            swap(bounding_box.size.width, bounding_box.size.height);
            if (bounding_box.angle < 0)
                bounding_box.angle += 90;
            else if (bounding_box.angle > 0)
                bounding_box.angle -= 90;
        }

        Point2f vertex[4];
        bounding_box.points(vertex);  // order: bl, tl, tr, br
        vector<Point2f> approx;
        for (int j = 0; j < 4; j++)
            approx.emplace_back(vertex[j]);
        vector<Point2f> polygon;
        unclip(approx, polygon, 3);
        boxes.push_back(polygon);
    }
}
