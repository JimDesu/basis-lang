#ifndef COMPILEOPTIONS_H
#define COMPILEOPTIONS_H

#include <string>
#include <vector>

namespace basis {
    struct CompileOptions {
        std::string filename;
        std::string output;
        bool readCompileOptions(std::vector<std::string>& arguments);
    };
}

#endif
