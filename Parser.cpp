#include "Parser.h"

using namespace basis;

Parser::Parser(const std::list<Token>& tokens, bool deferParseFn) : tokens(tokens) {
    if ( deferParseFn ) return;
    // parseFn =
}
Parser::~Parser() {}

void Parser::setParseFn(spParseFn fn) {
    parseTree.reset();
    parseFn = fn;
}

spParseFn Parser::buildParser() {
    return nullptr; // TODO
}

bool Parser::atLimit(itToken& iter, const Token* limit) const {
    return iter == tokens.cend() || (limit != nullptr && &(*iter) == limit);
}

bool Parser::parse() {
    itToken start = tokens.cbegin();
    return (*parseFn)(parseTree, start, start->bound);
}

bool Discard::operator()(spParseTree& _unused, itToken& iter, const Token* limit) {
    if ( parser.atLimit(iter, limit) ) return false;
    if (iter->type == type) {
        ++iter;
        return true;
    }
    return false;
}

std::shared_ptr<Discard> basis::discard(Parser& parser, const TokenType& type) {
    return std::make_shared<Discard>(parser, type);
}

bool Match::operator()(spParseTree& result, itToken& iter, const Token* limit) {
    if ( parser.atLimit(iter, limit) ) return false;
    if (iter->type == type) {
        result = std::make_shared<ParseTree>(production, &(*iter));
        ++iter;
        return true;
    }
    return false;
}

std::shared_ptr<Match> basis::match(const Production production, Parser& parser, const TokenType& type) {
    return std::make_shared<Match>(production, parser, type);
}

bool Maybe::operator()(spParseTree& result, itToken& iter, const Token* limit) {
    return (*fn)(result, iter, limit) || true;
}

std::shared_ptr<Maybe> basis::maybe(const spParseFn fn) {
    return std::make_shared<Maybe>(fn);
}

bool FirstOf::operator()(spParseTree& result, itToken& iter, const Token* limit) {
    itToken start = iter;
    for (auto& fn : fns) {
        if ( (*fn)(result, iter, limit) ) return true;
        iter = start;
    }
    return false;
}

std::shared_ptr<FirstOf> basis::firstOf(const std::vector<spParseFn> fns) {
    return std::make_shared<FirstOf>(fns);
}

bool AllOf::operator()(spParseTree& result, itToken& iter, const Token* limit) {
    spParseTree* next = &result;
    for ( auto& fn : fns ) {
        if ( (*fn)(*next, iter, limit) ) {
            // be sure to check for nothing in case the parseFn is a discard
            if ( *next ) next = &((*next)->spNext);
        } else {
            return false;
        }
    }
    return true;
}

std::shared_ptr<AllOf> basis::allOf(const Production production, const std::vector<spParseFn> fns) {
    return std::make_shared<AllOf>(fns);
}

bool OneOrMore::operator()(spParseTree& result, itToken& iter, const Token* limit) {
    if (! (*fn)(result, iter, limit) ) return false;
    spParseTree* next = result ? &(result->spDown) : &result;
    while( (*fn)(*next, iter, limit) ) {
        if ( *next ) next = &((*next)->spNext);
    }
    return true;
}

std::shared_ptr<OneOrMore> basis::oneOrMore(const Production production, spParseFn fn) {
    return std::make_shared<OneOrMore>(fn);
}


