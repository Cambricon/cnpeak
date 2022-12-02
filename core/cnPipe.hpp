//
// Created by cambricon on 18-12-13.
//

#ifndef CNPIPE_HPP
#define CNPIPE_HPP


#include <algorithm>
#include <functional>
#include <condition_variable>  // NOLINT
#include <fstream>
#include <iomanip>
#include <iosfwd>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <thread>  // NOLINT
#include <utility>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <glog/logging.h>

#include "core/blockqueue.h"
#include "cnPublicContext.h"
#include "data.h"
#include "cnStruct.h"
#include "report.hpp"

using std::bind;
using std::function;
using std::map;
using std::max;
using std::min;
using std::vector;
using std::thread;
using std::string;
using std::stringstream;

class Plugin{
public:
    virtual bool init_in_main_thread() = 0;
    virtual bool init_in_sub_thread() { return true; };
    virtual bool callback(TData *&pdata_in, vector<TData *>&pdatas_out) = 0;
    // virtual bool callback(cnStruct *pdata_in, cnStruct *pdatas_out) = 0;
    virtual string name() = 0;
    virtual int data_id(TData *pdata);

    void set_plugin_id(int id);
    void set_pipe_id(int id);
    void set_context(cnPublicContext *ctxt);
    void report(BlockQueue<TReportInfo*> *report_queue, TData* pdata);
    void enable_report();
    bool check_placeholder(TData *pdata);

public:
    int queue_cap;
    BlockQueue<TData*> *input_queue;
    BlockQueue<TData*> *output_queue;

protected:
    cnPublicContext *pCtxt;
    int cur_pipe_id;

private:
    bool is_report = false;
    int total_count = 0;
    float delay;
    int num = 20;
    int cur_plugin_id;

};
//
//class PluginWrap{
//public:
//    PluginWrap() {};
//    void add(Plugin *p);
//
//public:
//    std::vector<Plugin*> plugins;
//    int queue_cap;
//    BlockQueue<TData*> *input_queue;
//    BlockQueue<TData*> *output_queue;
//};

class cnPipe{
public:
    cnPipe(int id, bool print=true, bool report=false);
    void add(Plugin *p, int queue_cap=2);
    void init();
    void start();
    void sync();

private:
    void start_thread(Plugin* p);
    void init_report(BlockQueue<TReportInfo *> *q);
    void record_start_time(TData* pdata);
    void record_end_time(TData* pdata);
    void print_time(int pipe_id, string name, TData* pdata);

private:
    vector<Plugin*> plugin_list_;
    vector<thread*> thread_list_;

    cnPublicContext ctxt;
    int pipe_id;

    bool is_report;
    bool is_print;
    BlockQueue<TReportInfo*> *report_queue;
    Report *server_;
    int plugin_num = 0;
};


class Timer {
public:
    Timer() { gettimeofday(&time, NULL); }
    void duration(int pipe_id, const char* name, int data_id) {
        struct timeval now;
        gettimeofday(&now, NULL);
        auto elapse = 1000000 * (now.tv_sec - time.tv_sec) + now.tv_usec - time.tv_usec;
        elapse /= 1000;
        std::cout   << std::left
                    << "[pipe-id]: " << std::setw(6) << pipe_id
                    << "[data-id]: " << std::setw(6) << data_id
                    << "[plugin]: " << std::setw(40) << name
                    << "[time]: " << elapse << "ms"
                    << std::endl;
    }
    void duration(const char* msg) {
        struct timeval now;
        gettimeofday(&now, NULL);
        auto elapse = 1000000 * (now.tv_sec - time.tv_sec) + now.tv_usec - time.tv_usec;
        elapse /= 1000;
        std::cout << "[time] " << msg << ": " << elapse << " ms" << std::endl;
    }
    float duration() {
        struct timeval now;
        gettimeofday(&now, NULL);
        auto elapse = 1000000 * (now.tv_sec - time.tv_sec) + now.tv_usec - time.tv_usec;
        elapse /= 1000;
        return elapse;
    }
    struct timeval time;
};


string get_rtsp_url(string file, int idx);

#endif //CNPIPE_HPP
