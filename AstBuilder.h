#ifndef ASTBUILDER_H
#define ASTBUILDER_H

#include "Ast.h"
#include "ParseObject.h"

namespace basis {

    // Convert the ParseTree rooted at a COMPILATION_UNIT node into an AST.
    // Returns nullptr if pt is null or does not have production COMPILATION_UNIT.
    std::shared_ptr<CompilationUnit> buildAst(const spParseTree& pt);

} // namespace basis

#endif // ASTBUILDER_H

