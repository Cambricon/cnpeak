//
// Created by cambricon on 19-1-19.
//

#include "compare.hpp"

using namespace std;

#define OUTPUT "testcase/output/"
#define OUTPUT_OK "testcase/output_ok/"

void DeleteDirectory(const char *path);

string get_content(string filename){
    string str;
    ifstream fin;
    fin.open(filename, ios::in);
    assert(fin) ;
    stringstream buf;
    buf << fin.rdbuf();
    str = buf.str();
    fin.close();
    return str;
}

vector<string> get_all_files(string path){
    vector<string> files;
    struct dirent *ptr;
    DIR *dir;
    dir = opendir(path.c_str());
    while((ptr = readdir(dir)) != NULL){
        if(ptr->d_name[0] == '.') continue;
        files.push_back(ptr->d_name);
    }
    return files;
}

bool compare_file(string f1, string f2){
    string c1 = get_content(f1);
    string c2 = get_content(f2);
    assert(c1.compare(c2) == 0) ;
    return true;
}

bool compare_result(string casename){
    string cur_dir = OUTPUT + casename + "/";
    string ok_dir  = OUTPUT_OK + casename + "/";

//    DeleteDirectory(cur_dir.c_str());
    vector<string> cur_files;
    vector<string> ok_files;
    cur_files = get_all_files(cur_dir);
    ok_files = get_all_files(ok_dir);
    assert(cur_files == ok_files);

    for(size_t i=0; i<cur_files.size(); i++){
        compare_file(cur_dir + cur_files[i], ok_dir + ok_files[i]);
    }

    return true;
}