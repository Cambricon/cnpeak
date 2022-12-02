

#ifndef CNPIPE_SUPER_HPP
#define CNPIPE_SUPER_HPP

#include <mutex>
#include <thread>
#include <vector>
#include <sys/time.h>
#include <iomanip>
#include <unistd.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include "core/blockqueue.h"

typedef struct superInfo{
    int plugin_id;
    float delay;
    int total_count;
}TReportInfo;

class Report {
public:
    Report();
    static Report *create_report_server();
    bool start();
    void add_report_queue(BlockQueue<TReportInfo *> *q);

private:
    void main_loop();
    bool timeout();
    void output_result();
    void output_delay(int pipe_id);
    void output_count(int pipe_id);

private:
    bool inited_ = false;
    bool is_stop = false;
    std::mutex init_lock_;
    static Report* inst_;

    std::vector<BlockQueue<TReportInfo*>*> queue_list_;
    std::vector<std::vector<TReportInfo>> report_info_;
    struct timeval last_time;
    int timeout_;
    FILE *report_log_;
};

#endif
