//
// Created by cambricon on 19-5-23.
//
#include "tools.hpp"

string get_rtsp_url(string file, int idx){
    vector<string> url_list;
    std::string line;
    std::ifstream files(file.c_str(), std::ios::in);
    while (getline(files, line)) {
        url_list.push_back(line);
    }
    files.close();
    return url_list[idx % url_list.size()];
}

string get_video_path_random(string file){
    static bool inited=false;
    static vector<string> path_list;
    static int idx=0;

    if(!inited){
        std::string line;
        std::ifstream files(file.c_str(), std::ios::in);
        while (getline(files, line)) {
            path_list.push_back(line);
        }
        srand(unsigned(time(NULL)));
        std::random_shuffle(path_list.begin(), path_list.end());
        files.close();
        inited = true;
    }

    idx = (idx+1) % path_list.size();

    return path_list[idx];
}

double calc_time(struct timeval t1, struct timeval t2){
    return (t2.tv_sec-t1.tv_sec)*1000.0 + (t2.tv_usec-t1.tv_usec)/1000.0;
}