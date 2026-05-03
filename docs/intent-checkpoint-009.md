# Basis Language — Intent Checkpoint 009

**Status:** Working draft. Builds on intent-checkpoint-001.md through intent-checkpoint-008.md; supersedes them where they conflict. Notable refinement: this checkpoint sharpens checkpoint 004's productive-parameter rule to write-once (§2), with consequences for atomic initialization and the construction story.
**Date of this checkpoint:** April 2026
**Provenance:** Continuation of the intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-28. This checkpoint resolves OQ-10 (composite initializers) and substantially resolves OQ-17 (literal-passing surface). The resolution has three parts: (1) the productive-parameter rule is sharpened from "must write on every successful path" to "must write exactly once on every successful path"; (2) the `<-` operator's right-hand-side grammar is extended to a polymorphic form accepting call-shapes, structural literals (Aggregate `{- ... -}` and Sequence `[- ... -]`), bare identifiers (for value-copy on copyable types), and bare literals (when an `.implicit` constructor permits the elision); and (3) a new `.implicit` keyword introduces user-extensible literal coercion, narrowly scoped to prevent the Scala-implicit pathology while enabling clean ergonomic surface for literal use.

**Authority statement.** Where this document differs from earlier checkpoints, this document is authoritative. Where this document and the source code differ on syntactic matters, the code is authoritative; where they differ on semantic intent, this document is authoritative. The README and `language_notes.txt` remain non-authoritative.

---

## 1. Decision Summary

This checkpoint lands seven interlocking decisions about the construction story:

1. **Productive parameters are write-once.** Every successful return path through a command's body writes to each productive parameter **exactly once** — not "at least once." This sharpens checkpoint 004 §3.3, makes atomic initialization the only available pattern for productive slots, and integrates naturally with the rest of the construction story.

2. **`<-` is the universal placement operator with a polymorphic rhs.** The existing `<-` operator retains its role; its right-hand side is extended to accept five shapes: parenthesized call, Aggregate literal `{- field <- value, ... -}`, Sequence literal `[- value, value, ... -]`, bare identifier (for value-copy on copyable types), and bare literal (when an `.implicit` constructor exists). The typechecker dispatches semantics on the rhs shape and the lhs type.

3. **Aggregate and Sequence are the new structural literal forms.** Aggregate literals (`{- field <- value, ... -}`) initialize records, objects, unions, and variants by named field/candidate. Sequence literals (`[- value, value, ... -]`) initialize positional, vector-like targets such as `[N]T` typed buffers. Empty forms are written `{-}` and `[-]` as single tokens.

4. **Defaults are declared via definitional `=`.** A buffer-backed type's default-declaration uses the existing definitional `=` operator with an rhs accepting any shape from the `<-` rhs grammar. The two operators (`=` for definitions, `<-` for runtime placement) consume the same rhs grammar but apply at different lexical levels.

5. **`.implicit` introduces user-extensible literal coercion.** A constructor declared with `.implicit` (replacing `.cmd` for that constructor) registers as the elision target for literal-of-type-`L`-into-context-expecting-`T`. The mechanism is narrowly scoped: only literal source types are eligible, no transitive chains, ambiguity is rejected. This resolves the literal-ergonomics question without admitting Scala-style implicit chaos.

6. **Initialization is atomic across may-fail subexpressions.** A failure in any subexpression of an Aggregate, Sequence, or call-form rhs aborts the whole initialization; the lhs slot remains untouched. Failures propagate from the `<-` form. Order of evaluation is left-to-right with short-circuit on failure.

