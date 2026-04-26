#ifndef TEST_AST_HELPERS_H
#define TEST_AST_HELPERS_H

#include "../AstBuilder.h"
#include "../Grammar2.h"
#include "../Lexer.h"
#include "../Parsing2.h"
#include "AstSerialize.h"

#include <sstream>
#include <string>
#include <string_view>

namespace basis {

    // Strip ASCII whitespace from a string. Used to compare expected vs. actual
    // AST text where the expected form may be split across multiple C++ string
    // literals with arbitrary indentation but the canonical serializer emits
    // no whitespace at all.
    inline std::string stripWs(std::string_view s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            if (c != ' ' && c != '\t' && c != '\n' && c != '\r') out += c;
        }
        return out;
    }

    // Parse `input` as a COMPILATION_UNIT, build the AST, serialize it, and
    // compare the (whitespace-stripped) result to `expected`. Returns true iff
    // every step succeeds and the serialized AST matches the expected form.
    //
    // The expected form should be written using the canonical AST text format
    // described in AstSerialize.h. C++ adjacent-string-literal concatenation
    // lets us write it hierarchically across multiple lines for readability;
    // stripWs reduces both sides to a canonical no-whitespace form for the
    // equality check.
    inline bool testAst(std::string_view input, std::string_view expected) {
        std::istringstream in{std::string{input}};
        Lexer lexer(in, false);
        if (!lexer.scan()) {
            MESSAGE("lexer.scan() failed for input: " << input);
            return false;
        }

        Parser parser(lexer.output, getGrammar().COMPILATION_UNIT);
        if (!parser.parse()) {
            MESSAGE("parser.parse() failed for input: " << input);
            return false;
        }
        if (!parser.allTokensConsumed()) {
            MESSAGE("parser did not consume all tokens for input: " << input);
            return false;
        }
        if (!parser.parseTree) {
            MESSAGE("parser produced null parse tree for input: " << input);
            return false;
        }
        if (parser.parseTree->production != Production::COMPILATION_UNIT) {
            MESSAGE("parse tree root is not COMPILATION_UNIT for input: " << input);
            return false;
        }

        auto cu = buildAst(parser.parseTree);
        if (!cu) {
            MESSAGE("buildAst returned null for input: " << input);
            return false;
        }

        std::string got = serializeAst(*cu);
        bool ok = stripWs(got) == stripWs(expected);
        if (!ok) {
            MESSAGE("AST mismatch for input: " << input
                << "\n  expected: " << stripWs(expected)
                << "\n  actual:   " << got);
        }
        return ok;
    }

} // namespace basis

#endif // TEST_AST_HELPERS_H
