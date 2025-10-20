#ifndef PARSER2_H
#define PARSER2_H

#include <list>
#include <memory>
#include <vector>

#include "ParseObject.h"
#include "Token.h"

namespace basis {

    using itToken = std::list<spToken>::const_iterator;

    // Base class for all parse function combinators
    class ParseFn {
    public:
        virtual ~ParseFn() = default;
        virtual bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                          itToken* pIter, const Token* pLimit) const = 0;
        static bool atLimit(const std::list<spToken>& tokens, itToken* pIter, const Token* pLimit);
    };

    // Parser class that uses function objects
    class Parsing2 {
    public:
        explicit Parsing2(const std::list<spToken>& tokens, const ParseFn& parseFn);
        bool parse();

        spParseTree parseTree;

    private:
        const std::list<spToken>& tokens;
        const ParseFn& parseFn;
    };

    // Discard combinator - matches a token type but doesn't create parse tree node
    class Discard : public ParseFn {
    public:
        explicit Discard(TokenType type);
        bool parse(const std::list<spToken>& tokens, spParseTree** _unused,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        TokenType type;
    };

    // Match combinator - matches a token type and creates parse tree node
    class Match : public ParseFn {
    public:
        Match(Production prod, TokenType type);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        Production prod;
        TokenType type;
    };

    // Maybe combinator - optional parsing (always succeeds)
    class Maybe : public ParseFn {
    public:
        explicit Maybe(const ParseFn& parseFn);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        const ParseFn& parseFn;
    };

    // Any combinator - tries alternatives in order (first match wins)
    class Any : public ParseFn {
    public:
        explicit Any(std::vector<const ParseFn*> alternatives);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        std::vector<const ParseFn*> alternatives;
    };

    // All combinator - sequence of parsers (all must succeed)
    class All : public ParseFn {
    public:
        explicit All(std::vector<const ParseFn*> sequence);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        std::vector<const ParseFn*> sequence;
    };

    // OneOrMore combinator - one or more repetitions
    class OneOrMore : public ParseFn {
    public:
        explicit OneOrMore(const ParseFn& parseFn);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        const ParseFn& parseFn;
    };

    // Separated combinator - separated list (element, separator, element, ...)
    class Separated : public ParseFn {
    public:
        Separated(const ParseFn& element, const ParseFn& separator);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        const ParseFn& element;
        const ParseFn& separator;
    };

    // Bound combinator - uses token's bound as limit
    class Bound : public ParseFn {
    public:
        explicit Bound(const ParseFn& parseFn);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        const ParseFn& parseFn;
    };

    // Group combinator - creates a parent node with children
    class Group : public ParseFn {
    public:
        Group(Production prod, const ParseFn& parseFn);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        Production prod;
        const ParseFn& parseFn;
    };

    // BoundedGroup combinator - Group with Bound applied to sequence
    class BoundedGroup : public ParseFn {
    public:
        BoundedGroup(Production prod, std::vector<const ParseFn*> sequence);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        All all;
        Bound bound;
        Group group;
    };

}

#endif // PARSER2_H

