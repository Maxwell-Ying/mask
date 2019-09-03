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

    cout << b << endl << str << endl;
    return 0;
}