#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <cstddef>

namespace basis {

// ========================================================================
// Forward declarations of variant wrapper types
// ========================================================================
struct TypeNode;
struct ExprNode;

using TypeNodePtr = std::shared_ptr<TypeNode>;
using ExprNodePtr = std::shared_ptr<ExprNode>;

struct Located {
    size_t line = 0;
    size_t col = 0;
};

// ========================================================================
// Small helper / child structs (not in any variant themselves)
// ========================================================================
struct EnumItem : Located {
    std::string name;
    std::string value;   // literal text
};

struct FieldDecl : Located {
    TypeNodePtr type;
    std::string name;
};

struct UnionCandidate : Located {
    TypeNodePtr domain;
    std::string name;
};

struct VariantCandidate : Located {
    TypeNodePtr type;
    std::string name;
};

struct InstanceType : Located {
    std::string typeName;
    std::string delegate;   // identifier inside (...), empty if absent
};

struct CmdParam {
    TypeNodePtr type;
    std::string name;
    bool        isTypeVar   = false;
    std::string typeVarName;   // the T in (T : SomeType)
};

struct CmdReceiver {
    TypeNodePtr type;
    std::string name;
};

struct CallParam : Located {
    bool        isEmpty = false;
    ExprNodePtr expr;   // null when isEmpty
};

struct SuffixOp {
    enum class Kind { Deref, Index, Addr };
    Kind        kind = Kind::Deref;
    ExprNodePtr indexLoc;   // Index: location expression
    ExprNodePtr indexExt;   // Index: optional extent expression
};

// ========================================================================
// TypeNode alternatives
// ========================================================================
struct NamedType : Located {
    std::string              name;
    std::vector<TypeNodePtr> typeArgs;
    bool                     writeable = false;
};

struct PtrType : Located {
    int         depth = 1;
    TypeNodePtr inner;
};

struct RangeType : Located {
    std::string size;       // empty = unbounded
    TypeNodePtr element;    // optional element type
};

struct CmdTypeArg {
    TypeNodePtr type;
    bool        writeable = false;
};

struct CmdType : Located {
    enum class Kind { NoFail, MayFail, Fails };
    Kind                     kind = Kind::NoFail;
    std::vector<CmdTypeArg>  args;
};

struct InlineRecordType : Located {
    std::string              scopeName;   // optional
    std::vector<FieldDecl>   fields;
};

struct InlineObjectType : Located {
    std::string              scopeName;
    std::vector<FieldDecl>   fields;
};

struct InlineUnionType : Located {
    std::string                  scopeName;
    std::vector<UnionCandidate>  candidates;
};

struct InlineVariantType : Located {
    std::string                    scopeName;
    std::vector<VariantCandidate>  candidates;
};

// ---- TypeNode wrapper ----
struct TypeNode {
    using Variant = std::variant<
        NamedType, PtrType, RangeType, CmdType,
        InlineRecordType, InlineObjectType, InlineUnionType, InlineVariantType>;
    Variant v;
    template<typename T> TypeNode(T&& alt) : v(std::forward<T>(alt)) {}
};

// ========================================================================
// ExprNode alternatives
// ========================================================================
struct LiteralExpr : Located {
    std::string text;
};

// Qualified or unqualified identifier reference. `qualifiers` holds any
// `Std::Core::` prefix segments in order; `name` is the trailing identifier.
struct Identifier {
    std::vector<std::string> qualifiers;
    std::string              name;
};

struct IdentifierExpr : Located {
    Identifier ident;
    bool       isAlloc = false;   // true when prefixed with #
};

struct EnumDerefExpr : Located {
    std::string typeName;
    std::string memberName;
};

struct CallCommandExpr : Located {
    ExprNodePtr                target;    // IdentifierExpr or QuoteExpr
    std::vector<CallParam>     params;
};

struct CallConstructorExpr : Located {
    TypeNodePtr                typeName;
    std::vector<CallParam>     params;
};

struct CallVCommandExpr : Located {
    std::vector<std::string>   receivers;
    std::string                name;
    std::vector<CallParam>     params;
};

struct CallFailExpr : Located {
    ExprNodePtr expr;
};

struct SuffixExpr : Located {
    ExprNodePtr            base;
    std::vector<SuffixOp>  suffixes;
};

struct BinaryExpr : Located {
    struct OpTerm {
        std::string op;
        ExprNodePtr term;
    };
    ExprNodePtr          first;
    std::vector<OpTerm>  rest;
};

// Forward-declare CallGroup so QuoteExpr / CmdLiteralExpr can reference it
struct CallGroup;

struct QuoteExpr : Located {
    enum class Kind { Subquote, BlockNoFail, BlockMayFail, BlockFail };
    Kind                         kind = Kind::Subquote;
    ExprNodePtr                  invoke;   // Subquote: optional invoke
    std::shared_ptr<CallGroup>   group;    // Block quotes: body group
};

struct CmdLiteralExpr : Located {
    enum class Kind { NoFail, MayFail, MustFail };
    Kind                         kind = Kind::NoFail;
    std::vector<CmdParam>        params;
    std::shared_ptr<CallGroup>   body;
};

// ---- ExprNode wrapper ----
struct ExprNode {
    using Variant = std::variant<
        LiteralExpr, IdentifierExpr, EnumDerefExpr,
        CallCommandExpr, CallConstructorExpr, CallVCommandExpr, CallFailExpr,
        SuffixExpr, BinaryExpr, QuoteExpr, CmdLiteralExpr>;
    Variant v;
    template<typename T> ExprNode(T&& alt) : v(std::forward<T>(alt)) {}
};

// ========================================================================
// Statement-level types
// ========================================================================
struct AssignStat : Located {
    ExprNodePtr target;   // IdentifierExpr (possibly alloc)
    ExprNodePtr value;    // the RHS subcall expression
};

struct ExprStat : Located {
    ExprNodePtr expr;     // wraps a BinaryExpr or single-term expression
};

struct Block : Located {
    enum class Kind {
        DoWhen, DoWhenMulti, DoWhenFail, DoWhenSelect,
        DoElse, DoBlock, DoRewind,
        DoRecover, DoRecoverSpec,
        DoOnExit, DoOnExitFail
    };
    Kind                         kind = Kind::DoBlock;
    // DoRecoverSpec fields
    TypeNodePtr                  recoverType;   // optional TYPE_NAME_Q
    std::string                  recoverIdent;  // bound identifier
    ExprNodePtr                  recoverExpr;   // or CALL_EXPR_TERM fallback
    std::shared_ptr<CallGroup>   body;
};

// StatNode: one element of a CallGroup
struct StatNode {
    using Variant = std::variant<AssignStat, ExprStat, Block>;
    Variant v;
    template<typename T> StatNode(T&& alt) : v(std::forward<T>(alt)) {}
};

// CALL_GROUP: sequence of statements
struct CallGroup : Located {
    std::vector<StatNode> statements;
};

// DEF_CMD_BODY: = _ | = CALL_GROUP
struct CmdBody : Located {
    bool                       isEmpty = false;
    std::shared_ptr<CallGroup> group;   // null when isEmpty
};

// ========================================================================
// Command signature alternatives
// ========================================================================
enum class FailMode { NoFail, MayFail, Fails };

struct RegularSig {
    std::string            name;
    FailMode               failMode = FailMode::NoFail;
    std::vector<CmdParam>  params;
    std::vector<CmdParam>  implicitParams;
    std::string            returnVal;
};

struct VCommandSig {
    std::vector<CmdReceiver> receivers;
    std::string              name;
    FailMode                 failMode = FailMode::NoFail;
    std::vector<CmdParam>    params;
    std::vector<CmdParam>    implicitParams;
    std::string              returnVal;
};

struct ConstructorSig {
    CmdReceiver            receiver;
    std::vector<CmdParam>  params;
};

struct DestructorSig {
    CmdReceiver receiver;
};

struct FailHandlerSig {
    CmdReceiver receiver;
};

using CmdSignature = std::variant<
    RegularSig, VCommandSig, ConstructorSig, DestructorSig, FailHandlerSig>;

// ========================================================================
// Top-level declaration types
// ========================================================================
struct ModuleDecl : Located {
    std::string name;
};

struct ImportDecl : Located {
    enum class Kind { File, Standard };
    Kind        kind      = Kind::Standard;
    std::string path;
    std::string alias;
    std::string name;
};

struct AliasDecl : Located {
    std::string name;
    TypeNodePtr type;
};

struct DomainDecl : Located {
    std::string name;
    TypeNodePtr parent;
};

struct EnumDecl : Located {
    std::string            enumTypeName;   // optional constraining typename
    std::string            enumName;
    std::vector<EnumItem>  items;
};

struct RecordDecl : Located {
    std::string            name;
    std::vector<FieldDecl> fields;
};

struct ObjectDecl : Located {
    std::string            name;
    std::vector<FieldDecl> fields;
};

struct UnionDecl : Located {
    std::string                  name;
    std::vector<UnionCandidate>  candidates;
};

struct VariantDecl : Located {
    std::string                    name;
    std::vector<VariantCandidate>  candidates;
};

struct InstanceDecl : Located {
    std::string                  name;
    std::vector<InstanceType>    types;
};

struct CmdDecl : Located {
    CmdSignature signature;
};

struct IntrinsicDecl : Located {
    CmdSignature signature;
};

struct CmdDef : Located {
    CmdSignature              signature;
    std::shared_ptr<CmdBody>  body;
};

// ClassMember variant
using ClassMember = std::variant<CmdDecl, CmdDef>;

struct ClassDecl : Located {
    std::string                name;
    std::vector<ClassMember>   members;
};

struct ProgramDecl : Located {
    ExprNodePtr entryPoint;   // a call invoke expression
};

struct TestDecl : Located {
    std::string                label;
    std::shared_ptr<CallGroup> body;
};

// TopLevelDef variant
using TopLevelDef = std::variant<
    AliasDecl, DomainDecl, EnumDecl, RecordDecl, ObjectDecl,
    UnionDecl, VariantDecl, InstanceDecl,
    CmdDecl, IntrinsicDecl, CmdDef, ClassDecl,
    ProgramDecl, TestDecl>;

// ========================================================================
// Compilation unit (root)
// ========================================================================
struct CompilationUnit : Located {
    std::shared_ptr<ModuleDecl>              module;
    std::vector<std::shared_ptr<ImportDecl>> imports;
    std::vector<TopLevelDef>                 definitions;
};

// ========================================================================
// Traverser base class
// ========================================================================
struct Traverser {
    virtual ~Traverser() = default;

