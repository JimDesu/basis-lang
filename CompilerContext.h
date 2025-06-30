#ifndef COMPILERCONTEXT_H
#define COMPILERCONTEXT_H

#include <fstream>
#include <iostream>

#include "CompileOptions.h"

namespace basis {
    struct CompilerContext {
        ~CompilerContext();
        std::ifstream inputFile;
        CompileOptions options;

    };

} // basis

#endif //COMPILERCONTEXT_H
