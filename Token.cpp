#include "Token.h"

using namespace basis;

bool basis::operator==(const Token& lhs, const Token& rhs) {
    return lhs.type == rhs.type
        && lhs.text == rhs.text
        && lhs.lineNumber == rhs.lineNumber
        && lhs.columnNumber == rhs.columnNumber
        && (lhs.bound == nullptr && rhs.bound == nullptr || *lhs.bound == *rhs.bound );
}
