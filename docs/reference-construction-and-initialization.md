# Basis Language — Reference: Construction and Initialization

**Status:** Topic-organized authoritative reference for the Basis construction-and-initialization surface. Consolidates material from CP001 (foundational construction mechanics, expression-position calling convention), CP004 (initialization analysis, the lattice that construction must satisfy), CP006 (buffer-backed principle and OQ-9 atomic compound construction), CP007 (provision-chain rule, literals as untyped buffers), CP009 (the densest source — `<-` polymorphic RHS, Aggregate/Sequence literals, `.implicit`, `=` defaults, OQ-10 resolution), CP012 (typed-failure construction patterns), CP014 (`-> name` unified meaning), and the 2026-05-03 scope-confirmation dialog recorded in CP015 (eight substantive amendments and two new open questions, all encoded from inception). Cross-references the failure-system, operational-semantics, and type-system-and-modes references for material owned there; defers lambda/fexpr capture-bake-in mechanics and the class-system's instance-coherence story to their respective planned references.

**Date:** 2026-05-03

**Provenance:** Distilled by Claude (Opus 4.7) from the Basis intent-dialog corpus, in continuation of the topic-organized consolidation begun with `reference-failure-system.md` and continued through `reference-operational-semantics.md` and `reference-type-system-and-modes.md`.

**Authority statement.** Where this reference differs from the source checkpoints on a covered topic, this reference is authoritative; the source checkpoints remain useful as historical record. Where this reference and the source code differ on syntactic matters, the code is authoritative; on semantic intent, this reference is authoritative. Where this reference and an *earlier* reference differ on overlapping material, this reference is authoritative on the construction-side aspects and the earlier reference is authoritative on the type-side or operational-side aspects, with bridge notes flagging the boundary. Specifically: the `-<` operator's surface form (declared in type-system §3.5) lives in full at §11 here; the `${...}` / `$[...]` literal-token surface (revising CP009 §3 / §8) is canonical here from inception; the union → candidate-or-parent byte-reinterpretation subsumption (replacing the "interpretive cast operator" framing of type-system §2.6) lives at §10 here.

**Citation convention.** Decision-level rules carry inline citations of the form *[CPnnn §x.y]*. Section-level connective tissue carries footer citations identifying the section spans drawn upon. **Reconciliation markers** appear inline wherever the reference makes a choice not directly determined by the source checkpoints — typically resolving an ambiguity, recording a post-checkpoint correction, or carving in an amendment from the 2026-05-03 dialog. **Cross-references** to earlier references take the form *[failure-system §x]*, *[op-sem §x]*, *[type-system §x]*; **forward references** to planned but undrafted references are flagged as such.

**Standing principles carried in.** Three lenses from prior references govern the treatment here:

1. **Frame-ownership** *[type-system standing principle]*. Every slot is owned by some frame; "return" in the conventional sense does not exist; output flows through writeable parameter slots the caller owns, written by copy-restore on success. The construction story is, end-to-end, a story about how the caller's slot transitions from uninitialized to initialized exactly once on every successful path, with copy-restore preserving the pre-call state on failure.

