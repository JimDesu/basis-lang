#include <iostream>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

#include "compiler.h"

int compile(std::vector<std::string> arguments) {
    CompileOptions options;
    if ( !readCompileOptions(options, arguments) ) {
        usage();
        return 1;
    }

    return 0;
}

using CompileOptionsSetter = std::function<void(CompileOptions&, std::string&)>;
const std::unordered_map<std::string, CompileOptionsSetter> options_map = {
         {"-file", [](CompileOptions& o, std::string& arg) { o.filename = arg; }}
};
bool readCompileOptions(CompileOptions& options, std::vector<std::string> arguments) {
    if ( arguments.empty() ) return false;
    int size = arguments.size();
    if ( size % 2 != 0 ) return false;
    try {
        for ( int i = 0; i < size; i += 2 ) {
            std::string arg = arguments[i];
            options_map.at(arg)(options, arguments[i+1]);
        }
    } catch ( std::exception& e ) {
        return false;
    }
    return true;
}

void usage() {
    std::cout << "Usage: basis <options>" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -file <file>" << std::endl;
}

bool validateOptions(CompileOptions &options) {
    // for now, just require a filename
    if (options.filename.empty()) return false;
    return true;
};
