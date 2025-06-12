#include <iostream>
#include <string>
#include <vector>

#include "compiler.h"

int compile(std::vector<std::string> arguments) {
    CompileOptions options;
    if ( !options.readCompileOptions(arguments) ) {
        usage();
        return 1;
    }

    return 0;
}

void usage() {
    std::cout << "Usage: basis <options>" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -file <file>" << std::endl;
}

