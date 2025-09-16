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

bool Parser::atLimit(itToken* pIter, const Token* pLimit) const {
    return (*pIter) == tokens.cend() || (pLimit != nullptr && &(**pIter) == pLimit);
}

bool Parser::parse() {
    itToken start = tokens.cbegin();
    spParseTree* pTree = &parseTree;
    return (*parseFn)(&pTree, &start, nullptr);
}

bool Discard::operator()(spParseTree** _unused, itToken* pIter, const Token* pLimit) {
    if ( parser.atLimit(pIter, pLimit) ) return false;
    if ((*pIter)->type == type) {
        ++(*pIter);
        return true;
    }
    return false;
}

std::shared_ptr<Discard> basis::discard(Parser& parser, const TokenType& type) {
    return std::make_shared<Discard>(parser, type);
}

bool Match::operator()(spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
    if ( parser.atLimit(pIter, pLimit) ) return false;
    if ((*pIter)->type == type) {
        **dpspResult = std::make_shared<ParseTree>(production, &(**pIter));
        ++(*pIter);
        *dpspResult = &((**dpspResult)->spNext);
        return true;
    }
    return false;
}

std::shared_ptr<Match> basis::match(const Production production, Parser& parser, const TokenType& type) {
    return std::make_shared<Match>(production, parser, type);
}

bool Maybe::operator()(spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
    return (*fn)(dpspResult, pIter, pLimit) || true;
}

std::shared_ptr<Maybe> basis::maybe(const spParseFn fn) {
    return std::make_shared<Maybe>(fn);
}

bool Any::operator()(spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
    itToken start = *pIter;
    for (auto& fn : fns) {
        if ( (*fn)(dpspResult, pIter, pLimit) ) return true;
        (*pIter) = start;
    }
    return false;
}

std::shared_ptr<Any> basis::any(const std::vector<spParseFn> fns) {
    return std::make_shared<Any>(fns);
}

bool All::operator()(spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
    spParseTree* next = *dpspResult;
    for ( auto& fn : fns ) {
        if ( (*fn)(&next, pIter, pLimit) ) {
            // be sure to check for nothing in case the parseFn is a discard
            if ( *next ) next = &((*next)->spNext);
        } else {
            return false;
        }
    }
    *dpspResult = next;
    return true;
}

std::shared_ptr<All> basis::all(const std::vector<spParseFn> fns) {
    return std::make_shared<All>(fns);
}

bool OneOrMore::operator()(spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
    if (! (*fn)(dpspResult, pIter, pLimit) ) return false;
    spParseTree* next = *dpspResult;
    if ( *next ) next = &((*next)->spNext);
    while( (*fn)(&next, pIter, pLimit) ) {
        if ( *next ) next = &((*next)->spNext);
    }
    *dpspResult = next;
    return true;
}

std::shared_ptr<OneOrMore> basis::oneOrMore(spParseFn fn) {
    return std::make_shared<OneOrMore>(fn);
}

bool Bound::operator()(spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {
    return (*fn)(dpspResult, pIter, (*pIter)->bound);
}

std::shared_ptr<Bound> basis::bound(spParseFn fn) {
    return std::make_shared<Bound>(fn);
}

// TODO
bool Group::operator()(spParseTree** dpspResult, itToken* pIter, const Token* pLimit) {}
std::shared_ptr<Group> basis::group(const Production production, spParseFn fn) {}

