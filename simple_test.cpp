#include "Token.h"
#include "ParseObject.h"
#include <list>
#include <memory>

using namespace basis;

// Simple test to check if basic template structure compiles
template<TokenType Type>
struct SimpleDiscard {
    template<typename ParserType>
    static bool parse(ParserType& parser, spParseTree** _unused, std::list<Token>::const_iterator* pIter, const Token* pLimit) {
        return true; // Simplified for testing
    }
};

template<typename ParseFnType>
class SimpleParser {
public:
    explicit SimpleParser(const std::list<Token>& tokens) : tokens(tokens) {}
    
    bool parse() {
        auto start = tokens.cbegin();
        spParseTree* pTree = &parseTree;
        return ParseFnType::parse(*this, &pTree, &start, nullptr);
    }
    
    spParseTree parseTree;
    
private:
    const std::list<Token>& tokens;
};

int main() {
    std::list<Token> tokens;
    tokens.emplace_back();
    tokens.back().type = TokenType::IDENTIFIER;
    
    SimpleParser<SimpleDiscard<TokenType::IDENTIFIER>> parser(tokens);
    bool result = parser.parse();
    return 0;
}
