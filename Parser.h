#ifndef PARSER_H
#define PARSER_H

#include <functional>
#include <list>

#include "ParseObject.h"
#include "Token.h"

namespace basis {

    using itToken = std::list<Token>::const_iterator;
    struct ParseFn {
        virtual bool operator()(spParseTree** pspResult, itToken* pIter, const Token* pLimit) = 0;
    };
    using spParseFn = std::shared_ptr<ParseFn>;

    class Parser {
    public:
        Parser(const std::list<Token>& tokens, bool deferParseFn = false);
        ~Parser();

        bool parse();
        spParseTree parseTree;
        // unit testing only
        void setParseFn(spParseFn fn);
    private:
        friend class Discard;
        friend class Match;
        spParseFn buildParser();
        bool atLimit(itToken* pIter, const Token* pLimit) const;
        const std::list<Token>& tokens;
        spParseFn parseFn;
    };

    struct Discard : public ParseFn {
        TokenType type;
        Parser& parser;
        Discard(Parser p, const TokenType& t) : parser(p), type(t) {}
        bool operator()(spParseTree** _unused, itToken* pIter, const Token* pLimit) override;
    };
    std::shared_ptr<Discard> discard(Parser& parser, const TokenType& type);

    struct Match : public ParseFn {
        Production production;
        Parser& parser;
        TokenType type;
        Match(const Production pr, Parser p, const TokenType& t) : production(pr), parser(p), type(t) {}
        bool operator()(spParseTree**  pspResult, itToken* pIter, const Token* pLimit) override;
    };
    std::shared_ptr<Match> match(const Production production, Parser& parser, const TokenType& type);

    struct Maybe : public ParseFn {
        const spParseFn fn;
        Maybe(const spParseFn f) : fn(f) {}
        bool operator()(spParseTree** pspResult, itToken* pIter, const Token* pLimit) override;
    };
    std::shared_ptr<Maybe> maybe(const spParseFn fn);

    struct FirstOf : public ParseFn {
        const std::vector<spParseFn> fns;
        FirstOf(const std::vector<spParseFn> fs) : fns(fs) {}
        bool operator()(spParseTree** pspResult, itToken* pIter, const Token* pLimit) override;
    };
    std::shared_ptr<FirstOf> firstOf(const std::vector<spParseFn> fns);

    struct AllOf : public ParseFn {
        const std::vector<spParseFn> fns;
        AllOf(const std::vector<spParseFn> fs) : fns(fs) {}
        bool operator()(spParseTree** pspResult, itToken* pIter, const Token* pLimit) override;
    };
    std::shared_ptr<AllOf> allOf(const Production production, const std::vector<spParseFn> fns);

    struct OneOrMore : public ParseFn {
        spParseFn fn;
        OneOrMore(spParseFn f) : fn(f) {}
        bool operator()(spParseTree** pspResult, itToken* pIter, const Token* pLimit) override;
    };
    std::shared_ptr<OneOrMore> oneOrMore(const Production production, spParseFn fn);

}

#endif // PARSER_H
