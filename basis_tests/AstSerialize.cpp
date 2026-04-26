#include "AstSerialize.h"
#include <sstream>
#include <variant>

namespace basis {
namespace {

class Serializer {
public:
    std::string take() { return std::move(out); }

    // --- Top level ---

    void emit(const CompilationUnit& cu) {
        out += "CompilationUnit(";
        bool first = true;
        if (cu.module) { open(first); emit(*cu.module); }
        for (auto& imp : cu.imports) { if (imp) { open(first); emit(*imp); } }
        for (auto& d : cu.definitions) { open(first); emit(d); }
        out += ")";
    }

    void emit(const ModuleDecl& m) {
        out += "ModuleDecl(";
        emitStr(m.name);
        out += ")";
    }

    void emit(const ImportDecl& i) {
        out += "ImportDecl(";
        out += (i.kind == ImportDecl::Kind::File) ? "File" : "Standard";
        out += ",";
        if (i.kind == ImportDecl::Kind::File) {
            emitStr(i.path);
        } else {
            emitStr(i.name);
        }
        if (!i.alias.empty()) {
            out += ",alias=";
            emitStr(i.alias);
        }
        out += ")";
    }

    void emit(const TopLevelDef& d) {
        std::visit([this](const auto& alt) { emit(alt); }, d);
    }

    // --- Top-level definitions ---

    void emit(const AliasDecl& a) {
        out += "AliasDecl(";
        emitStr(a.name);
        out += ",";
        emit(a.type);
        out += ")";
    }

    void emit(const DomainDecl& d) {
        out += "DomainDecl(";
        emitStr(d.name);
        out += ",";
        emit(d.parent);
        out += ")";
    }

    void emit(const EnumDecl& e) {
        out += "EnumDecl(";
        emitStr(e.enumName);
        if (!e.enumTypeName.empty()) {
            out += ",constraint=";
            emitStr(e.enumTypeName);
        }
        for (auto& item : e.items) { out += ","; emit(item); }
        out += ")";
    }

    void emit(const EnumItem& it) {
        out += "EnumItem(";
        emitStr(it.name);
        if (it.value) {
            out += ",";
            emitStr(*it.value);
        }
        out += ")";
    }

    void emit(const RecordDecl& r) {
        out += "RecordDecl(";
        emitStr(r.name);
        for (auto& f : r.fields) { out += ","; emit(f); }
        out += ")";
    }

    void emit(const ObjectDecl& o) {
        out += "ObjectDecl(";
        emitStr(o.name);
        for (auto& f : o.fields) { out += ","; emit(f); }
        out += ")";
    }

    void emit(const UnionDecl& u) {
        out += "UnionDecl(";
        emitStr(u.name);
        for (auto& c : u.candidates) { out += ","; emit(c); }
        out += ")";
    }

    void emit(const VariantDecl& v) {
        out += "VariantDecl(";
        emitStr(v.name);
        for (auto& c : v.candidates) { out += ","; emit(c); }
        out += ")";
    }

    void emit(const InstanceDecl& i) {
        out += "InstanceDecl(";
        emitStr(i.name);
        for (auto& t : i.types) { out += ","; emit(t); }
        out += ")";
    }

    void emit(const CmdDecl& c) {
        out += "CmdDecl(";
        emit(c.signature);
        out += ")";
    }

    void emit(const IntrinsicDecl& i) {
        out += "IntrinsicDecl(";
        emit(i.signature);
        out += ")";
    }

    void emit(const CmdDef& c) {
        out += "CmdDef(";
        emit(c.signature);
        if (c.body) { out += ","; emit(*c.body); }
        out += ")";
    }

    void emit(const ClassDecl& c) {
        out += "ClassDecl(";
        emitStr(c.name);
        for (auto& m : c.members) {
            out += ",";
            std::visit([this](const auto& alt) { emit(alt); }, m);
        }
        out += ")";
    }

    void emit(const ProgramDecl& p) {
        out += "ProgramDecl(";
        emit(p.entryPoint);
        out += ")";
    }

    void emit(const TestDecl& t) {
        out += "TestDecl(";
        emitStr(t.label);
        if (t.body) { out += ","; emit(*t.body); }
        out += ")";
    }

    // --- Children of definitions ---

