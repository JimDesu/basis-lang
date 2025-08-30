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
    using spParseObject = std::variant<std::shared_ptr<ParseNode>, std::shared_ptr<ParseLeaf>>;

    struct ParseNode {
        ParseNode(Production p) : production(p) {}
        Production production;
        spParseObject next;
        spParseObject down;
    };

    struct ParseLeaf {
        ParseLeaf(Production p, const Token* pt) : production(p), pToken(pt) {}
        Production production;
        spParseObject next;
        const Token* pToken;
    };

    spParseObject makeParseNode(Production production, spParseObject next, spParseObject down);
    spParseObject makeParseLeaf(Production production, const Token* pToken);

    bool isLeaf(const spParseObject& obj);
    bool isNode(const spParseObject& obj);

    std::shared_ptr<ParseLeaf> getLeaf(const spParseObject& obj);
    std::shared_ptr<ParseNode> getNode(const spParseObject& obj);

    spParseObject& getNext(const spParseObject& obj);
    spParseObject& getDown(const spParseObject& obj);
}

#endif // PARSEOBJECT_H