    // ---- virtual visit hooks (override in subclasses) ----
    virtual void visit(CompilationUnit&) {}
    virtual void visit(ModuleDecl&)      {}
    virtual void visit(ImportDecl&)      {}
    // type nodes
    virtual void visit(NamedType&)         {}
    virtual void visit(PtrType&)           {}
    virtual void visit(RangeType&)         {}
    virtual void visit(CmdType&)           {}
    virtual void visit(InlineRecordType&)  {}
    virtual void visit(InlineObjectType&)  {}
    virtual void visit(InlineUnionType&)   {}
    virtual void visit(InlineVariantType&) {}
    // expression nodes
    virtual void visit(LiteralExpr&)         {}
    virtual void visit(IdentifierExpr&)      {}
    virtual void visit(EnumDerefExpr&)       {}
    virtual void visit(CallCommandExpr&)     {}
    virtual void visit(CallConstructorExpr&) {}
    virtual void visit(CallVCommandExpr&)    {}
    virtual void visit(CallFailExpr&)        {}
    virtual void visit(SuffixExpr&)          {}
    virtual void visit(BinaryExpr&)          {}
    virtual void visit(QuoteExpr&)           {}
    virtual void visit(CmdLiteralExpr&)      {}
    // statement nodes
    virtual void visit(AssignStat&)  {}
    virtual void visit(ExprStat&)    {}
    virtual void visit(Block&)       {}
    // structure nodes
    virtual void visit(CallGroup&)   {}
    virtual void visit(CmdBody&)     {}
    // declaration nodes
    virtual void visit(AliasDecl&)     {}
    virtual void visit(DomainDecl&)    {}
    virtual void visit(EnumDecl&)      {}
    virtual void visit(RecordDecl&)    {}
    virtual void visit(ObjectDecl&)    {}
    virtual void visit(UnionDecl&)     {}
    virtual void visit(VariantDecl&)   {}
    virtual void visit(InstanceDecl&)  {}
    virtual void visit(CmdDecl&)       {}
    virtual void visit(IntrinsicDecl&) {}
    virtual void visit(CmdDef&)        {}
    virtual void visit(ClassDecl&)     {}
    virtual void visit(ProgramDecl&)   {}
    virtual void visit(TestDecl&)      {}
    // signature nodes
    virtual void visit(RegularSig&)     {}
    virtual void visit(VCommandSig&)    {}
    virtual void visit(ConstructorSig&) {}
    virtual void visit(DestructorSig&)  {}
    virtual void visit(FailHandlerSig&) {}

