#ifndef PARSER_H
#define PARSER_H

#include <functional>
#include <list>
#include <memory>

#include "ParseObject.h"
#include "Token.h"

namespace basis {
    using itToken = std::list<Token>::const_iterator;
    using ParseFn = std::function<bool(spParseObject result, itToken& iter, const Token* limit)>;

    class Parser {
    public:
        explicit Parser(const std::list<Token>& tokens);
        Parser(const Parser& other);
        Parser& operator=(const Parser& rhs);
        ~Parser();

        bool parse();
        spParseObject parseTree;
        // exposed for testing
        bool atLimit(itToken& iter, const Token* limit) const;
        ParseFn match(Production production, TokenType type, bool keep = true) const;

        //static ParseFn maybe(ParseFn fn);
        //static ParseFn choice(std::vector<ParseFn> fns);
        //static ParseFn sequence(std::vector<ParseFn> fns);
        //static ParseFn object(ParseFn head, ParseFn body);
        //static ParseFn zeroOrMore(ParseFn fn);
        //static ParseFn oneOrMore(ParseFn fn);
    private:
        std::list<Token> tokens;
        ParseFn parseFn;
        //ParseFn buildParser();
    };
}

#endif // PARSER_H
