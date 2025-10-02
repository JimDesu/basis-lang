#ifndef PARSER_H
#define PARSER_H

#include <functional>
#include <list>

#include "ParseObject.h"
#include "Token.h"

namespace basis {

    using itToken = std::list<Token>::const_iterator;
    struct ParseFn {
        virtual bool operator()(spParseTree** dpspResult, itToken* pIter, const Token* pLimit) = 0;
        virtual ~ParseFn() = default;
    };
    using spParseFn = std::shared_ptr<ParseFn>;

    class Parser {
    public:
        explicit Parser(const std::list<Token>& tokens, bool deferParseFn = false);
        ~Parser();

        bool parse();
        spParseTree parseTree;
        // unit testing only
        void setParseFn(spParseFn fn);
    private:
        friend struct Discard;
        friend struct Match;
        spParseFn buildParser();
        bool atLimit(itToken* pIter, const Token* pLimit) const;
        const std::list<Token>& tokens;
        spParseFn parseFn;
    };

    // match a token but don't keep it
    struct Discard final : ParseFn {
        Discard(Parser& p, const TokenType& t) : parser(p), type(t) {}
        Parser& parser;
        TokenType type;
        bool operator()(spParseTree** _unused, itToken* pIter, const Token* pLimit) override;
    };
    std::shared_ptr<Discard> discard(Parser& parser, const TokenType& type);

    // match a token and keep it
    struct Match final: ParseFn {
        Match(const Production pr, Parser& p, const TokenType& t) : production(pr), parser(p), type(t) {}
        Production production;
        Parser& parser;
        TokenType type;
        bool operator()(spParseTree**  dpspResult, itToken* pIter, const Token* pLimit) override;
    };
    std::shared_ptr<Match> match(Production production, Parser& parser, const TokenType& type);

    // match zero or one instances
    struct Maybe final : ParseFn {
        explicit Maybe(const spParseFn f) : fn(f) {}
        const spParseFn fn;
        bool operator()(spParseTree** dpspResult, itToken* pIter, const Token* pLimit) override;
    };
    std::shared_ptr<Maybe> maybe(spParseFn fn);

    // match the first matching instance
    struct Any final : ParseFn {
        explicit Any(const std::vector<spParseFn> fs) : fns(fs) {}
        const std::vector<spParseFn> fns;
        bool operator()(spParseTree** dpspResult, itToken* pIter, const Token* pLimit) override;
    };
    std::shared_ptr<Any> any(std::vector<spParseFn> fns);

    // match all instances
    struct All final : ParseFn {
        explicit All(const std::vector<spParseFn> fs) : fns(fs) {}
        const std::vector<spParseFn> fns;
        bool operator()(spParseTree** dpspResult, itToken* pIter, const Token* pLimit) override;
    };
    std::shared_ptr<All> all(std::vector<spParseFn> fns);

    // match as many instances as possible but at least one
    struct OneOrMore final : ParseFn {
        explicit OneOrMore(spParseFn f) : fn(f) {}
        spParseFn fn;
        bool operator()(spParseTree** dpspResult, itToken* pIter, const Token* pLimit) override;
    };
    std::shared_ptr<OneOrMore> oneOrMore(spParseFn fn);

    // match up to the bound
    struct Bound final : ParseFn {
        explicit Bound(spParseFn f) : fn(f) {}
        spParseFn fn;
        bool operator()(spParseTree** dpspResult, itToken* pIter, const Token* pLimit) override;
    };
    std::shared_ptr<Bound> bound(spParseFn fn);

    // group a parse tree into a labelled parent node
    struct Group final : ParseFn {
        Group(const Production p, spParseFn f) : production(p), fn(f) {}
        Production production;
        spParseFn fn;
        bool operator()(spParseTree** dpspResult, itToken* pIter, const Token* pLimit) override;
    };
    std::shared_ptr<Group> group(const Production production, spParseFn fn);
}

#endif // PARSER_H
