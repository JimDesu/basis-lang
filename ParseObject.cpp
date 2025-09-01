#include "ParseObject.h"
#include <stdexcept>

using namespace basis;

spParseObject basis::makeParseNode(Production production) {
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

spParseObject& basis::getLinkNext(const spParseObject& obj) {
    if ( isLeaf(obj) ) return getLeaf(obj)->next;
    if ( isNode(obj) ) return getNode(obj)->next;
    throw std::runtime_error("object is not a node or leaf");
}

spParseObject& basis::getLinkDown(const spParseObject& obj) {
    if ( isNode(obj) ) return getNode(obj)->down;
    if ( isLeaf(obj) ) throw std::runtime_error("leaf has no down link");
    throw std::runtime_error("object is not a node or leaf");
}
void basis::clearParseObject(spParseObject& obj) {
    obj.emplace<std::monostate>();
}



