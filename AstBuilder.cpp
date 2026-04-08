#include "AstBuilder.h"
#include <stdexcept>
#include <cassert>

namespace basis {

// ========================================================================
// Parse-tree navigation helpers
// ========================================================================
static bool is(const spParseTree& pt, Production p) {
    return pt && pt->production == p;
}
static spParseTree down(const spParseTree& pt) {
    return pt ? pt->spDown : nullptr;
}
static spParseTree nxt(const spParseTree& pt) {
    return pt ? pt->spNext : nullptr;
}
static std::string txt(const spParseTree& pt) {
    return (pt && pt->pToken) ? pt->pToken->text : std::string{};
}
static const Token* firstTok(const spParseTree& pt) {
    if (!pt) return nullptr;
    if (pt->pToken) return pt->pToken;
    return firstTok(pt->spDown);
}
static std::string firstTxt(const spParseTree& pt) {
    auto* t = firstTok(pt);
    return t ? t->text : std::string{};
}
static size_t locL(const spParseTree& pt) { auto* t = firstTok(pt); return t ? t->lineNumber : 0; }
static size_t locC(const spParseTree& pt) { auto* t = firstTok(pt); return t ? t->columnNumber : 0; }

static spParseTree findChild(const spParseTree& pt, Production p) {
    for (auto c = down(pt); c; c = nxt(c))
        if (c->production == p) return c;
    return nullptr;
}
static std::vector<spParseTree> allChildren(const spParseTree& pt) {
    std::vector<spParseTree> v;
    for (auto c = down(pt); c; c = nxt(c)) v.push_back(c);
    return v;
}

static int countChildren(const spParseTree& pt, Production p) {
    int count = 0;
    for (auto c = down(pt); c; c = nxt(c))
        if (is(c, p)) ++count;
    return count;
}

static CmdType::Kind cmdTypeKind(const spParseTree& pt) {
    if (findChild(pt, Production::TYPE_CMD_MAYFAIL)) return CmdType::Kind::MayFail;
    if (findChild(pt, Production::TYPE_CMD_FAILS))   return CmdType::Kind::Fails;
    return CmdType::Kind::NoFail;
}

static CmdLiteralExpr::Kind cmdLiteralKind(const spParseTree& pt) {
    if (findChild(pt, Production::CALL_CMDLIT_MAYFAIL))   return CmdLiteralExpr::Kind::MayFail;
    if (findChild(pt, Production::CALL_CMDLIT_MUSTFAIL))  return CmdLiteralExpr::Kind::MustFail;
    return CmdLiteralExpr::Kind::NoFail;
}

static bool hasCmdArgWriteable(const spParseTree& pt) {
    if (!is(pt, Production::TYPE_CMDEXPR_ARG)) return false;
    if (findChild(pt, Production::TYPE_ARG_WRITEABLE)) return true;
    auto c = down(pt);
    while (c && is(c, Production::TYPE_EXPR_PTR)) c = nxt(c);
    if (c && is(c, Production::TYPE_CMDEXPR_ARG))
        return hasCmdArgWriteable(c);
    if (c && is(c, Production::TYPE_EXPR_RANGE)) {
        auto elem = nxt(c);
        if (elem && is(elem, Production::TYPE_CMDEXPR_ARG))
            return hasCmdArgWriteable(elem);
    }
    return false;
}

// Collect text from TYPENAME or QUALIFIED_TYPENAME
static std::string collectTypeName(const spParseTree& pt) {
    if (!pt) return {};
    if (is(pt, Production::QUALIFIED_TYPENAME)) {
        std::string r;
        for (auto c = down(pt); c; c = nxt(c)) {
            if (!r.empty()) r += "::";
            r += txt(c);
        }
        return r;
    }
    return txt(pt);
}

// Collect text from IDENTIFIER group (with optional qualifiers)
static std::string collectIdent(const spParseTree& pt) {
    if (!pt) return {};
    if (is(pt, Production::IDENTIFIER)) {
        std::string r;
        for (auto c = down(pt); c; c = nxt(c)) {
            if (c->production == Production::IDENTIFIER_QUALIFIER)
                r += txt(c) + "::";
            else if (c->production == Production::IDENTIFIER)
                r += txt(c);
        }
        return r;
    }
    if (is(pt, Production::ALLOC_IDENTIFIER)) {
        // children: IDENTIFIER group (POUND was discarded)
        return collectIdent(down(pt));
    }
    return txt(pt);
}

static bool isAllocIdent(const spParseTree& pt) {
    return is(pt, Production::ALLOC_IDENTIFIER);
}

// ========================================================================
// Forward declarations of builder functions
// ========================================================================
static TypeNodePtr buildTypeExpr(const spParseTree& pt);
static TypeNodePtr buildTypeExprDomain(const spParseTree& pt);
static TypeNodePtr buildTypeNameQ(const spParseTree& pt);
static TypeNodePtr buildCmdExprArgType(const spParseTree& pt);
static ExprNodePtr buildPrimaryExpr(const spParseTree& pt);
static ExprNodePtr buildCallExpression(const spParseTree& pt);
static ExprNodePtr buildSubcallExpr(const spParseTree& pt);
static ExprNodePtr buildInvokeExpr(const spParseTree& pt);
static std::shared_ptr<CallGroup> buildCallGroup(const spParseTree& pt);
static CmdParam buildCmdParm(const spParseTree& pt);
static CmdReceiver buildCmdReceiver(const spParseTree& pt);
static CmdSignature buildSignature(const spParseTree& pt);
static std::vector<FieldDecl> buildRecordFields(const spParseTree& pt);
static std::vector<FieldDecl> buildObjectFields(const spParseTree& pt);

// ========================================================================
// Type expression builders
// ========================================================================
static TypeNodePtr buildTypeNameQ(const spParseTree& pt) {
    if (!pt) return nullptr;
    NamedType nt;
    nt.line = locL(pt); nt.col = locC(pt);
    auto c = down(pt); // first child: TYPENAME or QUALIFIED_TYPENAME
    nt.name = collectTypeName(c);
    auto args = findChild(pt, Production::TYPE_NAME_ARGS);
    if (args) {
        for (auto a = down(args); a; a = nxt(a)) {
            if (is(a, Production::TYPE_ARG_TYPE)) {
                nt.typeArgs.push_back(buildTypeNameQ(findChild(a, Production::TYPE_NAME_Q)));
            } else if (is(a, Production::TYPE_ARG_VALUE)) {
                NamedType val; val.name = txt(down(a));
                val.line = locL(a); val.col = locC(a);
                nt.typeArgs.push_back(std::make_shared<TypeNode>(std::move(val)));
            }
        }
    }
    return std::make_shared<TypeNode>(std::move(nt));
}

static CmdType buildCmdTypeNode(const spParseTree& pt) {
    CmdType ct;
    ct.line = locL(pt); ct.col = locC(pt);
    ct.kind = cmdTypeKind(pt);
    for (auto c = down(pt); c; c = nxt(c)) {
        if (is(c, Production::TYPE_CMDEXPR_ARG)) {
            CmdTypeArg arg;
            arg.writeable = hasCmdArgWriteable(c);
            arg.type = buildCmdExprArgType(c);
            ct.args.push_back(std::move(arg));
        }
    }
    return ct;
}

static TypeNodePtr buildCmdExprArgType(const spParseTree& pt) {
    auto c = down(pt);
    if (!c) return nullptr;
    if (is(c, Production::TYPE_EXPR_PTR)) {
        PtrType ptr;
        ptr.depth = countChildren(pt, Production::TYPE_EXPR_PTR);
        ptr.line = locL(pt); ptr.col = locC(pt);
        while (c && is(c, Production::TYPE_EXPR_PTR)) c = nxt(c);
        ptr.inner = buildCmdExprArgType(c);
        return std::make_shared<TypeNode>(std::move(ptr));
    }
    if (is(c, Production::TYPE_NAME_Q))
        return buildTypeNameQ(c);
    if (is(c, Production::TYPE_EXPR_CMD)) {
        auto ct = buildCmdTypeNode(c);
        return std::make_shared<TypeNode>(std::move(ct));
    }
    if (is(c, Production::TYPE_EXPR_RANGE)) {
        RangeType rt; rt.line = locL(c); rt.col = locC(c);
        auto sizeNode = down(c);
        if (sizeNode) rt.size = txt(sizeNode);
        auto elemNode = nxt(c); // optional TYPE_CMDEXPR_ARG sibling
        if (elemNode && is(elemNode, Production::TYPE_CMDEXPR_ARG))
            rt.element = buildCmdExprArgType(elemNode);
        return std::make_shared<TypeNode>(std::move(rt));
    }
    return nullptr;
}

static std::vector<FieldDecl> buildRecordFields(const spParseTree& pt) {
    // DEF_RECORD_FIELDS: children are DEF_RECORD_FIELD groups
    std::vector<FieldDecl> fields;
    for (auto c = down(pt); c; c = nxt(c)) {
        if (!is(c, Production::DEF_RECORD_FIELD)) continue;
        FieldDecl f;
        f.line = locL(c); f.col = locC(c);
        auto dom = findChild(c, Production::DEF_RECORD_FIELD_DOMAIN);
        f.type = buildTypeExprDomain(dom ? down(dom) : nullptr);
        auto nm = findChild(c, Production::DEF_RECORD_FIELD_NAME);
        f.name = collectIdent(nm ? down(nm) : nullptr);
        fields.push_back(std::move(f));
    }
    return fields;
}

static std::vector<FieldDecl> buildObjectFields(const spParseTree& pt) {
    std::vector<FieldDecl> fields;
    for (auto c = down(pt); c; c = nxt(c)) {
        if (!is(c, Production::DEF_OBJECT_FIELD)) continue;
        FieldDecl f;
        f.line = locL(c); f.col = locC(c);
        auto tp = findChild(c, Production::DEF_OBJECT_FIELD_TYPE);
        f.type = buildTypeExpr(tp ? down(tp) : nullptr);
        auto nm = findChild(c, Production::DEF_OBJECT_FIELD_NAME);
        f.name = collectIdent(nm ? down(nm) : nullptr);
        fields.push_back(std::move(f));
    }
    return fields;
}

static std::vector<UnionCandidate> buildUnionCandidates(const spParseTree& pt) {
    std::vector<UnionCandidate> cands;
    for (auto c = down(pt); c; c = nxt(c)) {
        if (!is(c, Production::DEF_UNION_CANDIDATE)) continue;
        UnionCandidate uc;
        uc.line = locL(c); uc.col = locC(c);
        auto dom = findChild(c, Production::DEF_UNION_CANDIDATE_DOMAIN);
        uc.domain = buildTypeExprDomain(dom ? down(dom) : nullptr);
        auto nm = findChild(c, Production::DEF_UNION_CANDIDATE_NAME);
        uc.name = collectIdent(nm ? down(nm) : nullptr);
        cands.push_back(std::move(uc));
    }
    return cands;
}

static std::vector<VariantCandidate> buildVariantCandidates(const spParseTree& pt) {
    std::vector<VariantCandidate> cands;
    for (auto c = down(pt); c; c = nxt(c)) {
        if (!is(c, Production::DEF_VARIANT_CANDIDATE)) continue;
        VariantCandidate vc;
        vc.line = locL(c); vc.col = locC(c);
        auto tp = findChild(c, Production::DEF_VARIANT_CANDIDATE_TYPE);
        vc.type = buildTypeExpr(tp ? down(tp) : nullptr);
        auto nm = findChild(c, Production::DEF_VARIANT_CANDIDATE_NAME);
        vc.name = collectIdent(nm ? down(nm) : nullptr);
        cands.push_back(std::move(vc));
    }
    return cands;
}

static TypeNodePtr buildInlineType(const spParseTree& pt) {
    if (!pt) return nullptr;
    auto scopeNode = findChild(pt, Production::DEF_INLINE_SCOPE_NAME);
    std::string scopeName;
    if (scopeNode) scopeName = txt(findChild(scopeNode, Production::IDENTIFIER));

    if (is(pt, Production::DEF_INLINE_RECORD)) {
        InlineRecordType irt;
        irt.scopeName = scopeName;
        irt.line = locL(pt); irt.col = locC(pt);
        auto flds = findChild(pt, Production::DEF_RECORD_FIELDS);
        if (flds) irt.fields = buildRecordFields(flds);
        return std::make_shared<TypeNode>(std::move(irt));
    }
    if (is(pt, Production::DEF_INLINE_OBJECT)) {
        InlineObjectType iot;
        iot.scopeName = scopeName;
        iot.line = locL(pt); iot.col = locC(pt);
        auto flds = findChild(pt, Production::DEF_OBJECT_FIELDS);
        if (flds) iot.fields = buildObjectFields(flds);
        return std::make_shared<TypeNode>(std::move(iot));
    }
    if (is(pt, Production::DEF_INLINE_UNION)) {
        InlineUnionType iut;
        iut.scopeName = scopeName;
        iut.line = locL(pt); iut.col = locC(pt);
        auto cands = findChild(pt, Production::DEF_UNION_CANDIDATES);
        if (cands) iut.candidates = buildUnionCandidates(cands);
        return std::make_shared<TypeNode>(std::move(iut));
    }
    if (is(pt, Production::DEF_INLINE_VARIANT)) {
        InlineVariantType ivt;
        ivt.scopeName = scopeName;
        ivt.line = locL(pt); ivt.col = locC(pt);
        auto cands = findChild(pt, Production::DEF_VARIANT_CANDIDATES);
        if (cands) ivt.candidates = buildVariantCandidates(cands);
        return std::make_shared<TypeNode>(std::move(ivt));
    }
    return nullptr;
}

static TypeNodePtr buildTypeExpr(const spParseTree& pt) {
    // TYPE_EXPR group: first child determines kind
    if (!pt) return nullptr;
    // If pt IS a TYPE_EXPR group, look at its first child
    spParseTree c = is(pt, Production::TYPE_EXPR) ? down(pt) : pt;
    if (!c) return nullptr;

    if (is(c, Production::TYPEDEF_NAME_Q)) {
        // Named type: extract typename from TYPEDEF_NAME_Q
        // TYPEDEF_NAME_Q has TYPENAME + optional TYPEDEF_PARMS
        // But TYPE_EXPR uses TYPE_NAME_Q via TYPEDEF_NAME_Q
        // For type expressions, map to NamedType
        NamedType nt;
        nt.line = locL(c); nt.col = locC(c);
        auto tn = down(c); // TYPENAME or QUALIFIED_TYPENAME
        nt.name = collectTypeName(tn);
        // TYPEDEF_PARMS not used here (definition-side only)
        return std::make_shared<TypeNode>(std::move(nt));
    }
    if (is(c, Production::TYPE_NAME_Q))
        return buildTypeNameQ(c);
    if (is(c, Production::TYPE_EXPR_CMD)) {
        auto ct = buildCmdTypeNode(c);
        return std::make_shared<TypeNode>(std::move(ct));
    }
    if (is(c, Production::TYPE_EXPR_RANGE)) {
        RangeType rt;
        rt.line = locL(c); rt.col = locC(c);
        auto sizeNode = down(c);
        if (sizeNode) rt.size = txt(sizeNode);
        auto elemSibling = nxt(c);
        if (elemSibling && is(elemSibling, Production::TYPE_EXPR))
            rt.element = buildTypeExpr(elemSibling);
        return std::make_shared<TypeNode>(std::move(rt));
    }
    if (is(c, Production::TYPE_EXPR_PTR)) {
        PtrType ptr;
        ptr.depth = countChildren(pt, Production::TYPE_EXPR_PTR);
        ptr.line = locL(pt); ptr.col = locC(pt);
        while (c && is(c, Production::TYPE_EXPR_PTR)) c = nxt(c);
        ptr.inner = buildTypeExpr(c);
        return std::make_shared<TypeNode>(std::move(ptr));
    }
    if (is(c, Production::DEF_INLINE_RECORD) || is(c, Production::DEF_INLINE_OBJECT) ||
        is(c, Production::DEF_INLINE_UNION) || is(c, Production::DEF_INLINE_VARIANT))
        return buildInlineType(c);
    return nullptr;
}

static TypeNodePtr buildTypeExprDomain(const spParseTree& pt) {
    // TYPE_EXPR_DOMAIN: similar to TYPE_EXPR but restricted alternatives
    if (!pt) return nullptr;
    spParseTree c = is(pt, Production::TYPE_EXPR_DOMAIN) ? down(pt) : pt;
    if (!c) return nullptr;

    if (is(c, Production::TYPE_NAME_Q))
        return buildTypeNameQ(c);
    if (is(c, Production::TYPEDEF_NAME_Q)) {
        NamedType nt;
        nt.line = locL(c); nt.col = locC(c);
        nt.name = collectTypeName(down(c));
        return std::make_shared<TypeNode>(std::move(nt));
    }
    if (is(c, Production::TYPE_EXPR_RANGE)) {
        RangeType rt;
        rt.line = locL(c); rt.col = locC(c);
        auto sizeNode = down(c);
        if (sizeNode) rt.size = txt(sizeNode);
        auto elemSibling = nxt(c);
        if (elemSibling && is(elemSibling, Production::TYPE_EXPR_DOMAIN))
            rt.element = buildTypeExprDomain(elemSibling);
        return std::make_shared<TypeNode>(std::move(rt));
    }
    if (is(c, Production::DEF_INLINE_RECORD) || is(c, Production::DEF_INLINE_UNION))
        return buildInlineType(c);
    return nullptr;
}


// ========================================================================
// Expression builders
// ========================================================================

// Build a QuoteExpr from a CALL_QUOTE or CALL_BLOCK_* node
static ExprNodePtr buildQuoteExpr(const spParseTree& pt) {
    if (!pt) return nullptr;
    QuoteExpr q;
    q.line = locL(pt); q.col = locC(pt);
    if (is(pt, Production::CALL_QUOTE)) {
        q.kind = QuoteExpr::Kind::Subquote;
        auto inv = down(pt); // optional CALL_INVOKE child
        if (inv) q.invoke = buildInvokeExpr(inv);
    } else if (is(pt, Production::CALL_BLOCK_NOFAIL)) {
        q.kind = QuoteExpr::Kind::BlockNoFail;
        auto body = findChild(pt, Production::CALL_GROUP);
        if (body) q.group = buildCallGroup(body);
    } else if (is(pt, Production::CALL_BLOCK_MAYFAIL)) {
        q.kind = QuoteExpr::Kind::BlockMayFail;
        auto body = findChild(pt, Production::CALL_GROUP);
        if (body) q.group = buildCallGroup(body);
    } else if (is(pt, Production::CALL_BLOCK_FAIL)) {
        q.kind = QuoteExpr::Kind::BlockFail;
        auto body = findChild(pt, Production::CALL_GROUP);
        if (body) q.group = buildCallGroup(body);
    }
    return std::make_shared<ExprNode>(std::move(q));
}

// Build a CmdLiteralExpr from CALL_CMD_LITERAL
static ExprNodePtr buildCmdLiteral(const spParseTree& pt) {
    CmdLiteralExpr lit;
    lit.line = locL(pt); lit.col = locC(pt);
    lit.kind = cmdLiteralKind(pt);
    for (auto c = down(pt); c; c = nxt(c)) {
        if (is(c, Production::DEF_CMD_PARM))
            lit.params.push_back(buildCmdParm(c));
        else if (is(c, Production::CALL_GROUP))
            lit.body = buildCallGroup(c);
    }
    return std::make_shared<ExprNode>(std::move(lit));
}

// Build an ExprNode from a single primary child (leaf or invoke)
static ExprNodePtr buildPrimaryExpr(const spParseTree& pt) {
    if (!pt) return nullptr;
    auto p = pt->production;
    // Literals
    if (p == Production::DECIMAL || p == Production::HEXNUMBER ||
        p == Production::BINARY || p == Production::NUMBER || p == Production::STRING) {
        LiteralExpr le; le.text = txt(pt);
        le.line = locL(pt); le.col = locC(pt);
        return std::make_shared<ExprNode>(std::move(le));
    }
    if (p == Production::LITERAL) {
        return buildPrimaryExpr(down(pt));
    }
    // Identifier
    if (p == Production::IDENTIFIER) {
        IdentifierExpr ie; ie.text = collectIdent(pt);
        ie.line = locL(pt); ie.col = locC(pt);
        return std::make_shared<ExprNode>(std::move(ie));
    }
    if (p == Production::ALLOC_IDENTIFIER) {
        IdentifierExpr ie; ie.text = collectIdent(pt); ie.isAlloc = true;
        ie.line = locL(pt); ie.col = locC(pt);
        return std::make_shared<ExprNode>(std::move(ie));
    }
    // Enum deref
    if (p == Production::ENUM_DEREF) {
        EnumDerefExpr ed;
        ed.line = locL(pt); ed.col = locC(pt);
        auto c = down(pt);
        ed.typeName = txt(c);        // TYPENAME
        ed.memberName = collectIdent(nxt(c)); // IDENTIFIER
        return std::make_shared<ExprNode>(std::move(ed));
    }
    // Invocations
    if (p == Production::CALL_COMMAND || p == Production::CALL_CONSTRUCTOR ||
        p == Production::CALL_VCOMMAND || p == Production::CALL_FAIL)
        return buildInvokeExpr(pt);
    // Quotes
    if (p == Production::CALL_QUOTE || p == Production::CALL_BLOCK_NOFAIL ||
        p == Production::CALL_BLOCK_MAYFAIL || p == Production::CALL_BLOCK_FAIL)
        return buildQuoteExpr(pt);
    // Command literal
    if (p == Production::CALL_CMD_LITERAL)
        return buildCmdLiteral(pt);
    // Subcall expression (unwrap)
    if (p == Production::SUBCALL_EXPRESSION)
        return buildSubcallExpr(pt);
    // Call expression
    if (p == Production::CALL_EXPRESSION)
        return buildCallExpression(pt);
    return nullptr;
}

// Build invoke expressions (CALL_COMMAND, CALL_CONSTRUCTOR, CALL_VCOMMAND, CALL_FAIL)
static std::vector<CallParam> buildCallParams(const spParseTree& pt) {
    // Collect CALL_PARAMETER children from any parent
    std::vector<CallParam> params;
    for (auto c = down(pt); c; c = nxt(c)) {
        if (!is(c, Production::CALL_PARAMETER)) continue;
        CallParam cp;
        cp.line = locL(c); cp.col = locC(c);
        auto inner = down(c);
        if (inner && is(inner, Production::CALL_PARM_EMPTY)) {
            cp.isEmpty = true;
        } else if (inner && is(inner, Production::CALL_PARM_EXPR)) {
            cp.expr = buildPrimaryExpr(down(inner));
        }
        params.push_back(std::move(cp));
    }
    return params;
}

static ExprNodePtr buildInvokeExpr(const spParseTree& pt) {
    if (!pt) return nullptr;
    if (is(pt, Production::CALL_COMMAND)) {
        CallCommandExpr cc;
        cc.line = locL(pt); cc.col = locC(pt);
        auto tgt = findChild(pt, Production::CALL_CMD_TARGET);
        if (tgt) cc.target = buildPrimaryExpr(down(tgt));
        cc.params = buildCallParams(pt);
        return std::make_shared<ExprNode>(std::move(cc));
    }
    if (is(pt, Production::CALL_CONSTRUCTOR)) {
        CallConstructorExpr ce;
        ce.line = locL(pt); ce.col = locC(pt);
        auto tn = findChild(pt, Production::TYPE_NAME_Q);
        if (tn) ce.typeName = buildTypeNameQ(tn);
        ce.params = buildCallParams(pt);
        return std::make_shared<ExprNode>(std::move(ce));
    }
    if (is(pt, Production::CALL_VCOMMAND)) {
        CallVCommandExpr vc;
        vc.line = locL(pt); vc.col = locC(pt);
        // Children: IDENTIFIER* (receivers), IDENTIFIER (name), CALL_PARAMETER*
        auto kids = allChildren(pt);
        // Collect receivers (IDENTIFIERs before the command name)
        // The last IDENTIFIER before any CALL_PARAMETER is the name
        std::vector<size_t> identIdxs;
        for (size_t i = 0; i < kids.size(); ++i)
            if (is(kids[i], Production::IDENTIFIER)) identIdxs.push_back(i);
        if (identIdxs.size() >= 2) {
            for (size_t i = 0; i + 1 < identIdxs.size(); ++i)
                vc.receivers.push_back(collectIdent(kids[identIdxs[i]]));
            vc.name = collectIdent(kids[identIdxs.back()]);
        } else if (identIdxs.size() == 1) {
            vc.name = collectIdent(kids[identIdxs[0]]);
        }
        vc.params = buildCallParams(pt);
        return std::make_shared<ExprNode>(std::move(vc));
    }
    if (is(pt, Production::CALL_FAIL)) {
        CallFailExpr cf;
        cf.line = locL(pt); cf.col = locC(pt);
        auto expr = findChild(pt, Production::CALL_EXPRESSION);
        if (expr) cf.expr = buildCallExpression(expr);
        return std::make_shared<ExprNode>(std::move(cf));
    }
    return buildPrimaryExpr(pt);
}

static ExprNodePtr buildSubcallExpr(const spParseTree& pt) {
    // SUBCALL_EXPRESSION: child is CALL_CMD_LITERAL | CALL_EXPRESSION | CALL_QUOTE
    if (!pt) return nullptr;
    auto c = is(pt, Production::SUBCALL_EXPRESSION) ? down(pt) : pt;
    return buildPrimaryExpr(c);
}

static ExprNodePtr buildCallExpression(const spParseTree& pt) {
    // CALL_EXPRESSION: flattened children = term-parts (CALL_OPERATOR term-parts)*
    if (!pt) return nullptr;
    auto kids = allChildren(pt);
    if (kids.empty()) return nullptr;

    // Split into segments at CALL_OPERATOR boundaries
    std::vector<std::vector<spParseTree>> segments;
    std::vector<spParseTree> operators;
    segments.emplace_back();
    for (auto& k : kids) {
        if (is(k, Production::CALL_OPERATOR)) {
            operators.push_back(k);
            segments.emplace_back();
        } else {
            segments.back().push_back(k);
        }
    }

    // Build a term from a segment: primary + optional CALL_EXPR_INDEX suffixes
    auto buildTerm = [](const std::vector<spParseTree>& seg) -> ExprNodePtr {
        if (seg.empty()) return nullptr;
        ExprNodePtr primary;
        std::vector<SuffixOp> suffixes;
        for (auto& n : seg) {
            if (is(n, Production::CALL_EXPR_DEREF)) {
                SuffixOp sop; sop.kind = SuffixOp::Kind::Deref;
                suffixes.push_back(std::move(sop));
            } else if (is(n, Production::CALL_EXPR_INDEX)) {
                SuffixOp sop; sop.kind = SuffixOp::Kind::Index;
                auto loc = findChild(n, Production::CALL_EXPRINDEX_LOC);
                if (loc) sop.indexLoc = buildSubcallExpr(down(loc));
                auto ext = findChild(n, Production::CALL_EXPRINDEX_EXT);
                if (ext) sop.indexExt = buildSubcallExpr(down(ext));
                suffixes.push_back(std::move(sop));
            } else if (is(n, Production::CALL_EXPR_ADDR)) {
                SuffixOp sop; sop.kind = SuffixOp::Kind::Addr;
                suffixes.push_back(std::move(sop));
            } else if (!primary) {
                primary = buildPrimaryExpr(n);
            }
        }
        if (!suffixes.empty() && primary) {
            SuffixExpr se;
            se.line = locL(seg[0]); se.col = locC(seg[0]);
            se.base = primary;
            se.suffixes = std::move(suffixes);
            return std::make_shared<ExprNode>(std::move(se));
        }
        return primary;
    };

    if (segments.size() == 1)
        return buildTerm(segments[0]);

    // Multiple segments → BinaryExpr
    BinaryExpr be;
    be.line = locL(pt); be.col = locC(pt);
    be.first = buildTerm(segments[0]);
    for (size_t i = 0; i < operators.size(); ++i) {
        BinaryExpr::OpTerm ot;
        ot.op = firstTxt(operators[i]);
        ot.term = buildTerm(segments[i + 1]);
        be.rest.push_back(std::move(ot));
    }
    return std::make_shared<ExprNode>(std::move(be));
}

// ========================================================================
// Statement / block / group builders
// ========================================================================

static Block::Kind blockKindFromProd(Production p) {
    switch (p) {
        case Production::DO_WHEN:         return Block::Kind::DoWhen;
        case Production::DO_WHEN_MULTI:   return Block::Kind::DoWhenMulti;
        case Production::DO_WHEN_FAIL:    return Block::Kind::DoWhenFail;
        case Production::DO_WHEN_SELECT:  return Block::Kind::DoWhenSelect;
        case Production::DO_ELSE:         return Block::Kind::DoElse;
        case Production::DO_BLOCK:        return Block::Kind::DoBlock;
        case Production::DO_REWIND:       return Block::Kind::DoRewind;
        case Production::DO_RECOVER:      return Block::Kind::DoRecover;
        case Production::DO_RECOVER_SPEC: return Block::Kind::DoRecoverSpec;
        case Production::DO_ON_EXIT:      return Block::Kind::DoOnExit;
        case Production::DO_ON_EXIT_FAIL: return Block::Kind::DoOnExitFail;
        default:                          return Block::Kind::DoBlock;
    }
}

static bool isBlockProd(Production p) {
    return p == Production::DO_WHEN || p == Production::DO_WHEN_MULTI ||
           p == Production::DO_WHEN_FAIL || p == Production::DO_WHEN_SELECT ||
           p == Production::DO_ELSE || p == Production::DO_BLOCK ||
           p == Production::DO_REWIND || p == Production::DO_RECOVER ||
           p == Production::DO_RECOVER_SPEC || p == Production::DO_ON_EXIT ||
           p == Production::DO_ON_EXIT_FAIL;
}

static StatNode buildBlock(const spParseTree& pt) {
    Block blk;
    blk.line = locL(pt); blk.col = locC(pt);
    blk.kind = blockKindFromProd(pt->production);
    if (is(pt, Production::DO_RECOVER_SPEC)) {
        auto spec = findChild(pt, Production::RECOVER_SPEC);
        if (spec) {
            auto tnq = findChild(spec, Production::TYPE_NAME_Q);
            if (tnq) {
                blk.recoverType = buildTypeNameQ(tnq);
                // IDENTIFIER sibling = bound identifier
                for (auto c = down(spec); c; c = nxt(c))
                    if (is(c, Production::IDENTIFIER))
                        blk.recoverIdent = collectIdent(c);
            } else {
                // CALL_EXPR_TERM fallback (flattened children)
                blk.recoverExpr = buildPrimaryExpr(down(spec));
            }
        }
    }
    auto body = findChild(pt, Production::CALL_GROUP);
    if (body) blk.body = buildCallGroup(body);
    return StatNode(std::move(blk));
}

static StatNode buildStatement(const spParseTree& pt) {
    if (is(pt, Production::CALL_ASSIGNMENT)) {
        AssignStat as;
        as.line = locL(pt); as.col = locC(pt);
        auto kids = allChildren(pt);
        if (!kids.empty()) {
            // First child: ALLOC_IDENTIFIER or IDENTIFIER
            auto& tgt = kids[0];
            IdentifierExpr ie;
            ie.text = collectIdent(tgt);
            ie.isAlloc = isAllocIdent(tgt);
            ie.line = locL(tgt); ie.col = locC(tgt);
            as.target = std::make_shared<ExprNode>(std::move(ie));
        }
        // Second child: SUBCALL_EXPRESSION
        if (kids.size() > 1)
            as.value = buildSubcallExpr(kids[1]);
        return StatNode(std::move(as));
    }
    if (is(pt, Production::CALL_EXPRESSION)) {
        ExprStat es;
        es.line = locL(pt); es.col = locC(pt);
        es.expr = buildCallExpression(pt);
        return StatNode(std::move(es));
    }
    if (isBlockProd(pt->production))
        return buildBlock(pt);
    // Fallback: treat as expression
    ExprStat es;
    es.line = locL(pt); es.col = locC(pt);
    es.expr = buildPrimaryExpr(pt);
    return StatNode(std::move(es));
}

static std::shared_ptr<CallGroup> buildCallGroup(const spParseTree& pt) {
    if (!pt) return nullptr;
    auto cg = std::make_shared<CallGroup>();
    cg->line = locL(pt); cg->col = locC(pt);
    for (auto c = down(pt); c; c = nxt(c))
        cg->statements.push_back(buildStatement(c));
    return cg;
}

static std::shared_ptr<CmdBody> buildCmdBody(const spParseTree& pt) {
    if (!pt) return nullptr;
    auto body = std::make_shared<CmdBody>();
    body->line = locL(pt); body->col = locC(pt);
    if (findChild(pt, Production::DEF_CMD_EMPTY)) {
        body->isEmpty = true;
    } else {
        auto cg = findChild(pt, Production::CALL_GROUP);
        if (cg) body->group = buildCallGroup(cg);
    }
    return body;
}

// ========================================================================
// Command parameter and signature builders
// ========================================================================

static CmdParam buildCmdParm(const spParseTree& pt) {
    // DEF_CMD_PARM: children = DEF_CMD_PARMTYPE_NAME or DEF_CMD_PARMTYPE_VAR, then DEF_CMD_PARM_NAME
    CmdParam cp;
    auto nameNode = findChild(pt, Production::DEF_CMD_PARM_NAME);
    cp.name = txt(nameNode);
    auto typeNameNode = findChild(pt, Production::DEF_CMD_PARMTYPE_NAME);
    auto typeVarNode = findChild(pt, Production::DEF_CMD_PARMTYPE_VAR);
    if (typeVarNode) {
        cp.isTypeVar = true;
        auto c = down(typeVarNode); // TYPENAME (the T)
        cp.typeVarName = txt(c);
        auto inner = findChild(typeVarNode, Production::DEF_CMD_PARMTYPE_NAME);
        if (inner) cp.type = buildTypeExpr(down(inner));
    } else if (typeNameNode) {
        cp.type = buildTypeExpr(down(typeNameNode));
    }
    return cp;
}

static CmdReceiver buildCmdReceiver(const spParseTree& pt) {
    // DEF_CMD_RECEIVER: DEF_CMD_PARMTYPE_NAME + DEF_CMD_PARM_NAME
    CmdReceiver cr;
    auto typeNode = findChild(pt, Production::DEF_CMD_PARMTYPE_NAME);
    if (typeNode) cr.type = buildTypeExpr(down(typeNode));
    auto nameNode = findChild(pt, Production::DEF_CMD_PARM_NAME);
    cr.name = txt(nameNode);
    return cr;
}

static void parseNameSpec(const spParseTree& pt, std::string& name, FailMode& failMode) {
    // DEF_CMD_NAME_SPEC: optional DEF_CMD_MAYFAIL or DEF_CMD_FAILS, then DEF_CMD_NAME
    if      (findChild(pt, Production::DEF_CMD_MAYFAIL)) failMode = FailMode::MayFail;
    else if (findChild(pt, Production::DEF_CMD_FAILS))   failMode = FailMode::Fails;
    else                                                  failMode = FailMode::NoFail;
    auto nm = findChild(pt, Production::DEF_CMD_NAME);
    name = txt(nm);
}

static std::vector<CmdParam> buildParmList(const spParseTree& pt) {
    std::vector<CmdParam> parms;
    for (auto c = down(pt); c; c = nxt(c))
        if (is(c, Production::DEF_CMD_PARM)) parms.push_back(buildCmdParm(c));
    return parms;
}

static std::string getRetVal(const spParseTree& pt) {
    auto rv = findChild(pt, Production::DEF_CMD_RETVAL);
    if (!rv) return {};
    auto id = down(rv); // IDENTIFIER group
    return collectIdent(id);
}

static CmdSignature buildSignature(const spParseTree& pt) {
    if (!pt) return RegularSig{};

    if (is(pt, Production::DEF_CMD_REGULAR)) {
        RegularSig rs;
        auto nameSpec = findChild(pt, Production::DEF_CMD_NAME_SPEC);
        if (nameSpec) parseNameSpec(nameSpec, rs.name, rs.failMode);
        auto parms = findChild(pt, Production::DEF_CMD_PARMS);
        if (parms) {
            rs.params = buildParmList(parms);
            rs.returnVal = getRetVal(parms);
        }
        auto imparms = findChild(pt, Production::DEF_CMD_IMPARMS);
        if (imparms) rs.implicitParams = buildParmList(imparms);
        return rs;
    }
    if (is(pt, Production::DEF_CMD_VCOMMAND)) {
        VCommandSig vs;
        auto recvs = findChild(pt, Production::DEF_CMD_RECEIVERS);
        if (recvs) {
            for (auto c = down(recvs); c; c = nxt(c))
                if (is(c, Production::DEF_CMD_RECEIVER))
                    vs.receivers.push_back(buildCmdReceiver(c));
        }
        auto nameSpec = findChild(pt, Production::DEF_CMD_NAME_SPEC);
        if (nameSpec) parseNameSpec(nameSpec, vs.name, vs.failMode);
        auto parms = findChild(pt, Production::DEF_CMD_PARMS);
        if (parms) vs.params = buildParmList(parms);
        auto retval = findChild(pt, Production::DEF_CMD_RETVAL);
        if (retval) vs.returnVal = collectIdent(down(retval));
        auto imparms = findChild(pt, Production::DEF_CMD_IMPARMS);
        if (imparms) vs.implicitParams = buildParmList(imparms);
        return vs;
    }
    if (is(pt, Production::DEF_CMD_CTOR)) {
        ConstructorSig cs;
        auto recv = findChild(pt, Production::DEF_CMD_RECEIVER);
        if (recv) cs.receiver = buildCmdReceiver(recv);
        for (auto c = down(pt); c; c = nxt(c))
            if (is(c, Production::DEF_CMD_PARM)) cs.params.push_back(buildCmdParm(c));
        return cs;
    }
    if (is(pt, Production::DEF_CMD_RECEIVER_ATSTACK)) {
        DestructorSig ds;
        auto recv = findChild(pt, Production::DEF_CMD_RECEIVER);
        if (recv) ds.receiver = buildCmdReceiver(recv);
        return ds;
    }
    if (is(pt, Production::DEF_CMD_RECEIVER_ATSTACK_FAIL)) {
        FailHandlerSig fs;
        auto recv = findChild(pt, Production::DEF_CMD_RECEIVER);
        if (recv) fs.receiver = buildCmdReceiver(recv);
        return fs;
    }
    return RegularSig{};
}

// ========================================================================
// Top-level definition builders
// ========================================================================

// Find the first signature-kind child of a DEF_CMD/DEF_CMD_DECL/DEF_CMD_INTRINSIC node
static spParseTree findSigChild(const spParseTree& pt) {
    for (auto c = down(pt); c; c = nxt(c)) {
        auto p = c->production;
        if (p == Production::DEF_CMD_REGULAR || p == Production::DEF_CMD_VCOMMAND ||
            p == Production::DEF_CMD_CTOR || p == Production::DEF_CMD_RECEIVER_ATSTACK ||
            p == Production::DEF_CMD_RECEIVER_ATSTACK_FAIL)
            return c;
    }
    return nullptr;
}

static TopLevelDef buildTopLevel(const spParseTree& pt) {
    auto p = pt->production;

    if (p == Production::DEF_ALIAS) {
        AliasDecl ad;
        ad.line = locL(pt); ad.col = locC(pt);
        auto tn = findChild(pt, Production::TYPEDEF_NAME_Q);
        if (tn) ad.name = collectTypeName(down(tn));
        auto te = findChild(pt, Production::TYPE_EXPR);
        if (te) ad.type = buildTypeExpr(te);
        return ad;
    }
    if (p == Production::DEF_DOMAIN) {
        DomainDecl dd;
        dd.line = locL(pt); dd.col = locC(pt);
        auto nm = findChild(pt, Production::DEF_DOMAIN_NAME);
        if (nm) dd.name = collectTypeName(down(nm));
        auto par = findChild(pt, Production::DEF_DOMAIN_PARENT);
        if (par) {
            auto tn = down(par);
            if (tn && is(tn, Production::TYPENAME))  {
                NamedType nt; nt.name = txt(tn);
                nt.line = locL(tn); nt.col = locC(tn);
                dd.parent = std::make_shared<TypeNode>(std::move(nt));
            } else if (tn && is(tn, Production::DEF_DOMAIN_PARENT_RANGE)) {
                RangeType rt; rt.line = locL(tn); rt.col = locC(tn);
                auto sz = findChild(tn, Production::DEF_DOMAIN_PARENT_RANGE_SIZE);
                if (sz) rt.size = txt(down(sz));
                auto et = findChild(tn, Production::DEF_DOMAIN_PARENT_RANGE_TYPE);
                if (et) {
                    NamedType elem; elem.name = txt(down(et));
                    elem.line = locL(et); elem.col = locC(et);
                    rt.element = std::make_shared<TypeNode>(std::move(elem));
                }
                dd.parent = std::make_shared<TypeNode>(std::move(rt));
            }
        }
        return dd;
    }
    if (p == Production::DEF_ENUM) {
        EnumDecl ed;
        ed.line = locL(pt); ed.col = locC(pt);
        auto etn = findChild(pt, Production::DEF_ENUM_TYPENAME);
        if (etn) ed.enumTypeName = txt(etn);
        auto en = findChild(pt, Production::DEF_ENUM_NAME);
        if (en) ed.enumName = txt(en);
        // Items: pairs of DEF_ENUM_ITEM_NAME + LITERAL
        auto kids = allChildren(pt);
        for (size_t i = 0; i < kids.size(); ++i) {
            if (is(kids[i], Production::DEF_ENUM_ITEM_NAME)) {
                EnumItem item;
                item.name = txt(kids[i]);
                item.line = locL(kids[i]); item.col = locC(kids[i]);
                // Next sibling should be a literal
                if (i + 1 < kids.size()) item.value = txt(kids[i + 1]);
                ed.items.push_back(std::move(item));
            }
        }
        return ed;
    }
    if (p == Production::DEF_RECORD) {
        RecordDecl rd;
        rd.line = locL(pt); rd.col = locC(pt);
        auto nm = findChild(pt, Production::DEF_RECORD_NAME);
        if (nm) rd.name = collectTypeName(down(findChild(nm, Production::TYPEDEF_NAME_Q)));
        auto flds = findChild(pt, Production::DEF_RECORD_FIELDS);
        if (flds) rd.fields = buildRecordFields(flds);
        return rd;
    }
    if (p == Production::DEF_OBJECT) {
        ObjectDecl od;
        od.line = locL(pt); od.col = locC(pt);
        auto nm = findChild(pt, Production::DEF_OBJECT_NAME);
        if (nm) od.name = collectTypeName(down(findChild(nm, Production::TYPEDEF_NAME_Q)));
        auto flds = findChild(pt, Production::DEF_OBJECT_FIELDS);
        if (flds) od.fields = buildObjectFields(flds);
        return od;
    }
    if (p == Production::DEF_UNION) {
        UnionDecl ud;
        ud.line = locL(pt); ud.col = locC(pt);
        auto nm = findChild(pt, Production::DEF_UNION_NAME);
        if (nm) ud.name = collectTypeName(down(findChild(nm, Production::TYPEDEF_NAME_Q)));
        auto cands = findChild(pt, Production::DEF_UNION_CANDIDATES);
        if (cands) ud.candidates = buildUnionCandidates(cands);
        return ud;
    }
    if (p == Production::DEF_VARIANT) {
        VariantDecl vd;
        vd.line = locL(pt); vd.col = locC(pt);
        auto nm = findChild(pt, Production::DEF_VARIANT_NAME);
        if (nm) vd.name = collectTypeName(down(findChild(nm, Production::TYPEDEF_NAME_Q)));
        auto cands = findChild(pt, Production::DEF_VARIANT_CANDIDATES);
        if (cands) vd.candidates = buildVariantCandidates(cands);
        return vd;
    }
    if (p == Production::DEF_INSTANCE) {
        InstanceDecl id;
        id.line = locL(pt); id.col = locC(pt);
        auto nm = findChild(pt, Production::DEF_INSTANCE_NAME);
        if (nm) {
            auto tdnq = findChild(nm, Production::TYPEDEF_NAME_Q);
            if (tdnq) id.name = collectTypeName(down(tdnq));
        }
        auto types = findChild(pt, Production::DEF_INSTANCE_TYPES);
        if (types) {
            InstanceType cur;
            for (auto c = down(types); c; c = nxt(c)) {
                if (is(c, Production::TYPENAME)) {
                    if (!cur.typeName.empty()) { id.types.push_back(std::move(cur)); cur = {}; }
                    cur.typeName = txt(c);
                    cur.line = locL(c); cur.col = locC(c);
                } else if (is(c, Production::DEF_INSTANCE_DELEGATE)) {
                    cur.delegate = collectIdent(down(c));
                }
            }
            if (!cur.typeName.empty()) id.types.push_back(std::move(cur));
        }
        return id;
    }
    if (p == Production::DEF_CMD_DECL) {
        CmdDecl cd;
        cd.line = locL(pt); cd.col = locC(pt);
        auto sig = findSigChild(pt);
        if (sig) cd.signature = buildSignature(sig);
        return cd;
    }
    if (p == Production::DEF_CMD_INTRINSIC) {
        IntrinsicDecl intd;
        intd.line = locL(pt); intd.col = locC(pt);
        // Intrinsic always has DEF_CMD_REGULAR form (name_spec + parms + imparms as direct children)
        RegularSig rs;
        auto nameSpec = findChild(pt, Production::DEF_CMD_NAME_SPEC);
        if (nameSpec) parseNameSpec(nameSpec, rs.name, rs.failMode);
        auto parms = findChild(pt, Production::DEF_CMD_PARMS);
        if (parms) { rs.params = buildParmList(parms); rs.returnVal = getRetVal(parms); }
        auto imparms = findChild(pt, Production::DEF_CMD_IMPARMS);
        if (imparms) rs.implicitParams = buildParmList(imparms);
        intd.signature = std::move(rs);
        return intd;
    }
    if (p == Production::DEF_CMD) {
        CmdDef cd;
        cd.line = locL(pt); cd.col = locC(pt);
        auto sig = findSigChild(pt);
        if (sig) cd.signature = buildSignature(sig);
        auto body = findChild(pt, Production::DEF_CMD_BODY);
        if (body) cd.body = buildCmdBody(body);
        return cd;
    }
    if (p == Production::DEF_CLASS) {
        ClassDecl cls;
        cls.line = locL(pt); cls.col = locC(pt);
        auto nm = findChild(pt, Production::DEF_CLASS_NAME);
        if (nm) cls.name = txt(down(nm));
        auto cmds = findChild(pt, Production::DEF_CLASS_CMDS);
        if (cmds) {
            for (auto c = down(cmds); c; c = nxt(c)) {
                if (is(c, Production::DEF_CMD_DECL)) {
                    CmdDecl cd; cd.line = locL(c); cd.col = locC(c);
                    auto sig2 = findSigChild(c);
                    if (sig2) cd.signature = buildSignature(sig2);
                    cls.members.push_back(std::move(cd));
                } else if (is(c, Production::DEF_CMD)) {
                    CmdDef cdef; cdef.line = locL(c); cdef.col = locC(c);
                    auto sig2 = findSigChild(c);
                    if (sig2) cdef.signature = buildSignature(sig2);
                    auto body = findChild(c, Production::DEF_CMD_BODY);
                    if (body) cdef.body = buildCmdBody(body);
                    cls.members.push_back(std::move(cdef));
                }
            }
        }
        return cls;
    }
    if (p == Production::DEF_PROGRAM) {
        ProgramDecl pd;
        pd.line = locL(pt); pd.col = locC(pt);
        auto inv = down(pt); // first child is CALL_INVOKE result
        if (inv) pd.entryPoint = buildInvokeExpr(inv);
        return pd;
    }
    if (p == Production::DEF_TEST) {
        TestDecl td;
        td.line = locL(pt); td.col = locC(pt);
        auto str = findChild(pt, Production::STRING);
        if (str) td.label = txt(str);
        auto cg = findChild(pt, Production::CALL_GROUP);
        if (cg) td.body = buildCallGroup(cg);
        return td;
    }
    // Fallback: return an empty alias
    return AliasDecl{};
}

// ========================================================================
// Entry point
// ========================================================================

std::shared_ptr<CompilationUnit> buildAst(const spParseTree& pt) {
    if (!pt || !is(pt, Production::COMPILATION_UNIT))
        return nullptr;

    auto cu = std::make_shared<CompilationUnit>();
    cu->line = locL(pt); cu->col = locC(pt);

    for (auto c = down(pt); c; c = nxt(c)) {
        auto p = c->production;

        if (p == Production::DEF_MODULE) {
            auto mod = std::make_shared<ModuleDecl>();
            mod->line = locL(c); mod->col = locC(c);
            auto mn = findChild(c, Production::DEF_MODULE_NAME);
            if (mn) mod->name = collectTypeName(down(mn));
            cu->module = mod;
            continue;
        }
        if (p == Production::DEF_IMPORT) {
            auto imp = std::make_shared<ImportDecl>();
            imp->line = locL(c); imp->col = locC(c);
            auto file = findChild(c, Production::DEF_IMPORT_FILE);
            auto std_ = findChild(c, Production::DEF_IMPORT_STANDARD);
            if (file) {
                imp->kind = ImportDecl::Kind::File;
                auto fn = findChild(file, Production::DEF_IMPORT_FILENAME);
                if (fn) imp->path = txt(fn);
                auto al = findChild(file, Production::DEF_IMPORT_ALIAS);
                if (al) imp->alias = collectTypeName(down(al));
            } else if (std_) {
                imp->kind = ImportDecl::Kind::Standard;
                // Name: the TYPENAME child (not the alias)
                for (auto sc = down(std_); sc; sc = nxt(sc)) {
                    if (is(sc, Production::DEF_IMPORT_ALIAS))
                        imp->alias = collectTypeName(down(sc));
                    else if (is(sc, Production::TYPENAME))
                        imp->name = txt(sc);
                }
            }
            cu->imports.push_back(imp);
            continue;
        }
        // All other top-level definitions
        cu->definitions.push_back(buildTopLevel(c));
    }

    return cu;
}

} // namespace basis