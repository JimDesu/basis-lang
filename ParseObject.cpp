#include "ParseObject.h"
#include <stdexcept>

using namespace basis;

vParseObject basis::makeParseNode(Production production) {
    return vParseObject( std::make_shared<ParseNode>(production) );
}
vParseObject basis::makeParseLeaf(Production production, const Token* pToken) {
    return vParseObject( std::make_shared<ParseLeaf>(production, pToken) );
}

bool basis::isLeafVariant(const vParseObject& obj) {
    return std::holds_alternative<std::shared_ptr<ParseLeaf>>(obj);
}

bool basis::isNodeVariant(const vParseObject& obj) {
    return std::holds_alternative<std::shared_ptr<ParseNode>>(obj);
}

bool basis::isEmptyVariant(const vParseObject& obj) {
    return std::holds_alternative<std::monostate>(obj);
}

std::shared_ptr<ParseLeaf> basis::asParseLeaf(const vParseObject& obj) {
    if ( !isLeafVariant(obj) ) throw std::runtime_error("object is not a leaf");
    return std::get<std::shared_ptr<ParseLeaf>>(obj);
}

std::shared_ptr<ParseNode> basis::asParseNode(const vParseObject& obj) {
    if ( !isNodeVariant(obj) ) throw std::runtime_error("object is not a node");
    return std::get<std::shared_ptr<ParseNode>>(obj);
}

vParseObject& basis::getLinkNext(const vParseObject& obj) {
    if ( isLeafVariant(obj) ) return asParseLeaf(obj)->next;
    if ( isNodeVariant(obj) ) return asParseNode(obj)->next;
    throw std::runtime_error("no next: object is not a node or leaf");
}

vParseObject& basis::getLinkDown(const vParseObject& obj) {
    if ( isNodeVariant(obj) ) return asParseNode(obj)->down;
    throw std::runtime_error("no down: object is not a node or leaf");
}
void basis::clearParseObject(vParseObject& obj) {
    obj.emplace<std::monostate>();
}



