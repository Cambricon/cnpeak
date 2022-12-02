
#include "cnPipe.hpp"

int Plugin::data_id(TData *pdata){
    return pdata->id;
};

void Plugin::set_plugin_id(int id){
    cur_plugin_id = id;
}

void Plugin::set_pipe_id(int id){
    cur_pipe_id = id;
}

void Plugin::set_context(cnPublicContext *ctxt){
    pCtxt = ctxt;
}

void Plugin::enable_report(){
    is_report = true;
}

void Plugin::report(BlockQueue<TReportInfo*> *report_queue, TData* pdata){
    if(!is_report){
        return;
    }
    float elapse = 1000000 * (pdata->end_time.tv_sec - pdata->start_time.tv_sec)
                  + pdata->end_time.tv_usec - pdata->start_time.tv_usec;
    elapse /= 1000;

    total_count ++;
    delay += elapse;
    if(total_count % num == 0){
        TReportInfo *s = new TReportInfo();
        s->total_count = total_count;
        s->delay = delay/num;
        s->plugin_id = cur_plugin_id;
        delay = 0;
        report_queue->push_non_block(s);
    }
}

cnPipe::cnPipe(int id, bool print, bool report){
    pipe_id = id;
    is_report = report;
    is_print = print;
}


void cnPipe::add(Plugin *p, int cap)
{
    p->queue_cap = cap;

    if(plugin_list_.size() > 0){
        BlockQueue<TData*> *input = new BlockQueue<TData*>(cap);

        p->input_queue = input;
        p->output_queue = nullptr;
        Plugin *pre_p = plugin_list_.back();
        pre_p->output_queue = input;
    } else{
        p->input_queue = nullptr;
        p->output_queue = nullptr;
    }

    plugin_list_.push_back(p);
}


void cnPipe::start_thread(Plugin* p) {
    p->init_in_sub_thread();

    while(true){
        TData *pdata_in = nullptr;

        if(p->input_queue){
            p->input_queue->peek(pdata_in);
        } else{
            pdata_in = new TData();
        }

        vector<TData *> pdatas_out;

        if(pdata_in){
            record_start_time(pdata_in);
            p->callback(pdata_in, pdatas_out);

            for(auto d : pdatas_out){
                if(!d) continue;
                record_end_time(d);
                print_time(pipe_id, p->name(), d);
                p->report(report_queue, d);
            }
        }

        if(p->input_queue) {
            TData *out = nullptr;
            p->input_queue->pop(out);
        }

        if(p->output_queue){
            for(auto d : pdatas_out){
                p->output_queue->push(d);
            }
        }
        else{
            for(auto d : pdatas_out){
                delete d;
            }
//            delete pdata_in;
        }

        // plugin-thread die when receive nullptr
        if(pdata_in == nullptr) {
            if(p->output_queue){
                p->output_queue->push(nullptr);
            }
            break;
        }

    }
}

void cnPipe::init(){
    for(auto p : plugin_list_)
    {
        p->set_context(&ctxt);
        p->set_plugin_id(plugin_num++);
        p->set_pipe_id(pipe_id);
        p->init_in_main_thread();
        if(is_report){
            p->enable_report();
        }
    }

    if(is_report){
        report_queue =  new BlockQueue<TReportInfo*>(20);
        init_report(report_queue);
    }
}


void cnPipe::init_report(BlockQueue<TReportInfo *> *q){
    server_ = Report::create_report_server();
    server_->add_report_queue(q);
}

void cnPipe::start(){
    for(auto p : plugin_list_)
    {
        thread *th = new thread(&cnPipe::start_thread, this, p);
        thread_list_.push_back(th);
    }

    if(is_report) {
        server_->start();
    }
}

void cnPipe::sync(){
    for (auto th : thread_list_) th->join();
}

void cnPipe::record_start_time(TData* pdata){
    gettimeofday(&pdata->start_time, NULL);
}

void cnPipe::record_end_time(TData* pdata){
    gettimeofday(&pdata->end_time, NULL);
}

void cnPipe::print_time(int pipe_id, string name, TData* pdata){
    if(!is_print){
        return;
    }
    auto elapse = 1000000 * (pdata->end_time.tv_sec - pdata->start_time.tv_sec)
            + pdata->end_time.tv_usec - pdata->start_time.tv_usec;
    elapse /= 1000;
    std::cout   << std::left
                << "[pipe-id]: " << std::setw(6) << pipe_id
                << "[data-id]: " << std::setw(6) << pdata->id
                << "[plugin]: " << std::setw(40) << name
                << "[time]: " << elapse << "ms"
                << std::endl;
}

