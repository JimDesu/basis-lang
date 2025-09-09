#include "ParseObject.h"
#include <stdexcept>

using namespace basis;


bool basis::operator==(const ParseTree& lhs, const ParseTree& rhs) {
    return lhs.production == rhs.production
        && ((lhs.pToken == nullptr && rhs.pToken == nullptr) || *lhs.pToken == *rhs.pToken)
        && ((lhs.spNext == nullptr && rhs.spNext == nullptr) || *lhs.spNext == *rhs.spNext)
        && ((lhs.spDown == nullptr && rhs.spDown == nullptr) || *lhs.spDown == *rhs.spDown);
}
