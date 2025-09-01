#include "Parser.h"

using namespace basis;

Parser::Parser(const std::list<Token>& tokens, bool deferParseFn) : tokens(tokens) {
    if ( deferParseFn ) return;
    // parseFn =
}
Parser::~Parser() {}

void Parser::setParseFn(spParseFn fn) {
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

bool Discard::operator()(vParseObject& result, itToken& iter, const Token* limit) {
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

bool Match::operator()(vParseObject& result, itToken& iter, const Token* limit) {
    if ( parser.atLimit(iter, limit) ) return false;
    if (iter->type == type) {
        result = makeParseLeaf(production, &(*iter));
        ++iter;
        return true;
    }
    return false;
}

std::shared_ptr<Match> basis::match(const Production production, Parser& parser, const TokenType& type) {
    return std::make_shared<Match>(production, parser, type);
}

bool Maybe::operator()(vParseObject& result, itToken& iter, const Token* limit) {
    return (*fn)(result, iter, limit) || true;
}

std::shared_ptr<Maybe> basis::maybe(const spParseFn fn) {
    return std::make_shared<Maybe>(fn);
}

bool FirstOf::operator()(vParseObject& result, itToken& iter, const Token* limit) {
    itToken start = iter;
    for (auto& fn : fns) {
        if ( (*fn)(result, iter, limit) ) return true;
        iter = start;
    }
    clearParseObject(result);
    return false;
}

std::shared_ptr<FirstOf> basis::firstOf(const std::vector<spParseFn> fns) {
    return std::make_shared<FirstOf>(fns);
}

bool AllOf::operator()(vParseObject& result, itToken& iter, const Token* limit) {
    vParseObject seq = makeParseNode(production);
    vParseObject& next = getLinkDown(seq); // contents; not the actual next
    for ( auto& fn : fns ) {
        if ( (*fn)(next, iter, limit) ) {
            // be sure to check for nothing in case the parseFn is a discard
            if ( !isEmptyVariant(next) ) next = getLinkNext(next);
        } else {
            return false;
        }
    }
    result = seq;
    return true;
}
std::shared_ptr<AllOf> basis::allOf(const Production production, const std::vector<spParseFn> fns) {
    return std::make_shared<AllOf>(production, fns);
}

bool OneOrMore::operator()(vParseObject& result, itToken& iter, const Token* limit) {
    if (! (*fn)(result, iter, limit) ) return false;
    vParseObject& next = getLinkNext(result);
    while( (*fn)(next, iter, limit) ) {
        next = getLinkNext(next);
    }
    return true;
}
std::shared_ptr<OneOrMore> basis::oneOrMore(const Production production, spParseFn fn) {
    return std::make_shared<OneOrMore>(production, fn);
}

bool Obliged::operator()(vParseObject& result, itToken& iter, const Token* limit) {
    clearParseObject(result);
    if ( !(*prefix)(result, iter, limit) ) return false;
    if ( isEmptyVariant(result) ) {
        // the prefix added no production, create one
        result = makeParseNode(production);
    } else {
        // the prefix added a production, subsume it
        vParseObject temp = makeParseNode(production);
        getLinkDown(temp) = result;
        result = temp;
    }
    return (*suffix)(getLinkDown(result), iter, limit);
}
std::shared_ptr<Obliged> basis::obliged(const Production production, spParseFn prefix, spParseFn suffix) {
    return std::make_shared<Obliged>(production, prefix, suffix);
}


