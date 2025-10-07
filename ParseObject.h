#ifndef PARSEOBJECT_H
#define PARSEOBJECT_H

#include <memory>

#include "Token.h"

namespace basis {
    // production rule identifiers
    enum class Production {
        VARNAME,
        temp2
    };

    class ParseTree;
    using spParseTree = std::shared_ptr<ParseTree>;

    // generic parse tree representation
    class ParseTree {
    public:
        ParseTree(Production p) : production(p), spNext(nullptr), spDown(nullptr), pToken(nullptr) {}
        ParseTree(Production p, const Token* pT) : production(p), spNext(nullptr), spDown(nullptr), pToken(pT) {}
        // test support
        ParseTree(Production p, const Token* pT, spParseTree spN): production(p), pToken(pT), spNext(spN)  {}
        ParseTree(Production p, const Token* pT, spParseTree spN, spParseTree spD)
            : production(p), pToken(pT), spNext(spN), spDown(spD) {}
        Production production;
        spParseTree spNext;
        spParseTree spDown;
        const Token* pToken;
    };
    bool operator==(const ParseTree& lhs, const ParseTree& rhs);
}

#endif // PARSEOBJECT_H
