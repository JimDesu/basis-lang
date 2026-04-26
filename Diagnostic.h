#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

namespace basis {

    enum class Severity { Note, Warning, Error, Fatal };
    enum class Phase    { Lex, Parse, Build, Sema };

    struct SourceLoc {
        size_t line = 0;
        size_t col  = 0;
        bool present() const { return line != 0 || col != 0; }
    };

    struct Diagnostic {
        Severity    severity = Severity::Error;
        Phase       phase    = Phase::Lex;
        SourceLoc   loc;
        std::string message;
        SourceLoc   relatedLoc;
        std::string related;
    };

    class Diagnostics {
    public:
        void report(Diagnostic d);

        void note   (Phase p, SourceLoc loc, std::string msg);
        void warning(Phase p, SourceLoc loc, std::string msg);
        void error  (Phase p, SourceLoc loc, std::string msg);
        void fatal  (Phase p, SourceLoc loc, std::string msg);

        bool   hasErrors() const { return errors > 0 || fatals > 0; }
        bool   hasFatal()  const { return fatals > 0; }
        size_t errorCount() const { return errors + fatals; }

        const std::vector<Diagnostic>& all() const { return diags; }
        void clear();

        void append(Diagnostics&& other);

    private:
        std::vector<Diagnostic> diags;
        size_t errors = 0;
        size_t fatals = 0;
    };

    void printDiagnostics(std::ostream& os, const Diagnostics& d);
    void printDiagnostic(std::ostream& os, const Diagnostic& d);

    // Process-wide sink for callers that don't care to inspect (mostly tests).
    Diagnostics& discardDiagnostics();

}

#endif