    void emit(const FieldDecl& f) {
        out += "FieldDecl(";
        emit(f.type);
        out += ",";
        emitStr(f.name);
        out += ")";
    }

    void emit(const UnionCandidate& c) {
        out += "UnionCandidate(";
        emit(c.domain);
        out += ",";
        emitStr(c.name);
        out += ")";
    }

    void emit(const VariantCandidate& c) {
        out += "VariantCandidate(";
        emit(c.type);
        out += ",";
        emitStr(c.name);
        out += ")";
    }

    void emit(const InstanceType& t) {
        out += "InstanceType(";
        emitStr(t.typeName);
        if (!t.delegate.empty()) {
            out += ",delegate=";
            emitStr(t.delegate);
        }
        out += ")";
    }

    // --- Signatures ---

    void emit(const CmdSignature& s) {
        std::visit([this](const auto& alt) { emit(alt); }, s);
    }

    void emit(const RegularSig& r) {
        out += "RegularSig(";
        emitStr(r.name);
        if (r.failMode != FailMode::NoFail) {
            out += ",fail=";
            out += failModeName(r.failMode);
        }
        for (auto& p : r.params) { out += ",param="; emit(p); }
        for (auto& p : r.implicitParams) { out += ",implicit="; emit(p); }
        if (!r.returnVal.empty()) {
            out += ",ret=";
            emitStr(r.returnVal);
        }
        out += ")";
    }

    void emit(const VCommandSig& v) {
        out += "VCommandSig(";
        emitStr(v.name);
        if (v.failMode != FailMode::NoFail) {
            out += ",fail=";
            out += failModeName(v.failMode);
        }
        for (auto& r : v.receivers) { out += ",receiver="; emit(r); }
        for (auto& p : v.params) { out += ",param="; emit(p); }
        for (auto& p : v.implicitParams) { out += ",implicit="; emit(p); }
        if (!v.returnVal.empty()) {
            out += ",ret=";
            emitStr(v.returnVal);
        }
        out += ")";
    }

    void emit(const ConstructorSig& c) {
        out += "ConstructorSig(";
        emit(c.receiver);
        for (auto& p : c.params) { out += ","; emit(p); }
        out += ")";
    }

    void emit(const DestructorSig& d) {
        out += "DestructorSig(";
        emit(d.receiver);
        out += ")";
    }

    void emit(const FailHandlerSig& f) {
        out += "FailHandlerSig(";
        emit(f.receiver);
        out += ")";
    }

    void emit(const CmdReceiver& r) {
        out += "CmdReceiver(";
        emit(r.type);
        out += ",";
        emitStr(r.name);
        out += ")";
    }

    void emit(const CmdParam& p) {
        out += "CmdParam(";
        emit(p.type);
        out += ",";
        emitStr(p.name);
        if (p.isTypeVar) {
            out += ",typeVar=";
            emitStr(p.typeVarName);
        }
        out += ")";
    }

    // --- Body / call-group / statements ---

    void emit(const CmdBody& b) {
        out += "CmdBody(";
        bool first = true;
        for (auto& sub : b.subs) { open(first); out += "sub="; emit(sub); }
        if (b.isEmpty) {
            open(first);
            out += "empty";
        } else if (b.group) {
            open(first);
            emit(*b.group);
        }
        out += ")";
    }

    void emit(const CallGroup& g) {
        out += "CallGroup(";
        bool first = true;
        for (auto& s : g.statements) { open(first); emit(s); }
        out += ")";
    }

    void emit(const StatNode& s) {
        std::visit([this](const auto& alt) { emit(alt); }, s.v);
    }

    void emit(const AssignStat& a) {
        out += "AssignStat(";
        emit(a.target);
        out += ",";
        emit(a.value);
        out += ")";
    }

    void emit(const ExprStat& e) {
        out += "ExprStat(";
        emit(e.expr);
        out += ")";
    }

    void emit(const Block& b) {
        out += "Block(";
        out += blockKindName(b.kind);
        if (b.kind == Block::Kind::DoRecoverSpec) {
            if (b.recoverType) { out += ",type="; emit(b.recoverType); }
            if (!b.recoverIdent.empty()) {
                out += ",ident=";
                emitStr(b.recoverIdent);
            }
            if (b.recoverExpr) { out += ",expr="; emit(b.recoverExpr); }
        }
        if (b.body) { out += ","; emit(*b.body); }
        out += ")";
    }