    // ---- virtual revisit hooks: called after child traversal (override in subclasses) ----
    virtual void revisit(CompilationUnit&) {}
    // type nodes
    virtual void revisit(NamedType&)         {}
    virtual void revisit(PtrType&)           {}
    virtual void revisit(RangeType&)         {}
    virtual void revisit(CmdType&)           {}
    virtual void revisit(InlineRecordType&)  {}
    virtual void revisit(InlineObjectType&)  {}
    virtual void revisit(InlineUnionType&)   {}
    virtual void revisit(InlineVariantType&) {}
    // expression nodes
    virtual void revisit(CallCommandExpr&)     {}
    virtual void revisit(CallConstructorExpr&) {}
    virtual void revisit(CallVCommandExpr&)    {}
    virtual void revisit(CallFailExpr&)        {}
    virtual void revisit(SuffixExpr&)          {}
    virtual void revisit(BinaryExpr&)          {}
    virtual void revisit(QuoteExpr&)           {}
    virtual void revisit(CmdLiteralExpr&)      {}
    // statement nodes
    virtual void revisit(AssignStat&)  {}
    virtual void revisit(ExprStat&)    {}
    virtual void revisit(Block&)       {}
    // structure nodes
    virtual void revisit(CallGroup&)   {}
    virtual void revisit(CmdBody&)     {}
    // declaration nodes
    virtual void revisit(AliasDecl&)     {}
    virtual void revisit(DomainDecl&)    {}
    virtual void revisit(RecordDecl&)    {}
    virtual void revisit(ObjectDecl&)    {}
    virtual void revisit(UnionDecl&)     {}
    virtual void revisit(VariantDecl&)   {}
    virtual void revisit(CmdDecl&)       {}
    virtual void revisit(IntrinsicDecl&) {}
    virtual void revisit(CmdDef&)        {}
    virtual void revisit(ClassDecl&)     {}
    virtual void revisit(ProgramDecl&)   {}
    virtual void revisit(TestDecl&)      {}
    // signature nodes
    virtual void revisit(RegularSig&)     {}
    virtual void revisit(VCommandSig&)    {}
    virtual void revisit(ConstructorSig&) {}
    virtual void revisit(DestructorSig&)  {}
    virtual void revisit(FailHandlerSig&) {}


