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

    using SPFN = std::shared_ptr<ParseFn>;

    // Parser class that uses function objects
    class Parser {
    public:
        explicit Parser(const std::list<spToken>& tokens, SPFN spParseFn);
        bool parse();

        spParseTree parseTree;

    private:
        const std::list<spToken>& tokens;
        SPFN spfn;
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
    SPFN discard(TokenType type);

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
    SPFN match(Production prod, TokenType type);

    // Maybe combinator - optional parsing (always succeeds)
    class Maybe : public ParseFn {
    public:
        explicit Maybe(SPFN spParseFn);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        SPFN spfn;
    };
    SPFN maybe(SPFN parseFn);

    // Any combinator - tries alternatives in order (first match wins)
    class Any : public ParseFn {
    public:
        explicit Any(std::vector<SPFN> alternatives);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        std::vector<SPFN> alternatives;
    };

    template<typename... Args>
    inline SPFN any(const Args&... args) {
        return std::make_shared<Any>(std::vector<SPFN>{args...});
    }

    // All combinator - sequence of parsers (all must succeed)
    class All : public ParseFn {
    public:
        explicit All(std::vector<SPFN> sequence);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        std::vector<SPFN> sequence;
    };

    template<typename... Args>
    inline SPFN all(const Args&... args) {
        return std::make_shared<All>(std::vector<SPFN>{args...});
    }

    // OneOrMore combinator - one or more repetitions
    class OneOrMore : public ParseFn {
    public:
        explicit OneOrMore(SPFN spParseFn);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        SPFN spfn;
    };
    SPFN oneOrMore(SPFN parseFn);

    // Separated combinator - separated list (element, separator, element, ...)
    class Separated : public ParseFn {
    public:
        Separated(SPFN spElement, SPFN spSeparator);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        SPFN spElement;
        SPFN spSeparator;
    };
    SPFN separated(SPFN element, SPFN separator);

    // Bound combinator - uses token's bound as limit
    class Bound : public ParseFn {
    public:
        explicit Bound(SPFN spParseFn);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        SPFN spfn;
    };
    SPFN bound(SPFN parseFn);

    // Group combinator - creates a parent node with children
    class Group : public ParseFn {
    public:
        Group(Production prod, SPFN spParseFn);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        Production prod;
        SPFN spfn;
    };
    SPFN group(Production prod, SPFN parseFn);

    // BoundedGroup combinator - Group with Bound applied to sequence
    class BoundedGroup : public ParseFn {
    public:
        BoundedGroup(Production prod, std::vector<SPFN> sequence);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        SPFN spfn;
    };

    template<typename... Args>
    inline SPFN boundedGroup(Production prod, const Args&... args) {
        return std::make_shared<BoundedGroup>(prod, std::vector<SPFN>{args...});
    }

}

#endif // PARSER2_H

