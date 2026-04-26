#include "doctest.h"

#include "../Diagnostic.h"
#include "../Lexer.h"
#include "../Parsing2.h"
#include "../Grammar2.h"

#include <sstream>

using namespace basis;

TEST_CASE("Diagnostics::counts and severity flags") {
    Diagnostics d;
    CHECK_FALSE(d.hasErrors());
    CHECK_FALSE(d.hasFatal());
    CHECK_EQ(d.errorCount(), 0);

    d.note(Phase::Lex, {1, 1}, "hi");
    CHECK_FALSE(d.hasErrors());
    CHECK_EQ(d.all().size(), 1);

    d.warning(Phase::Lex, {1, 1}, "be careful");
    CHECK_FALSE(d.hasErrors());

    d.error(Phase::Parse, {2, 3}, "oops");
    CHECK(d.hasErrors());
    CHECK_FALSE(d.hasFatal());
    CHECK_EQ(d.errorCount(), 1);

    d.fatal(Phase::Parse, {2, 3}, "stop");
    CHECK(d.hasErrors());
    CHECK(d.hasFatal());
    CHECK_EQ(d.errorCount(), 2);
    CHECK_EQ(d.all().size(), 4);
}

TEST_CASE("Diagnostics::clear and append") {
    Diagnostics a;
    a.error(Phase::Lex, {1, 1}, "a1");
    a.warning(Phase::Lex, {1, 2}, "a2");

    Diagnostics b;
    b.error(Phase::Parse, {2, 1}, "b1");

    a.append(std::move(b));
    CHECK_EQ(a.all().size(), 3);
    CHECK_EQ(a.errorCount(), 2);
    CHECK_EQ(b.all().size(), 0);
    CHECK_FALSE(b.hasErrors());

    a.clear();
    CHECK_EQ(a.all().size(), 0);
    CHECK_FALSE(a.hasErrors());
}

TEST_CASE("Diagnostics::printer formats location and related info") {
    Diagnostic d{Severity::Error, Phase::Parse, {3, 7}, "boom",
                 SourceLoc{1, 4}, "started here"};
    std::ostringstream os;
    printDiagnostic(os, d);
    std::string s = os.str();
    CHECK(s.find("[parse]") != std::string::npos);
    CHECK(s.find("error:") != std::string::npos);
    CHECK(s.find("(3:7)") != std::string::npos);
    CHECK(s.find("boom") != std::string::npos);
    CHECK(s.find("(1:4)") != std::string::npos);
    CHECK(s.find("started here") != std::string::npos);
}

TEST_CASE("Diagnostics::lexer reports through Diagnostics") {
    Diagnostics diags;
    std::istringstream in("0xA ");  // invalid hex: odd digit count
    Lexer lexer(in, diags);
    lexer.scan();
    CHECK(diags.hasErrors());
    bool sawLex = false;
    for (auto& d : diags.all()) {
        if (d.phase == Phase::Lex && d.severity == Severity::Error) sawLex = true;
    }
    CHECK(sawLex);
}

TEST_CASE("Diagnostics::parser produces a structured diagnostic on failure") {
    Diagnostics diags;
    std::istringstream in(".cmd missing equals\n");
    Lexer lexer(in, diags);
    lexer.scan();
    Parser parser(lexer.output, getGrammar().COMPILATION_UNIT);
    bool ok = parser.parse() && parser.allTokensConsumed();
    REQUIRE_FALSE(ok);
    Diagnostic d = parser.getErrorDiagnostic();
    CHECK_EQ(d.phase, Phase::Parse);
    CHECK_EQ(d.severity, Severity::Error);
    CHECK_FALSE(d.message.empty());
}
