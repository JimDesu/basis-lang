#ifndef PARSER_H
#define PARSER_H

#include <functional>
#include <list>

#include "ParseObject.h"
#include "Token.h"

namespace basis {
    using itToken = std::list<Token>::const_iterator;
    using ParseFn = std::function<bool(spParseObject& result, itToken& iter, const Token* limit)>;

    class Parser {
    public:
        Parser(const std::list<Token>& tokens, bool deferParseFn = false);
        ~Parser();

        bool parse();
        spParseObject parseTree;
        void setParseFn(ParseFn fn);

        // exposed for testing
        bool atLimit(itToken& iter, const Token* limit) const;
        ParseFn match(const TokenType& type) const;
        ParseFn match(const Production& production, const TokenType& type) const;

        static ParseFn maybe(ParseFn fn);
        static ParseFn choice(std::vector<ParseFn> fns);
        static ParseFn sequence(const Production& production, std::vector<ParseFn> fns);
        static ParseFn oneOrMore(ParseFn fn);
    private:
        const std::list<Token>& tokens;
        ParseFn parseFn;
        //ParseFn buildParser();
    };
}

#endif // PARSER_H
