//
// Created by cambricon on 19-1-21.
//


#include "plugin_id_gen.hpp"

static int total_num = 0;
static int inc_num = 0;
static std::mutex idgen_lock;
static struct timeval idgen_begin;
static struct timeval idgen_inc_begin;
static bool inited = false;

IdGenerator :: IdGenerator() {
    cur_id_ = 0;
    cur_offline_ = NULL;
}

string IdGenerator :: name() {
    return "IdGenerator";
}

bool IdGenerator :: init_in_main_thread() {
    return true;
}

bool IdGenerator :: init_in_sub_thread() {
    idgen_lock.lock();
    if(pCtxt->offline.size() > 0){
        cur_offline_ = pCtxt->offline[0][0];
    }

    if(!inited){
        inited = true;
        gettimeofday(&idgen_begin, NULL);
        gettimeofday(&idgen_inc_begin, NULL);
    }
    idgen_lock.unlock();
    return true;
}

bool IdGenerator ::callback(TData *&pdata_in, vector<TData *>&pdatas_out) {
    if (pdata_in == nullptr) {
        pdatas_out.push_back(pdata_in);
        return true;
    }
    pdata_in->id = cur_id_;
    cur_id_++;
    add();

    pdatas_out.push_back(pdata_in);
    return true;
}


// =========================== private function ===============================


bool IdGenerator ::add() {
    idgen_lock.lock();
    total_num += 1;
    inc_num += 1;
    if(total_num % 100 == 0){
        print_fps();
    }
    idgen_lock.unlock();
    return true;
}

bool IdGenerator ::print_fps() {
    struct timeval now;
    gettimeofday(&now, NULL);

    float total_elapse = 1000000 * (now.tv_sec - idgen_begin.tv_sec) + now.tv_usec - idgen_begin.tv_usec;
    total_elapse /= 1000 * 1000;
    float inc_elapse = 1000000 * (now.tv_sec - idgen_inc_begin.tv_sec) + now.tv_usec - idgen_inc_begin.tv_usec;
    inc_elapse /= 1000 * 1000;

    float total_fps = total_num / total_elapse;
    float inc_fps = inc_num / inc_elapse;

    inc_num = 0;
    gettimeofday(&idgen_inc_begin, NULL);

    std::cout << "============ fps : "<< inc_fps << " ("  << total_fps << ") =============" << std::endl;

    return true;
}