#include <iostream>
#include <fstream>
#include <vector>
#include "parsecmd.hpp"
#include "get_file_list.h"

using namespace std;


int main(int argc, char* argv[]) {
    util::Options options("mask", "a new CDC way for duplicated data");
    options.add_options()
    ("d,debug", "enable debugging")
    ("f,filename", "filename of input", util::value<std::string>())
    ;
    auto result = options.parse(argc, argv);
    bool debug = false;
    if (result.count("d") > 0) {
        debug = true;
        cout << "mode debug on" << endl;
    }


    cout << result.count("f") << endl;

    string dir = result["f"].as<std::string>();

    vector<string> files = getFilesList(dir);

    for (auto file : files) {
        cout << file << endl;
    }

    // string s;

    // for (auto file : path) {
    //     auto first_chunk = get_first_chunk(file);

    //     if (current_pool.contain(first_chunk)) {
    //         if (current_pool.contain(file)) {
    //             continue;
    //         }
    //         else {
    //             get_chunk(file);
    //         }
    //     } else {
    //         get_without_prev(file);
    //     }
    // }

    // cout << b << endl << str << endl;
    return 0;
}