#include <iostream>
#include <string>
#include <vector>

#include "compiler.h"
#include "CompilerContext.h"

int compile(std::vector<std::string> arguments) {
    CompilerContext ctx;
    if ( !ctx.options.readCompileOptions(arguments) ) {
        usage();
        return 1;
    }
    if ( !openInputFile(ctx) ) {
        std::cerr << "Error opening input file: " << ctx.options.filename << std::endl;
        return 1;
    }

    return 0;
}

void usage() {
    std::cout << "Usage: basis <options>" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -file <file>" << std::endl;
}

bool openInputFile(CompilerContext& ctx) {
    ctx.inputFile.open(ctx.options.filename);
    return ctx.inputFile.is_open();
}