7. **Bare-identifier `<-` is a value-copy primitive for copyable types.** Buffer-backed types, pointers, and command-typed values support `# y <- x` as a primitive value-copy. Objects and variants do not (they have identity/reference semantics; copying isn't well-defined as a primitive).

The checkpoint also resolves OQ-10 (this checkpoint), substantially resolves OQ-17 (via the `.implicit` mechanism plus the new literal forms), introduces three new open questions, and updates the grammar enhancements rollup.

---

## 2. Productive Parameters Are Write-Once

### 2.1 The Sharpening

Checkpoint 004 §3.3 stated that for each productive `'` parameter, "every successful return path through the command's body writes to that parameter." This checkpoint sharpens the rule to:

> Every successful return path through a command's body writes to each productive parameter **exactly once**.

The strengthening is from "at-least-once" to "exactly-once" on a per-path basis.

### 2.2 Why Exactly-Once Is The Right Rule

The exactly-once rule has four structural consequences that align with the language's other commitments:

- **Atomic initialization is structurally enforced, not merely conventional.** Productive slots transition from uninitialized to initialized in one step on every path. There is no "write a placeholder, then write the real value" idiom; that's two writes, which the rule rejects. Compute-then-commit (per checkpoint 006 §2.4) is now mandatory rather than idiomatic.

- **The flow lattice has cleaner monotonicity.** Once a productive slot transitions to `init` on a path, it stays `init` on that path; there is no "must write again" obligation that would re-engage. The lattice from checkpoint 004 §3 already supports this; the sharpening just makes it the only valid transition pattern for productive slots.

- **Recovery patterns are unaffected.** A `?compute` followed by a `|` recovery sub-body is two textual writes on disjoint paths — one on the success path of `?compute`, one on the recovery path. Each *path* has exactly one write; the rule operates on paths, not textual occurrences. Code remains expressible.

- **Loops cannot write productive slots.** A productive write inside a loop body would mean N writes per N-iteration completion, violating exactly-once on every path that completes one or more iterations. This was already idiomatically discouraged; now it's structurally rejected. Loops that compute values into a productive slot must do so via a single write after the loop body — which is the natural shape anyway.

### 2.3 Edge Cases Resolved

**Conditional re-writing.** `'r <- a; if X then 'r <- b` writes twice on the X-taken path (rejected by exactly-once) and once on the X-not-taken path. The program is rejected. The fix is the natural rewrite: `if X then 'r <- b else 'r <- a` — every path writes exactly once.

**Conditional initial-write.** `if X then 'r <- a` writes once on the X-taken path and zero times on the X-not-taken path. This is rejected by checkpoint 004's at-least-once and remains rejected by exactly-once. No change.

**Nested productive calls.** A productive sub-call to another command counts as one write (the sub-call's exactly-once obligation discharges the outer one's exactly-once obligation). No special handling needed.

### 2.4 Relation to Atomic Initialization

The composite-initializer mechanism (§3 below) produces a single `<-` write to a productive slot. Under the exactly-once rule, this single `<-` *is* the productive obligation discharge — one operation, one write, one transition from uninitialized to initialized.

The compute-then-commit pattern from checkpoint 006 §2.4 is thus structurally enforced: Phase 1 (compute) operates on ordinary local slots; Phase 2 (commit) is exactly one `<-` against the productive receiver. The grammar of `<-` accepts the rich rhs forms of §3, but the constructor body's structural shape is fixed.

### 2.5 Resolution

This sharpening updates checkpoint 004 §3.3 in place; the at-least-once formulation is superseded. Checkpoint 006 §2.4's "compute-then-commit" pattern is now structurally mandatory rather than conventional.

---

## 3. The `<-` Operator's Right-Hand-Side Grammar

### 3.1 The Polymorphic RHS

The `<-` operator retains its role from checkpoints 001 and 004 (slot placement, copy-restore on success, bit-identical on failure). Its right-hand side is extended to accept five distinct shapes:

| Shape | Surface | LHS types accepted | Desugars to |
| --- | --- | --- | --- |
| Parenthesized call | `(cmd: args)` | Any (per the cmd's productive output) | Existing call mechanism (checkpoint 001 §3.3) |
| Aggregate literal | `{- field <- value, ... -}` | Records, objects, unions, variants | Atomic structural placement (§3.3) |
| Sequence literal | `[- value, value, ... -]` | `[N]`, `[N]T`, `[]T` | Atomic positional placement (§3.4) |
| Bare identifier | `name` | Buffer-backed, pointers, command-typed | Value-copy (§3.7) |
| Bare literal | `3.14`, `"hello"`, `0x41` | Whatever a matching `.implicit` constructor accepts | Implicit constructor call (§4) |

The typechecker dispatches on the rhs shape and the lhs type. A given lhs type accepts a subset of these shapes; an rhs shape fits a subset of lhs types; the typechecker reports an error when the intersection is empty.

### 3.2 Desugaring Note

The desugaring chain for parenthesized call form proceeds in two visible steps:

```
# r <- (cmd: x, y)              ; surface
cmd: # r, x, y                  ; intermediate (slot introduced in argument position)
# r; cmd: 'r, x, y              ; final (separate introduction and call)
```

The intermediate form, with the `#`-introduction in argument position, is the natural step for the typechecker to reason about: the slot is introduced uninitialized and bound to a productive parameter position simultaneously. The productive obligation discharges on the call's successful return.

For Aggregate and Sequence forms, no parameter-position desugaring applies; the placement is direct (§3.3, §3.4).

### 3.3 Aggregate Literals

An Aggregate literal `{- field-name <- value, field-name <- value, ... -}` initializes a record, object, union, or variant. The rules:

- **Field names are nominal.** The typechecker matches the literal's field names to the lhs type's declared field names. Order in the literal does not need to match declaration order.
- **Repeated field names are rejected at parse time.** `{- x <- 1, x <- 2 -}` is a syntactic error.
- **Extra field names are permitted but generate a warning.** The literal may include `field-name <- value` entries that don't correspond to any field in the lhs type. These are silently ignored at runtime; the warning ensures the user is aware. The motivation: tooling that generates literals from heterogeneous sources should not be defeated by spurious data.
- **Required fields must be present, except where defaults cover omission.** A field is required unless the lhs type's declaration provides a default for that field. A field with a declared default may be omitted from the literal; if so, the default value is supplied. Omission of a defaulted field generates a compiler warning; the user can suppress it by writing the field explicitly.
- **The empty form is `{-}`.** This is a single token, distinct from `{- -}` with whitespace. It denotes an Aggregate literal with no fields, valid only when the lhs type has every field defaulted (or has no fields, e.g., an empty record).

Variants and unions are addressed via the single-candidate form: `{- Candidate <- value -}` selects the named candidate and supplies its value. The lhs type disambiguates: a brace literal with one field `Foo` initializes a variant if the lhs is a variant with candidate `Foo`, or initializes a single-field record/object if the lhs is such a record/object.

### 3.4 Sequence Literals

A Sequence literal `[- value, value, ... -]` initializes a positional, vector-like target. The rules:

- **Elements are positional.** The typechecker assigns elements to slots by position.
- **Element count must match the lhs type's expected count, unless the lhs is unbounded.** For `[N]T`, exactly N elements; for `[]T`, any count (the count becomes part of the buffer's runtime length).
- **Each element must be of the lhs type's element type, with implicit-conversion eligibility per §4.** `[- 1, 2, 3 -]` for an `[3]Float32` lhs requires each element to be either a `Float32` value or a literal that has an `.implicit` constructor producing `Float32`.
- **The empty form is `[-]`.** This is a single token. It denotes a Sequence literal with zero elements, valid for `[0]T` and `[]T` (empty unbounded buffer).

Sequences are not used to initialize records/objects (which use Aggregate). The two surface forms are disjoint in the lhs types they accept.

### 3.5 Failure Atomicity

A subexpression in any rhs form may be a may-fail expression. The semantics:

- **Atomic failure.** A failure in any subexpression aborts the whole `<-` operation. The lhs slot's pre-call state is preserved (per the standard copy-restore rule of checkpoint 001 §3.2 — failed `<-` does not write back).
- **Order of evaluation is left-to-right.** For Aggregate, fields are evaluated in textual order. For Sequence, elements are evaluated in textual order.
- **Short-circuit on failure.** Once a subexpression fails, subsequent subexpressions in the same `<-` are not evaluated. The whole operation propagates the failure.
- **Failure propagates from the `<-` form.** The form itself acquires the failure mode of any may-fail subexpression. A `<-` containing one or more `?`-marked subexpressions is may-fail; a `<-` containing a `!`-marked subexpression is must-fail. The standard failure-handling rules apply.

This is consistent with the language's atomicity story (mutation either succeeds fully or fails fully, per checkpoint 001) and requires no new machinery — copy-restore on the lhs slot already provides the rollback.

The implications for nested constructors are uniform: a failure inside a constructor's Phase 1 (computing constituent values) propagates to its caller, where the productive `'r` slot remains untouched. The whole construction fails atomically. This composes with whatever is initializing the receiver of the failed constructor — that receiver's `<-` also fails, leaving its own caller's slot untouched, all the way up.

### 3.6 Embedded Objects in Aggregates

When a record or object has an embedded object field, the Aggregate literal references the embedded object by name:

```
.cmd buildContainer : Container 'r =
    # contained <- (InnerObj: someArgs)         ; Phase 1: construct constituent
    'r <- {- inner <- contained, count <- 5 -}  ; Phase 2: commit
```

The embedded object's at-stack registration migrates to the new container's owning frame at the moment of the Aggregate literal's atomic placement. This composes with checkpoint 002 §3.4's existing migration story; no new mechanism is required.

If the constructor `buildContainer` fails (e.g., `?something` inside Phase 1), the partially-constructed `contained` object is registered with `buildContainer`'s frame and is cleaned up via the failure-exit machinery there (per checkpoint 002 §3.3 Case 3). The atomicity story holds.

### 3.7 Bare-Identifier `<-` (Value-Copy Primitive)

The bare-identifier form `# y <- x` is a value-copy primitive. It is well-defined for types whose values can be copied as bytes:

- **Buffer-backed types** (records, domains, unions, buffer primitives `[N]`, `[N]T`, `[]`, `[]T`): the bytes of `x` are copied to `y`'s storage. The lhs and rhs types must be compatible (identical, or one a parent domain of the other).
- **Pointers** (`^T` for any T): the pointer value is copied; both pointers point at the same target. The pointed-to storage is not affected.
- **Command-typed values** (`:<...>`, `?<...>`, `!<...>`): the command-value (including any captures, per checkpoint 007) is copied. For lambdas with reference captures, the ceiling of the copy equals the ceiling of the source (per checkpoint 008 §5.2).

The bare-identifier form is **forbidden** for:

- **Objects.** Objects have identity. "Copying" an object is not a primitive operation; the user must invoke a copy-constructor (if defined) to produce a new object, or share access via a `^Object` pointer.
- **Variants.** Variants have reference semantics (per checkpoint 001). Same reasoning as objects.

Bare-identifier `<-` is the one rhs shape that does not look like a "construction" — no command runs, no literal is supplied, no implicit conversion fires. It's a primitive value-move, comparable to `memcpy` for the relevant byte representations. The typechecker verifies type compatibility; the runtime is a byte-copy.

### 3.8 Bare-Literal Form (via `.implicit`)

A bare literal on the rhs (e.g., `# x : Float32 <- 3.14`) is well-typed if and only if an `.implicit` constructor exists matching the literal's type to the lhs type (per §4). Without the matching constructor, the literal must pass through an explicit constructor call (per checkpoint 007 §5).

This form is the user-facing ergonomic surface for literal-into-typed-slot initialization. It is the only rhs form whose well-typedness depends on user-supplied declarations rather than on language-defined rules.

---

## 4. The `.implicit` Keyword

### 4.1 The Mechanism

A constructor declared with `.implicit` registers itself as the elision target for "literal-of-type-`L` appearing in a context expecting `T`." Surface form:

```
.implicit Float32 'r : Decimal d =
    ; ... constructor body ...
```

This declares an `.implicit` constructor that:

- Produces values of type `Float32`.
- Takes a single parameter of literal type `Decimal`.
- May be invoked explicitly (as `(Float32: 3.14)`) — `.implicit` does not prevent explicit calls.
- May be invoked implicitly when a `Decimal` literal appears in a `Float32`-expecting context, with the typechecker inserting the call.

`.implicit` is parallel to `.cmd` in the declaration prefix grammar — like `.intrinsic` and `.decl`, it is a variation on the standard command-declaration prefix that signals additional semantic properties. `.implicit` may only be applied to constructor commands (commands with a productive `'r` receiver and the constructor signature shape from checkpoint 005 §2.3); applying it to a v-command, a regular command, or an at-stack method is a compile-time error.

### 4.2 Restrictions

Three restrictions keep `.implicit` from collapsing into Scala-style implicit chaos:

**Restriction 1: Source type must be a literal type.** The implicit constructor's parameter type must be a recognized literal type. The current set of literal types:

| Literal type | Source surface |
| --- | --- |
| `Decimal` | Decimal numeric literals (`42`, `3.14`) |
| `Hex` | Hex numeric literals (`0x41`) |
| `String` | String literals (`"hello"`) |
| `Char` | Character literals (`'A'`) |
| `Aggregate` | Aggregate literals (`{- field <- value, ... -}`) |
| `Sequence` | Sequence literals (`[- value, value, ... -]`) |

A constructor whose parameter is any other type may not be marked `.implicit`. This narrows the mechanism to its motivating use case (literal coercion) and prevents the Scala-style cascade where any value can implicitly convert to many target types based on cross-cutting rules.

**Restriction 2: No transitive chains.** A literal `L` is converted to `T` via at most one implicit constructor. If no `.implicit T 'r : L l` exists, but `.implicit T 'r : M m` and `.implicit M 'r : L l` both exist, the chain `L → M → T` is **not** considered. The typechecker rejects the use site as if no implicit conversion were available, requiring the user to write the chain explicitly.

**Restriction 3: Unambiguous resolution.** If multiple `.implicit T 'r : L l` constructors are visible at a use site, the compilation is rejected with an ambiguity error. The user resolves the ambiguity either by removing one of the declarations, by scoping them so only one is visible, or by writing the constructor call explicitly.

### 4.3 Aggregate and Sequence Implicit Constructors

The Aggregate and Sequence literal types are parameterized by their internal structure. An Aggregate literal `{- x <- 1, y <- 2.5 -}` synthesizes the structural type `Aggregate{x: Decimal, y: Decimal}`; a Sequence literal `[- 1, 2, 3 -]` synthesizes `Sequence{Decimal}` (with element count determined at the use site).

An `.implicit` constructor for an Aggregate or Sequence type must specify the matching structure:

```
.implicit Point 'r : Aggregate{x: Decimal, y: Decimal} a = ...
.implicit Vec3 'r : Sequence{Decimal, length: 3} v = ...
```

The exact surface for these structural types is open (see OQ-22 below). The mechanism's intent is clear: structural matching of the literal's shape against the implicit constructor's parameter type.

**Implementation phasing.** Initial implementation may support only the simple literal types (`Decimal`, `Hex`, `String`, `Char`); Aggregate and Sequence implicit support comes when the structural-matching machinery is in place. The language commits to the design; the compiler may stage its support incrementally.

### 4.4 Explicit Calls Are Always Available

`.implicit` is purely additive. An `.implicit Float32 'r : Decimal d` constructor may be invoked as `(Float32: 3.14)` — exactly the same surface as a non-implicit constructor — at any use site. The implicit elision is a *convenience* on top of the explicit form; it is not a replacement for it.

Code that wishes to be explicit about constructor calls remains exactly as readable as it is today. The user opts into implicit elision per-constructor, and any call site can be written either way.

### 4.5 Resolution of OQ-17

OQ-17 (compound literal syntax and literal-as-input rules) is substantially resolved by this section combined with §3.3 and §3.4:

- The literal-passing surface is the parenthesized call form (`(Float32: 3.14)`) for explicit construction, or the bare-literal form (`3.14`) when an `.implicit` constructor permits.
- Bracket-form types `[N]` and `[]` accept Sequence literals directly via §3.4 (a `[3]Int32` lhs with a Sequence rhs `[- 1, 2, 3 -]` is well-typed when each element is `Int32` or implicitly-convertible to `Int32`).
- Implicit type conversions are exactly those licensed by `.implicit` constructors and by domain-hierarchy parent-child relationships (per checkpoint 003).
- Implicit constructor sugar for the bare-literal-on-typed-slot case is the `.implicit` mechanism itself, narrowly scoped per §4.2.

---

## 5. Defaults Declaration Syntax

### 5.1 The `=` Form

A buffer-backed type that lacks a meaningful zero (per checkpoint 006 §3.1) declares its default using definitional `=`:

```
.domain Positive : Int32 = (Int32: 1)
```

The rhs of the definitional `=` accepts any shape from the `<-` rhs grammar (§3): parenthesized call, Aggregate literal, Sequence literal, bare identifier, or bare literal (subject to `.implicit` constructor availability).

A record default uses the Aggregate form:

```
.record Point :
    x : Float32
    y : Float32
    = {- x <- (Float32: 0), y <- (Float32: 0) -}
```

A buffer-backed type whose every field has a meaningful zero or a declared default has a zero-default by composition (per checkpoint 006 §3.1) and need not declare one explicitly; the explicit declaration is for cases where the composition rule doesn't yield the desired value.

### 5.2 Two Operators, One RHS Grammar

The split between `=` (definitional) and `<-` (runtime placement) is preserved exactly:

- `=` appears in declaration positions (`.cmd ... =`, `.domain T = ...`, `.record T : ... = ...`).
- `<-` appears in expression-position placement (`# x <- ...`, `'r <- ...`, etc.).

Both consume the same rhs grammar. The split keeps definitions distinguishable from runtime operations at the syntactic level: a reader sees `=` and knows they are looking at a declaration; sees `<-` and knows they are looking at a runtime operation.

### 5.3 Defaults for Non-Buffer Types

Per checkpoint 006 §3.2, non-buffer types (pointers, command-typed values, objects, variants) cannot have defaults; bare `# x : T` for such types is rejected. The `=` mechanism does not provide an escape from this rule — declaring a default for an object type is a compile-time error. Construction of non-buffer slots is always explicit, via a constructor call or other rhs form at each `# x : T <- ...` site.

---

## 6. Worked Examples

### 6.1 Constructor Body Using Aggregate Literal

```
.record Point :
    x : Float32
    y : Float32

.cmd makePoint : Point 'r, Float32 a, Float32 b =
    'r <- {- x <- a, y <- b -}
```

The constructor body is a single Phase 2 step (no Phase 1 needed; inputs are already Float32). The Aggregate literal places `a` at the `x` field offset and `b` at the `y` field offset. The single `<-` discharges the productive obligation per the exactly-once rule (§2).

Caller side:

```
.cmd useIt =
    # p : Point
    makePoint: 'p, (Float32: 1.0), (Float32: 2.0)
```

Or, using expression-position sugar:

```
.cmd useIt =
    # p : Point <- (makePoint: (Float32: 1.0), (Float32: 2.0))
```

Or, with `.implicit Float32 'r : Decimal d` declared:

```
.cmd useIt =
    # p : Point <- (makePoint: 1.0, 2.0)
```

### 6.2 Constructor Body With Phase 1 May-Fail Computation

```
.cmd parsePoint : Point 'r, String input =
    # parts <- (?splitOn: input, ",")        ; Phase 1: may fail
    # x <- (?parseFloat32: parts[0])         ; Phase 1: may fail
    # y <- (?parseFloat32: parts[1])         ; Phase 1: may fail
    'r <- {- x <- x, y <- y -}               ; Phase 2: never fails
```

Each `?` call may fail. On failure of any Phase 1 step, the body propagates failure; `'r` is never written; the caller's slot retains its pre-call state per copy-restore. On full Phase 1 success, the Phase 2 Aggregate literal commits atomically (exactly one write to `'r`).

The shorthand `x <- x` in the literal is the natural expression: assign the local-named-`x`'s value to the field-named-`x`. The brace literal's left-of-`<-` is field name; right-of-`<-` is value expression. Identical names are not a special case.

### 6.3 Sequence Literal for a Typed Buffer

```
.cmd makeVec3 : [3]Float32 'r, Float32 a, Float32 b, Float32 c =
    'r <- [- a, b, c -]
```

The Sequence literal places the three values at successive offsets in the typed buffer. The element count (3) matches the lhs type's declared length.

### 6.4 Variant Construction via Aggregate Form

```
.variant Shape :
    Circle : Float32                         ; radius
    Rectangle : Float32, Float32             ; width, height

.cmd makeCircle : Shape 'r, Float32 radius =
    'r <- {- Circle <- radius -}

.cmd makeRectangle : Shape 'r, Float32 w, Float32 h =
    'r <- {- Rectangle <- {- w <- w, h <- h -} -}      ; (Approximate; depends on
                                                       ;  variant candidate-arg shape)
