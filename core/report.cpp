

#include "report.hpp"


using namespace std;

Report* Report::inst_ = NULL;
Report* Report::create_report_server(){
    if(!inst_){
        inst_ = new Report();
    }
    return inst_;
}

Report::Report() {
    timeout_ = 30; //s

    string name = "report.txt";
    report_log_ = fopen(name.c_str(), "w");
}


bool Report::start() {
    init_lock_.lock();
    if(!inited_){
        new std::thread(&Report::main_loop, this);
        inited_ = true;
    }
    init_lock_.unlock();
    return true;
}

void Report::main_loop(){
    gettimeofday(&last_time, NULL);
    report_info_.resize(queue_list_.size());
    while (!is_stop){
        for(size_t i=0; i<queue_list_.size(); i++){
            TReportInfo* info = NULL;
            queue_list_[i]->pop_non_block(info);

            if(info){
                if((size_t)(info->plugin_id + 1) > report_info_[i].size()){
                    report_info_[i].resize(info->plugin_id+1);
                }
                report_info_[i][info->plugin_id].plugin_id = info->plugin_id;
                report_info_[i][info->plugin_id].delay = info->delay;
                report_info_[i][info->plugin_id].total_count = info->total_count;
                delete info;
            }
        }
        if(timeout()){
            output_result();
        }
    }
}

bool Report::timeout(){
    struct timeval now;
    gettimeofday(&now, NULL);
    float elapse = 1000000 * (now.tv_sec - last_time.tv_sec) + now.tv_usec - last_time.tv_usec;
    elapse /= 1000 * 1000;
    if(elapse > timeout_){
        last_time = now;
        return true;
    }else{
        usleep(10);
        return false;
    }
}


void Report::output_result(){
    int plugnum = 10 + 5;
    for(int i=0; i<plugnum; i++){
        fprintf(report_log_, "=========");
    }
    fprintf(report_log_, "\n");

    for(size_t i=0; i<report_info_.size(); i++){
        output_delay(i);
    }
    for(int i=0; i<plugnum; i++){
        fprintf(report_log_, "---------");
    }
    fprintf(report_log_, "\n");

    for(size_t i=0; i<report_info_.size(); i++){
        output_count(i);
    }
}

void Report::output_delay(int pipe_id){
    std::stringstream out;
    out.flags(ios::left);
    out << "pipe-" << setw(3) << pipe_id << "| delay(ms)|";
    out.flags(ios::right);
    for(size_t i=0; i<report_info_[pipe_id].size(); i++){
        out << setw(8) << report_info_[pipe_id][i].delay << "|";
    }
    out << endl;
    fprintf(report_log_, "%s", out.str().c_str());
}

void Report::output_count(int pipe_id){
    std::stringstream out;
    out.flags(ios::left);
    out << "pipe-" << setw(3) << pipe_id << "|     count|";
    out.flags(ios::right);
    for(size_t i=0; i<report_info_[pipe_id].size(); i++){
        out << setw(8) << report_info_[pipe_id][i].total_count << "|";
    }
    out << endl;
    fprintf(report_log_, "%s", out.str().c_str());
}

void Report::add_report_queue(BlockQueue<TReportInfo *> *q){
    queue_list_.push_back(q);
}

