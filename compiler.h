#ifndef COMPILER_H
#define COMPILER_H

#include <string>
#include <vector>

#include "CompileOptions.h"

using namespace basis;

int compile(std::vector<std::string> arguments);
bool readCompileOptions(CompileOptions& options, std::vector<std::string> arguments);
void usage();
bool validateOptions(CompileOptions& options);

#endif
