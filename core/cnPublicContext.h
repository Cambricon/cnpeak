//
// Created by cambricon on 19-1-4.
//

#ifndef CNPUBLICCONTEXT_H
#define CNPUBLICCONTEXT_H

#include <iostream>
#include <vector>
#include "cnrt.h"  //NOLINT

using std::vector;
using std::string;
using std::stringstream;

const int output_num = 20;
const int input_num = 4;

struct cnOfflineInfo{
    int inputNum, outputNum;
    int input_n[input_num], input_c[input_num], input_h[input_num], input_w[input_num];
    int in_count[input_num];
    int output_n[output_num], output_c[output_num], output_h[output_num], output_w[output_num];
    int out_count[output_num];
    int input_datatype[input_num];
    int output_datatype[output_num];
};

struct cnPublicContext{
    vector<vector<cnOfflineInfo *>> offline;
};


#endif //CNPUBLICCONTEXT_H
