#include "Parser.h"

using namespace basis;

Parser::Parser(const std::list<Token>& tokens) : tokens(tokens) {
    //parseFn = buildParser();
}
Parser::Parser(const Parser& other) : tokens(other.tokens) {}
Parser& Parser::operator=(const Parser& rhs) {
    if (this != &rhs) {
        tokens = rhs.tokens;
    }
    return *this;
}
Parser::~Parser() {}

bool Parser::atLimit(itToken& iter, const Token* limit) const {
    return iter != tokens.cend() && limit != nullptr && &(*iter) == limit;
}

ParseFn Parser::match(Production production, TokenType type, bool keep) const {
    return [&](spParseObject result, itToken& iter, const Token* limit) {
        if ( atLimit(iter, limit) ) return false;
        if (iter->type == type) {
            if ( keep ) result = makeParseLeaf( production, &(*iter) );
            ++iter;
            return true;
        }
        return false;
    };
}

//ParseFn Parser::maybe(ParseFn fn) {
    //return [&](spParseTree& result, itToken& iter, const Token* limit) {
        //return fn(result, iter, limit) || true;
    //};
//}

//ParseFn Parser::choice(std::vector<ParseFn> fns) {
    //return [&](spParseTree& result, itToken& iter, const Token* limit) {
        //itToken start = iter;
        //for (auto& fn : fns) {
            //if ( fn(result, iter, limit) ) return true;
            //iter = start;
        //}
        //return false;
    //};
//}

//ParseFn Parser::sequence(std::vector<ParseFn> fns) {
    //return [&](spParseTree& result, itToken& iter, const Token* limit) {
        //for (auto& fn : fns) {
            //// TODO: fix result accumulation
            //if ( !fn(result, iter, limit) ) return false;
        //}
        //return true;
    //};
//}

//ParseFn Parser::object(ParseFn head, ParseFn body) {
    //return [&](spParseTree& result, itToken& iter, const Token* limit) {
        ////TODO
        //return false;
    //};
//}

//ParseFn Parser::zeroOrMore(ParseFn fn) {
    //return [&](spParseTree& result, itToken& iter, const Token* limit) {
        ////TODO
        //return true;
    //};
//}
//ParseFn Parser::oneOrMore(ParseFn fn) {
    //return [&](spParseTree& result, itToken& iter, const Token* limit) {
        ////TODO
        //return true;
    //};
//}

bool Parser::parse() {
    itToken start = tokens.cbegin();
    return parseFn(parseTree, start, start->bound);
}

