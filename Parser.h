#ifndef PARSER_H
#define PARSER_H

#include <functional>
#include <list>

#include "ParseObject.h"
#include "Token.h"

namespace basis {

    using itToken = std::list<Token>::const_iterator;
    struct ParseFn {
        virtual bool operator()(vParseObject& result, itToken& iter, const Token* limit) = 0;
    };
    using spParseFn = std::shared_ptr<ParseFn>;

    class Parser {
    public:
        Parser(const std::list<Token>& tokens, bool deferParseFn = false);
        ~Parser();

        bool parse();
        vParseObject parseTree;
        // unit testing only
        void setParseFn(spParseFn fn);
    private:
        friend class Discard;
        friend class Match;
        spParseFn buildParser();
        bool atLimit(itToken& iter, const Token* limit) const;
        const std::list<Token>& tokens;
        spParseFn parseFn;
    };

    struct Discard : public ParseFn {
        TokenType type;
        Parser& parser;
        Discard(Parser p, const TokenType& t) : parser(p), type(t) {}
        bool operator()(vParseObject& result, itToken& iter, const Token* limit) override;
    };
    std::shared_ptr<Discard> discard(Parser& parser, const TokenType& type);

    struct Match : public ParseFn {
        Production production;
        Parser& parser;
        TokenType type;
        Match(const Production pr, Parser p, const TokenType& t) : production(pr), parser(p), type(t) {}
        bool operator()(vParseObject& result, itToken& iter, const Token* limit) override;
    };
    std::shared_ptr<Match> match(const Production production, Parser& parser, const TokenType& type);

    struct Maybe : public ParseFn {
        const spParseFn fn;
        Maybe(const spParseFn f) : fn(f) {}
        bool operator()(vParseObject& result, itToken& iter, const Token* limit) override;
    };
    std::shared_ptr<Maybe> maybe(const spParseFn fn);

    struct FirstOf : public ParseFn {
        const std::vector<spParseFn> fns;
        FirstOf(const std::vector<spParseFn> fs) : fns(fs) {}
        bool operator()(vParseObject& result, itToken& iter, const Token* limit) override;
    };
    std::shared_ptr<FirstOf> firstOf(const std::vector<spParseFn> fns);

    struct AllOf : public ParseFn {
        Production production;
        const std::vector<spParseFn> fns;
        AllOf(Production pr, const std::vector<spParseFn> fs) : production(pr), fns(fs) {}
        bool operator()(vParseObject& result, itToken& iter, const Token* limit) override;
    };
    std::shared_ptr<AllOf> allOf(const Production production, const std::vector<spParseFn> fns);

    struct OneOrMore : public ParseFn {
        Production production;
        spParseFn fn;
        OneOrMore(Production p, spParseFn f) : production(p), fn(f) {}
        bool operator()(vParseObject& result, itToken& iter, const Token* limit) override;
    };
    std::shared_ptr<OneOrMore> oneOrMore(const Production production, spParseFn fn);

    struct Obliged : public ParseFn {
        Production production;
        spParseFn prefix;
        spParseFn suffix;
        Obliged(Production p, spParseFn pre, spParseFn suf) : production(p), prefix(pre), suffix(suf) {}
        bool operator()(vParseObject& result, itToken& iter, const Token* limit) override;
    };
    std::shared_ptr<Obliged> obliged(const Production production, spParseFn prefix, spParseFn suffix);
}

#endif // PARSER_H
