#include <iostream>
#include "parsecmd.hpp"
using namespace std;


int main(int argc, char* argv[]) {
    util::Options options("mask", "a new CDC way for duplicated data");
    options.add_options()
    ("d,debug", "enable debugging")
    ("f,filename", "filename of input", util::value<std::string>())
    ;
    auto result = options.parse(argc, argv);
    cout << result.count("d");
    bool b = result["d"].as<bool>();
    std::string str = result["f"].as<std::string>();

    string path = result["dir"].as<std::string>();

    for (auto file : path) {
        auto first_chunk = get_first_chunk(file);

        if (current_pool.contain(first_chunk)) {
            if (current_pool.contain(file)) {
                continue;
            }
            else {
                get_chunk(file);
            }
        } else {
            get_without_prev(file);
        }
    }

    cout << b << endl << str << endl;
    return 0;
}