2. **Buffer-backed principle and its containment rule** *[type-system §1.2; CP006 §1.2]*. A buffer-backed type may contain only buffer-backed types. Aggregate and Sequence literals respect this rule structurally: a record's Aggregate-literal field-positions are buffer-backed-typed; a typed buffer's Sequence-literal element-positions are buffer-backed-typed. Non-buffer types appear at top-level slots and as object fields; their construction surface differs accordingly (no Sequence literals for object containers; Aggregate literals admitted only at the object's own field-set).

3. **No-defaults discipline** *[type-system §6; CP004]*. There is no language-imposed default value for any type. Every slot's value arrives via explicit construction at every use site. The construction surface is the explicit-construction surface; the `=` defaults declaration form (§12) is not an exception but the user-facing mechanism for declaring a buffer-backed type's invariant-respecting default, where the type's bytes have no meaningful zero.

A fourth principle is introduced in this reference and is structural for the construction surface specifically:

4. **Compute-then-commit, structurally enforced.** A constructor body's natural shape is two-phase: Phase 1 computes constituent values into ordinary local slots; Phase 2 atomically initializes the productive receiver via a single `<-`. The write-once productive rule (CP009 §2; §8 below) makes the single-write Phase 2 the only valid pattern for productive slots; OQ-9's whole-slot tracking (CP006 §2) makes the slot's transition atomic. The two together turn "compute then commit" from convention into structural enforcement.

---

## 2. The `<-` Operator and Its Polymorphic Right-Hand Side

### 2.1 Role

The `<-` operator is the language's runtime placement primitive. It writes a value into a slot, observing the slot's parameter-mode contract and the language's failure-atomicity discipline. *[CP001 §3.2; CP009 §3.1.]*

`<-` appears in three syntactic positions:

- **Local introduction.** `# x : T <- ‹rhs›` introduces a new local slot `x` of type `T` initialized from `‹rhs›`. The introduction-with-initializer is the natural shape; bare `# x : T` (without initializer) is admitted only for buffer-backed types whose bytes have a meaningful zero, which is rare in practice (see §12 for the `=` defaults declaration mechanism that closes the gap for non-zero-defaultable buffer-backed types).
- **Productive write.** `'r <- ‹rhs›` writes the constructor's productive receiver. Under the write-once rule (§8.1), this is the constructor body's single Phase 2 commit.
- **Reference write.** `&x <- ‹rhs›` rewrites the slot bound to a REFERENCE-mode parameter or to a previously-initialized local. The slot must already be initialized at the point of the write; the write replaces its value.

In all three positions, the typechecker enforces failure-atomicity: a may-fail `‹rhs›` whose evaluation fails leaves the lhs slot in its pre-write state (uninitialized for `# x : T` with a may-fail rhs that fails, or whatever the slot held for `'r` and `&x`). See §7 for the full failure-atomicity story.

### 2.2 The Polymorphic RHS

The right-hand side of `<-` accepts five distinct surface shapes. The typechecker dispatches semantics on the rhs shape and the lhs type; each shape fits a subset of lhs types, and a given lhs type accepts a subset of shapes.

| Shape | Surface | LHS types accepted | Section |
|---|---|---|---|
| Parenthesized call | `(cmd: args)` | Any (the cmd's productive output type) | §2.3 |
| Aggregate literal | `${field <- value, …}` (named) or `${value, …}` (positional) or `${}` (empty) | Records, objects, unions, variants | §3 |
| Sequence literal | `$[value, …]` or `$[]` (empty) | Buffer primitives `[N]` / `[]`; typed buffers `[N]T` / `[]T` | §4 |
| Bare identifier | `name` | Buffer-backed types, pointers, command-typed values | §6 |
| Bare literal | `3.14`, `"hello"`, `0x41`, `'A'` | Whatever a matching `.implicit` constructor accepts | §5 |

*[Reconciliation: the literal-token surface is the `$`-prefixed form per the 2026-05-03 dialog (CP015 §2). CP009 §3 / §8 used `{- ... -}` and `[- ... -]`; this reference encodes the revised tokens from inception. The change is substantive: `$`-prefixed openers are unambiguous to the lexer (`$` has no other use in current Basis source materials), eliminating the lookahead/context-sensitive-lexing problem CP009 §8 acknowledged and OQ-23 tracked. The lexer-disambiguation half of OQ-23 closes here; the residual `.inline`-placement question forwards to the implementation thread (§14).]*

The typechecker's dispatch matrix is dense — five shapes by however many lhs categories — but the rules are local: each shape's well-formedness against an lhs type is decided independently of context, and a shape-vs-type mismatch is reported as a single error at the `<-` site.

### 2.3 The Parenthesized Call Form

The parenthesized call form `(cmd: args)` is the existing call mechanism from CP001 §3.3, used in expression position. The desugaring proceeds in two visible steps: *[CP009 §3.2.]*

```
# r <- (cmd: x, y)              ; surface
cmd: # r, x, y                  ; intermediate (slot introduced in argument position)
# r; cmd: 'r, x, y              ; final (separate introduction and call)
```

The intermediate form, with the `#`-introduction in argument position, is the natural step for the typechecker to reason about: the slot is introduced uninitialized and bound to a productive parameter position simultaneously. The productive obligation discharges on the call's successful return per CP004 §3.

The implicit `-> name` form (CP001 §3.3; type-system §4.9) determines which writeable parameter receives the placement when the called command has exactly one such parameter; otherwise an explicit `-> name` clause is required at signature-declaration time. The expression-position result type follows the unified rule from CP014: the post-write-back value for PRODUCE, the post-call slot value for REFERENCE, the pre-call value for READ. *[CP014 §2.2; type-system §4.9.]*

For Aggregate, Sequence, bare-identifier, and bare-literal forms, no parameter-position desugaring applies; the placement is direct (§§3, 4, 5, 6).

### 2.4 The Typechecker's Shape-vs-Type Matrix

The acceptance matrix is summarized for reference; sections §§3–6 spell out the per-shape rules:

| LHS type category | Call | `${...}` | `$[...]` | Bare ident | Bare literal |
|---|---|---|---|---|---|
| Buffer primitive `[N]` / `[]` | yes | — | yes (positional) | yes (copy) | yes (via `.implicit` matching `[N]`) |
| Typed buffer `[N]T` / `[]T` | yes | — | yes (positional) | yes (copy) | via `.implicit` |
| Plain domain | yes | — | — | yes (copy, on type-compat) | via `.implicit` |
| Record | yes | yes (named or positional-when-clear) | — | yes (copy, identical type) | via `.implicit` |
| Union | yes | yes (single-candidate form) | — | yes (copy, identical type) | via `.implicit` |
| Object | yes | yes (named) | — | **no** (identity; §6.2) | via `.implicit` |
| Variant | yes | yes (single-candidate or `_` for absent) | — | **no** (reference semantics; §6.2) | via `.implicit` |
| Pointer `^T` | yes | — | — | yes (copy of pointer value) | — |
| Command-typed value | yes | — | — | yes (copy of command value) | — |

A dash in a cell indicates the shape is ill-formed for that lhs type. Wrap to: any `<-` form not listed under the lhs's row is a static error reported at the `<-` site, with the typechecker naming the lhs type and the shape encountered.

*Sources for §2: CP001 §§3.2, 3.3 (`<-` original role; expression-position sugar; `-> name`); CP009 §§3.1, 3.2 (polymorphic-RHS table; desugaring); CP014 §2 (`-> name` unified across modes); CP015 §2 (literal-token surface revision); type-system §4.9 (`-> name` type-side rule); op-sem §5.2 (parameter-passing operational mechanics).*

---

## 3. Aggregate Literals

### 3.1 Surface Form

An Aggregate literal initializes a record, object, union, or variant by named field-or-candidate. The named surface form is:

```
${field-name <- value, field-name <- value, …}
```

The `${` opening token is unambiguous (the `$` character has no other use in current Basis source); the closing `}` is the matching brace. Inside the literal, `<-` is reused at a new grammatical position as the field-name-to-value separator. *[CP009 §3.3, §8.2; CP015 §2.]*

The empty form `${}` is a single token denoting an Aggregate literal with no field entries. It is well-formed only when the lhs type has every field defaulted, has no required fields, or — for variants — when the explicit absent-state introduction at §3.5 below is intended (where `${}` is *not* the syntactic carrier; the absent introduction uses bare `# x : SomeVariant`).

### 3.2 Field-Name Nominal Matching

The typechecker matches the literal's field names to the lhs type's declared field names. The matching is nominal, not positional:

- **Order in the literal does not need to match declaration order.** The typechecker resolves entries by name. `${y <- 2, x <- 1}` and `${x <- 1, y <- 2}` are equivalent for an lhs type whose declared fields are `x` then `y`.
- **Repeated field names are rejected at parse time.** `${x <- 1, x <- 2}` is a syntactic error.
- **Extra field names (names not declared on the lhs type) are admitted but generate a compiler warning.** Unmatched entries are silently ignored at runtime; the warning surfaces the discrepancy. The motivation: tooling that emits Aggregate literals from heterogeneous sources should not be defeated by spurious data, but the user should be aware the data is being dropped. *[CP009 §3.3.]*
- **Required fields must be present, except where defaults cover omission.** A field is required unless the lhs type's declaration provides a default for that field (via the `=` form, §12). A defaulted field may be omitted; the default value is supplied. Omission of a defaulted field generates a warning, suppressible by writing the field explicitly with its default value. *[CP009 §3.3.]*
- **Variant-typed fields are *always* required, with `_` as the absent value.** This refines the "required unless defaulted" rule for variant-typed fields specifically; see §3.4.

### 3.3 Positional Form Under Contextual Clarity

The positional surface form `${value, value, …}` is admitted when the target type is contextually explicit — that is, when the typechecker can determine the lhs type at the literal's position without reference to the literal's contents. *[Reconciliation: the positional form is the 2026-05-03 amendment recorded in CP015 §4. CP009 §3.3 admitted only the named form. The user's motivation for the positional form is ergonomics for record types whose field order is well-known at the call site (a `Point` literal `${1, 2}` for `${Decimal x, Decimal y}` reads naturally). The contextual-clarity restriction is the safeguard against the "what's the lhs type" ambiguity at a bare expression position.]*

The contextual-clarity rule:

- A positional Aggregate literal as the rhs of `# x : T <- ${…}` is well-formed: the lhs type `T` is explicit at the introduction.
- A positional Aggregate literal as an argument at a typed parameter position is well-formed: the parameter's declared type provides the context.
- A positional Aggregate literal as the rhs of `'r <- ${…}` is well-formed: the productive receiver's declared type provides the context.
- A positional Aggregate literal as the value of an outer Aggregate-literal entry whose field is typed is well-formed at the inner position; the outer field's type provides the inner literal's context.

A positional Aggregate literal in a context where the lhs type is not contextually explicit (a hypothetical generic-position case) is rejected with a "positional Aggregate requires explicit type context" error. The user resolves by switching to the named form (`${x <- 1, y <- 2}`) or by introducing a typed local. The named form is always well-formed, contextual-clarity considerations aside. *[CP015 §4.1.]*

The element-to-field assignment under positional form is by declaration order. The lhs type's first declared field receives the first literal entry; the second receives the second; and so on. The literal must supply exactly one entry per required-or-defaulted field of the lhs type, in declaration order. Defaults cannot be omitted from the positional form (the omission would create an off-by-one alignment hazard); a positional literal for an lhs type with a defaulted field must either supply a value for that field or switch to the named form.

### 3.4 Variant-Typed Fields Require Explicit `_`

A variant-typed field of an Aggregate literal's lhs type requires an explicit value entry — including, when the absent state is intended, an explicit `_` marker. *[Reconciliation: this refines CP009 §3.3's "field is required unless the type provides a default" for variant-typed fields specifically. The 2026-05-03 dialog (CP015 §4.2) tightened the rule to *always required, with `_` as the absent value*. Variants admit an absent state inherently (type-system §3.4 / OPS §1.6); were variant fields admitted to fall back to a "default" of absent, positional-form Aggregate literals would risk silent off-by-one alignment when a variant field's entry was simply omitted. The explicit-`_` rule preserves positional-transcription integrity.]*

Concretely, given an lhs record type with a variant-typed field `kind`:

```
${
    // … other fields …,
    kind <- _                           ; named form, explicit absent
}
```

```
${ /* … other fields …, */ _ }         ; positional form, explicit absent
```

Omission — `${ /* other fields */ }` with `kind` missing — is rejected. The diagnostic names the variant-typed field whose entry is absent and reminds the user that variant fields require explicit `_` for the absent state.

The `_` marker has two roles in the construction surface: as the absent value for variant-typed fields (here) and as the discard marker on either side of `-<` for variant operands (§11.3, §11.4). Both roles share the same lexical token, the same "no value or value-discard" reading, and the same restriction: `_` is well-formed only at variant positions.

### 3.5 Variant Construction (Forward Reference)

Aggregate-literal surface for variant *candidate selection* — the `${Candidate <- value}` form for selecting a non-absent candidate — is treated in §9. The two surfaces (variant-as-field of an outer record/object; variant-as-lhs of a `<-` directly) compose: a variant lhs takes an Aggregate literal whose single entry names the candidate (or, for the absent state, uses the bare `# x : SomeVariant` introduction without an Aggregate literal).

### 3.6 Embedded Objects in Aggregate Literals

When a record or object has an embedded object field, the Aggregate literal references the embedded object by a previously-introduced local name: *[CP009 §3.6.]*

```
.cmd buildContainer : Container 'r =
    # contained <- (InnerObj: someArgs)         ; Phase 1: construct constituent
    'r <- ${inner <- contained, count <- 5}     ; Phase 2: commit
```

The embedded object's at-stack registration migrates to the new container's owning frame at the moment of the Aggregate literal's atomic placement. This composes with CP002 §3.4's existing migration story; no new mechanism is introduced. *[CP009 §3.6; op-sem §6.5; CP002 §§3.3, 3.4.]*

If the constructor `buildContainer` fails — say, a `?something` inside Phase 1 — the partially-constructed `contained` object is registered with `buildContainer`'s frame and is cleaned up via the failure-exit machinery there (CP002 §3.3 Case 3). The atomicity story holds: no half-built container exists at any caller's frame.

### 3.7 Summary

| Form | Surface | Semantics |
|---|---|---|
| Named (general) | `${field <- value, …}` | Field-name nominal matching; order-independent |
| Positional (contextual) | `${value, …}` | By declaration order; lhs type must be contextually explicit |
| Empty | `${}` | All-defaulted record/object case |
| Absent variant field | `${ …, kind <- _, … }` or `${ …, _, … }` | Variant-typed field set to absent state |

*Sources for §3: CP009 §3.3 (named form, repeated/extra/required-field rules); CP009 §3.6 (embedded objects); CP015 §4 (positional form admitted under contextual clarity; variant fields require explicit `_`); type-system §3.4 (variant absent state).*

---

## 4. Sequence Literals

### 4.1 Surface Form

A Sequence literal initializes a positional, vector-like target — a buffer primitive `[N]` or `[]`, or a typed buffer `[N]T` or `[]T`. The surface form is:

```
$[value, value, …]
```

The `$[` opening token is unambiguous (the `$` character has no other use in current Basis source); the closing `]` is the matching bracket. *[CP009 §3.4, §8.1; CP015 §2.]*

The empty form `$[]` is a single token denoting a Sequence literal with zero elements. It is well-formed for `[0]T` (the zero-length typed buffer; rare but admitted) and for `[]T` (the unbounded typed buffer's empty case). It is not well-formed for `[N]T` with `N > 0`.

### 4.2 Element Count and Element Type

The typechecker enforces two rules at the Sequence literal:

- **Element count must match the lhs type's expected count, unless the lhs is unbounded.** For `[N]T`, exactly `N` elements; for `[]T`, any count, with the count becoming part of the buffer's runtime length. *[CP009 §3.4.]*
- **Each element must be of the lhs type's element type, with implicit-conversion eligibility per §5.** `$[1, 2, 3]` for an `$[3]Float32` lhs requires each element to be either a `Float32` value or a literal whose type has an `.implicit` constructor producing `Float32`. *[CP009 §3.4; §5 below.]*

For untyped buffer primitives `[N]` and `[]`, the element type is the byte (a one-byte cell with no domain interpretation). A Sequence literal targeting `[N]` requires each element to be a value of byte width — typically a numeric literal of byte-fitting magnitude or a `[1]`-typed value. The bracket-form types are the literal-bearing types in CP007 §5.1's sense: their construction is "take the bytes," and the Sequence literal feeds bytes positionally.

### 4.3 Typed-Buffer Forms

The type forms `$[N]T` and `$[]T` declare buffers laid out as sequences of `T`-values. *[CP015 §7; type-system §2.1.]*

These forms appear in two construction-relevant contexts:

- **As lhs types** at `# x : $[3]Float32 <- $[…]` and similar — the lhs declares a typed-buffer shape, and the Sequence literal supplies the elements.
- **As `.implicit` constructor parameter types** at `${Type field, …}` and `$[N]T` declarations — the literal-type-mirror form (see §5.4 below; OQ-22 resolution).

The forms are otherwise CP001 §2.1's bracket-form type system: they introduce no new type identity beyond the buffer-substrate-with-element-type interpretation, they subsume up to `[N]` and `[]` (type-system §1.5; OPS §1.5), and they admit indexing per CP001 §2.1.

### 4.4 Disjointness from Aggregate Literals

Aggregate and Sequence literals are *disjoint* in the lhs types they accept. Sequences do not initialize records, objects, unions, or variants. Aggregates do not initialize buffer primitives or typed buffers. The two surface forms occupy non-overlapping construction territory: *[CP009 §3.4.]*

| LHS type | Aggregate `${...}` | Sequence `$[...]` |
|---|---|---|
| Record / object | yes | no |
| Union | yes | no |
| Variant | yes | no |
| Buffer primitive `[N]` / `[]` | no | yes |
| Typed buffer `[N]T` / `[]T` | no | yes |

The disjointness is design, not coincidence: records have *named* field structure (Aggregate's field-name matching is the natural surface); typed buffers have *positional* element structure (Sequence's element-by-position matching is the natural surface). A typed buffer is not a record-with-numbered-fields; the user-side mental model is "an array of values," and the Sequence literal matches that model.

### 4.5 Element-Count Mismatches

For `[N]T` lhs types, a Sequence literal whose element count is not exactly `N` is rejected at the typechecker. The diagnostic names the expected count and the literal's actual count. The user resolves by adjusting the literal's contents or by changing the lhs type to match.

For `[]T` lhs types, the element count is recorded with the buffer at construction; subsequent indexing operations are checked against that runtime length. *[CP001 §2.1; type-system §2.1.]*

For empty Sequence literals `$[]` against `[N]T` with `N > 0`, the rejection diagnostic specializes: "empty Sequence literal not well-formed for `[N]T`; use `$[]T` for an empty unbounded buffer or supply elements."

*Sources for §4: CP009 §3.4 (element-count, element-type rules; empty `[-]` form, here revised to `$[]`); CP015 §2 (literal-token surface); type-system §2.1 (buffer/typed-buffer type-form definitions); CP007 §5 (literals as untyped buffers).*

---

## 5. The `.implicit` Mechanism

### 5.1 Role

Literals in Basis are bytes — untyped buffers in CP007 §5.1's sense. A literal cannot be used as a value of any domain, record, or union directly; it must pass through a constructor that interprets the bytes as a value of the target type. The user-facing surface for that interpretation is either the explicit constructor call `(Float32: 3.14)` or the bare-literal form `3.14` enabled by an `.implicit` constructor declaration. *[CP007 §5.1; CP009 §4.]*

`.implicit` is a constructor-declaration prefix parallel to `.cmd`. A constructor declared with `.implicit` registers as the elision target for "literal-of-type-`L` appearing in a context expecting `T`." The typechecker, upon seeing a bare literal at a typed slot, looks up an `.implicit` constructor whose parameter type matches the literal's source type and whose productive output type matches the slot's type, and inserts the call. The user writes `# x : Float32 <- 3.14`; the typechecker rewrites to `# x : Float32 <- (Float32: 3.14)` if a matching `.implicit Float32 'r : Decimal d` constructor is in scope. *[CP009 §4.1.]*

The mechanism is purely additive: an `.implicit` constructor is *also* an explicit constructor. `(Float32: 3.14)` works regardless of whether `Float32`'s constructor is declared `.implicit` or `.cmd`. The implicit elision is a convenience on top of the explicit form, not a replacement. *[CP009 §4.4.]*

### 5.2 Declaration

The declaration form is parallel to `.cmd`'s constructor form, with `.implicit` as the prefix:

```
.implicit Float32 'r : Decimal d =
    ; … constructor body …
```

This declares an `.implicit` constructor that:

- Produces values of type `Float32` (the receiver's type).
- Takes a single parameter of literal type `Decimal`.
- May be invoked explicitly as `(Float32: 3.14)`.
- May be invoked implicitly when a `Decimal` literal appears in a `Float32`-expecting context.

`.implicit` may only be applied to constructor commands (commands with a productive `'r` receiver of buffer-backed type, and the constructor signature shape from CP005 §2.3 / type-system §4.8). Applying it to a v-command, a regular command, an at-stack method, or any other shape is a compile-time error. *[CP009 §4.1; type-system §4.8.]*

**`.implicit` constructors may not declare `.context` parameters.** *[Reconciliation: this restriction is the 2026-05-03 dialog's resolution of OQ-13 on the construction side (CP015 §8). The `.implicit` mechanism elides a construction step at the surface; admitting `.context` parameters in `.implicit` constructors would compound elision (the literal's elision plus the context resolution's elision), making call-site reasoning brittle. The restriction is enforced at both grammar and semantic-analyzer levels: the grammar's `.implicit`-declaration production omits the `/`-separator that introduces context parameters; a semantic-analyzer check rejects any `.implicit` constructor whose body invokes a context-bearing inner command in a way that would re-introduce the elision. Constructor commands declared `.cmd` retain the standard `.context` capability (§13).]*

### 5.3 The Three Restrictions

Three restrictions keep `.implicit` from collapsing into Scala-style implicit chaos: *[CP009 §4.2.]*

**Restriction 1: Source type must be a literal type.** The implicit constructor's parameter type must be a recognized literal type. The current set:

| Literal type | Source surface |
|---|---|
| `Decimal` | Decimal numeric literals (`42`, `3.14`) |
| `Hex` | Hex numeric literals (`0x41`) |
| `String` | String literals (`"hello"`) |
| `Char` | Character literals (`'A'`) |
| `Aggregate` (parameterized; see §5.4) | Aggregate literals `${field <- value, …}` |
| `Sequence` (parameterized; see §5.4) | Sequence literals `$[value, …]` |

A constructor whose parameter is any other type may not be marked `.implicit`. This narrows the mechanism to its motivating use case (literal coercion) and prevents the cascade pathology where any value implicitly converts to many target types based on cross-cutting rules.

**Restriction 2: No transitive chains.** A literal `L` is converted to `T` via at most one implicit constructor. If `.implicit T 'r : M m` and `.implicit M 'r : L l` both exist but `.implicit T 'r : L l` does not, the chain `L → M → T` is *not* considered. The typechecker rejects the use site as if no implicit conversion were available; the user resolves by writing the chain explicitly.

**Restriction 3: Unambiguous resolution.** If multiple `.implicit T 'r : L l` constructors are visible at a use site (typically by competing imports), the compilation is rejected with an ambiguity error. The user resolves either by removing or rescoping declarations so only one is visible, or by writing the constructor call explicitly.

### 5.4 Parameterized Literal Types: `Aggregate` and `Sequence`

The `Aggregate` and `Sequence` literal types are parameterized by their internal structure. An Aggregate literal `${x <- 1, y <- 2.5}` synthesizes the structural literal type `Aggregate{Decimal x, Decimal y}`; a Sequence literal `$[1, 2, 3]` synthesizes the structural literal type `Sequence{Decimal, length: 3}` (with the length captured at the use site for fixed-length matching). *[CP009 §4.3; CP015 §7.]*

An `.implicit` constructor for an Aggregate or Sequence type uses **type-form mirrors of the value forms** in its parameter declaration: *[Reconciliation: this is the 2026-05-03 dialog's resolution of OQ-22 (CP015 §7). CP009 §4.3 sketched a placeholder structural-type surface and registered OQ-22 to track the exact form. The resolution: the type form for an Aggregate-shaped literal type is `${Type field, …}` (the value-form `${field <- value, …}` mirrored into type-position); the type form for a Sequence-shaped literal type is `$[N]T` for fixed-length and `$[]T` for unbounded. The mirroring keeps the surface symmetric — readers see the same `$`-prefixed shape on both sides — and reuses the existing literal-token grammar at type-position with no new tokens.]*

```
.implicit Point 'r : ${Decimal x, Decimal y} a =
    'r <- ${x <- (Float32: a.x), y <- (Float32: a.y)}

.implicit Vec3 'r : $[3]Decimal v =
    ; … construct Vec3 from positional Decimal triple …
```

The mirroring is structural: the `.implicit Point 'r : ${Decimal x, Decimal y} a` constructor matches Aggregate literals `${x <- 1, y <- 2}`, `${x <- 1, y <- 2.5}`, and so on — any Aggregate literal whose synthesized structural type is `Aggregate{Decimal x, Decimal y}`. Field name and type both participate in the match.

### 5.5 Explicit Calls Always Available

`.implicit` is purely additive. An `.implicit Float32 'r : Decimal d` constructor may be invoked as `(Float32: 3.14)` — exactly the same surface as a non-implicit constructor — at any use site. Code that wishes to be explicit about constructor calls remains exactly as readable as it is today. The user opts into implicit elision per-constructor; any call site can be written either way.

### 5.6 Implementation Phasing

The phasing of `.implicit` support — specifically, whether the simple literal types (`Decimal`, `Hex`, `String`, `Char`) are supported before the parameterized literal types (`Aggregate`, `Sequence`) — is **implementation latitude**, not a language-spec concern. *[Reconciliation: this resolves OQ-24 (CP015 §9). CP009 §4.3 noted that initial implementation might support only the simple literal types and registered OQ-24 to track whether the language committed to phased support or required full support from the start. The resolution: the language commits to the design as specified here (all six literal types), and the implementation may stage its support incrementally — supporting only the simple types in an early release, adding parameterized support later — without language-spec implications. Source code targeting parameterized `.implicit` is well-formed under the language spec; whether a particular compiler release accepts it is the compiler's release-management concern.]*

### 5.7 Resolution of OQ-17

OQ-17 (compound literal syntax and literal-as-input rules) is substantially resolved by the combination of §3 (Aggregate), §4 (Sequence), and this section: *[CP009 §4.5.]*

- The literal-passing surface is the parenthesized call form (`(Float32: 3.14)`) for explicit construction, or the bare-literal form (`3.14`) when an `.implicit` constructor permits.
- Bracket-form types `[N]` and `[]` accept Sequence literals directly via §4 (a `[3]Int32` lhs with a Sequence rhs `$[1, 2, 3]` is well-typed when each element is `Int32` or implicitly-convertible to `Int32`).
- Implicit type conversions are exactly those licensed by `.implicit` constructors and by domain-hierarchy parent-child relationships *[type-system §5.2]*.
- Implicit constructor sugar for the bare-literal-on-typed-slot case is the `.implicit` mechanism itself, narrowly scoped per §5.3.

*Sources for §5: CP009 §§4.1–4.5 (mechanism, restrictions, Aggregate/Sequence parameterized types, explicit-calls-always-available, OQ-17 resolution); CP015 §§7–9 (OQ-22 resolution, OQ-13 resolution on construction side, OQ-24 resolution); CP007 §5 (literals as untyped buffers).*

---

## 6. Bare-Identifier `<-` (Value-Copy Primitive)

### 6.1 The Operation

The bare-identifier form `# y <- x` (or `'r <- x`, or `&y <- x`) is a primitive value-copy. No command runs, no literal is supplied, no `.implicit` conversion fires. The runtime is a byte-copy from `x`'s storage to the lhs slot, suitable for types whose values can be copied as bytes. *[CP009 §3.7.]*

The form is well-defined for three categories of types:

- **Buffer-backed types** (records, plain domains, unions, buffer primitives `[N]` / `[]`, typed buffers `[N]T` / `[]T`). The bytes of `x` are copied to the lhs slot's storage. The lhs and rhs types must be compatible (identical, or one a parent domain of the other in the buffer-backed hierarchy of type-system §1.5).
- **Pointers** (`^T` for any `T`). The pointer value is copied; both pointers reference the same target. The pointed-to storage is unaffected.
- **Command-typed values** (`:<…>`, `?<…>`, `!<…>`). The command-value is copied — including hidden capture fields for lambda values (CP008) and slot references for fexpr values (CP010). Capture-ceiling considerations apply per §6.3 below.

### 6.2 The Two Forbidden Cases

The bare-identifier form is **rejected at the typechecker** for two non-buffer types:

- **Objects.** Objects have identity. "Copying" an object would either share the identity (which would be a pointer-bind, not a copy) or duplicate the storage (which would require a copy-constructor specific to the object's class). Neither is the right meaning for a primitive `<-`. The user must invoke a copy-constructor — or a class method that does the appropriate value-copy — explicitly, or share access via a `^Object` pointer (which is itself a pointer-copy, well-formed under §6.1's pointer case). *[CP009 §3.7.]*

- **Variants.** Variants have reference semantics (type-system §3.4): the slot is a 3-word triple (tag, candidate-pointer, witness) where the candidate-pointer references storage potentially owning at-stack handlers. A primitive byte-copy of the slot would create two slots referencing the same candidate storage — an aliasing the language does not track and that would defeat the at-stack discipline for the candidate. The user must invoke a constructor (which produces a fresh variant slot) or share access via a `^Variant` pointer. *[CP009 §3.7; CP015 §3 for the variant-clearing surface `v -< _` covered in §11.3.]*

Both rejections are typechecker-side; the diagnostic names the type's identity-bearing-or-reference character and points the user at the appropriate constructor or pointer surface.

### 6.3 Capture-Ceiling for Command-Typed Values

For lambdas with reference captures (CP008 §5.2 / type-system §3.2), the ceiling of the copy equals the ceiling of the source. The byte-copy of the lambda's hidden-fields includes any captured slot references as part of the lambda's identity; the language's existing capture-ceiling tracking applies to the copy as it does to the source. *[CP009 §3.7; CP008 §5.2.]*

For fexprs (CP010), the slot-reference machinery is similar: a fexpr value's identity includes the captured frame's slot references, and a copy preserves them. The fexpr's lifetime ceiling is the capture frame's ceiling, regardless of how many copies of the fexpr value are in flight.

The construction-side observation: a bare-identifier `<-` on a command-typed value does not extend any value's lifetime ceiling. The copy is a peer to the source; both retire when the capture frame retires.

### 6.4 Type-Compatibility Rule

For buffer-backed lhs and rhs, type compatibility is one of: *[type-system §5.5; CP001 §2.2.]*

- **Identical types.** Same nominal declaration, alias erasure aside.
- **Implicit upcast**: rhs is a child domain whose buffer-backed parent chain reaches the lhs type. The upcast is a typing-acceptance rule, not a value-rewriting rule (type-system §5.2): the rhs's domain identity is preserved across the upcast even though the lhs slot's static type is the parent.

Sibling domains do not implicitly convert (`Inches` value `<-` `Centimeters`-typed slot is rejected). Two distinct records over the same parent are *siblings* and do not implicitly convert peer-to-peer; both subsume separately to the shared ancestor (OPS §1.5; type-system §1.5). The user resolves cross-sibling moves by invoking a conversion constructor or by routing through a shared ancestor with explicit `(SharedAncestor: …)` calls.

### 6.5 Worked Examples

```
.cmd duplicate : Int32 input, Int32 'a, Int32 'b =
    # local <- input                ; bare-identifier copy: Int32 → Int32
    'a <- local                     ; bare-identifier copy
    'b <- local                     ; bare-identifier copy
```

Each `<-` is a primitive value-copy. The lhs and rhs types match; no constructor or implicit conversion is involved.

```
.cmd shareObj : SomeObj input, ^SomeObj 'p =
    'p <- (input&)                  ; pointer-bind via address-of
```

The bare-identifier form `'a <- input` for an object lhs would be rejected. The user takes the address explicitly (`input&` is the address-of operator — type-system §3.1) and copies the resulting pointer into a `^SomeObj`-typed slot.

```
.cmd duplicateVariant : Shape input, Shape 'out =
    'out <- input                   ; ERROR — bare-identifier copy not valid for variants
```

The user resolves by invoking a copy-style constructor (defined per the variant's class, if any) or by sharing access through `^Shape`.

*Sources for §6: CP009 §3.7 (the bare-identifier form, copyable categories, forbidden cases); CP008 §5.2 (capture-ceiling on lambda copy); type-system §§3.1, 3.2 (pointer and command-type semantics); type-system §1.5, §5.2 (buffer-backed subsumption hierarchy and one-directional upcast).*

---

## 7. Failure Atomicity in Construction

### 7.1 The Discipline

A `<-` whose right-hand side contains may-fail subexpressions is itself a may-fail operation. The discipline: *[CP009 §3.5; CP001 §3.2.]*

- **Atomic failure.** A failure in any subexpression aborts the whole `<-` operation. The lhs slot's pre-write state is preserved.
- **Order of evaluation is left-to-right.** For Aggregate literals, fields are evaluated in textual order (whether named or positional). For Sequence literals, elements are evaluated in textual order. For parenthesized calls, arguments are evaluated in textual order before the call fires.
- **Short-circuit on failure.** Once a subexpression fails, subsequent subexpressions in the same `<-` are not evaluated. The whole operation propagates the failure.
- **Failure propagates from the `<-` form.** The form acquires the failure mode of any may-fail subexpression: a `<-` containing one or more `?`-marked subexpressions is may-fail; a `<-` containing a `!`-marked subexpression is must-fail. The standard failure-system conformance rules apply *[failure-system §3, §4.9]*.

The atomicity falls out of copy-restore (op-sem §5.2; CP001 §3.2): the language commits no half-written state on failure, regardless of which subexpression failed and at what point. No new transactional machinery is introduced; the existing parameter-passing mechanics suffice.

### 7.2 Copy-Restore on the LHS

The atomic-failure semantics is implemented by holding the computed-but-uncommitted values in temporary slots until every subexpression of the `<-` has succeeded; only then does the placement into the lhs slot fire. *[CP009 IN-12; op-sem §5.2.]*

For Aggregate literals: each field's right-of-`<-` value is computed into a temporary slot. If all field values compute successfully, the lhs is populated by writing each temporary into its corresponding field offset; the operation is the placement step proper. If any field's value fails to compute, the operation aborts before any placement; the lhs slot is bit-identical to its pre-write state.

For Sequence literals: each element is computed into a temporary slot. If all succeed, the lhs is populated by writing each temporary into its corresponding offset. If any fails, the operation aborts; the lhs slot is bit-identical to its pre-write state.

For parenthesized calls: arguments are computed into temporary slots, then the call fires with the temporaries bound to the parameter positions. The call itself is subject to ordinary copy-restore (op-sem §5.2): on success, the productive parameters' values are written back to the caller's slots; on failure, no write-back occurs. The `<-`'s lhs is one such slot; failure of the called command preserves its pre-call state.

The "temporary slots" are an implementation device. The user-side observation: the lhs is unchanged across the entire `<-` operation if any subexpression fails.

### 7.3 Composition with Nested Constructors

A nested construction `'r <- ${field <- (innerCtor: …)}` composes failure-atomicity transitively. A `?`-marked failure inside `innerCtor`'s Phase 1 propagates to `innerCtor`'s caller; copy-restore at the `innerCtor` call leaves the field's temporary slot uninitialized; the outer `<-`'s atomic-failure rule sees a failed field-value computation; the outer `<-` aborts; the outer `'r` is unchanged. *[CP009 §3.5.]*

The chain reaches arbitrary depth without new mechanism. At every level, copy-restore preserves the local slot's pre-call state on the failure path; at every level, the `<-` form's atomic-failure rule rejects the partial computation. The "no half-built thing exists at any caller's frame" guarantee is end-to-end.

The frame-ownership lens applies cleanly: each level's productive slot is *that level's caller's* slot, and each level's failure leaves that caller's slot in its pre-call state. No cross-frame propagation rule is needed (OPS §1.2 / §3.1); the mechanism is local at each frame boundary.

### 7.4 Composition with Typed Failures

Under typed failures (CP012; failure-system §4), a `?`-marked subexpression carries a propagating set; the `<-` form's overall failure mode acquires the union of its subexpressions' propagating sets. *[failure-system §4.9.]*

- A `<-` whose subexpressions are all `:`-marked is itself `:`-marked (no failures).
- A `<-` with a `?`-marked subexpression is `?`-marked, with propagating set the union of subexpressions' sets.
- A `<-` with a `!`-marked subexpression is `!`-marked: the must-fail subexpression always fails, so reaching the post-`<-` program point is statically impossible. The body's conformance check sees the `!`-call's propagating set, no successful-path exit through the `<-`, and applies the standard `!`-conformance rules.

The handler-side surface uses the standard recovery primitives (failure-system §2.4, §4.6): a `|: SomeTag t` block following a `<-` engages on the `<-`'s failure if `<-`'s tag is at-or-below `SomeTag`, binds the payload, and proceeds with the slot in its pre-write state. The construction surface introduces no new failure-handling primitives; existing failure-system machinery covers the cases.

### 7.5 Construction in Recovery Bodies

A `<-` may appear in a recovery body — typically following a `|` or `|: Tag` that engaged on a propagating failure. The recovery body's initialization analysis takes its starting state from the failure-path states of the slots (CP004 §3.7); a `<-` in the recovery body is well-formed under those starting states.

The pattern is the standard "try-then-recover" idiom:

```
.cmd readConfigOrDefault : Config 'r =
    'r <- (?readConfigFromFile: defaultPath)
    |: ParseFailure
        'r <- ${useDefaultsRecord <- defaults}        ; recovery: use defaults
```

The first `<-` is `?`-marked (because `?readConfigFromFile` is may-fail). The `|: ParseFailure t` engages on a tag-at-or-below `ParseFailure` and writes to `'r` from the recovery body. The combined body satisfies the productive write-once obligation (§8.1): every successful exit path writes `'r` exactly once — via the first `<-` on the no-failure path or via the second `<-` on the recovery path.

### 7.6 Worked Example

```
.cmd parsePoint : Point 'r, String input =
    # parts <- (?splitOn: input, ",")        ; Phase 1: may fail (BadInput)
    # x <- (?parseFloat32: parts[0])         ; Phase 1: may fail (BadFloat)
    # y <- (?parseFloat32: parts[1])         ; Phase 1: may fail (BadFloat)
    'r <- ${x <- x, y <- y}                  ; Phase 2: never fails
```

Each `?` call may fail. On failure of any Phase 1 step:

- The body propagates failure with the relevant tag (`BadInput` for the split; `BadFloat` for either parse).
- `'r` is never written; the caller's slot retains its pre-call state per copy-restore.
- The locals `parts`, `x`, `y` introduced earlier in Phase 1 are abandoned at frame retirement; the failure-exit machinery handles any at-stack registrations (none in this example, since `parts`, `x`, `y` are buffer-backed).

On full Phase 1 success, the Phase 2 Aggregate literal commits atomically: a single `<-` discharges the productive write-once obligation (§8.1).

The shorthand `x <- x` in the literal — left-of-`<-` is the field name `x`; right-of-`<-` is the local-named-`x`'s value — is the natural expression and not a special case.

*Sources for §7: CP009 §3.5 (atomic-failure rules; left-to-right; short-circuit; failure-mode propagation from `<-`); CP001 §3.2 (copy-restore preserves pre-call state); CP004 §3.7 (recovery bodies' starting state); failure-system §§3, 4.9 (conformance rules and typed-failure integration); op-sem §5.2 (parameter-passing operational mechanics).*

---

## 8. Atomic Compound Construction

### 8.1 The Write-Once Productive Rule

Every successful return path through a command's body writes to each productive `'`-marked parameter **exactly once** — not "at least once." *[CP009 §2.1.]*

The strengthening from CP004 §3.3's at-least-once is from "every successful path writes at least once" to "every successful path writes exactly once." The four structural consequences: *[CP009 §2.2.]*

- **Atomic initialization is structurally enforced.** Productive slots transition from uninitialized to initialized in one step on every path. There is no "write a placeholder, then write the real value" idiom; that is two writes, which exactly-once rejects. Compute-then-commit is mandatory rather than idiomatic.
- **The initialization lattice has cleaner monotonicity.** Once a productive slot transitions to `init` on a path, it stays `init` on that path. The lattice from CP004 §3 is unchanged; the sharpening just makes single-write the only valid transition pattern.
- **Recovery patterns are unaffected.** A `?compute` followed by a `|`-recovery sub-body is two textual writes on disjoint paths — one on the `?compute` success path, one on the recovery path. Each *path* has exactly one write; the rule operates on paths, not textual occurrences.
- **Loops cannot write productive slots.** A productive write inside a loop body would mean N writes per N-iteration completion, violating exactly-once on every path that completes one or more iterations. Loops that compute values into a productive slot must do so via a single write after the loop body — which is the natural shape regardless.

The rule applies to productive parameters and productive receivers uniformly; both are the "must-write-exactly-once" category, both discharge their obligation by participating as the lhs of exactly one `<-` on each successful path.

### 8.2 Whole-Slot Tracking and Compound Construction

Initialization tracking is per-slot; there is no per-field state for compound types. *[CP006 §2.1; OQ-9 resolved; type-system §6.1.]*

Every slot — record, object, union, variant, plain domain, buffer, pointer, command-typed — has a single initialization state in the lattice `{init, uninit, uncertain}` (with `uncertain` at control-flow joins, treated as `uninit` for read-checking). Compounds are slots like any other; field-level state does not exist.

The structural consequence for construction: a compound is initialized by a single conceptual write that fills the entire slot. There is no procedural field-by-field construction. The constructor body's natural shape is two-phase: *[CP006 §2.3, §2.4.]*

- **Phase 1: Compute the constituent values.** The body invokes whatever commands are needed to produce the values that will populate the new instance. Each computed value lives in an ordinary local slot — typically a parameter, a `#`-introduced local, or the result of a may-fail command in expression-position. These slots are tracked individually under the standard whole-slot rules; their initialization states are independent.
- **Phase 2: Atomically initialize the receiver.** The body performs a single `<-` to the productive receiver `'r`. This single `<-` fills the entire `'r` slot. Before this write, `'r` is `uninit`; after this write, `'r` is `init`; there is no intermediate state.

The Phase 2 `<-`'s rhs is, in practice, an Aggregate literal (for records, objects, unions, variants), a Sequence literal (for typed buffers), a parenthesized call (where another constructor produces the value), a bare identifier (for value-copy, when applicable), or a bare literal (when an `.implicit` constructor permits). Whatever the rhs shape, the single `<-` discharges the write-once obligation.

### 8.3 Why Compute-then-Commit Is the Right Pattern

The pattern is not a workaround for whole-slot tracking; it is structurally aligned with the language's other commitments: *[CP006 §2.5.]*

- **Mutation either succeeds fully or fails fully.** Atomic initialization is the cleanest realization of this for construction. A partially-built compound cannot exist; therefore, a failure during construction cannot leave a half-built thing behind.
- **No hidden control flow.** A constructor body is read top-to-bottom: compute values, then commit. No hidden reordering, no implicit zero-fill, no constructor-chain hidden in an initializer list. Each line says what it does.
- **Failure semantics integrate naturally.** A may-fail computation in Phase 1 either produces its value successfully (and Phase 2 proceeds) or propagates a failure (and Phase 2 never fires; the caller's slot is untouched per copy-restore).

Whole-slot tracking is the tracking discipline that fits the language's design. Field-level tracking would have been a compromise — adding bookkeeping to support a pattern (procedural field-by-field construction) that the language's other principles already discouraged.

### 8.4 Edge Cases Resolved by Exactly-Once

**Conditional re-writing rejected.** *[CP009 §2.3.]*

```
'r <- a
?: condition
    'r <- b                      ; ERROR — second write on the condition-true path
```

The condition-true path writes twice; exactly-once rejects. The fix is the natural rewrite — choose the value first, write once:

```
?: condition
    'r <- b
?: ; default arm
    'r <- a
```

Both paths now write exactly once. The `?:` chain (op-sem §4.4 / failure-system §2.2) is the n-ary alternation primitive; both arms fall under the chain's "the first guard to succeed engages its body" rule, with the trailing non-`?:` sibling acting as the default arm.

**Conditional initial-write rejected.**

```
?: condition
    'r <- b                      ; condition-false path doesn't write 'r
                                 ; ERROR — at-least-once already rejected this; exactly-once does too
```

The condition-false path doesn't write `'r`; the rule rejects. The fix is to provide a default arm or to introduce a recovery path that writes.

**Nested productive calls compose.** *[CP009 §2.3.]*

```
.cmd outer : Result 'r =
    inner: 'r                    ; inner's productive output discharges outer's
```

A productive sub-call to another command counts as one write. The sub-call's exactly-once obligation discharges the outer one's exactly-once obligation; no special handling is needed.

**Loops cannot write productive slots.** *[CP009 §2.2.]*

```
'r <- ‹initial›
?? ?:
    ‹iteration body›
    'r <- ‹updated›              ; ERROR — a loop iteration writes 'r in addition to the prior write
```

Even a single rewind (`^` in the loop body — op-sem §4.5) would mean two writes: the initial write before the loop and the rewind-iteration's write. The fix: compute the value into an ordinary local across the loop, then commit with one `<-` after the loop:

```
# accum <- ‹initial›
?? ?:
    ‹iteration body›
    accum <- ‹updated›           ; ordinary local; can be rewritten freely
'r <- accum                      ; single Phase 2 commit
```

Note: the local `accum` is `&`-mode-equivalent for the body's analysis (REFERENCE-style), not productive. The body's writes to `accum` are unrestricted as long as `accum` was initialized at the loop's entry. Exactly-once is a productive-parameter rule, not a general-write rule.

### 8.5 The Contract Decomposed by Frame

Frame-ownership (OPS §1.1; type-system standing principle) makes the rule's locality explicit. The exactly-once obligation is the *callee's* obligation on the *callee's* productive parameters; the rule is enforced at the callee's body's analysis, frame-locally. The caller, observing only the call-boundary, sees exactly the consequences:

- Pre-call: caller's slot may be `init` or `uninit`. Caller may pass either.
- Post-call success: caller's slot is `init` (the callee's exactly-one write was committed via copy-restore).
- Post-call failure: caller's slot is bit-identical to its pre-call state (no copy-restore, no write).

The caller does not need to inspect the callee's body to reason about its own slot's state across the call. The signature's productive-`'`-marker plus the call's outcome (success vs. failure) suffices. The frame-locality of the rule (OPS §1.2; type-system §4.6) is preserved.

*Sources for §8: CP006 §§2.1–2.5 (whole-slot tracking; OQ-9 resolution; compute-then-commit pattern; rationale); CP009 §2 (write-once productive rule; edge cases); CP004 §3 (initialization analysis as the lattice the construction must satisfy); type-system §§4.1, 4.4, 6.1 (parameter-mode contracts and call-boundary mechanics); op-sem §§4.4, 4.5, 5.2 (`?:` chains, `^` rewind, copy-restore).*

---

## 9. Variant Construction

### 9.1 The Surfaces

Variant construction has three distinct surfaces, each fitting a different intended outcome: *[type-system §3.4; CP009 §3.3 for variant Aggregate-literal; CP015 §3 for variant `-<` extensions.]*

- **Constructing with a chosen candidate.** An Aggregate literal whose single entry names the candidate: `${Candidate <- value}` for a variant lhs.
- **Introducing the absent state.** A bare introduction with no initializer: `# x : SomeVariant`. The 3-word slot zero-fills (null candidate-pointer, null witness, zero tag) and is in the absent state from inception.
- **Clearing an existing variant to absent.** The `-<` discard form `v -< _` (full treatment at §11.3).

The first two are construction-site forms; the third is a runtime operation on an already-existing variant slot (§11). Together they cover the four operations a variant slot admits at the construction surface: introduce-absent, introduce-with-candidate, replace-with-candidate (a `<-` to an already-introduced slot), replace-with-absent.

### 9.2 The Aggregate-Literal Surface for Candidates

The candidate-selection surface is an Aggregate literal with exactly one entry whose left-of-`<-` names a declared candidate of the variant: *[CP009 §3.3.]*

```
.variant Shape :
    Circle : Float32                           ; radius
    Rectangle : Float32, Float32               ; width, height
    Triangle : Float32, Float32, Float32

.cmd makeCircle : Shape 'r, Float32 radius =
    'r <- ${Circle <- radius}
```

The lhs type (here `Shape`) disambiguates the literal's interpretation. A brace literal with one entry `Foo` initializes a variant if the lhs is a variant with candidate `Foo`; the same brace literal initializes a single-field record if the lhs is such a record. The user-side reading is consistent — the field-or-candidate name matches a declared element of the lhs type — and the typechecker resolves on the lhs type at the literal's position.

For multi-argument candidates (e.g., `Rectangle : Float32, Float32`), the candidate's value is itself an Aggregate or Sequence, depending on how the candidate's argument list is declared. The two natural patterns:

```
.cmd makeRectangle : Shape 'r, Float32 w, Float32 h =
    'r <- ${Rectangle <- ${width <- w, height <- h}}      ; nested Aggregate

.cmd makeTriangle : Shape 'r, Float32 a, Float32 b, Float32 c =
    'r <- ${Triangle <- $[a, b, c]}                       ; nested Sequence (positional)
```

The exact surface for variant candidates with multiple arguments depends on whether the candidate declaration is field-named (then nested Aggregate is natural) or position-only (then nested Sequence is natural). The variant declaration grammar's surface for the multi-arg case is forwarded to the implementation thread as part of OQ-23's residue (§14.4) and is not the central concern of this reference; the construction-side principle is uniform: the candidate's value is a value of the candidate's declared type, supplied by whatever surface form fits that type.

### 9.3 Introducing the Absent State

A bare introduction `# x : SomeVariant` is well-formed and produces an absent-state slot directly, without a Phase 2 commit and without an Aggregate literal: *[type-system §3.4; OPS §1.6.]*

```
# state : Shape                                ; absent
```

The bare-introduction form is admitted *only* for variants among the non-buffer types. Pointers, command-typed values, and objects all reject bare introduction (CP006 §3.2; type-system §6.1) — they have no zero-default representation that the language admits. Variants alone admit a zero-default, by virtue of the absent state being a structural property of the type (the 3-word slot's all-zero pattern *is* the absent state, by construction).

The user-side reading: a variant introduces in the absent state and gains a candidate by an explicit `<-` later in the body. This is the natural shape for variant slots used as accumulators in `?:` chains:

```
.cmd findFirstShape : Shape 'r, $[]Shape candidates =
    # match : Shape                            ; absent
    ?? ?:
        ‹search loop body filling match›
    'r <- match                                ; copy match into the productive receiver
```

The variant slot `match` is introduced absent, possibly assigned a candidate during the loop, and committed to the productive receiver after the loop. The bare-identifier `<-` from §6 is the appropriate copy form for variant-into-variant of the same nominal type.

*[Reconciliation: §6.2 forbids bare-identifier `<-` for variants. The contradiction is apparent only — §6.2's prohibition is on bare-identifier copy as a primitive byte-copy, which would create an aliasing the language cannot track. The pattern shown here is well-formed *as a value-move*: the `<-` from `match` to `'r` is the productive write that discharges `'r`'s obligation, and the underlying mechanism is the standard productive-write copy-restore (op-sem §5.2). The `match` local is not subsequently used; the move semantics are well-defined at the typechecker level. The stricter reading of §6.2 — bare-identifier `<-` as a *primitive* operation — does not apply at the productive-write site, where the standard call-boundary copy-restore mechanics apply instead. Where the rules' boundary is potentially confusing, the user resolves by writing the constructor call explicitly.]*

### 9.4 Pattern-Matching Idioms

Variant case analysis composes via `?:` chains plus the `-<` dynamic-narrowing operator (§11), with `?-` available for explicit absent-only tests. No new dedicated `match` keyword surface is introduced. *[Reconciliation: this is the 2026-05-03 dialog's resolution of the pattern-matching question (CP015 §6). The absence of a `match` keyword is an instance of OPS §3.5 / §1.8: existing language facilities (`?:` chains; `-<`; `?-`) compose to do the work; reaching for a new keyword would violate the orthogonality preference. The "enum-and-match" idiom referenced in type-system §§2.6, 3.4 is *shorthand* for the `?:` chain over enum discriminants — not a planned dedicated mechanism; both type-system sections receive a clarifying note in a future consolidated pass (work plan §6).]*

The canonical pattern for a variant `v` with candidates `A`, `B`, … and an absent-case to handle:

```
?- _ -< v                              ; absent test
    handle_absent
?: 'narrow_a -< v                      ; narrow to candidate A on success
    handle_a                           ; uses 'narrow_a as the A-candidate value
?: 'narrow_b -< v                      ; narrow to candidate B on success
    handle_b
handle_unmatched_candidate             ; default arm; reaches here if v has a candidate not narrowed above
```

The pieces:

- The first guard `_ -< v` (§11.4) tests whether `v` has any candidate. It succeeds iff `v` is non-absent; it fails iff `v` is absent. The `?-` block engages on guard failure — i.e., when `v` *is* absent — running its body to handle the absent case.
- Each `?:` block's guard `'narrow_x -< v` (§11.2) attempts to narrow `v` into the typed slot `narrow_x`. The narrow succeeds if `v`'s active candidate matches `narrow_x`'s declared type (or is at-or-below it under hierarchical-tag rules); on success the narrowed value is bound to `narrow_x` and the body runs with that binding in scope. On mismatch, the narrow fails, the `?:` body is skipped, and the chain advances.
- The trailing non-`?:` sibling is the chain's default arm, reached when no `?:` guard succeeded. In a complete pattern-match, this arm typically handles the "unexpected candidate" case — useful when the variant declaration is open for downstream extension or when defensive coverage is wanted.

Coverage discipline is the user's responsibility. The typechecker does not currently enforce exhaustive variant-case coverage at `?:` chains; a chain that omits a candidate falls through to the default arm with the variant in its current state. Future work may add coverage checking; the surface needs no extension to support it (the existing `?:` chain identifies the cases at face value).

For pattern-matching against an enum-style variant (each candidate carries no payload, distinct-tag-only), the pattern simplifies — the `'narrow -< v` form still applies, but the narrowed `narrow_a` slot is the candidate-tag-as-value, not a payload-bearing slot:

```
.variant Color :
    Red                                ; no payload
    Green
    Blue

.cmd describeColor : String 'desc, Color c =
    ?: 'r : Red -< c
        'desc <- (String: "warm")
    ?: 'g : Green -< c
        'desc <- (String: "neutral")
    ?: 'b : Blue -< c
        'desc <- (String: "cool")
    'desc <- (String: "unknown")       ; default arm; covers the absent case as well
```

This is the "enum-and-match" idiom in its standard form. The case-arms cover the named candidates; the default arm covers anything else — most commonly the absent case.

### 9.5 The Absent Case in Coverage

The absent case is *always* a path through the variant. Code that operates on a variant's candidate must either explicitly handle the absent case (as in §9.4's first canonical pattern, with the `?- _ -< v` block) or rely on the default arm of the `?:` chain catching it (as in the simplified enum example). *[OPS §1.6; type-system §3.4.]*

Failing to consider the absent case is a class of bug analogous to NULL-pointer dereferences in languages that admit NULL — a "missing case" that the language's structural rules surface but does not yet statically enforce. The pattern of the canonical chain (`?- _ -< v` first, `?:` candidate-narrowings next, default arm last) is the user-discipline shape that addresses every path.

The asymmetry with NULL-bearing types is structural: a NULL pointer in a NULL-admitting language *could be* anywhere a pointer can be, and the language admits no syntactic distinction. The absent state in Basis variants *is* admitted (every variant slot has it; the type system surfaces it), and the construction surface introduces the state via a distinct path (`# x : SomeVariant` for absent; `${Candidate <- value}` for non-absent). The code path that handles "a variant in the absent state" is structurally visible in the source.

### 9.6 Worked Examples

**Variant accumulator with conditional commit.**

```
.variant Result :
    Success : Float32
    Failure : String

.cmd attemptOperation : Result 'r, Float32 input =
    ?: ‹input passes validation›
        'r <- ${Success <- (computation: input)}
    ?: ; default arm: validation failed
        'r <- ${Failure <- (String: "validation rejected")}
```

Each `?:` arm writes `'r` exactly once with a different candidate; the productive write-once obligation (§8.1) is satisfied on every path.

**Optional via single-candidate variant.**

```
.variant Maybe :
    Some : Float32

.cmd lookup : Maybe 'r, $[]Float32 keys, Float32 target =
    # found : Maybe                            ; absent
    ?? ?:
        ‹iteration over keys›
        ?: keys[i] == target
            found <- ${Some <- target}
    'r <- found
```

A single-candidate variant is an optional (type-system §3.4 / OPS §1.6); the absent state encodes "not found." The accumulator pattern uses the bare introduction (`# found : Maybe`) for the initial absent state and the Aggregate-literal surface to commit a candidate when found.

**Pattern match with hierarchical narrowing.**

If the variant declaration carries a hierarchy on its candidate types, the `'narrow -< v` form supports at-or-below narrowing — a `'shape : RoundShape -< v` succeeds for any candidate at-or-below `RoundShape` in the candidates' hierarchy (§11.2 forward-references the at-or-below semantics). The pattern reads identically; the typechecker's narrowing rule does the hierarchical work.

*Sources for §9: type-system §3.4 (variant 3-word slot, absent state); CP009 §3.3 (variant Aggregate-literal surface); CP015 §3 (variant `-<` extensions); CP015 §6 (pattern-matching as composition); op-sem §§4.2, 4.4 (`?-`, `?:` chains); failure-system §2.2 (guard-only recovery markers); OPS §1.6 / §3.6 (variants are the unique may-be-absent type).*

---

## 10. Union → Candidate-or-Parent Byte-Reinterpretation

### 10.1 The Relation

A union value implicitly subsumes — by zero-cost byte reinterpretation — to any buffer-backed type `T` such that `T` appears on the standard subsumption chain of *at least one* declared candidate of the union. *[Reconciliation: this section replaces the "interpretive cast operator" framing of type-system §2.6 entirely. The framing of type-system §2.6 forwarded a runtime-cast surface to this reference under OQ-1's union half; the 2026-05-03 dialog (CP015 §5) reframed the relation as an implicit subsumption rule with no operator. The type-system reference's §§2.6, 3 will be revised in a future consolidated pass to fold in this reframe (work plan §6); this reference is canonical from inception.]*

The relation is *existential* in the candidate set: at least one candidate's parent chain reaches `T`, not every candidate's. A union with candidates `{A, B, C}` where `A`'s parent chain includes `T`, regardless of whether `B`'s or `C`'s chain reaches `T`, admits the subsumption to `T`.

The relation is *one-way*: a union value flows into a `T`-typed slot. The reverse direction — a `T`-typed value into a union slot — uses a constructor (parenthesized call) or an Aggregate literal (§3); the implicit subsumption does not run backward.

### 10.2 What Distinguishes This from Liskov Subsumption

Standard subsumption (the buffer-backed parent-to-child upcast of type-system §1.5 / §5.2) preserves Liskov substitutability: a value of the subtype can be substituted for a value of the supertype with semantics preserved. The runtime value's domain identity is preserved across the upcast (type-system §5.2); class dispatch on the value resolves to the subtype's instance even when the slot's static type is the supertype.

The union → candidate-or-parent relation is **not** a Liskov-preserving subsumption. The bytes are the same, but their *meaning* depends on which candidate is currently active in the union, which the language does not track. *[CP015 §5; OPS pending §1.5 update in CP015 §11.3.]*

The user-side reading: the language admits the byte-reinterpretation, but the semantic validity — that the bytes the union currently holds *are*, in fact, a valid `T`-value — is the user's responsibility. The discipline is the C-style discipline: a union is a tool for byte-aliasing across known structurally-compatible representations; the user manages the discriminator and the validity.

The two relations are kept terminologically distinct:

- **Subsumption** (without qualifier) — the Liskov-preserving upward relation; the standard buffer-backed parent-to-child upcast.
- **Byte-reinterpretation** (or **byte-reinterpretation subsumption** when the parallel needs to be drawn) — the union → candidate-or-parent relation introduced here.

The terminology choice is per the 2026-05-03 dialog's standard-terminology guideline (CP015 §11.2): the divergence between the standard meaning of "subsumption" and what the union → candidate-or-parent relation actually does was surfaced as a distinct concept rather than absorbed silently. The user's standing instruction: "Always use standard terminology and suggest corrections like this where likely."

### 10.3 The Surface — No New Operator

The relation is fired implicitly at any position where a `T`-typed slot expects a value, when the rhs is a union value whose existential subsumption to `T` holds: *[CP015 §5.5.]*

```
.union RGBA8 :
    AsBytes  : [4]
    AsPacked : Int32                           ; common interpretation
    AsRgba   : ${UInt8 r, UInt8 g, UInt8 b, UInt8 a}

.cmd asInt32 : Int32 'out, RGBA8 c =
    'out <- c                                  ; implicit byte-reinterpretation to Int32
```

The `<-` here is the bare-identifier form (§6) writing a union into an `Int32`-typed productive slot. The typechecker checks that `Int32` is on the subsumption chain of at least one declared candidate (here, the `AsPacked` candidate); finding it, the operation is admitted; the runtime is a four-byte byte-copy from the union's storage to `'out`'s storage. No new surface form is introduced — neither a cast operator, a `reinterpret`-style intrinsic, nor a special-purpose method. *[CP015 §5.4; OPS §1.8 / §3.5.]*

The same surface admits the operation through pointer indirection:

```
.cmd interpretAsPacked : ^Int32 'p, ^RGBA8 c =
    'p <- (c&)                                 ; pointer rebind; not a copy
```

Here the `<-` is a pointer-value rebind (§6.1's pointer case). The typechecker checks that `^Int32` and `^RGBA8` admit the byte-reinterpretation under the same existential rule applied at the pointee types; the rebind is well-typed.

### 10.4 The Typechecker's Check

For a candidate `<- ` operation whose rhs is union-typed and whose lhs type is `T`:

1. The typechecker enumerates the union's declared candidates `{C_1, …, C_n}`.
2. For each candidate `C_i`, the typechecker walks `C_i`'s buffer-backed subsumption chain (`C_i`, `C_i`'s parent, that parent's parent, …, up to `[N]` and `[]`).
3. If `T` appears on at least one such chain, the operation is admitted; otherwise the operation is rejected with a "no candidate-chain reaches `T`" error.

The check is per-candidate-chain, not per-candidate-byte-content; the language does not inspect the active candidate's bytes nor verify that they are a valid `T`-value at the moment of the operation. The user is responsible for ensuring the bytes are valid for the target interpretation — typically by tracking which candidate is active alongside the union, in a containing record's discriminator field or by the program's surrounding logic.

The check is symmetric across the existential: if the user wants to enforce a *universal* subsumption (every candidate's chain reaches `T`), the user does so by declaring the union over candidates that all share `T` in their chains. The language admits the existential because it is the more permissive rule and matches the C-style overlay semantics unions are designed for; tightening to universal would forbid common patterns (e.g., a union of a record and a buffer where the buffer interpretation is the byte-aliasing escape hatch).

### 10.5 What This Replaces

Type-system §2.6 (the union case of OQ-1) noted "interpretive casting" as the operational consequence of the untagged byte-overlay choice and forwarded the operator-surface treatment to this reference. The 2026-05-03 dialog (CP015 §5.7) replaced the "interpretive cast operator" framing entirely; no operator is introduced. The type-side commitment recorded in type-system §2.6 — that the typechecker checks `T` is on at-least-one declared candidate's chain — is preserved and sharpened: the rule is now precise where §2.6 was qualitative. *[CP015 §5.7.]*

The orthogonality preserved: the language admits the byte-reinterpretation at the standard `<-` surface; no new operator surface, no new keyword, no new intrinsic. The user resolves cases the language does not admit — siblings, non-buffer-backed candidate chains, narrowing back to the union from a candidate-typed value — by writing explicit conversion constructors or by routing through a discriminator-aware idiom in the surrounding code.

### 10.6 Distinguishing from Variant Construction

The reader may notice a structural parallel: variants admit narrowing to a candidate-typed slot via the `-<` operator (§11), and unions admit byte-reinterpretation to a candidate-or-parent type implicitly. The two surfaces are deliberately different: *[CP015 §5.7; type-system §3.5.]*

- **Variants** carry a tag in the slot (the 3-word triple — type-system §3.4); the language tracks which candidate is active. Narrowing is *language-checked at runtime* (`-<` produces a propagating failure on tag-mismatch).
- **Unions** carry no tag in the slot (type-system §2.6); the language does not track which candidate is active. Reinterpretation is *user-asserted, language-infallible* — the language admits the byte-reinterpretation and trusts the user.

The two discrimination models match the two intended uses: variants for safe tagged sums (the "Optional", the "Either", the "AST node" cases); unions for byte-overlay efficiency at the cost of programmer-managed discrimination (the "RGBA pixel as bytes or as Int32", the "network packet header" cases). The construction-and-initialization surfaces reflect the difference.

*Sources for §10: CP015 §§5, 11.3 (the byte-reinterpretation reframe; pending OPS §1.5 update on the relation's distinct-from-Liskov character); type-system §§2.6, 5.2 (the prior framing this section replaces; the standard upward-subsumption relation this section contrasts with); OPS §§1.5, 1.8, §3.5 (orthogonality, special-case-intrinsic drift); CP001 §2.5 (the original union-as-byte-overlay characterization).*

---

## 11. The `-<` Dynamic-Narrowing Operator

### 11.1 Role and Surface

The `-<` operator performs a runtime type check and, on success, binds the narrowed value (or no value, in the discard cases) to its lhs slot. On mismatch, it produces a propagating failure — the same surface-form as the rest of the failure system. *[type-system §3.5; CP015 §3.]*

The operator is parallel to `<-` syntactically and contrasts on the static-vs-dynamic axis: `<-` writes a value the typechecker has confirmed at compile time; `-<` writes a value the typechecker has confirmed at compile time *might* fit, with a runtime check making the determination. Both are infix operators on slots; `-<` is `<-`'s may-fail counterpart for narrowing.

The operator's surface forms and their meanings: *[CP015 §3.]*

| Form | Domain | Meaning | Failure shape |
|---|---|---|---|
| `'narrow -< v` | variant `v` | If `v`'s active candidate matches `narrow`'s declared type (or is at-or-below it under hierarchical-tag rules), bind the candidate value to `narrow`. | Tag-mismatch failure on candidate type-mismatch; tag-mismatch failure on `v` absent. |
| `'narrow -< obj` | object `obj` (or `^Object`) | If `obj`'s runtime type is at-or-below `narrow`'s declared type, bind the narrowed reference. | Tag-mismatch failure on type-mismatch. |
| `'narrow -< p` | pointer `^T` to non-buffer admitting class-hierarchy narrowing | Same as object case applied to pointee type. | Tag-mismatch failure on type-mismatch. |
| `v -< _` | variant `v` | Set `v` to the absent state. | Always succeeds (no failure path). |
| `_ -< v` | variant `v` | Test whether `v` has any candidate. Succeeds iff `v` is non-absent; fails iff `v` is absent. | Tag-mismatch failure on `v` absent. |

The first three forms are the existing surface declared in type-system §3.5; the latter two are the 2026-05-03 dialog's amendment (CP015 §3). All five forms are may-fail-shaped commands; the always-succeeds `v -< _` is a may-fail-shaped command whose failure path is statically dead, paralleling `:`-marked subsumption under the failure-mark partial order (failure-system §1.2 / type-system §4.5). *[Reconciliation: classifying `v -< _` as may-fail-shaped-with-dead-failure-path keeps the operator's signature uniform across all five uses; alternative — declaring `v -< _` as `:`-marked specifically — would require the typechecker to read the operator's failure mode from the lhs/rhs shape, which is more brittle than uniform may-fail.]*

### 11.2 Variant-to-Candidate Narrowing: `'narrow -< v`

The form `'narrow -< v` checks whether the variant `v`'s active candidate matches the declared type of the productive `narrow` slot. On match, the candidate value moves into `narrow`; on mismatch, a propagating failure with the standard tag-mismatch tag fires. *[type-system §3.5.]*

The "at-or-below" semantics applies when the variant's candidates carry hierarchical tags (a candidate's tag is `RoundShape` and the user narrows to `RoundShape`'s ancestor `Shape` — succeeds; the user narrows to `RoundShape`'s descendant `Circle` — succeeds iff the candidate's tag is at-or-below `Circle`). For flat-tag variants (no hierarchy on candidates), the rule simplifies to exact-tag equality. The user-side surface and failure shape are uniform across both cases.

Use in a guard position:

```
?: 'shape : Circle -< drawable
    ‹handle Circle case using shape›
```

The `?:` block's guard is the `-<` operation; on success, `shape` is bound and the body runs; on failure (mismatch or absent), the `?:` block is skipped and the chain advances. *[op-sem §4.4; CP015 §6.]*

The narrowed slot `narrow` is productive (`'`-marked) — the operator's productive-side commits a value on success, paralleling the `<-` form's productive write. The same write-once obligation applies if `narrow` is the constructor's productive receiver (which would be unusual; `narrow` is most commonly a `#`-introduced local).

### 11.3 Variant Discard: `v -< _`

The form `v -< _` clears the variant `v` to its absent state. The operation always succeeds; `v` is in the absent state after it. *[CP015 §3.]*

```
# state : Shape <- ${Circle <- 1.0}            ; non-absent
state -< _                                     ; clear to absent
```

The variant's candidate-pointer is set to null (the absent-state representation); the candidate's storage is released per the standard at-stack-handler discipline if the candidate type carried at-stack methods (op-sem §6.4 / failure-system §5.8). The handler-fire timing is the holding-frame retirement that the variant's discard implies — typically immediate, since the candidate's home is the variant slot's frame.

The form is the explicit "this variant is now empty" surface, distinct from a fresh introduction (`# x : Variant`) in that it operates on an existing slot. The two are semantically equivalent for the slot's post-state but operationally distinct: the `<-` form is a fresh introduction that the analysis sees as a new slot; `v -< _` is a write to an existing slot that the analysis sees as a state transition `init → init` (the slot remains `init` because its absent state is a valid state; only the candidate-content has changed).

### 11.4 Variant Non-Absence Test: `_ -< v`

The form `_ -< v` tests whether `v` has any candidate. It succeeds iff `v` is non-absent; it fails iff `v` is absent. The operation has no lhs slot to bind — `_` is the discard marker on the lhs, signaling no value is captured. *[CP015 §3.]*

The form is may-fail-shaped, not Boolean-valued: Basis has neither Boolean values nor a ternary operator (OPS §3.8 / pending §3.9). The user-facing pattern is as a `?:`-block guard or a `?-` block for the absent-only test:

```
?: _ -< drawable                               ; "drawable has any candidate"
    ‹operate on drawable in some candidate›

?- _ -< drawable                               ; "drawable is absent"
    ‹handle absent case›
```

The first form's body runs when `drawable` has a candidate; the second's body runs when `drawable` is absent. The two compose with the candidate-narrowing forms of §11.2 in the canonical pattern of §9.4, with the `?-`-absent-test as the first guard and the `?:`-narrow-attempts as the chain.

### 11.5 The Operator Does Not Apply to Unions

The `-<` operator does *not* apply to union slots. Union narrowing is the byte-reinterpretation subsumption of §10, which is implicit and language-infallible — the language admits the byte-reinterpretation and trusts the user's discrimination. The `-<` form's runtime check has nothing to check for unions: there is no language-tracked tag to compare against the target type. *[type-system §3.5; CP015 §5.7.]*

A `-<` operation applied to a union lhs or rhs is a type error reported at the operator. The user resolves by using the implicit byte-reinterpretation (`<-` from a union to a candidate-or-parent type, §10) or by routing through an explicit discriminator field (typically in a containing record).

### 11.6 Failure Integration

The `-<` operator's failure-set is "tag-mismatch" — a single failure tag the language emits for `-<` mismatches (and for variant absent-cases on the `'narrow -< v` and `_ -< v` forms). The tag integrates with typed failures (failure-system §4) per the standard machinery: a `?: 'narrow -< v` chain implicitly emits the tag-mismatch tag on the failure path; a `|: TagMismatch` recovery elsewhere catches it. The exact tag identity (whether it is a single language-level tag, a per-narrow-target hierarchy, or a payload-bearing tag carrying the runtime-type-vs-target-type pair) is forwarded to standard-library design and is not pinned here. *[type-system §3.5; failure-system §4.]*

The handler-side surface is the standard recovery surface; the construction-and-narrowing layer introduces no new failure-handling primitives. A `?-`-block paired with `-<` is the natural "narrowing failed; do recovery" idiom; a `|: TagMismatch t` block elsewhere is the cross-cutting recovery for any narrowing failure in a region.

### 11.7 Worked Examples

**Pattern matching with absent-case handling.**

```
.cmd describeShape : String 'desc, Shape s =
    ?- _ -< s
        'desc <- (String: "no shape")
    ?: 'c : Circle -< s
        'desc <- (String: "circle")
    ?: 'r : Rectangle -< s
        'desc <- (String: "rectangle")
    'desc <- (String: "other shape")           ; default arm
```

Every path writes `'desc` exactly once (§8.1).

**Object class-hierarchy narrowing in a guard.**

```
.cmd handleEvent : ^Event e =
    ?: 'mouse : ^MouseEvent -< e
        ‹mouse-specific handling, using mouse›
    ?: 'keyboard : ^KeyboardEvent -< e
        ‹keyboard-specific handling›
    ‹fallback: log unknown event›
```

The `^Event` parameter is narrowed to the more-specific subclasses; on success, the narrowed pointer binding is in scope for that arm.

**Resetting a variant slot.**

```
.cmd reset : Shape & current =
    current -< _
```

The `&`-mode parameter `current` is reset to absent; the call site sees the variant as absent after the call.

*Sources for §11: type-system §3.5 (`-<` operator declaration; object/pointer cases); CP015 §3 (variant `_`-marked extensions); failure-system §4 (typed-failure integration); op-sem §§4.2, 4.4 (`?-`, `?:` chains in which `-<` typically appears as guard); CP015 §6 (pattern-matching as composition).*

---

## 12. The `=` Defaults Declaration Form

### 12.1 Role

A buffer-backed type whose bytes do not have a meaningful zero (per CP006 §3.1 / type-system §6.1) declares its default value via the definitional `=` form. The form supplies the value used when the type's bare introduction (`# x : T`) would otherwise be rejected. *[CP009 §5.1; CP006 §3.]*

```
.domain Positive : Int32 = (Int32: 1)
```

A `Positive` slot introduced as `# p : Positive` (no initializer) acquires the declared default value at introduction; the slot is `init` from inception.

### 12.2 Two Operators, One RHS Grammar

The split between `=` (definitional) and `<-` (runtime placement) is exact: *[CP009 §5.2.]*

- `=` appears in declaration positions (`.cmd ... =`, `.domain T = ...`, `.record T : ... = ...`).
- `<-` appears in expression-position placement (`# x <- ...`, `'r <- ...`, etc.).

Both consume the same five-shape rhs grammar (§§3–6). A reader sees `=` and knows they are looking at a declaration; sees `<-` and knows they are looking at a runtime operation. The unification of the rhs grammar means the user-side surface forms are uniform — Aggregate, Sequence, parenthesized call, bare identifier, bare literal — across both positions.

A record default uses the Aggregate form:

```
.record Point :
    Float32 x
    Float32 y
    = ${x <- (Float32: 0), y <- (Float32: 0)}
```

A typed-buffer default uses the Sequence form:

```
.domain InitialValues : $[3]Int32 = $[(Int32: 1), (Int32: 2), (Int32: 3)]
```

### 12.3 The No-Defaults Discipline Preserved

The `=` form is not an exception to the no-defaults discipline (type-system §6.1) but the user-facing mechanism for declaring a type's invariant-respecting default. The discipline is:

- **Buffer-backed types whose bytes have a meaningful zero** acquire a zero-default by composition (CP006 §3.1; type-system §6.1). No `=` declaration is needed; `# x : T` produces the zero-filled `init` slot.
- **Buffer-backed types whose invariants forbid zero** declare an explicit default via `=`. Without the declaration, `# x : T` is rejected; only `# x : T <- ‹rhs›` works.
- **Non-buffer types** (pointers, command-typed values, objects, variants) cannot have defaults via `=`. Their bare introduction is rejected at the type level *[CP006 §3.2; type-system §6.1]*; the only valid construction is via `<-` at every use site. Variants are the structural exception within the non-buffer category — their absent state is the inherent representation of an "empty" slot, and bare `# x : Variant` produces an absent slot directly without any `=` mechanism (§9.3).

The `=` form is therefore narrow in scope: it is for buffer-backed types whose author wants `# x : T` to produce a non-zero value on bare introduction. Pointers, command-typed values, and objects route through `<-` at every site; their `=` declaration would have no meaning the language admits.

### 12.4 The `=` RHS and Failure

The rhs of `=` is evaluated at *declaration time*, not at every use site. Implementations may treat this as a compile-time constant fold or as a per-use-site lazy initialization (the latter when the rhs has runtime-determined components). The semantic guarantee is that a `# x : T` introduction acquires the declared-default value, irrespective of when the rhs is realized.

A `?`-marked sub-expression in a `=` rhs is admitted but unusual: the declaration becomes may-fail in a sense, but a default that may fail at compile-time-constant-fold-or-equivalent is a degenerate case. The implementation latitude is to either admit the failure (the type's default introduction may fail at module load) or reject the declaration at typechecking. The language commits to the semantic — failure of the default's evaluation is a hard module-load failure — without pinning the implementation strategy.

### 12.5 Constructor-as-Default Integration

A common pattern is declaring a constructor and using it as the type's default:

```
.cmd Float32 : Float32 'r =
    'r <- (Float32_fromBytes: zeros)           ; or however Float32 zero is built

.record AnnotatedFloat :
    Float32 value
    String  label
    = ${value <- (Float32: 0), label <- (String: "")}
```

The `Float32: 0` invocation in the `=` rhs is the same parenthesized-call surface used at runtime; the typechecker resolves the constructor identically. If `Float32` is `.implicit Float32 'r : Decimal d`, the bare-literal form `0` would also be admitted (`= ${value <- 0, label <- ""}`); the choice between explicit and implicit forms is the standard `.implicit` convenience choice (§5.5).

*Sources for §12: CP009 §§5.1, 5.2 (the `=` form, shared rhs grammar with `<-`); CP006 §3 (default-initialization rules under the buffer-backed principle); type-system §6 (no-defaults discipline at the type level).*

---

## 13. Construction-Side Context Parameters

This section resolves OQ-13 on the construction side, fully and without forwarding to the class-system reference. *[CP015 §8.]* Implicit context parameters are a parameter-passing matter, not a class-system matter; the class system and the implicit-context-resolution mechanism are conceptually parallel but operationally distinct, and the construction-side mechanics belong here. *[Reconciliation: this section is encoded from inception per CP015 §8.3; the op-sem reference's §5.5 currently forwards OQ-13 jointly to construction and class-system, and will be revised in a future consolidated pass to redirect entirely here.]*

### 13.1 The Mechanism Recap

Context parameters are declared with the `/` separator in command signatures. The portion of the signature before `/` is the regular-parameter list; the portion after `/` is the implicit-context-parameter list. *[CP001 §3.5; op-sem §5.5.]*

```
.cmd doSomething : Result 'r, Foo a, Bar b / Logger logger, Config cfg =
    ; logger and cfg are implicit context parameters
```

At a call site, regular parameters are supplied positionally (or in the `name <- value` form if the call site supports it); context parameters are *automatically supplied* by the typechecker from the caller's lexical scope, by *type-based uniqueness*. For each context parameter, the typechecker scans the caller's lexical scope for in-scope values whose type matches the parameter's type:

- **Exactly one in-scope match** — the typechecker inserts that value as the parameter; no surface mention is required at the call site.
- **Multiple matches (ambiguity)** — compile-time error. The user resolves by passing the value explicitly via `/` at the call site.
- **No match (absence)** — compile-time error. The user resolves by passing the value explicitly, or by introducing a value of the right type into scope.

The explicit-pass form is universally available and serves as the disambiguation fallback:

```
(doSomething: foo, bar / specificLogger, specificCfg)
```

*[CP001 §3.5; op-sem §5.5.]*

This preserves the no-non-local-state principle: implicit resolution is *plumbing*, not *reaching*. The value must actually exist in the caller's lexical scope to be eligible; the parameter list remains a complete inventory of the command's dependencies at the *signature* level. The implicit mechanism reduces the *syntactic noise at call sites* without weakening the semantic guarantee. *[CP007 §1.2; op-sem §5.5.]*

### 13.2 Init State at the Resolution Site

The auto-supplied value must be `init` at the resolution site. The standard parameter-passing rule applies; there is no special case for implicit resolution. *[CP015 §8.4; CP004 §3.]*

This means: a context parameter cannot be supplied from a lexical-scope value whose initialization state at the call site is `uninit` or `failing(?)`-pending. The typechecker's flow analysis tracks the value's state through the caller's body; the implicit-resolution lookup considers a candidate eligible only when its state is `init` at the program point of the call. *[CP004 §3.4; op-sem §5.2.]*

The rule is what one would expect from "the caller passes the value as if explicitly," with no surprises: implicit resolution is just the typechecker writing the explicit-pass form for the user.

### 13.3 Mode and Taint

The receiving parameter's declared mode (READ / PRODUCE / REFERENCE per CP013 / type-system §4) determines passing. The supplied value's access-path taint propagates per the mode, exactly as for explicitly-passed parameters. *[CP015 §8.4; type-system §§4.1, 4.6.]*

- **READ context parameter.** The supplied value is read-only at the receiving frame; the caller's view of it is unchanged after the call (transitive READ contract per type-system §4.6).
- **PRODUCE context parameter.** The supplied value is a productive slot; on success of the call, the value is written exactly once. Rare in practice — back-channel mutation via implicit resolution is unusual — but not prohibited.
- **REFERENCE context parameter.** The supplied value may be read and may be written; copy-restore semantics apply per type-system §4.4.

### 13.4 Mode Permissibility

All three modes (READ / PRODUCE / REFERENCE) are permitted on implicit context parameters. The rule is "allow unless incoherent" — and PRODUCE on a context parameter is not incoherent, merely unusual. *[CP015 §8.4.]*

The typical use case is READ — a logger, a configuration record, an environment-style read-only piece of context that callees inspect. PRODUCE and REFERENCE on context parameters have niche uses (e.g., a "logging buffer" that a deeply-nested call writes to), permitted to keep the rule uniform with regular parameters.

### 13.5 `.implicit` Exclusion (Cross-Reference)

`.implicit` constructors do not admit `.context` parameters. The grammar prohibits the declaration; the semantic analyzer flags any attempt as an error. The rule is recorded in §5.3 and is recapitulated here for completeness in the context-parameter material. *[CP015 §8.4.]*

The rationale: `.implicit` is a single-implicit elision mechanism (literal-to-type ergonomics). Allowing `.context` on top would compound implicit invocation with implicit parameter resolution — exactly the "Scala-implicit chaos" the three restrictions of §5.2 exist to prevent.

### 13.6 Defaults and Context

A `=` default declaration (§12) is resolved at *declaration scope*, where the caller's lexical context is unavailable — there is no caller at the site where the default is written. Therefore, a default's rhs cannot use implicit-resolution for context parameters. Any context parameter required by a constructor invoked in a default rhs must be supplied **explicitly** via `/`. *[CP015 §8.4.]*

```
.cmd makeWidget : Widget 'r, Int32 size / Logger logger =
    (logger :: log: "constructing widget")
    'r <- ${size <- size}

.record DefaultedWidget :
    Widget primary
    Widget fallback
    = ${
        primary  <- (makeWidget: 100 / nullLogger),     ; explicit / required
        fallback <- (makeWidget: 50  / nullLogger)
    }
```

The `nullLogger` referenced in the `=` rhs must be in scope at the declaration site. The explicit-`/` form is universally valid as a fallback (just as it is the fallback when implicit resolution would have multiple candidates or no candidate). This situation should be uncommon in practice but is always available. *[CP015 §8.4.]*

### 13.7 Lambda-Capture Interaction Forwarded

OQ-21 — the interaction of context parameters with lambda capture lists — remains forwarded to the lambda-and-fexpr reference. The construction surface here is unaffected by OQ-21's resolution; lambda-capture mechanics concern how the lambda's invocation frame inherits or excludes context bindings, which is lambda-construction territory rather than the construction surface this reference covers. *[CP015 §8.4.]*

### 13.8 Class-System Uninvolvement

The class-system reference (#6, future) has no involvement with OQ-13. Implicit context parameters are parameter-passing machinery (selecting a value from lexical scope by type uniqueness), distinct from class-instance dispatch (selecting a method from a typeclass dictionary by receiver type). The two mechanisms have analogous "implicitness" but operate in different layers; the construction reference owns the context-parameter mechanics outright. *[CP015 §§8.3, 8.5.]*

The op-sem reference's §5.5, which currently forwards OQ-13 to "the class-system reference," will be updated in a future consolidated pass (work plan §6) to redirect to this section.

*Sources for §13: CP001 §3.5 (the `/` separator and implicit-resolution rule); op-sem §5.5 (operational mechanics of implicit resolution); CP015 §8 (Position 1 decision; full mechanics; `.implicit` exclusion; defaults handling); CP013 / type-system §4 (mode rules per CP013 carried through implicit resolution unchanged); CP007 §1.2 (provision-chain framing).*

---

## 14. Open Questions

This section catalogs the construction-and-initialization-related open questions. Resolutions covered by this reference are noted at the section where they are settled (with cross-references here for completeness); forwarded sub-questions are recorded; genuinely open items registered by this reference are described in full.

### 14.1 OQ-22 — Resolved Here (§5.4)

OQ-22 was open and forwarded to this reference: how should `.implicit` constructor parameter types denote Aggregate-shape and Sequence-shape literal sources? The resolution: type-form mirrors of value forms. *[CP015 §7; §5.4 above.]*

```
.implicit Point 'r : ${Decimal x, Decimal y} a = ...
.implicit Vec3  'r : $[3]Decimal v = ...
```

The Aggregate type form `${Type field, ...}` parallels the value form, swapping `<-` for type-first declaration. The Sequence type form `$[N]T` reuses the typed-buffer length-bracket surface with the `$`-prefix. The forms are generally usable in any structural-type position, but their primary motivating use is `.implicit` constructor parameter slots.

### 14.2 OQ-24 — Resolved Here (§5.5)

OQ-24 asked whether the language commits to full Aggregate-shape and Sequence-shape `.implicit` support from inception, or whether initial implementations may defer structural-shape support. The resolution: CP009 §4.3's design is committed; the phasing is **implementation latitude**. *[CP015 §9; §5.5 above.]*

The reference does not legislate phasing. An initial implementation may support only simple literal types (`Decimal`, `Hex`, `String`, `Char`) and add Aggregate/Sequence support when structural-matching machinery is ready, without that constituting a language-spec change. Once added, the support must behave as specified.

### 14.3 OQ-13 — Resolved Here (§13)

OQ-13 was open and forwarded jointly to this reference and the future class-system reference. The resolution: construction-side fully resolved here; class-system uninvolved. *[CP015 §8; §13 above.]*

The construction reference encodes the full mechanics: parameter-passing semantics by type-based uniqueness from the caller's lexical scope; init-state requirement at the resolution site; mode permissibility (all three of READ/PRODUCE/REFERENCE); `.implicit` exclusion as a hard restriction; defaults handling via explicit-`/` fallback. OQ-21 (lambda-capture interaction with context) remains forwarded to the lambda-and-fexpr reference.

### 14.4 OQ-23 — Lexer-Disambiguation Half Closed; `.inline`-Placement Remainder Forwarded

OQ-23 was registered in CP009 §8.6 to track two lexer-disambiguation matters: (a) the original `{-`/`-}` and `[-`/`-]` literal-fence tokens (the disambiguation problem with `{` followed by `-` or `-` followed by `}`), and (b) the surface placement of the `.inline` modifier on record fields (carved into the type-system reference §2.4 / §7.12).

**Half (a) is closed by this reference.** The 2026-05-03 literal-token surface revision (CP015 §2; encoded throughout this reference at §§3, 4) replaces the original tokens with `${...}` and `$[...]`. The `$`-prefix is unambiguous (the `$` character has no other use in Basis source materials), eliminating the lexer-disambiguation problem the original tokens introduced. The grammar simplifies accordingly.

**Half (b) remains open and forwarded to the implementation thread.** The `.inline` modifier's surface placement on record fields is a small lexer-disambiguation matter that does not bear on the construction-and-initialization story; resolution is part of the broader implementation-thread agenda. *[Type-system §7.12; CP015 §2.3.]*

The work plan's §5.3 retains OQ-23 in the forwarded-to-implementation-thread state for the `.inline`-placement question and any future small lexer-disambiguation matters.

### 14.5 OQ-30 — Heap Allocation Language-Level Mechanism (Newly Registered)

**Statement.** The `#` prefix operator allocates a slot in the calling frame, suitable for small local values. Larger values would benefit from heap-based storage, but no language-level mechanism for heap allocation exists yet. *[CP015 §10.1.]*

**Tension.** The user explicitly wants to keep the language and the standard library orthogonal — to avoid Java's flaw of welding the language spec to its library spec. A general allocation mechanism that doesn't require some level of library intrinsics (Zig-style allocators, for example) is not yet apparent. *[CP015 §10.1.]*

**Candidate directions sketched in dialog.**

- **Region-style allocation tied to lexical scope.** Operating principles §1.10 already flags this as a latent constraint of the design; a region/scope-bound allocator could fit naturally. The frame-ownership lens combined with the buffer-backed containment rule already preserves the option.
- **Allocation-via-class-dispatch.** Push allocator selection to the class system: an allocator type with a single `allocate` method, dispatched on the type of the allocator parameter; the standard library provides allocators but the language semantics depend on no specific library type. This avoids library intrinsics by making allocation a method dispatch on a user-provided type.
- **Small-primitive-set.** One or two intrinsics (e.g., `.intrinsic alloc`, `.intrinsic free`) that the standard library wraps but doesn't depend on for *language* semantics. The language would have a minimal heap-allocation primitive; everything ergonomic is library-level.

**Status.** Resolution requires a separate design dialog. This reference notes the gap with a "see OQ-30" cross-reference where heap-allocation would be relevant; no current section of this reference depends on the resolution.

### 14.6 OQ-31 — Tuple-Style Positional Access via `Bar::N` (Newly Registered)

**Statement.** Aggregate values are positionally ordered by their declared field order (per CP006 layout discipline; recorded in this reference's positional/named rule at §3.3). The proposal: extend `::` (the scope operator, op-sem §5.6 / type-system §3.5 / operating-principles §1.7) to admit an integer literal as its rhs, so `Bar::1` denotes the first positional field of a `Bar`-typed aggregate, `Bar::2` the second, and so on. The relaxation would make any aggregate accessible as a tuple without giving up the safety the typed-LHS-fixes-order rule provides. *[CP015 §10.2.]*

**Open coherence questions.**

- **Lexer / parser.** The rhs of `::` would need to admit numeric literals; today it admits identifiers. The grammar production for `::` widens.
- **Subsumption interaction.** Records subsume to `[N]` (their buffer-backed parent), where positional access is array indexing, not `::`. So `record::1` and `(record as [N])[0]` would be different surface forms despite touching the same byte. Acceptable, but worth recording explicitly.
- **Range checking.** `Bar::N` where N exceeds Bar's field count is a compile-time error (the typechecker knows Bar's declared field count).
- **Scope of relaxation.** Does the access form apply to all aggregate-typed values (every record, every variant with positional candidates), or only to types explicitly marked as tuple-shaped? Records already have positional layout per CP006; restricting `::N` access to a tuple-marked subset would be artificial.
- **Interaction with `.inline` records.** A flattened-into-parent inline record's positional indices either continue the parent's numbering, or remain local to the inline. The choice affects `Bar::N` semantics for records with inline children.

**Status.** Resolution deferred. This reference records the proposal here; resolution will require a separate design dialog (or fold-in to a future related reference). *[CP015 §10.2.]*

### 14.7 Forwarded to Other References

The following items, mentioned in this reference but owned elsewhere, are recorded here to make the boundary explicit:

| Item | Owner | Status |
|---|---|---|
| OQ-21 (lambda capture vs. context) | Lambda-and-fexpr reference | Forwarded |
| OQ-25 (capture-shadowing) | Lambda-and-fexpr reference | Forwarded |
| Class-system mechanics for instance dispatch | Class-system reference | Forwarded; conceptually parallel to but distinct from §13's context-parameter mechanism |
| Block-quote `:{...}` and command-literal `:<...>{...}` parameter-passing details | Lambda-and-fexpr reference | Forwarded; their construction surface is uniform with the rules in this reference, but mechanism specifics belong with their owning reference |
| Heap allocation patterns (when OQ-30 resolves) | A future heap-allocation thread | Deferred |

*Sources for §14: CP015 §§7, 8, 9, 10 (resolutions and new OQ registrations); §§5.4, 5.5, 13 above (resolved-here references); work plan §5 (OQ registry).*

---

## 15. Provenance

**Authored:** Distilled by Claude (Opus 4.7) from the Basis intent-dialog corpus, on 2026-05-03, in continuation of the topic-organized consolidation begun with `reference-failure-system.md` (2026-04-29), continued with `reference-operational-semantics.md` (2026-04-29) and `reference-type-system-and-modes.md` (2026-05-02). Drafted in a fresh single-session pass per the 2026-05-03 scope-confirmation dialog (CP015), encoding the eight amendments and two new OQs from that dialog from inception.

**Source materials:** intent-checkpoint-001.md (foundational construction mechanics, expression-position calling convention, `<-` operator's original role, the `/` separator for context parameters, command-typed values as first-class); intent-checkpoint-002.md (block-marker constructs and at-stack registration migration, in service of §3.7's embedded-object handling); intent-checkpoint-004.md (parameter-mode mechanics, initialization analysis, the lattice that construction satisfies, OQ-13's original framing); intent-checkpoint-005.md (constructor signature shape carved in by R1+R2; receiver mode rules); intent-checkpoint-006.md (the buffer-backed principle made structural; OQ-9 resolution to whole-slot tracking; the compute-then-commit pattern; default-initialization rules); intent-checkpoint-007.md (provision-chain rule constraining well-formed construction patterns; lambda capture mechanics); intent-checkpoint-009.md (the densest source for this reference: the polymorphic `<-` rhs grammar, Aggregate and Sequence literals, the `.implicit` mechanism, the `=` defaults form, OQ-10 resolution, the write-once productive sharpening, OQ-22 placeholder, OQ-23 origin, OQ-24 origin); intent-checkpoint-010.md (fexpr design — consulted for cross-reference completeness; not directly drawn upon for construction surface); intent-checkpoint-012.md (typed-failure design — consulted for failure-system cross-reference completeness); intent-checkpoint-014.md (`-> name` interpretation across modes; format conventions); intent-checkpoint-015.md (the 2026-05-03 scope-confirmation dialog: literal-token surface revision; `-<` operator's variant-surface extensions; Aggregate literal positional-form rule; variant-absent `_` syntax; union → candidate-or-parent byte-reinterpretation reframe; pattern-matching as composition; OQ-13 / OQ-22 / OQ-24 resolutions; new OQ-30 / OQ-31 registrations).

Secondary references consulted for cross-cutting context: `reference-failure-system.md` (failure-handling primitives in service of the pattern-matching idiom verification at §9; copy-restore framing at §7); `reference-operational-semantics.md` (parameter-passing mechanics at §13; frame-exit lifecycle at §3.7's at-stack migration); `reference-type-system-and-modes.md` (variant absent state at §§9, 11; buffer-backed hierarchy at §6; READ/PRODUCE/REFERENCE modes at §13; `-<` operator's type-side declaration at §11; union OQ-1 resolution as the foundation for §10's byte-reinterpretation).

**Resolutions recorded in this reference:**

- **OQ-22** resolved at §5.4 — `.implicit` constructor parameter types use type-form mirrors of value forms (`${Type field, ...}` for Aggregate-shape, `$[N]T` for Sequence-shape). *[CP015 §7.]*
- **OQ-24** resolved at §5.5 — `.implicit` Aggregate/Sequence support phasing is implementation latitude. *[CP015 §9.]*
- **OQ-13** resolved at §13 — implicit context parameters fully treated; class-system uninvolved; `.implicit` excludes `.context` parameters as a hard restriction. *[CP015 §8.]*
- **OQ-23 (lexer-disambiguation half)** closed at §14.4 — the literal-token surface revision (`${...}` and `$[...]`) eliminates the disambiguation problem the original `{- -}` / `[- -]` tokens introduced. *[CP015 §2.]*

**Forwarded sub-questions:**

- **OQ-21** (lambda capture-list interaction with context parameters) forwarded to the lambda-and-fexpr reference; concerns lambda-capture mechanics rather than the construction surface.
- **OQ-23 (`.inline`-placement remainder)** forwarded to the implementation thread; small lexer-disambiguation matter not bearing on construction.
- **Block-quote and command-literal parameter-passing details** forwarded to the lambda-and-fexpr reference; their construction surface is uniform with this reference's rules, but mechanism specifics belong with the owning reference.

**Newly registered open questions:**

- **OQ-30 — Heap allocation language-level mechanism.** Recorded at §14.5. Resolution requires a separate design dialog. Candidate directions sketched: region-style allocation tied to lexical scope; allocation-via-class-dispatch; small-primitive-set.
- **OQ-31 — Tuple-style positional access via `Bar::N`.** Recorded at §14.6. Resolution deferred. Coherence questions on grammar widening, subsumption interaction, range checking, scope of relaxation, and `.inline` index numbering all sketched in §14.6.

**Reference-document impact (pending consolidations).** The following revisions to other references are required as a consequence of the commitments in this reference; collected in the work plan's §6 for a future consolidated pass rather than applied here:

- **Type-system reference §2.6** — replace the "interpretive cast operator forwarded to construction reference" framing with the byte-reinterpretation subsumption rule (this reference's §10). The type-side commitment that the typechecker checks T is on at-least-one declared candidate's chain is preserved (sharpened, even — the rule is now precise where §2.6 was qualitative). *[CP015 §5.7.]*
- **Type-system reference §3** (subsumption-relations description) — add the union → candidate-or-parent byte-reinterpretation as a distinct kind of subsumption, alongside the Liskov-preserving upward subsumption that §3 currently describes. User-responsibility for semantic validity is part of the description. *[CP015 §5.7.]*
- **Type-system reference §§2.6 and 3.4** — sharpen the "enum-and-match" wording, which presently reads as if it might denote a planned dedicated mechanism. The phrase denotes the user-side `?:` chain over an enum discriminant idiom; clarification belongs in a future consolidated pass. *[CP015 §6.4.]*
- **Operational-semantics reference §5.5** — redirect OQ-13's class-system forwarding to this construction reference (OQ-13 is fully owned by construction now). *[CP015 §8.5; this reference §13.8.]*
- **Operating-principles document** — three pending updates batched per that document's update protocol: a new §3.9 codifying mainstream-language syntactic-shape drift (type-annotation order; ternary-shaped construction); a new §2.X codifying the standard-terminology guideline; a refinement to §1.5 acknowledging the union byte-reinterpretation relation as distinct from Liskov-preserving upward subsumption. *[CP015 §11.]*

**Carried-in commitments encoded from inception (not from prior reference's framing):**

- **Literal-token surface revision** (CP015 §2): `${...}` and `$[...]` replace CP009's `{- -}` and `[- -]` throughout. CP009 §§3.3, 3.4, 4.3, 8.1, 8.2 are revised; this reference uses the new tokens from inception.
- **`-<` operator extensions** (CP015 §3): the `_` discard marker on either side of `-<` for variant operands (`v -< _` always-succeeds clear; `_ -< v` may-fail non-absent test) is encoded in §11 from inception.
- **Aggregate-literal positional form** (CP015 §4.1): admitted under the contextual-clarity rule; encoded in §3.3 from inception.
- **Variant-absent `_` syntax** (CP015 §4.2): variant-typed fields in Aggregate literals require explicit `_` for the absent state; encoded in §3.4 from inception.
- **Union → candidate-or-parent byte-reinterpretation** (CP015 §5): replaces the "interpretive cast operator" framing of type-system §2.6; encoded in §10 from inception, with the `byte-reinterpretation subsumption` terminology used to flag the non-Liskov property at the docs level (per the pending operating-principles §2.X amendment).
- **Pattern-matching as composition** (CP015 §6): no new keyword surface; encoded in §9 from inception.
- **OQ-22 / OQ-24 / OQ-13 resolutions** as detailed above.

**Recommended next step.** User review section-by-section, revisions in place. After this reference is settled, the next topic-organized reference per the agreed sequence is **Lambda and Fexpr** (#5) — owning CP007–CP008–CP010 in full, OQ-7 history, OQ-18 / OQ-21 / OQ-25; lambda capture mechanics and ceiling-tracking; fexpr's locality rule and per-invocation invocation frame; partial-application beyond receiver-elision (OQ-6 partial owner). The sequence after that is the class-system reference (#6), which owns OQ-5 / OQ-6 / OQ-16 / OQ-29 and the cross-module Liskov question for failure tags. The full sequence is in the work plan's §1.2 inventory.

The pending consolidations enumerated above (type-system §§2.6 / 3 / 3.4 revisions; op-sem §5.5 redirect; operating-principles §§3.9 / 2.X / 1.5 updates) should be applied in a single consolidated pass at a convenient point — they are individually small revisions and benefit from being batched.
