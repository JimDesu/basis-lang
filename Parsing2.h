#ifndef PARSER2_H
#define PARSER2_H

#include <list>
#include <memory>
#include <vector>

#include "RollbackGuard.h"
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

    using SPPF = std::shared_ptr<ParseFn>;

    // Parser class that uses function objects
    class Parser {
    public:
        explicit Parser(const std::list<spToken>& tokens, SPPF spParseFn);
        bool parse();
        // test support; will not be used at runtime
        bool allTokensConsumed() const;

        spParseTree parseTree;

    private:
        const std::list<spToken>& tokens;
        SPPF spfn;
        itToken finalPosition;
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
    SPPF discard(TokenType type);

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
    SPPF match(Production prod, TokenType type);

    // Maybe combinator - optional parsing (always succeeds)
    class Maybe : public ParseFn {
    public:
        explicit Maybe(SPPF spParseFn);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        SPPF spfn;
    };
    SPPF maybe(SPPF parseFn);

    // Prefix combinator - if first arg matches, all must match; otherwise succeeds without consuming
    class Prefix : public ParseFn {
    public:
        explicit Prefix(std::vector<SPPF> sequence);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        std::vector<SPPF> sequence;
    };

    template<typename... Args>
    inline SPPF prefix(const Args&... args) {
        return std::make_shared<Prefix>(std::vector<SPPF>{args...});
    }

    // Any combinator - tries alternatives in order (first match wins)
    class Any : public ParseFn {
    public:
        explicit Any(std::vector<SPPF> alternatives);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        std::vector<SPPF> alternatives;
    };

    template<typename... Args>
    inline SPPF any(const Args&... args) {
        return std::make_shared<Any>(std::vector<SPPF>{args...});
    }

    // All combinator - sequence of parsers (all must succeed)
    class All : public ParseFn {
    public:
        explicit All(std::vector<SPPF> sequence);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        std::vector<SPPF> sequence;
    };

    template<typename... Args>
    inline SPPF all(const Args&... args) {
        return std::make_shared<All>(std::vector<SPPF>{args...});
    }

    // OneOrMore combinator - one or more repetitions
    class OneOrMore : public ParseFn {
    public:
        explicit OneOrMore(SPPF spParseFn);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        SPPF spfn;
    };
    SPPF oneOrMore(SPPF parseFn);

    // Separated combinator - separated list (element, separator, element, ...)
    class Separated : public ParseFn {
    public:
        Separated(SPPF spElement, SPPF spSeparator);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        SPPF spElement;
        SPPF spSeparator;
    };
    SPPF separated(SPPF element, SPPF separator);

    // Bound combinator - uses token's bound as limit
    class Bound : public ParseFn {
    public:
        explicit Bound(SPPF spParseFn);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        SPPF spfn;
    };
    SPPF bound(SPPF parseFn);

    // Group combinator - creates a parent node with children
    class Group : public ParseFn {
    public:
        Group(Production prod, SPPF spParseFn);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        Production prod;
        SPPF spfn;
    };
    SPPF group(Production prod, SPPF parseFn);

    // BoundedGroup combinator - Group with Bound applied to sequence
    class BoundedGroup : public ParseFn {
    public:
        BoundedGroup(Production prod, std::vector<SPPF> sequence);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        SPPF spfn;
    };

    template<typename... Args>
    inline SPPF boundedGroup(Production prod, const Args&... args) {
        return std::make_shared<BoundedGroup>(prod, std::vector<SPPF>{args...});
    }

    // Forward combinator - for forward/circular initialization dependencies
    class Forward : public ParseFn {
    public:
        explicit Forward(const SPPF& spfnRef);
        bool parse(const std::list<spToken>& tokens, spParseTree** dpspResult,
                  itToken* pIter, const Token* pLimit) const override;
    private:
        const SPPF& spfnRef;
    };
    SPPF forward(const SPPF& spfnRef);

}

#endif // PARSER2_H