```

(The exact surface for variant candidates with multiple arguments depends on the variant declaration syntax, which is not the subject of this checkpoint; the example shows the principle.)

### 6.5 `.implicit` Constructor

```
.implicit Float32 'r : Decimal d =
    # bytes <- (decimalToFloat32Bytes: d)
    'r <- bytes                              ; bare-identifier copy (buffer-backed)
```

With this constructor in scope:

```
.cmd useIt =
    # x : Float32 <- 3.14                   ; well-typed; desugars to (Float32: 3.14)
    # y : Float32 <- (Float32: 2.71)        ; explicit form, also well-typed
```

Both surface forms produce identical runtime effects. The user chooses based on code-clarity preferences.

### 6.6 Bare-Identifier Copy

```
.cmd duplicate : Int32 input, Int32 'a, Int32 'b =
    # local <- input
    'a <- local                              ; bare-identifier; value-copy
    'b <- local                              ; bare-identifier; value-copy
```

Each `<-` is a primitive value-copy. The lhs and rhs types match (both `Int32`); no constructor or implicit conversion is involved.

For an object type, the analogous code is rejected:

```
.cmd duplicateObj : SomeObj input, SomeObj 'a =
    'a <- input                              ; ERROR — bare-identifier copy not valid for objects
```

The user must use a copy-constructor or a pointer.

### 6.7 Field Omission with Default

```
.record Settings :
    timeout : Int32 = (Int32: 30)            ; default declared
    verbose : Bool = false                   ; default declared (assumes .implicit Bool : ?)
    name : String                            ; no default; required

