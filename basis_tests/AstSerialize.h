#ifndef ASTSERIALIZE_H
#define ASTSERIALIZE_H

#include "../Ast.h"
#include <string>

namespace basis {

// Serialize an AST to canonical text. Output is a single line, no whitespace.
// Format spec (mirroring test_grammar2.cpp's parse-tree text style, adapted
// for AST-level data):
//   - Node types use struct names from Ast.h (e.g., RecordDecl, NamedType).
//   - String values are single-quoted, e.g. 'name'. Embedded backslash and
//     single-quote are escaped as \\ and \'.
//   - Enum values are bare identifiers (File, NoFail, Subquote, ...).
//   - Bool values: true / false. Integer values: bare digits.
//   - Vectors are emitted as additional positional args inside the parent.
//   - Optional fields with default/empty values are omitted; when present
//     they are emitted as key=value (e.g. alias='Std', constraint='Int',
//     scope='myScope', writeable, alloc, typeVar='T').
//   - std::variant alternatives are serialized as the active type directly;
//     the variant wrapper itself does not appear.
//   - Identifier { qualifiers; name; } serializes as a single quoted string
//     joined with '::', e.g. 'Std::Core::foo'.
std::string serializeAst(const CompilationUnit& cu);

} // namespace basis

#endif // ASTSERIALIZE_H
