//
// Created by cambricon on 19-1-21.
//

#ifndef PLUGIN_ID_GEN_HPP
#define PLUGIN_ID_GEN_HPP

#include "core/cnPipe.hpp"

class IdGenerator: public Plugin{
public:
    IdGenerator();
    virtual bool init_in_main_thread();
    virtual bool init_in_sub_thread();
    virtual bool callback(TData *&pdata_in, vector<TData *>&pdatas_out);
    virtual string name();

private:
    bool add();
    bool print_fps();

private:
    int cur_id_;
    cnOfflineInfo *cur_offline_;
};

#endif //PLUGIN_ID_GEN_HPP