.cmd useDefaults =
    # s : Settings <- {- name <- "test" -}   ; WARNING — timeout, verbose omitted; defaults supplied
```

The compiler warns about the omitted defaulted fields. To suppress the warnings, the user writes them explicitly:

```
    # s : Settings <- {- name <- "test", timeout <- (Int32: 30), verbose <- false -}
```

---

## 7. Implementation Notes

### IN-9: Aggregate Literal Field Matching

The typechecker matches Aggregate literal field names to the lhs type's declared field names. The matching is by name, not by position. The implementation needs:

- A fast lookup from field-name to declaration-position for each Aggregate-targetable type.
- Tracking of which fields are present in the literal, so unmatched required fields can be reported and matched extras can be warned about.
- Default-substitution for omitted defaulted fields, with warning emission.

### IN-10: Sequence Literal Element-Count Checking

For `[N]T` lhs types, the Sequence literal's element count must match N exactly; mismatches are compile-time errors. For `[]T` lhs types, the element count becomes the buffer's runtime length and is recorded with the buffer.

### IN-11: `.implicit` Lookup Cost

At each `<-` site with a literal rhs and a typed lhs, the typechecker performs an `.implicit` lookup keyed on (literal-source-type, lhs-target-type). The lookup may be implemented as a hash map or similar; the cost is amortized constant per use site.

The lookup table is built at compile time from `.implicit` declarations across the compilation unit and its imports.

### IN-12: Failure-Atomicity Implementation

The atomic-failure semantics for Aggregate/Sequence literals are implemented as standard copy-restore: the literal's evaluation produces a sequence of computed values that, only at the end, are placed into the lhs slot. A failure mid-evaluation aborts the placement entirely.

For Aggregate literals targeting objects, the placement is the object-construction step (allocate, populate fields, register at-stack). A pre-placement failure abandons the object before allocation; no cleanup is needed.

### IN-13: Bare-Identifier Copy as Memcpy

For buffer-backed types, the bare-identifier `<-` compiles to a byte-copy from source to destination. The compiler may use machine-level instructions (memcpy, vectorized copy) where appropriate. For pointers and command-typed values, the copy is a single machine-word (or equivalent) move.

For object and variant types, the bare-identifier form is rejected at the typechecker level; no runtime support is needed.

---

## 8. Grammar Enhancements (Updated Rollup)

This section extends the rollup from checkpoint 008 §9 with the new productions and tokens introduced by this checkpoint.

### 8.1 New Tokens

- **`{-` and `-}`** — Aggregate literal delimiters. The opening token includes the hyphen; the closing token includes the hyphen.
- **`{-}`** — Empty Aggregate literal. A single token, distinct from `{- -}`.
- **`[-` and `-]`** — Sequence literal delimiters.
- **`[-]`** — Empty Sequence literal. A single token, distinct from `[- -]`.

(The exact lexer rules for distinguishing these from other uses of `{`, `}`, `[`, `]`, `-` will likely require lookahead or context-sensitive lexing. The user has noted this may be revisited at implementation time.)

### 8.2 `<-` Inside Aggregate Literals

Inside an Aggregate literal, `<-` is the field-name-to-value separator. This is a re-use of the existing `<-` token at a new grammatical position. The grammar production for Aggregate-literal-field-entry is `all(IDENTIFIER, LARROW, expression)` (or equivalent).

### 8.3 New Keyword: `.implicit`

The `.implicit` keyword is a declaration prefix parallel to `.cmd`. It accepts only constructor signatures (commands with productive `'r` receiver and the constructor signature shape from checkpoint 005 §2.3).