    // ---- non-virtual traverse: encode child walking ----
    // variant dispatchers
    void traverse(TypeNode& n)    { std::visit([this](auto& a){ traverse(a); }, n.v); }
    void traverse(ExprNode& n)    { std::visit([this](auto& a){ traverse(a); }, n.v); }
    void traverse(StatNode& n)    { std::visit([this](auto& a){ traverse(a); }, n.v); }
    void traverse(CmdSignature& s){ std::visit([this](auto& a){ traverse(a); }, s); }
    void traverse(TopLevelDef& d) { std::visit([this](auto& a){ traverse(a); }, d); }
    void traverse(ClassMember& m) { std::visit([this](auto& a){ traverse(a); }, m); }

    // pointer helpers
    void traverse(TypeNodePtr& p) { if (p) traverse(*p); }
    void traverse(ExprNodePtr& p) { if (p) traverse(*p); }

    // ---- per-type traverse methods ----
    void traverse(CompilationUnit& n) {
        visit(n);
        if (n.module) traverse(*n.module);
        for (auto& i : n.imports) if (i) traverse(*i);
        for (auto& d : n.definitions) traverse(d);
        revisit(n);
    }
    void traverse(ModuleDecl& n)  { visit(n); }
    void traverse(ImportDecl& n)  { visit(n); }

    // type nodes
    void traverse(NamedType& n) {
        visit(n);
        for (auto& a : n.typeArgs) traverse(a);
        revisit(n);
    }
    void traverse(PtrType& n) {
        visit(n);
        traverse(n.inner);
        revisit(n);
    }
    void traverse(RangeType& n) {
        visit(n);
        traverse(n.element);
        revisit(n);
    }
    void traverse(CmdType& n) {
        visit(n);
        for (auto& a : n.args) traverse(a.type);
        revisit(n);
    }
    void traverse(InlineRecordType& n) {
        visit(n);
        for (auto& f : n.fields) traverse(f.type);
        revisit(n);
    }
    void traverse(InlineObjectType& n) {
        visit(n);
        for (auto& f : n.fields) traverse(f.type);
        revisit(n);
    }
    void traverse(InlineUnionType& n) {
        visit(n);
        for (auto& c : n.candidates) traverse(c.domain);
        revisit(n);
    }
    void traverse(InlineVariantType& n) {
        visit(n);
        for (auto& c : n.candidates) traverse(c.type);
        revisit(n);
    }

