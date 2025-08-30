#include "ParseObject.h"
#include <stdexcept>

using namespace basis;

spParseObject basis::makeParseNode(Production production, spParseObject next, spParseObject down) {
    return spParseObject( std::make_shared<ParseNode>(production) );
}
spParseObject basis::makeParseLeaf(Production production, const Token* pToken) {
    return spParseObject( std::make_shared<ParseLeaf>(production, pToken) );
}

bool basis::isLeaf(const spParseObject& obj) {
    return std::holds_alternative<std::shared_ptr<ParseLeaf>>(obj);
}

bool basis::isNode(const spParseObject& obj) {
    return std::holds_alternative<std::shared_ptr<ParseNode>>(obj);
}

std::shared_ptr<ParseLeaf> basis::getLeaf(const spParseObject& obj) {
    if ( !isLeaf(obj) ) throw std::runtime_error("object is not a leaf");
    return std::get<std::shared_ptr<ParseLeaf>>(obj);
}

std::shared_ptr<ParseNode> basis::getNode(const spParseObject& obj) {
    if ( !isNode(obj) ) throw std::runtime_error("object is not a node");
    return std::get<std::shared_ptr<ParseNode>>(obj);
}

spParseObject& basis::getNext(const spParseObject& obj) {
    if ( isLeaf(obj) ) {
        return getLeaf(obj)->next;
    } else {
        return getNode(obj)->next;
    }
}