    // --- Types ---

    void emit(const TypeNodePtr& p) {
        if (!p) { out += "null"; return; }
        emit(*p);
    }

    void emit(const TypeNode& n) {
        std::visit([this](const auto& alt) { emit(alt); }, n.v);
    }

    void emit(const NamedType& n) {
        out += "NamedType(";
        emitStr(n.name);
        for (auto& a : n.typeArgs) { out += ","; emit(a); }
        if (n.writeable) out += ",writeable";
        out += ")";
    }

    void emit(const PtrType& p) {
        out += "PtrType(";
        out += std::to_string(p.depth);
        out += ",";
        emit(p.inner);
        out += ")";
    }

    void emit(const RangeType& r) {
        out += "RangeType(";
        emitStr(r.size);
        if (r.element) { out += ","; emit(r.element); }
        out += ")";
    }

    void emit(const CmdType& c) {
        out += "CmdType(";
        out += cmdTypeKindName(c.kind);
        for (auto& a : c.args) {
            out += ",";
            out += a.writeable ? "CmdTypeArg(true," : "CmdTypeArg(false,";
            emit(a.type);
            out += ")";
        }
        out += ")";
    }

    void emit(const InlineRecordType& t) {
        out += "InlineRecordType(";
        bool first = true;
        if (!t.scopeName.empty()) {
            out += "scope=";
            emitStr(t.scopeName);
            first = false;
        }
        for (auto& f : t.fields) { open(first); emit(f); }
        out += ")";
    }

    void emit(const InlineObjectType& t) {
        out += "InlineObjectType(";
        bool first = true;
        if (!t.scopeName.empty()) {
            out += "scope=";
            emitStr(t.scopeName);
            first = false;
        }
        for (auto& f : t.fields) { open(first); emit(f); }
        out += ")";
    }

    void emit(const InlineUnionType& t) {
        out += "InlineUnionType(";
        bool first = true;
        if (!t.scopeName.empty()) {
            out += "scope=";
            emitStr(t.scopeName);
            first = false;
        }
        for (auto& c : t.candidates) { open(first); emit(c); }
        out += ")";
    }

    void emit(const InlineVariantType& t) {
        out += "InlineVariantType(";
        bool first = true;
        if (!t.scopeName.empty()) {
            out += "scope=";
            emitStr(t.scopeName);
            first = false;
        }
        for (auto& c : t.candidates) { open(first); emit(c); }
        out += ")";
    }

    // --- Expressions ---

    void emit(const ExprNodePtr& p) {
        if (!p) { out += "null"; return; }
        emit(*p);
    }

    void emit(const ExprNode& n) {
        std::visit([this](const auto& alt) { emit(alt); }, n.v);
    }

    void emit(const LiteralExpr& l) {
        out += "LiteralExpr(";
        emitStr(l.text);
        out += ")";
    }

    void emit(const IdentifierExpr& i) {
        out += "IdentifierExpr(";
        emitIdent(i.ident);
        if (i.isAlloc) out += ",alloc";
        out += ")";
    }

    void emit(const EnumDerefExpr& e) {
        out += "EnumDerefExpr(";
        emitStr(e.typeName);
        out += ",";
        emitStr(e.memberName);
        out += ")";
    }

    void emit(const CallCommandExpr& c) {
        out += "CallCommandExpr(";
        emit(c.target);
        for (auto& p : c.params) { out += ","; emit(p); }
        out += ")";
    }

    void emit(const CallConstructorExpr& c) {
        out += "CallConstructorExpr(";
        emit(c.typeName);
        for (auto& p : c.params) { out += ","; emit(p); }
        out += ")";
    }

    void emit(const CallVCommandExpr& v) {
        out += "CallVCommandExpr(";
        emitStr(v.name);
        for (auto& r : v.receivers) {
            out += ",receiver=";
            emitStr(r);
        }
        for (auto& p : v.params) { out += ","; emit(p); }
        out += ")";
    }

    void emit(const CallFailExpr& f) {
        out += "CallFailExpr(";
        emit(f.expr);
        out += ")";
    }

