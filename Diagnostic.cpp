#include "Diagnostic.h"

#include <ostream>
#include <utility>

namespace basis {

    void Diagnostics::report(Diagnostic d) {
        if (d.severity == Severity::Error) ++errors;
        else if (d.severity == Severity::Fatal) ++fatals;
        diags.push_back(std::move(d));
    }

    void Diagnostics::note(Phase p, SourceLoc loc, std::string msg) {
        report({Severity::Note, p, loc, std::move(msg), {}, {}});
    }
    void Diagnostics::warning(Phase p, SourceLoc loc, std::string msg) {
        report({Severity::Warning, p, loc, std::move(msg), {}, {}});
    }
    void Diagnostics::error(Phase p, SourceLoc loc, std::string msg) {
        report({Severity::Error, p, loc, std::move(msg), {}, {}});
    }
    void Diagnostics::fatal(Phase p, SourceLoc loc, std::string msg) {
        report({Severity::Fatal, p, loc, std::move(msg), {}, {}});
    }

    void Diagnostics::clear() {
        diags.clear();
        errors = 0;
        fatals = 0;
    }

    void Diagnostics::append(Diagnostics&& other) {
        diags.reserve(diags.size() + other.diags.size());
        for (auto& d : other.diags) diags.push_back(std::move(d));
        errors += other.errors;
        fatals += other.fatals;
        other.clear();
    }

    static const char* severityLabel(Severity s) {
        switch (s) {
            case Severity::Note:    return "note";
            case Severity::Warning: return "warning";
            case Severity::Error:   return "error";
            case Severity::Fatal:   return "fatal";
        }
        return "?";
    }

    static const char* phaseLabel(Phase p) {
        switch (p) {
            case Phase::Lex:   return "lex";
            case Phase::Parse: return "parse";
            case Phase::Build: return "build";
            case Phase::Sema:  return "sema";
        }
        return "?";
    }

    void printDiagnostic(std::ostream& os, const Diagnostic& d) {
        os << '[' << phaseLabel(d.phase) << "] " << severityLabel(d.severity) << ": ";
        if (d.loc.present()) os << '(' << d.loc.line << ':' << d.loc.col << ") ";
        os << d.message;
        if (!d.related.empty()) {
            os << "\n  ";
            if (d.relatedLoc.present()) os << '(' << d.relatedLoc.line << ':' << d.relatedLoc.col << ") ";
            os << d.related;
        }
        os << '\n';
    }

    void printDiagnostics(std::ostream& os, const Diagnostics& d) {
        for (auto& diag : d.all()) printDiagnostic(os, diag);
    }

    Diagnostics& discardDiagnostics() {
        // Cleared on each access so callers that don't inspect don't accumulate.
        // Not thread-safe; the test runner is sequential.
        static Diagnostics sink;
        sink.clear();
        return sink;
    }

}