The grammar production for command-declaration prefix becomes a choice: `.cmd | .implicit | .intrinsic | .decl | ...` (and any other parallel forms). The exact set of valid prefixes is enumerated by the grammar; `.implicit` is one of them.

### 8.4 `<-` RHS Grammar Extension

The right-hand side of `<-` in expression position accepts five shapes (per §3.1):

- Parenthesized call: existing production, unchanged.
- Aggregate literal: new production, per §3.3 and §8.1–8.2.
- Sequence literal: new production, per §3.4 and §8.1.
- Bare identifier: existing identifier production accepted as rhs.
- Bare literal: existing literal productions accepted as rhs.

The typechecker enforces lhs-type-vs-rhs-shape compatibility; the parser accepts any shape and defers compatibility checking.

### 8.5 Definitional `=` Accepts Same RHS Grammar

The rhs of definitional `=` (in `.domain T = ...`, `.record T : ... = ...`, etc.) accepts the same five shapes as the rhs of `<-`. The grammar productions for definitional and runtime rhs unify on a shared rhs-grammar production.

### 8.6 Cross-Reference to Earlier Grammar Items

The grammar enhancements from checkpoints 004, 005, and 008 (identifier-shape markers, parameter mode markers in command definitions, mode markers in command-type-expressions, lambda slash-list, etc.) are all unaffected by this checkpoint and remain as documented in checkpoint 008 §9.