    void emit(const SuffixExpr& s) {
        out += "SuffixExpr(";
        emit(s.base);
        for (auto& sop : s.suffixes) {
            out += ",";
            switch (sop.kind) {
                case SuffixOp::Kind::Deref: out += "Deref"; break;
                case SuffixOp::Kind::Addr:  out += "Addr"; break;
                case SuffixOp::Kind::Index:
                    out += "Index(";
                    emit(sop.indexLoc);
                    if (sop.indexExt) { out += ","; emit(sop.indexExt); }
                    out += ")";
                    break;
            }
        }
        out += ")";
    }

    void emit(const BinaryExpr& b) {
        out += "BinaryExpr(";
        emit(b.first);
        for (auto& ot : b.rest) {
            out += ",";
            emitStr(ot.op);
            out += ",";
            emit(ot.term);
        }
        out += ")";
    }

    void emit(const QuoteExpr& q) {
        out += "QuoteExpr(";
        out += quoteKindName(q.kind);
        if (q.invoke) { out += ","; emit(q.invoke); }
        if (q.group)  { out += ","; emit(*q.group); }
        out += ")";
    }

    void emit(const CmdLiteralExpr& c) {
        out += "CmdLiteralExpr(";
        out += cmdLitKindName(c.kind);
        for (auto& p : c.params) { out += ",param="; emit(p); }
        if (c.body) { out += ","; emit(*c.body); }
        out += ")";
    }

    void emit(const CallParam& p) {
        out += "CallParam(";
        if (p.isEmpty) {
            out += "empty";
        } else {
            emit(p.expr);
        }
        out += ")";
    }

private:
    std::string out;

    void open(bool& first) {
        if (first) first = false;
        else out += ",";
    }

    void emitStr(const std::string& s) {
        out += '\'';
        for (char c : s) {
            if (c == '\\' || c == '\'') out += '\\';
            out += c;
        }
        out += '\'';
    }

    void emitIdent(const Identifier& id) {
        std::string joined;
        for (auto& q : id.qualifiers) { joined += q; joined += "::"; }
        joined += id.name;
        emitStr(joined);
    }

    static const char* failModeName(FailMode m) {
        switch (m) {
            case FailMode::NoFail:  return "NoFail";
            case FailMode::MayFail: return "MayFail";
            case FailMode::Fails:   return "Fails";
        }
        return "?";
    }

    static const char* cmdTypeKindName(CmdType::Kind k) {
        switch (k) {
            case CmdType::Kind::NoFail:  return "NoFail";
            case CmdType::Kind::MayFail: return "MayFail";
            case CmdType::Kind::Fails:   return "Fails";
        }
        return "?";
    }

    static const char* cmdLitKindName(CmdLiteralExpr::Kind k) {
        switch (k) {
            case CmdLiteralExpr::Kind::NoFail:   return "NoFail";
            case CmdLiteralExpr::Kind::MayFail:  return "MayFail";
            case CmdLiteralExpr::Kind::MustFail: return "MustFail";
        }
        return "?";
    }

    static const char* quoteKindName(QuoteExpr::Kind k) {
        switch (k) {
            case QuoteExpr::Kind::Subquote:     return "Subquote";
            case QuoteExpr::Kind::BlockNoFail:  return "BlockNoFail";
            case QuoteExpr::Kind::BlockMayFail: return "BlockMayFail";
            case QuoteExpr::Kind::BlockFail:    return "BlockFail";
        }
        return "?";
    }

    static const char* blockKindName(Block::Kind k) {
        switch (k) {
            case Block::Kind::DoWhen:        return "DoWhen";
            case Block::Kind::DoWhenMulti:   return "DoWhenMulti";
            case Block::Kind::DoWhenFail:    return "DoWhenFail";
            case Block::Kind::DoWhenSelect:  return "DoWhenSelect";
            case Block::Kind::DoElse:        return "DoElse";
            case Block::Kind::DoBlock:       return "DoBlock";
            case Block::Kind::DoRewind:      return "DoRewind";
            case Block::Kind::DoRecover:     return "DoRecover";
            case Block::Kind::DoRecoverSpec: return "DoRecoverSpec";
            case Block::Kind::DoOnExit:      return "DoOnExit";
            case Block::Kind::DoOnExitFail:  return "DoOnExitFail";
        }
        return "?";
    }
};

} // namespace

std::string serializeAst(const CompilationUnit& cu) {
    Serializer s;
    s.emit(cu);
    return s.take();
}

} // namespace basis
