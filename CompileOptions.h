#ifndef COMPILEOPTIONS_H
#define COMPILEOPTIONS_H

#include <cstdint>
#include <string>
#include <vector>

namespace basis {
    struct CompileOptions {
        std::string filename;
        std::string outputFile;
        bool readCompileOptions(std::vector<std::string>& arguments);
    };
}

#endif
