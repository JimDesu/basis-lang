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

    struct ParseTree;
    using spParseTree = std::shared_ptr<ParseTree>;

    // generic parse tree representation
    class ParseTree {
    public:
        ParseTree(Production p) : production(p) {}
        ParseTree(Production p, const Token* pT) : production(p), pToken(pT) {}
        Production production;
        spParseTree spNext;
        spParseTree spDown;
        const Token* pToken;
    };
    bool operator==(const ParseTree& lhs, const ParseTree& rhs);
}

#endif // PARSEOBJECT_H
