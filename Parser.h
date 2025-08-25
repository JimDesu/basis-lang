#ifndef PARSER_H
#define PARSER_H

#include <functional>
#include <list>
#include <memory>

#include "ParseTree.h"
#include "Token.h"

namespace basis {
    using spParseTree = std::shared_ptr<ParseTree>;
    using itToken = std::list<Token>::const_iterator;
    using ParseFn = std::function<bool(spParseTree& result, itToken& iter, const Token* limit)>;

    class Parser {
    public:
        explicit Parser(const std::list<Token>& tokens);
        Parser(const Parser& other);
        Parser& operator=(const Parser& rhs);
        ~Parser();

        bool parse();
        std::shared_ptr<ParseTree> parseTree;
        // exposed for testing
        bool atLimit(itToken& iter, const Token* limit) const;
        ParseFn match(TokenType type) const;
        static ParseFn maybe(ParseFn fn);
        static ParseFn first(std::vector<ParseFn> fns);
        static ParseFn chain(std::vector<ParseFn> fns);
    private:
        std::list<Token> tokens;
    };
}

#endif // PARSER_H
