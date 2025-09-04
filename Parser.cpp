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
    return (*parseFn)(&pTree, &start, start->bound);
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

bool Match::operator()(spParseTree** pspResult, itToken* pIter, const Token* pLimit) {
    if ( parser.atLimit(pIter, pLimit) ) return false;
    if ((*pIter)->type == type) {
        **pspResult = std::make_shared<ParseTree>(production, &(**pIter));
        ++(*pIter);
        *pspResult = &((**pspResult)->spNext);
        return true;
    }
    return false;
}

std::shared_ptr<Match> basis::match(const Production production, Parser& parser, const TokenType& type) {
    return std::make_shared<Match>(production, parser, type);
}

bool Maybe::operator()(spParseTree** pspResult, itToken* pIter, const Token* pLimit) {
    return (*fn)(pspResult, pIter, pLimit) || true;
}

std::shared_ptr<Maybe> basis::maybe(const spParseFn fn) {
    return std::make_shared<Maybe>(fn);
}

bool FirstOf::operator()(spParseTree** pspResult, itToken* pIter, const Token* pLimit) {
    itToken start = *pIter;
    for (auto& fn : fns) {
        if ( (*fn)(pspResult, pIter, pLimit) ) return true;
        (*pIter) = start;
    }
    return false;
}

std::shared_ptr<FirstOf> basis::firstOf(const std::vector<spParseFn> fns) {
    return std::make_shared<FirstOf>(fns);
}

bool AllOf::operator()(spParseTree** pspResult, itToken* pIter, const Token* pLimit) {
    spParseTree* next = *pspResult;
    for ( auto& fn : fns ) {
        if ( (*fn)(&next, pIter, pLimit) ) {
            // be sure to check for nothing in case the parseFn is a discard
            if ( *next ) next = &((*next)->spNext);
        } else {
            return false;
        }
    }
    *pspResult = next;
    return true;
}

std::shared_ptr<AllOf> basis::allOf(const Production production, const std::vector<spParseFn> fns) {
    return std::make_shared<AllOf>(fns);
}

bool OneOrMore::operator()(spParseTree** pspResult, itToken* pIter, const Token* pLimit) {
    if (! (*fn)(pspResult, pIter, pLimit) ) return false;
    spParseTree* next = *pspResult;
    if ( *next ) next = &((*next)->spNext);
    while( (*fn)(&next, pIter, pLimit) ) {
        if ( *next ) next = &((*next)->spNext);
    }
    *pspResult = next;
    return true;
}

std::shared_ptr<OneOrMore> basis::oneOrMore(const Production production, spParseFn fn) {
    return std::make_shared<OneOrMore>(fn);
}

