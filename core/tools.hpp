//
// Created by cambricon on 19-7-16.
//

#ifndef CNPIPE_TOOLS_HPP
#define CNPIPE_TOOLS_HPP


#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <time.h>
#include "sys/stat.h"
#include <iostream>

using namespace std;
using std::string;

#define ERROR(s)                                                      \
  {                                                                   \
    std::stringstream _message;                                       \
    _message << __FILE__ << ':' << __LINE__ << std::string(s);        \
    std::cerr << _message.str() << "\nAborting...\n";                 \
    exit(EXIT_FAILURE);                                               \
  }

#define LOG(s)                                                        \
  {                                                                   \
    std::stringstream _message;                                       \
    _message << __FILE__ << ':' << __LINE__  << std::string(s);       \
    std::cout << _message.str() << "\n";                              \
  }

// CHECK with a error if condition is not true.
#define CNPEAK_CHECK(condition, ...)                             \
  if (!(condition)) {                                     \
    ERROR(" Check failed: " #condition "." #__VA_ARGS__); \
  }

#define CNPEAK_CHECK_CNRT(cnrtFunc, ...)                                                 \
  { cnrtRet_t ret_code = (cnrtFunc);                                                      \
    if ( cnrtSuccess != ret_code ) {                                                 \
      std::cerr << "cnrt function " << #cnrtFunc << " call failed. code:"                 \
                << ret_code << ". (" << __FILE__ << ":"<< __LINE__ << ")." #__VA_ARGS__;  \
      exit(EXIT_FAILURE);                                                                 \
    }                                                                                     \
  }

#define CNPEAK_CHECK_CNCV(cncvFunc, ...)                                                 \
  { cncvStatus_t ret_code = (cncvFunc);                                                   \
    if ( CNCV_STATUS_SUCCESS != ret_code ) {                                              \
      std::cerr << "cncv function " << #cncvFunc << " call failed. code:"                 \
                << ret_code << ". (" << __FILE__ << ":"<< __LINE__ << ")." #__VA_ARGS__;  \
      exit(EXIT_FAILURE);                                                                 \
    }                                                                                     \
  }

#define CNPEAK_CHECK_PARAM(condition, ...)                                               \
  if (!(condition)) {                                                                     \
    std::stringstream _message;                                                           \
    _message << __FILE__ << ':' << __LINE__ << " "                                        \
             << "parameter check failed: " #condition ". " #__VA_ARGS__;                  \
    std::cerr << _message.str() << "\nAborting...\n";                                     \
    exit(EXIT_FAILURE);                                                                   \
  }

string get_rtsp_url(string file, int idx);
string get_video_path_random(string file);
double calc_time(struct timeval t1, struct timeval t2);

inline bool check_file_exist(std::string path){
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0)
    {
        if ((buffer.st_mode & S_IFDIR) == 0)
            return true;
        return false;
    }
    return false;
}

inline bool check_folder_exist(std::string path){
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0)
    {
        if ((buffer.st_mode & S_IFDIR) == 0)
            return false;
        return true;
    }
    return false;
}


#endif //CNPIPE_TOOLS_HPP
