#include "Parser.h"

using namespace basis;

Parser::Parser(const std::list<Token>& tokens, bool deferParseFn) : tokens(tokens) {
    if ( deferParseFn ) return;
    // parseFn =
}
Parser::~Parser() {}

void Parser::setParseFn(ParseFn fn) {
    parseFn = fn;
}

bool Parser::atLimit(itToken& iter, const Token* limit) const {
    return iter == tokens.cend() || (limit != nullptr && &(*iter) == limit);
}

ParseFn Parser::match(const TokenType& type) const {
    return [&](spParseObject& _unused, itToken& iter, const Token* limit) {
        if ( atLimit(iter, limit) ) return false;
        if (iter->type == type) {
            ++iter;
            return true;
        }
        return false;
    };
}

ParseFn Parser::match(const Production& production, const TokenType& type) const {
    return [&](spParseObject& result, itToken& iter, const Token* limit) {
        clearParseObject(result);
        if ( atLimit(iter, limit) ) return false;
        if (iter->type == type) {
             result = makeParseLeaf( production, &(*iter) );
            ++iter;
            return true;
        }
        return false;
    };
}

ParseFn Parser::maybe(ParseFn fn) {
    return [&](spParseObject& result, itToken& iter, const Token* limit) {
        return fn(result, iter, limit) || true;
    };
}

ParseFn Parser::choice(std::vector<ParseFn> fns) {
    return [&](spParseObject& result, itToken& iter, const Token* limit) {
        itToken start = iter;
        for (auto& fn : fns) {
            if ( fn(result, iter, limit) ) return true;
            iter = start;
        }
        clearParseObject(result);
        return false;
    };
}

ParseFn Parser::sequence(const Production& production, std::vector<ParseFn> fns) {
    return [&](spParseObject& result, itToken& iter, const Token* limit) {
        spParseObject seq = makeParseNode(production);
        spParseObject& next = getLinkDown(seq); // contents; not the actual next
        for ( auto& fn : fns ) {
            if ( fn(next, iter, limit) ) {
                next = getLinkNext(next);
            } else {
                clearParseObject(result);
                return false;
            }
        }
        return true;
    };
}

bool Parser::parse() {
    itToken start = tokens.cbegin();
    return parseFn(parseTree, start, start->bound);
}

