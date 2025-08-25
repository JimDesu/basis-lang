#include "Parser.h"

using namespace basis;

Parser::Parser(const std::list<Token>& tokens) : tokens(tokens) {}
Parser::Parser(const Parser& other) : tokens(other.tokens) {}
Parser& Parser::operator=(const Parser& rhs) {
    if (this != &rhs) {
        tokens = rhs.tokens;
    }
    return *this;
}
Parser::~Parser() {}

// check to see if the current iterator refers to the specified limiter
bool Parser::atLimit(itToken& iter, const Token* limit) const {
    return iter != tokens.cend() && limit != nullptr && &(*iter) == limit;
}

// match a token by its type
ParseFn Parser::match(TokenType type) const {
    return [&](spParseTree& result, itToken& iter, const Token* limit) {
        if ( atLimit(iter, limit) ) return false;
        if (iter->type == type) {
            result = std::make_shared<ParseTree>(&(*iter));
            ++iter;
            return true;
        }
        return false;
    };
}

ParseFn Parser::maybe(ParseFn fn) {
    return [&](spParseTree& result, itToken& iter, const Token* limit) {
        return fn(result, iter, limit) || true;
    };
}

ParseFn Parser::first(std::vector<ParseFn> fns) {
    return [&](spParseTree& result, itToken& iter, const Token* limit) {
        itToken start = iter;
        for (auto& fn : fns) {
            if ( fn(result, iter, limit) ) return true;
            iter = start;
        }
        return false;
    };
}

ParseFn Parser::chain(std::vector<ParseFn> fns) {
    return [&](spParseTree& result, itToken& iter, const Token* limit) {
        for (auto& fn : fns) {
            if ( !fn(result, iter, limit) ) return false;
        }
        return true;
    };
}

bool Parser::parse() {

    return false;
}
