#ifndef CNPEAK_PLUGIN_POST_CRNN_HPP
#define CNPEAK_PLUGIN_POST_CRNN_HPP

#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <memory>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include "core/cnPipe.hpp"
// #include "plugin/common_face/common_face.hpp"

using namespace cv;
using namespace std;

class PostCrnnProcesser: public Plugin{
public:
    PostCrnnProcesser(int infer_id=0);
    virtual bool init_in_main_thread();
    virtual bool init_in_sub_thread();
    virtual bool callback(TData *&pdata_in, vector<TData *>&pdatas_out);
    virtual string name();

private:
    bool post_processer(TData *&pdata_in);
    void log_softmax_cpu(float *logits, int max_len, int dict_len);
    void softmax_cpu(float *logits, int max_len, int dict_len);
    string labels2string(vector<int> &label);
    string B_operation(vector<int> &labels, int blank);
    void argmax(float *emission_log_prob, vector<int> &labels, int max_len, int dict_len);    
    string greedy_decode(float *emission_log_prob, int max_len, int dict_len);
    string get_result(TData *&pdata_in);
    vector<string> get_results(TData *&pdata_in);

private:
    int infer_id_;    
    cnOfflineInfo *cur_offline_;
};



#endif //CNPEAK_PLUGIN_POST_DBNET_HPP
