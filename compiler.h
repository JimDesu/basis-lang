#ifndef COMPILER_H
#define COMPILER_H

#include <string>
#include <vector>

#include "CompileOptions.h"
#include "CompilerContext.h"

using namespace basis;

int compile(std::vector<std::string> arguments);
void usage();
bool openInputFile(CompilerContext& ctx);

#endif
