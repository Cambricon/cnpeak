#include "plugin_post_crnn.hpp"

#define DEBUG 0

#if DEBUG//debug
#include <opencv2/opencv.hpp>
#endif

using namespace cv;
using namespace std;

PostCrnnProcesser::PostCrnnProcesser(int infer_id) {
    infer_id_ = infer_id;
}

string PostCrnnProcesser::name(){
    return "PostCrnnProcesser";
}

bool PostCrnnProcesser::init_in_main_thread() {
    return true;
}

bool PostCrnnProcesser::init_in_sub_thread() {
    cur_offline_ = pCtxt->offline[infer_id_][0];
    return true;
}

// void debug_dump_cpu_output2(TData*& pdata_in){
//     float *a = (float *)(pdata_in->cpu_output_multi_data[0][0]);
//     for(int i=0; i<20; i++){
//         cout << *(a+i) << endl;
//     }
//     cout << "----------------------------" << endl;
// }

bool PostCrnnProcesser::callback(TData *&pdata_in, vector<TData *>&pdatas_out) {
    if (pdata_in == nullptr) {
        pdatas_out.push_back(pdata_in);
        return true;
    }
#if DEBUG//debug
    cout << "start PostCrnnProcesser" << endl;
#endif
    post_processer(pdata_in);
    pdatas_out.push_back(pdata_in);
    return true;
}

// ================================= below is private function =========================
bool PostCrnnProcesser::post_processer(TData *&pdata_in) 
{
    // string results = get_result(pdata_in);    
    // cout << "********result:" << results[i] << endl;

    vector<string> results = get_results(pdata_in);
    for(int i=0; i<results.size(); ++i)
    {
        cout << "result[" << i << "]: " << results[i] << endl;
    }    
    return true;
}

void PostCrnnProcesser::log_softmax_cpu(float *logits, int max_len, int dict_len)
{
    for(int i=0; i<max_len; ++i)
    {
#if 0//debug
        cout << "i =" << i << endl;
#endif
        double sum_exp = 0;        
        float alpha = *max_element(logits+i*dict_len, logits+(i+1)*dict_len);
        for(int j=0; j<dict_len; ++j)
        {
            int idx = i*dict_len+j;            
            logits[idx] -= alpha;
            sum_exp += exp(logits[idx]);
        }
        double log_sum_exp = log(sum_exp);
        for(int j=0; j<dict_len; ++j)
        {            
            int idx = i*dict_len+j;
            logits[idx] -= log_sum_exp;
        }
    }
}

void PostCrnnProcesser::softmax_cpu(float *logits, int max_len, int dict_len)
{
    for(int i=0; i<max_len; ++i)
    {
        double sum_exp = 0;
        
        float alpha = *max_element(logits+i*dict_len, logits+(i+1)*dict_len);
        for(int j=0; j<dict_len; ++j)
        {
            int idx = i*dict_len+j;
            logits[i*dict_len+j] = exp(logits[i*dict_len+j] - alpha);            
            sum_exp += logits[idx];
        }

        for(int j=0; j<dict_len; ++j)
        {        
            int idx = i*dict_len+j;
            logits[idx] /= sum_exp;
        }
    }
}


string PostCrnnProcesser::labels2string(vector<int> &label)
{
    char dictionary[38] = "-0123456789abcdefghijklmnopqrstuvwxyz";
    string result = "";
    for(int i=0; i<label.size(); ++i)
    {
        result += dictionary[label[i]];
    }
    return result;
}

string PostCrnnProcesser::B_operation(vector<int> &labels, int blank=0)
{
    vector<int> new_labels;
    int previous = -1;
    for(int i=0; i<labels.size(); ++i)
    {
        if(labels[i] != previous)
        {
            previous = labels[i];
            if(labels[i] != blank)
            {
                new_labels.push_back(labels[i]);
                previous = labels[i];
            }                
        }
    }
    string result = labels2string(new_labels);
    return result;
}

void PostCrnnProcesser::argmax(float *emission_log_prob, vector<int> &labels, int max_len, int dict_len)
{
    for(int i=0; i<max_len; ++i)
    {
        // cout << "labels.size() =" << labels.size() << endl;
        int max_index = -1;
        int max_elem = -1;
        int index = i * dict_len;
        for(int j=0; j<dict_len; ++j)
        {

            if(emission_log_prob[index+j]>max_elem)
            {
                max_elem = emission_log_prob[index+j];
                max_index = j;
            }
#if 0//debug
            cout << "index+j =" << index+j << endl;
#endif
        }
#if 0//debug
        cout << "i =" << i << endl;
        cout << "max_index =" << max_index << endl;
#endif
        labels.push_back(max_index);
    }
}

string PostCrnnProcesser::greedy_decode(float *emission_log_prob, int max_len, int dict_len)
{
#if DEBUG//debug
    cout << "greedy_decode" << endl;
#endif    
    vector<int> labels;
    argmax(emission_log_prob, labels, max_len, dict_len);
    // argmax(emission_log_prob, max_len, dict_len);
#if DEBUG//debug
    cout << "argmax finish" << endl;
#endif
    // cout << "*****labels.size() =" << labels.size() << endl;
    // for(int i=0; i<labels.size(); ++i)
    // {
    //     cout << "*****labels["<< i << "]:" << labels[i] << endl;
    // }
    string result = B_operation(labels);
    // string result = B_operation();
    return result;
}

string PostCrnnProcesser::get_result(TData *&pdata_in)
{    
    int max_len = 24;
    int dict_len = 37;    
    float *logits = (float*)pdata_in->cpu_output[0];
    log_softmax_cpu(logits, max_len, dict_len);
    // softmax_cpu(logits, max_len, dict_len);
    for(int i=0; i<(max_len*dict_len); ++i)
    {
        printf("logits[%d]:%f\n", i, logits[i]);
    }      
    string result = greedy_decode(logits, max_len, dict_len);        
    return result;
}

vector<string> PostCrnnProcesser::get_results(TData *&pdata_in)
{    
    int max_len = 24;
    int dict_len = 37;
    // cout << "******get_results" << endl;
    // cout << "pdata_in->cpu_multi_outputs.size() =" << pdata_in->cpu_multi_outputs.size() << endl;
    vector<string> results;
    std::cout<<pdata_in->cpu_multi_outputs.size()<<std::endl;
    for (int i=0; i<pdata_in->cpu_multi_outputs.size(); i++) {
        auto logits = reinterpret_cast<float*>(pdata_in->cpu_multi_outputs[i][0]);
        // float *logits = (float*)pdata_in->cpu_output[0];
        log_softmax_cpu(logits, max_len, dict_len);
        // softmax_cpu(logits, max_len, dict_len);
        // for(int i=0; i<(max_len*dict_len); ++i)
        // {
        //     printf("logits[%d]:%f\n", i, logits[i]);
        // }      
        string result = greedy_decode(logits, max_len, dict_len);    
        // cout << "@@@@@@result:" << result << endl;
        results.push_back(result);
    }
    return results;
}