---

## 9. Open Questions Updated

| OQ | Status | Notes |
| --- | --- | --- |
| OQ-1 | Open | Union discriminator representation. |
| OQ-2 | Open | Implementation latitude for IN parameter passing. |
| OQ-3 | Mostly resolved (per checkpoint 004) | |
| OQ-4 | Resolved (per checkpoint 004) | |
| OQ-5 | Open | Single-class instance coherence. |
| OQ-6 | Substantially resolved (per checkpoint 008) | Surface syntax for partial application of v-command receiver tuples remains. |
| OQ-7 | Refined (per checkpoint 007) | Fexpr design proper. |
| OQ-8 | Mostly resolved (per checkpoint 002) | |
| OQ-9 | Resolved (per checkpoint 006) | |
| OQ-10 | **Resolved by this checkpoint** | Composite initializers via the polymorphic `<-` rhs grammar with Aggregate and Sequence literal forms. See §3. |
| OQ-11 | Resolved (per checkpoint 005) | |
| OQ-12 | Resolved (per checkpoint 005) | |
| OQ-13 | Open | Implicit context parameters and initialization. |
| OQ-14 | Open | Same-scope rule for `&x` and `x`. |
| OQ-15 | Open | Full design of the downcast intrinsic. |
| OQ-16 | Open | Overloading restriction on dynamically-dispatched commands. |
| OQ-17 | **Substantially resolved by this checkpoint** | Literal-passing via parenthesized call form and bare-literal form (the latter via `.implicit`). Bracket-form types accept Sequence literals directly. See §3 and §4. |
| OQ-18 | Open | Lambda visible-signature representation. |
| OQ-19 | Resolved (per checkpoint 008) | |
| OQ-20 | Open | Slash-list internal grammar details. |
| OQ-21 | Open | Capture-list interaction with implicit context parameters. |
| OQ-22 | **New** | Surface form for parameterized literal types in `.implicit` declarations. The Aggregate and Sequence literal types are parameterized by their internal structure; the exact syntactic form for declaring an `.implicit` constructor that accepts a structured literal (e.g., `Aggregate{x: Decimal, y: Decimal}` vs. some other form) is open. |
| OQ-23 | **New** | Lexer disambiguation rules for `{-`, `-}`, `[-`, `-]`, `{-}`, `[-]`. The new tokens overlap with existing uses of `{`, `}`, `[`, `]`, `-`. The exact lexer rules — whether by lookahead, by context-sensitive lexing, or by declaring these as multi-character tokens with specific recognition rules — need resolution at implementation time. The user has noted the syntax may be revisited; OQ-23 captures the deferred decision. |
| OQ-24 | **New** | The phasing of `.implicit` Aggregate/Sequence support. Initial implementation may defer structural-matching of literal types, supporting only simple literal types (`Decimal`, `Hex`, `String`, `Char`) at first. The decision of whether to commit the language to phased support, or whether full support is required from the start, is open. |

