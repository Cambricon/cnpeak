#ifndef PLUGIN_PLUGIN_POST_OPENPOSE_OPENPOSE_TOOLS_H
#define PLUGIN_PLUGIN_POST_OPENPOSE_OPENPOSE_TOOLS_H

#include <algorithm>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <functional>

#include <opencv2/opencv.hpp>

using std::string;

/* Pair (label, confidence) representing a prediction. */
typedef std::pair<string, float> Prediction;

// Use op::round/max/min for basic types (int, char, long, float, double, etc). Never with classes! std:: alternatives uses 'const T&' instead of 'const T' as argument.
// E.g. std::round is really slow (~300 ms vs ~10 ms when I individually apply it to each element of a whole image array (e.g. in floatPtrToUCharCvMat)

// Round functions
// Signed
template<typename T>
inline char charRound(const T a)
{
    return char(a+0.5f);
}

template<typename T>
inline signed char sCharRound(const T a)
{
    return (signed char)(a+0.5f);
}

template<typename T>
inline int intRound(const T a)
{
    return int(a+0.5f);
}

template<typename T>
inline long longRound(const T a)
{
    return long(a+0.5f);
}

template<typename T>
inline long long longLongRound(const T a)
{
    return (long long)(a+0.5f);
}

// Unsigned
template<typename T>
inline unsigned char uCharRound(const T a)
{
    return (unsigned char)(a+0.5f);
}

template<typename T>
inline unsigned int uIntRound(const T a)
{
    return (unsigned int)(a+0.5f);
}

template<typename T>
inline unsigned long ulongRound(const T a)
{
    return (unsigned long)(a+0.5f);
}

template<typename T>
inline unsigned long long uLongLongRound(const T a)
{
    return (unsigned long long)(a+0.5f);
}

// Max/min functions
template<typename T>
inline T fastMax(const T a, const T b)
{
    return (a > b ? a : b);
}

template<typename T>
inline T fastMin(const T a, const T b)
{
    return (a < b ? a : b);
}

template<class T>
inline T fastTruncate(T value, T min = 0, T max = 1)
{
    return fastMin(max, fastMax(min, value));
}

struct BlobData{
    int count;
    float* list;
    int num;
    int channels;
    int height;
    int width;
    int capacity_count;		//保留空间的元素个数长度，字节数请 * sizeof(float)
};

#define POSE_COCO_COLORS_RENDER_GPU \
    255.f, 0.f, 85.f, \
    255.f, 0.f, 0.f, \
    255.f, 85.f, 0.f, \
    255.f, 170.f, 0.f, \
    255.f, 255.f, 0.f, \
    170.f, 255.f, 0.f, \
    85.f, 255.f, 0.f, \
    0.f, 255.f, 0.f, \
    0.f, 255.f, 85.f, \
    0.f, 255.f, 170.f, \
    0.f, 255.f, 255.f, \
    0.f, 170.f, 255.f, \
    0.f, 85.f, 255.f, \
    0.f, 0.f, 255.f, \
    255.f, 0.f, 170.f, \
    170.f, 0.f, 255.f, \
    255.f, 0.f, 255.f, \
    85.f, 0.f, 255.f

const std::vector<float> POSE_COCO_COLORS_RENDER{ POSE_COCO_COLORS_RENDER_GPU };
const std::vector<unsigned int> POSE_COCO_PAIRS_RENDER{1, 2, 1, 5, 2, 3, 3, 4, 5, 6, 6, 7, 1, 8, 8, 9, 9, 10, 1, 11, 11, 12, 12, 13, 1, 0, 0, 14, 14, 16, 0, 15, 15, 17};
const unsigned int POSE_MAX_PEOPLE = 96;

//656x368
bool getScale(const cv::Mat& im, cv::Size baseSize = cv::Size(656, 368), float* scale = 0, int* pad_x = 0, int* pad_y = 0);

//根据得到的结果，连接身体区域
void connectBodyPartsCpu(std::vector<float>& poseKeypoints, const float* const heatMapPtr, const float* const peaksPtr,
    const cv::Size& heatMapSize, const int maxPeaks, const int interMinAboveThreshold,
    const float interThreshold, const int minSubsetCnt, const float minSubsetScore, const float scaleFactor, std::vector<int>& keypointShape);

//bottom_blob是输入，top是输出
void nms(BlobData* bottom_blob, BlobData* top_blob, float threshold);

void renderKeypointsCpu(cv::Mat& frame, const std::vector<float>& keypoints, std::vector<int> keyshape, const std::vector<unsigned int>& pairs,
    const std::vector<float> colors, const float thicknessCircleRatio, const float thicknessLineRatioWRTCircle,
    const float threshold, float scale, int pad_x, int pad_y);

void renderPoseKeypointsCpu(cv::Mat& frame, const std::vector<float>& poseKeypoints, std::vector<int> keyshape,
    const float renderThreshold, float scale, int pad_x, int pad_y, const bool blendOriginalFrame = true);

BlobData* createBlob_local(int num, int channels, int height, int width);

BlobData* createEmptyBlobData();

void releaseBlob_local(BlobData** blob);

#endif