    // expression nodes
    void traverse(LiteralExpr& n)    { visit(n); }
    void traverse(IdentifierExpr& n) { visit(n); }
    void traverse(EnumDerefExpr& n)  { visit(n); }
    void traverse(CallCommandExpr& n) {
        visit(n);
        traverse(n.target);
        for (auto& p : n.params) traverse(p.expr);
        revisit(n);
    }
    void traverse(CallConstructorExpr& n) {
        visit(n);
        traverse(n.typeName);
        for (auto& p : n.params) traverse(p.expr);
        revisit(n);
    }
    void traverse(CallVCommandExpr& n) {
        visit(n);
        for (auto& p : n.params) traverse(p.expr);
        revisit(n);
    }
    void traverse(CallFailExpr& n) {
        visit(n);
        traverse(n.expr);
        revisit(n);
    }
    void traverse(SuffixExpr& n) {
        visit(n);
        traverse(n.base);
        for (auto& s : n.suffixes) {
            traverse(s.indexLoc);
            traverse(s.indexExt);
        }
        revisit(n);
    }
    void traverse(BinaryExpr& n) {
        visit(n);
        traverse(n.first);
        for (auto& ot : n.rest) traverse(ot.term);
        revisit(n);
    }
    void traverse(QuoteExpr& n) {
        visit(n);
        traverse(n.invoke);
        if (n.group) traverse(*n.group);
        revisit(n);
    }
    void traverse(CmdLiteralExpr& n) {
        visit(n);
        for (auto& p : n.params) traverse(p.type);
        if (n.body) traverse(*n.body);
        revisit(n);
    }

    // statement nodes
    void traverse(AssignStat& n) {
        visit(n);
        traverse(n.target);
        traverse(n.value);
        revisit(n);
    }
    void traverse(ExprStat& n) {
        visit(n);
        traverse(n.expr);
        revisit(n);
    }
    void traverse(Block& n) {
        visit(n);
        traverse(n.recoverType);
        traverse(n.recoverExpr);
        if (n.body) traverse(*n.body);
        revisit(n);
    }
    void traverse(CallGroup& n) {
        visit(n);
        for (auto& s : n.statements) traverse(s);
        revisit(n);
    }
    void traverse(CmdBody& n) {
        visit(n);
        if (n.group) traverse(*n.group);
        revisit(n);
    }

    // declaration nodes
    void traverse(AliasDecl& n) {
        visit(n);
        traverse(n.type);
        revisit(n);
    }
    void traverse(DomainDecl& n) {
        visit(n);
        traverse(n.parent);
        revisit(n);
    }
    void traverse(EnumDecl& n) { visit(n); }
    void traverse(RecordDecl& n) {
        visit(n);
        for (auto& f : n.fields) traverse(f.type);
        revisit(n);
    }
    void traverse(ObjectDecl& n) {
        visit(n);
        for (auto& f : n.fields) traverse(f.type);
        revisit(n);
    }
    void traverse(UnionDecl& n) {
        visit(n);
        for (auto& c : n.candidates) traverse(c.domain);
        revisit(n);
    }
    void traverse(VariantDecl& n) {
        visit(n);
        for (auto& c : n.candidates) traverse(c.type);
        revisit(n);
    }
    void traverse(InstanceDecl& n) { visit(n); }
    void traverse(CmdDecl& n) {
        visit(n);
        traverse(n.signature);
        revisit(n);
    }
    void traverse(IntrinsicDecl& n) {
        visit(n);
        traverse(n.signature);
        revisit(n);
    }
    void traverse(CmdDef& n) {
        visit(n);
        traverse(n.signature);
        if (n.body) traverse(*n.body);
        revisit(n);
    }
    void traverse(ClassDecl& n) {
        visit(n);
        for (auto& m : n.members) traverse(m);
        revisit(n);
    }
    void traverse(ProgramDecl& n) {
        visit(n);
        traverse(n.entryPoint);
        revisit(n);
    }
    void traverse(TestDecl& n) {
        visit(n);
        if (n.body) traverse(*n.body);
        revisit(n);
    }

    // signature nodes
    void traverse(RegularSig& n) {
        visit(n);
        for (auto& p : n.params) traverse(p.type);
        for (auto& p : n.implicitParams) traverse(p.type);
        revisit(n);
    }
    void traverse(VCommandSig& n) {
        visit(n);
        for (auto& r : n.receivers) traverse(r.type);
        for (auto& p : n.params) traverse(p.type);
        for (auto& p : n.implicitParams) traverse(p.type);
        revisit(n);
    }
    void traverse(ConstructorSig& n) {
        visit(n);
        traverse(n.receiver.type);
        for (auto& p : n.params) traverse(p.type);
        revisit(n);
    }
    void traverse(DestructorSig& n) {
        visit(n);
        traverse(n.receiver.type);
        revisit(n);
    }
    void traverse(FailHandlerSig& n) {
        visit(n);
        traverse(n.receiver.type);
        revisit(n);
    }
};

} // namespace basis

#endif // AST_H


