#ifndef PARSEOBJECT_H
#define PARSEOBJECT_H

#include <memory>
#include <variant>

#include "Token.h"

namespace basis {
    // production rule identifiers
    enum class Production {
        VARNAME,
        temp2
    };

    // generic parse tree representation
    struct ParseNode;
    struct ParseLeaf;
    using vParseObject = std::variant<std::monostate, std::shared_ptr<ParseNode>, std::shared_ptr<ParseLeaf>>;

    struct ParseNode {
        ParseNode(Production p) : production(p) {}
        Production production;
        vParseObject next;
        vParseObject down;
    };

    struct ParseLeaf {
        ParseLeaf(Production p, const Token* pt) : production(p), pToken(pt) {}
        Production production;
        vParseObject next;
        const Token* pToken;
    };

    vParseObject makeParseNode(Production production);
    vParseObject makeParseLeaf(Production production, const Token* pToken);

    bool isNodeVariant(const vParseObject& obj);
    bool isLeafVariant(const vParseObject& obj);
    bool isEmptyVariant(const vParseObject& obj);

    std::shared_ptr<ParseLeaf> asParseLeaf(const vParseObject& obj);
    std::shared_ptr<ParseNode> asParseNode(const vParseObject& obj);

    vParseObject& getLinkNext(const vParseObject& obj);
    vParseObject& getLinkDown(const vParseObject& obj);

    void clearParseObject(vParseObject& obj);

}

#endif // PARSEOBJECT_H
