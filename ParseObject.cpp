#include "ParseObject.h"
#include <stdexcept>

using namespace basis;

spParseObject basis::makeParseNode(Production production, spParseObject next, spParseObject down) {
    return spParseObject( std::make_shared<ParseNode>(production) );
}
spParseObject basis::makeParseLeaf(Production production, const Token* token) {
    return spParseObject( std::make_shared<ParseLeaf>(production, token) );
}
