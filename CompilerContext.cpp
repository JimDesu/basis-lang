#include "CompilerContext.h"

namespace basis {
    CompilerContext::~CompilerContext() {
        if( inputFile.is_open() ) {
          inputFile.close();
        }
    }
} // basis