---

## 10. Summary of Changes from Prior Checkpoints

| Topic | Prior State | This Checkpoint |
| --- | --- | --- |
| Productive parameter rule | "Must write on every successful path" (at-least-once) | "Must write exactly once on every successful path" |
| `<-` rhs grammar | Parenthesized call only | Five shapes: call, Aggregate, Sequence, bare identifier, bare literal |
| Composite initializers | Open (OQ-10) | Aggregate `{- field <- value -}` and Sequence `[- value -]` forms |
| Defaults declaration | Mechanism unspecified | Definitional `=` with shared rhs grammar |
| Literal coercion | Must use explicit constructor call | `.implicit` mechanism enables elision (narrowly scoped) |
| Construction story | Compute-then-commit conventional | Compute-then-commit structurally enforced (via write-once rule) |
| Bare-identifier copy | Not specified | Primitive value-copy for buffer-backed/pointer/command-typed; rejected for object/variant |
| Failure during initialization | Implicit (copy-restore) | Explicit: atomic, left-to-right, short-circuit |

---

## 11. Provenance

**Authored:** Distilled from continued intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-28.

**Source materials read for this checkpoint:** intent-checkpoint-001.md through intent-checkpoint-008.md.

**Grammar changes implied:** §8 of this checkpoint enumerates them. The new tokens (`{-`, `-}`, `[-`, `-]`, `{-}`, `[-]`), the `.implicit` keyword, the polymorphic `<-` rhs grammar, and the unified `=`/`<-` rhs grammar at the production level are the main items. The lexer disambiguation rules are flagged as OQ-23 and may require lookahead or context-sensitive lexing.

**Recommended next step:** Commit alongside the prior checkpoints (suggested path: `docs/intent-checkpoint-009.md`). The natural next threads are now: the refined OQ-7 (fexpr design), with the construction story complete; the failure-mode-and-typechecker integration thread that has been pending since checkpoint 002; and the smaller resolutions of OQ-13 (implicit context parameters), OQ-14 (same-scope rule for `&x` and `x`), OQ-15 (downcast intrinsic), and OQ-16 (overloading restriction). The order can be set by what the implementation work prioritizes.
