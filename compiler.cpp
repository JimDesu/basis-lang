#include <iostream>
#include <string>
#include <vector>

#include "compiler.h"
#include "CompilerContext.h"
#include "Lexer.h"
#include "Parsing2.h"
#include "Grammar2.h"


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

    Lexer lexer(ctx.inputFile, ctx.diagnostics);
    lexer.scan();

    if ( !ctx.diagnostics.hasFatal() ) {
        Parser parser(lexer.output, getGrammar().COMPILATION_UNIT);
        if ( !parser.parse() ) {
            ctx.diagnostics.report(parser.getErrorDiagnostic());
        }
    }

    printDiagnostics(std::cerr, ctx.diagnostics);
    return ctx.diagnostics.hasErrors() ? 1 : 0;
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

