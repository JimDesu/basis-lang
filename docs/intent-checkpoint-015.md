# Basis Language — Intent Checkpoint 015

**Status:** Working draft. Builds on intent-checkpoint-001.md through intent-checkpoint-014.md; supersedes them where they conflict. This checkpoint records the 2026-05-03 scope-confirmation dialog for the Construction and Initialization reference (the fourth topic-organized reference; not yet drafted at the time of this checkpoint). The dialog produced eight substantive amendments to existing material, two new open questions, and three pending updates to the operating-principles document. The reference itself will be drafted in a fresh session, encoding the commitments recorded here.

**Date of this checkpoint:** 2026-05-03

**Provenance:** Continuation of the intent dialog between JimDesu and Claude (Opus 4.7) on 2026-05-03, in the construction-reference scope-confirmation thread. The dialog was structured around the work plan's §3.4 scope-confirmation questions plus three substantive amendments introduced mid-dialog by the user (literal-token surface change, new `-<` variant uses, heap-allocation gap acknowledgment) and one substantive overnight reframe (union → byte-reinterpretation, replacing the prior "interpretive cast operator" framing).

**Authority statement.** Where this document differs from earlier checkpoints, this document is authoritative. Where this document and the source code differ on syntactic matters, the code is authoritative; where they differ on semantic intent, this document is authoritative. The README and `language_notes.txt` remain non-authoritative.

---

## 1. Decision Summary

This checkpoint lands the following decisions and registrations:

1. **Literal-token surface revised.** Aggregate and Sequence literals now use `$`-prefixed fences: `${ ... }` and `$[ ... ]`, replacing CP009's `{- ... -}` and `[- ... -]` forms. The change substantially closes OQ-23 by eliminating the lexer-disambiguation problem the original tokens introduced. (§2)

2. **The `-<` operator gains two new variant uses, both `_`-marked.** `v -< _` clears a variant to its absent state (always succeeds); `_ -< v` tests whether a variant has any candidate (succeeds iff non-absent; fails iff absent). Both are may-fail-shaped commands, consistent with the existing `'target -< v` form. (§3)

3. **Aggregate literal refinements.** Positional form (`${value, value, ...}`) is well-formed only when the target type is contextually explicit; otherwise named form is required. Variant-typed fields in Aggregate literals require explicit `_` for the absent state — omission is rejected, to preserve positional-transcription integrity. (§4)

4. **Union → candidate-or-parent byte-reinterpretation.** Replaces the "interpretive cast operator" framing of type-system §2.6. A union value implicitly subsumes (zero-cost byte reinterpretation) to any buffer-backed type T appearing on the standard subsumption chain of *at least one* declared candidate. C-style discipline: language-level infallible, user-responsible for semantic validity. No new operator. (§5)

5. **Pattern-matching: composition via existing primitives.** Variant case analysis composes via `?:` chains + `-<` (and `?-` for explicit absent-only tests); no new dedicated `match` keyword surface. The "enum-and-match" idiom referenced in type-system §2.6 / §3.4 is shorthand for `?:` chains over enum discriminants, not a planned dedicated mechanism. (§6)

6. **OQ-22 resolved.** `.implicit` constructor parameter types use type-form mirrors of value forms: `${Type field, ...}` for Aggregate-shape, `$[N]T` for Sequence-shape. (§7)

7. **OQ-13 resolved on the construction side; class-system uninvolved.** Implicit context parameters are a parameter-passing matter, not a class-system matter. The construction reference encodes the full mechanics. `.implicit` constructors may not declare `.context` parameters (hard restriction at grammar and semantic-analyzer levels), to prevent compound implicitness. (§8)

8. **OQ-24 resolved as implementation latitude.** CP009 §4.3's design is committed; the phasing of `.implicit` Aggregate/Sequence support is an implementation choice, not a language-spec concern. (§9)

9. **Two new open questions registered, both deferred.** OQ-30 (language-level heap allocation mechanism) and OQ-31 (tuple-style positional access via `Bar::N`). Both require separate design dialogs. (§10)

10. **Three pending updates to the operating-principles document.** A new §3.9 (mainstream-language syntactic-shape drift), a new §2.X (standard-terminology guideline), and a refinement to §1.5 (union byte-reinterpretation). Batched per the operating-principles document's update protocol. (§11)

The construction reference (the fourth topic-organized reference) will encode all of the above. Drafting is to occur in a fresh session with full context budget.

---

## 2. Literal-Token Surface Revision

### 2.1 Background

CP009 §3 established two new structural literal surfaces — Aggregate literals (for record-shaped values) and Sequence literals (for buffer-shaped values) — using `{- ... -}` and `[- ... -]` as the fence tokens. The choice was motivated by visual distinctiveness from ordinary `{` / `[` uses and a desire for symmetry between the open and close fences. CP009 §8 acknowledged that the new tokens would require lexer-disambiguation work and registered OQ-23 to track it. The disambiguation problem was real: the lexer had to decide, on encountering `{` followed by `-` or `-` followed by `}`, whether to treat the pair as the new fence token or as a `{` followed by an arithmetic-minus expression (or an arithmetic-minus expression followed by `}`). Resolution required either lookahead, context-sensitive lexing, or declaring the multi-character tokens with specific recognition rules — all viable but each adding implementation cost.

### 2.2 The Decision

Aggregate and Sequence literals now use `$`-prefixed fences:

| Form | Surface |
|---|---|
| Aggregate value (named) | `${field <- value, ...}` |
| Aggregate value (positional, contextually clear) | `${value, value, ...}` |
| Aggregate value (empty) | `${}` |
| Aggregate type | `${Type field, ...}` |
| Sequence value | `$[value, value, ...]` |
| Sequence value (empty) | `$[]` |
| Sequence type (fixed) | `$[N]T` |
| Sequence type (unbounded) | `$[]T` |

The `$` character does not appear elsewhere in current Basis source materials (verified by exhaustive search of all checkpoints and references); reserving it as the literal-fence prefix introduces no conflicts.

### 2.3 Rationale

**Ergonomic.** The user noted the `{-` / `-}` / `[-` / `-]` tokens are awkward to type, especially in IDE contexts where opening a fence and immediately backing up to insert a `-` is friction. `${` and `$[` are typed in a single forward motion.

**Lexer-mechanics.** The `$`-prefix renders the tokens unambiguous: `$[` and `${` are unambiguous opening tokens (since `$` has no other use), and `]` and `}` are matched to their openers via standard bracket-matching. No lookahead, no context-sensitive lexing, no special multi-character recognition rules. The original CP009 §8.1 / §8.2 grammar coverage simplifies accordingly.

**OQ-23 impact.** OQ-23 was originally registered to track the `{-`/`-}`/`[-`/`-]` lexer-disambiguation problem (CP009 §8 / OQ registry entry). With the new surface, that half of OQ-23 closes. The work plan §5.3 retains OQ-23 in the forwarded-to-implementation-thread state for the remaining `.inline`-placement question and any future small lexer-disambiguation matters that accumulate, but the original motivating problem is resolved.

### 2.4 Scope of Change

CP009 §3.3, §3.4, §4.3, §8.1, and §8.2 are revised throughout to use the new tokens. The construction reference (when drafted) will use the new tokens from inception with a *[Reconciliation: post-CP009 token surface revision per CP015 §2]* marker. No semantic content changes; the change is purely surface.

### 2.5 The Aggregate Type Form

The type form `${Type field, ...}` parallels the value form `${field <- value, ...}` with two surface differences:

- Field declarations use Basis's standard type-first order (`Type field`), not Scala-style trailing-type (`field : Type`).
- The `<-` placement operator is absent in the type form (there is no value to place; only types and field names).

This mirrors the existing pattern Basis uses elsewhere: `:` for typing in declarations, `<-` for placement at construction time. The two surfaces — type form and value form — share the visual structure but use different inner connectives, with the structural shape (curly fences, comma-separated entries, named fields) common to both.

### 2.6 The Sequence Type Form

The type forms `$[N]T` (fixed-length) and `$[]T` (unbounded) reuse the existing typed-buffer surface from the type system, prefixed with `$`. The prefix marks it as a Sequence-literal *type* rather than a buffer type (the two are different at the type level: Sequence is a literal-source type that an `.implicit` constructor matches against; `[N]T` and `[]T` are destination buffer types). At the lexer level, `$[N]T` and `[N]T` are unambiguous (the leading `$` distinguishes them); at the type-system level, the destination buffer type and the Sequence-literal type are nominally distinct.

---

## 3. The `-<` Operator's Variant Surface Extensions

### 3.1 Background

The type-system reference §3.5 introduced `-<` as the dynamic-narrowing operator, with the variant case `'narrow -< v` performing a runtime tag-check on a variant `v` and binding its candidate to a narrowed slot on success, propagating a tag-mismatch failure on type mismatch or when `v` is absent. The operator was declared with the "narrowing" framing — placing the variant's candidate into a target slot of more specific type. Object and pointer cases were also covered.

### 3.2 The New Uses

The user introduced two additional variant uses of `-<`, both employing `_` as a discard/empty marker:

| Form | Meaning | Failure shape |
|---|---|---|
| `'narrow -< v` | Narrow `v`'s candidate into `narrow` if type matches; bind on success | May-fail (tag-mismatch tag, or absent-state failure) |
| `v -< _` | Set `v` to absent state | Always succeeds (no failure path) |
| `_ -< v` | Test whether `v` has any candidate | May-fail; succeeds iff `v` is non-absent, fails iff `v` is absent |

All three forms use the same `-<` operator. The `_` token serves as the discard/empty marker on either side: write-side `_` means "no real value, set to absent"; read-side `_` means "no real target, just test for non-absence."

### 3.3 The Failure-Shape of `_ -< v`

`_ -< v` is a may-fail command, not a Boolean expression. Basis has neither Boolean values nor a ternary operator (operating principles §3.8, plus the pending §3.9 amendment recorded in §11 below). The "true/false" intuition of "does `v` have a candidate?" is carried through the failure system: the command succeeds when `v` is non-absent (no failure produced), and fails when `v` is absent (failure produced; recoverable via `?:`, `?-`, `|`, or any other failure-handling primitive per failure-system reference §2).

The design choice between the may-fail-command shape (Reading A) and a Boolean-returning expression shape (Reading B) was raised in dialog. Reading A is the orthogonal choice: it composes with the existing failure-handling primitives without introducing a new mechanism, and it is consistent with the rest of the `-<` family (which is may-fail throughout). Reading B would have introduced a Boolean primitive that Basis does not have. Reading A was chosen.

### 3.4 The Failure-Shape of `v -< _`

`v -< _` always succeeds. There is no failure path: setting a variant to its absent state is unconditionally well-formed regardless of `v`'s prior state (whether previously absent or carrying any candidate). The form is included in the may-fail-shaped family for surface consistency with the other `-<` forms; the typechecker recognizes the always-succeeds property and may, at its discretion, optimize callers who unnecessarily wrap it in failure-handling.

### 3.5 Construction Reference Coverage

The construction reference's §11 (dynamic narrowing operator) records all three variant uses plus the existing object/pointer uses. The grammar admits `_` on either side of `-<` exclusively for the variant case; using `_` on either side with non-variant operands is rejected at the type level (since the discard/empty-marker semantics is variant-specific).

---

## 4. Aggregate Literal Refinements

### 4.1 Positional vs. Named: the Contextual-Clarity Rule

CP009 §3.3 specified Aggregate literals using only the named form: `{-field <- value, ...-}` (under the old syntax) — every field-value pair carries an explicit field name. The user introduced a positional form for cases where the target type is contextually explicit:

```
${Type field declarations}     ; type form (e.g., ${Decimal x, Decimal y})
${field <- value, ...}         ; named value form (always well-formed)
${value, value, ...}           ; positional value form (only when contextually clear)
```

The well-formedness condition for the positional form is that the expected type at the literal's position fixes the field order. Contextual clarity is satisfied by:

- A typed binding LHS — `# Point p <- ${1, 2}` ✓
- A parameter position with declared structural type — `(make_point: ${1, 2})` ✓ (the parameter's declared type fixes the field order)
- An `.implicit` constructor's parameter slot at an elision site

It is **not** satisfied by:

- A bare LHS without type annotation — `# p <- ${1, 2}` ✗ (rejected; the named form `${x <- 1, y <- 2}` is required)
- Being inside a constructor body, when the aggregate's target is *not* the productive `'r` — `# bar <- ${1, 2}` inside any command body ✗

The last rule deserves emphasis: simply being inside a constructor body does not provide contextual clarity for *intermediate* aggregates in the body. Clarity comes from the LHS type (or parameter type) at the specific construction site, not from the surrounding command's productive obligations.

```
.cmd makeFoo : Foo 'r =
    # bar <- ${1, 2}             ; ✗ rejected — bar's type not contextually fixed
    # Bar bar <- ${1, 2}         ; ✓ accepted — Bar fixes the field order
    # bar <- ${x <- 1, y <- 2}   ; ✓ accepted — named form synthesizes structural type
    'r <- ${1, 2}                ; ✓ accepted — 'r has declared type Foo
```

### 4.2 Variant-Absent Fields: Explicit `_` Required

The interaction between Aggregate literals and variant-typed fields raises the question of how the absent state is expressed when a field has variant type. Three options were considered:

| Option | Description |
|---|---|
| (a-1) | Omit the variant field; absent is the implicit default. |
| (a-2) | Require explicit `_` for absent; omission is rejected. |
| (a-3) | Explicit by default, but the type declaration can opt in to (a-1). |

The user chose **(a-2)**: variant-typed fields in Aggregate literals require explicit `_` (in either named or positional form); omission is rejected.

**Rationale.** The motivation is preserving positional-transcription integrity. Under (a-1), positional literals with omitted variant fields would have ambiguous field-to-value correspondence — a literal `${1, 2, 3}` for a record with mixed variant and non-variant fields could parse multiple ways depending on which positions are "absent variant" vs. real values. Under (a-2), the explicit `_` ensures that positional literals always have a clean one-to-one correspondence between syntactic positions and declared fields:

```
# Person p <- ${name <- "Alice", age <- 30, role <- _}    ; named, role absent
# Person p <- ${"Alice", 30, _}                            ; positional, role absent
```

The `_` token's other uses (parameter-discard in patterns, write-side and read-side variants of `-<` per §3) coexist; within Aggregate literals the only supported `_` use is "absent variant value," and the meaning is unambiguous from context.

**Refinement to CP009 §3.3.** CP009 §3.3 said "a field is required unless the lhs type's declaration provides a default for that field." Under (a-2), variant-typed fields are *always* required (with `_` as the absent value), independent of whether the type declaration provides a default. Non-variant fields retain CP009 §3.3's rule unchanged.

### 4.3 The Aggregate Type Form (Reiteration)

For completeness alongside the value-form rules: Aggregate type forms (used in `.implicit` constructor parameter slots and similar positions) use the structural type syntax `${Type field, Type field, ...}` per §2.5. Note the type-first order (`Type field`), not Scala-style `field: Type` — operating principles §3.9 (pending) codifies this discipline.

---

## 5. Union → Candidate-or-Parent Byte-Reinterpretation

### 5.1 Background and Reframe

The type-system reference §2.6 declared that reading a candidate from a union is an *interpretive cast*, with the cast operator's surface forwarded to the construction reference. The original conception was a dedicated cast operator in the `<-`-family (e.g., a hypothetical `<<-`) that the user would invoke explicitly to assert reinterpretation.

The user introduced a substantive overnight reframe: the C-language-style approach is preferred. Union access works by *implicit subsumption* from the union to whichever candidate type the bytes are being interpreted as — no dedicated operator. The reinterpretation is dangerous (the bytes might not be semantically valid as the target candidate) but this is a low-level construct, and discipline is the programmer's responsibility. The user's framing: "this 'bright line' standard is the only way to avoid injecting complications into the language that don't really improve the situation that much."

### 5.2 The Subsumption Rule

A union value implicitly subsumes (zero-cost, byte reinterpretation) to any buffer-backed type T such that T appears on the standard subsumption chain of *at least one* declared candidate.

The rule is **existential, not universal.** For a union `U` with candidates `C₁, C₂, ..., Cₙ`, T is a valid subsumption target iff T is on the upward-subsumption chain of at least one `Cᵢ`. T need not be on every candidate's chain.

**Concrete example.** A union `IntOrLong` with candidates `Int32 i, Int64 j`:

| Target T | Valid? | Reason |
|---|---|---|
| `Int32` | ✓ | Declared candidate (trivially on its own chain) |
| `Int64` | ✓ | Declared candidate |
| `[4]` | ✓ | Parent of `Int32` |
| `[8]` | ✓ | Parent of `Int64` |
| `[]` | ✓ | Parent of every buffer-backed type |
| `[6]` | ✗ | On no candidate's chain |
| `[7]`, `[16]`, etc. | ✗ | On no candidate's chain |
| `Inches` (sibling of `Int32`) | ✗ | Sibling of a candidate, not a parent |
| Some other record over `[8]` | ✗ | Sibling of `Int64`, not a parent |

The existential rule is what makes `[4]` valid for the Int32/Int64 union: `Int32`'s chain reaches `[4]` (even though `Int64`'s does not). The user's articulation: "If the union contained an Int32 and an Int64, one would expect either a [4] or an [8] to subsume correctly, but subsuming to [6] would be invalid."

### 5.3 Read Direction: Implicit, Infallible

The union-to-target reinterpretation is implicit (no operator required) and infallible at the language level. The typechecker enforces the bright line — T must be on at-least-one candidate's chain — and nothing more. The bytes' actual interpretation, when read, is the user's responsibility:

- Whether the union currently contains a value of the candidate type T (or a parent on T's chain) is not language-tracked.
- Whether the bytes form a semantically valid value of type T (e.g., a valid Float32 vs. NaN-or-trap pattern, a valid Inches value rather than a sentinel) is not language-tracked.

The user accepts this responsibility as part of choosing the union (vs. a variant, which carries a tag and is language-checked).

### 5.4 Write Direction: Explicit via Aggregate Literal

Placing a value *into* a union is explicit, via an Aggregate literal naming the candidate:

```
# IntOrLong u <- ${i <- 42}    ; explicitly placing 42 into the Int32 candidate
```

CP009 §3.3's single-candidate Aggregate-literal form (now in `$`-syntax) is the canonical write surface. There is no symmetric "implicit downward write" — writes require a candidate-specifier because they are placements, not reinterpretations. The asymmetry is intentional: reads are byte-reinterpretation (the bytes are already there), but writes determine which candidate the bytes will be interpreted as, which the language requires the user to specify.

### 5.5 No `-<` on Unions

Type-system §3.5 already excludes unions from the `-<` operator's domain. The new byte-reinterpretation framing reinforces this: unions have no runtime tag, so a runtime narrowing check has nothing to check; the implicit subsumption supersedes any need for a runtime-checked narrowing form. Variants retain `-<` (with the new `_`-marker extensions per §3); the divide between unions (untagged, byte-overlay) and variants (tagged, language-tracked) is preserved cleanly.

### 5.6 Terminology

The byte-reinterpretation relation is *not* a Liskov-preserving subsumption: a value of the union type, when reinterpreted as a candidate type, does not necessarily satisfy the candidate's invariants (the bytes might be from a different candidate or unset). Calling this "subsumption" is a slight extension of the term's standard meaning. The construction reference distinguishes it as **byte-reinterpretation subsumption** to flag the non-Liskov property at the docs level. This terminology question prompted the standard-terminology principle recorded in §11 below.

### 5.7 Cross-Document Impact

The reframe requires substantive revisions to material outside the construction reference:

- **Type-system §2.6** — replace "interpretive cast operator forwarded to construction reference" framing with the byte-reinterpretation subsumption rule. The type-side commitment that the typechecker checks T is on at-least-one candidate's chain is preserved (and sharpened — the rule is now precise where §2.6 was qualitative).
- **Type-system §3** (subsumption-relations description) — add the union → candidate-or-parent byte-reinterpretation as a distinct kind of subsumption, alongside the Liskov-preserving upward subsumption that §3 currently describes. User-responsibility for semantic validity is part of the description.
- **Operating principles §1.5** — add a sentence acknowledging that unions admit downward byte-reinterpretation distinct from the Liskov-preserving upward subsumption. (Recorded as a pending OPS update; see §11.)

---

## 6. Pattern-Matching: Composition, Not New Surface

### 6.1 Background

The work plan's §3.4 question 4 asked whether pattern-matching surface forms are owned by the construction reference, owned by a future dedicated reference, or absent (composition only). Three positions were considered:

| Position | Description |
|---|---|
| A | Compose existing primitives (`?:` chains + `-<` + failure-handling); no new keyword surface. |
| B | Commit a dedicated `match`-style surface here (substantial new design). |
| C | Defer entirely to a future "pattern-matching" reference. |

### 6.2 The Decision: Position A

The user chose **Position A**. Variant case analysis composes via `?:` chains + `-<`, with `?-` for explicit absent-only tests. The construction reference's §9 (variant construction) and §11 (`-<` operator) record the canonical composition idioms with worked examples. No new keyword surface is introduced.

### 6.3 The Canonical Idiom

The general form, with explicit absent-handling:

```
?- _ -< v
   handle_absent
?: 'narrow_a -< v
   handle_a
?: 'narrow_b -< v
   handle_b
handle_unmatched_candidate
```

This is a `?-` block (testing `_ -< v`, which fails when `v` is absent — the `?-` body engages on failure, so `handle_absent` runs when `v` is absent) followed by a `?:` chain over narrowing attempts. Each `?:` arm engages when its narrowing succeeds (i.e., when `v`'s active candidate matches the arm's narrowed type). The non-`?:` sibling after the chain is the default arm, engaging when none of the `?:` guards succeeded — which, given the absent-case is already handled by the leading `?-`, means "v is non-absent and not a match for any of the listed candidates."

The expected pattern is that each handler either recovers the failure or terminates its case (returns, fails, branches). If a handler does neither, the typechecker may warn about unreachable code in subsequent siblings of the chain.

The form remains valid when the `?-` absent-test is omitted; in that case the default arm engages on either "absent" or "unmatched candidate," and the user discriminates within if needed. The leading `?-` is the explicit form for cases where absent gets distinct handling.

### 6.4 Enum-and-Match for Unions

Unions, lacking runtime tags, do not use `-<` for case analysis (per §5.5). The "enum-and-match" idiom referenced in type-system §2.6 / §3.4 is shorthand for the user-side discrimination idiom: store an enumeration value alongside the union in a containing record, dispatch on the enum value via a `?:` chain, and within each arm interpret the union's bytes as the candidate the enum value indicates. This is composition of existing primitives — not a planned dedicated mechanism.

The phrase "enum-and-match" in the type-system reference will be sharpened in a future pass to clarify it denotes this user-side composition idiom rather than a language feature. (Recorded in the work plan's §6 pending consolidations.)

### 6.5 Rejected Alternatives

**Position B** was rejected because committing to a new pattern-matching surface would require substantive new design dialog inside the construction-reference work, expanding scope significantly. No checkpoint had committed any specific match syntax; introducing one here would have stretched the reference into territory better handled in a dedicated session if pattern-matching ever warrants its own surface.

**Position C** was rejected because it would leave a gap — readers of the construction reference would find no canonical case-analysis idiom anywhere. Position A fills the gap with composition rather than new surface, which is the orthogonality-respecting default per operating principles §1.8.

### 6.6 A Drift Incident

During the dialog, a sketched composition idiom used `?-` chained with `|` in a pattern that visually resembled a ternary `cond ? a : b ? c : d` chain — but the construct was not valid Basis. Three errors stacked:

1. `?-` engages on failure, not success — wrong primitive for matching the "type matches" case.
2. `|` is failure recovery, not n-ary alternation — wrong primitive for chaining branches.
3. The overall ternary-chain shape doesn't exist in the language at all.

The correct primitive for n-ary alternation is the `?:` chain (failure-system reference §2.2). This drift incident motivated extending the operating principles §3.9 amendment (originally just covering Scala-style type-annotation drift) to also cover ternary-shaped construction drift. See §11.

---

## 7. OQ-22 Resolution: `.implicit` Constructor Type Forms

### 7.1 The Question

OQ-22 asked: what is the syntactic form for declaring an `.implicit` constructor whose parameter is a *structurally-typed* Aggregate or Sequence literal? CP009 §4.3 left placeholder forms with `Aggregate{...}` and `Sequence{...}` named-constructor type syntax.

### 7.2 Options Considered

| Option | Aggregate Type | Sequence Type |
|---|---|---|
| A | `${Type field, ...}` (mirrors value form) | `$[N]T` (mirrors typed-buffer surface) |
| B | `Aggregate{Type field, ...}` (named-constructor type form, CP009 placeholder) | `Sequence{T, length: N}` |
| C | Defer | Defer |

### 7.3 The Decision: Option A

The user chose **Option A**. `.implicit` constructor parameter types use type-form mirrors of the value forms:

```
.implicit Point 'r : ${Decimal x, Decimal y} a = 'r <- a
.implicit Vec3  'r : $[3]Decimal v = ...
```

The Aggregate type form `${Type field, ...}` parallels the value form `${field <- value, ...}` by swapping `<-` (placement) for the field-declaration order (Type name). The Sequence type form `$[N]T` reuses the existing typed-buffer surface, prefixed with `$` to mark it as a Sequence-literal type rather than a destination buffer type.

### 7.4 Length Syntax for Sequence Types

The length is positional (`$[3]Decimal`), not keyword (`$[length: 3]Decimal`). This reuses the existing syntactic pattern of typed buffers, where the length appears in brackets directly. The keyword form was considered for discoverability but rejected for verbosity and inconsistency with the established surface.

### 7.5 Rejected Alternatives

**Option B** was rejected because it introduces a new "named-constructor type form" (`Aggregate{...}`, `Sequence{...}`) that is otherwise not part of Basis's type-language. Option A reuses surfaces that already exist in different but related contexts (Aggregate value form, typed-buffer length-bracket form), which is more economical.

**Option C** (defer) was rejected because OQ-22 was specifically forwarded to the construction reference for resolution; deferring further would have left the `.implicit` mechanism incomplete in the reference.

### 7.6 A Side Effect: Type-Form Coherence

The Option A choice has an additional payoff for the type system as a whole: the Aggregate value form and type form share the same visual structure (curly fences with comma-separated entries), and the Sequence value form and type form share the same visual structure (square brackets with content). Readers can grasp the type forms by analogy to the value forms with one small surface adjustment (drop `<-`, add type names; or for Sequence, prepend `[N]`).

---

## 8. OQ-13 Resolution: Implicit Context Parameters and Construction

### 8.1 The Question

OQ-13 was open and forwarded jointly to the construction reference and the future class-system reference. The construction-side question: when a constructor takes implicit context parameters, how do those parameters interact with the construction story — particularly with `.implicit` constructors, the write-once productive parameter rule, and failure atomicity?

### 8.2 Positions Considered

| Position | Description |
|---|---|
| 1 | Resolve the construction-side fully here; class-system uninvolved. |
| 2 | Resolve only the mechanical parts here; forward context-parameter resolution-at-call-site to class-system reference. |
| 3 | Forward entirely to class-system. |

### 8.3 The Decision: Position 1

The user chose **Position 1**, with the framing that OQ-13 is a parameter-passing question, not a class-dispatch question. Implicit context parameters work via type-based uniqueness from the caller's lexical scope (CP001 §3.5, op-sem §5.5); this mechanism is parameter-passing machinery, distinct from the class system's instance dispatch. The construction reference encodes the full mechanics.

### 8.4 The Resolved Mechanics

The construction reference's §13 (construction-side context parameters) records:

- **Mechanism unchanged.** Context parameters are declared with the `/` separator in command signatures; the typechecker resolves them at the call site by type-based uniqueness from the caller's lexical scope (per CP001 §3.5 / op-sem §5.5). Ambiguity (multiple visible candidates of the same type) and absence (no visible candidate) are both compile-time errors.

- **Init state at resolution site.** The auto-supplied value must be `init` at the resolution site. Standard parameter-passing rule applies; no special case for implicit resolution.

- **Mode and taint.** The receiving parameter's declared mode (READ / PRODUCE / REFERENCE per CP013) determines passing; the supplied value's access-path taint propagates per the mode, exactly as for explicitly-passed parameters.

- **Mode permissibility.** All three modes (READ / PRODUCE / REFERENCE) are permitted on implicit context parameters. PRODUCE is rare in practice (back-channel mutation via implicit resolution is unusual), but it is not prohibited. The rule is "allow unless incoherent" — and PRODUCE on a context parameter is not incoherent, merely unusual.

- **`.implicit` exclusion (hard restriction).** `.implicit` constructors do not admit `.context` parameters. The grammar prohibits passing context variables at `.implicit` constructor invocations, and the semantic analyzer flags any such attempt as an error. (Equivalently, since `.implicit` constructors cannot use context parameters, declaring `.context` on an `.implicit` declaration is ill-formed; both the declaration and any invocation are rejected.) Rationale: `.implicit` is a single-implicit elision mechanism for literal-to-type ergonomics. Allowing `.context` on top would compound implicit invocation with implicit parameter resolution — exactly the "Scala-implicit chaos" CP009 §4.2's three restrictions exist to prevent. The user's framing: "implicit constructors exist solely for ergonomic conversion from literals to proper types, and we do not want to open Pandora's box."

- **Defaults and context.** A `=` default declaration is resolved at declaration scope, where caller's lexical context is unavailable (there is no caller). Therefore, a default's rhs cannot use implicit-resolution for context parameters. Instead, any context parameter required by a constructor in a default rhs must be supplied **explicitly** via `/`. The explicit `/` form is universally valid as a fallback (just as it is the fallback when implicit resolution would have multiple candidates or no candidate). This situation should be uncommon in practice but is always available.

- **OQ-21 (lambda capture-list interaction with context) remains forwarded** to the lambda-and-fexpr reference — it concerns capture mechanics rather than construction.

### 8.5 Class-System Uninvolvement

Class-system reference has no involvement with OQ-13. The op-sem reference's §5.5, which currently forwards OQ-13 to "the class-system reference," will be updated in a future pass to redirect to the construction reference. (Recorded in the work plan's §6 pending consolidations.)

### 8.6 Rejected Alternatives

**Position 2** was rejected because it relied on a distinction between "mechanical parts" and "resolution at call site" that doesn't survive scrutiny: the mechanical parts (declaration syntax, write-once, failure atomicity) are nearly trivial given the rest of the construction story, while the resolution mechanism is parameter-passing machinery (already settled in CP001 §3.5 and op-sem §5.5), not class dispatch. There is no separable "hard part" to forward.

**Position 3** was rejected because it would leave a hole in the construction reference where readers reasonably expect a short treatment, and because the framing as a class-system matter is itself a category mistake.

---

## 9. OQ-24 Resolution: `.implicit` Phasing as Implementation Latitude

### 9.1 The Question

OQ-24 asked about the phasing of `.implicit` Aggregate/Sequence support: whether the language commits to full Aggregate-shape and Sequence-shape `.implicit` constructors from the start, or whether initial implementations may support only simple literal types and add structural-shape support later.

### 9.2 The Decision

CP009 §4.3's design is committed (Aggregate-shape and Sequence-shape `.implicit` constructors are part of the language; structural matching against literal types is part of the typechecker's responsibilities). The phasing is treated as **implementation latitude**: an initial implementation may support only simple literal types and add Aggregate/Sequence support when structural-matching machinery is ready, without that constituting a language-spec change. The construction reference does not legislate phasing.

### 9.3 Closure

OQ-24 is resolved to the extent the construction reference is concerned. The implementation decision (when to add Aggregate/Sequence support) belongs to the implementation thread.

---

## 10. New Open Questions Registered

Two new open questions surfaced during the scope-confirmation dialog. Both are deferred to separate design dialogs.

### 10.1 OQ-30 — Heap Allocation Language-Level Mechanism

**Statement.** The `#` prefix operator allocates a slot in the calling frame, suitable for small local values. Larger values would benefit from heap-based storage, but no language-level mechanism for heap allocation exists yet.

**Tension.** The user explicitly wants to keep the language and the standard library orthogonal — to avoid Java's flaw of welding the language spec to its library spec. A general allocation mechanism that doesn't require some level of library intrinsics (Zig-style allocators, for example) is not yet apparent.

**Candidate directions sketched in dialog.**

- **Region-style allocation tied to lexical scope.** Operating principles §1.10 already flags this as a latent constraint of the design; a region/scope-bound allocator could fit naturally.
- **Allocation-via-class-dispatch.** Push allocator selection to the class system: an allocator type with a single `allocate` method, dispatched on the type of the allocator parameter; the standard library provides allocators but the language semantics depend on no specific library type. This avoids library intrinsics by making allocation a method dispatch on a user-provided type.
- **Small-primitive-set.** One or two intrinsics (e.g., `.intrinsic alloc`, `.intrinsic free`) that the standard library wraps but doesn't depend on for *language* semantics. The language would have a minimal heap-allocation primitive; everything ergonomic is library-level.

**Status.** Resolution requires a separate design dialog. The construction reference notes the gap with a "see OQ-30" cross-reference where heap-allocation would be relevant; if mid-draft a section discovers it cannot proceed without OQ-30 resolution, the drafting session should flag this rather than improvise.

### 10.2 OQ-31 — Tuple-Style Positional Access via `Bar::N`

**Statement.** Aggregate values are positionally ordered by their declared field order (per CP006 layout discipline; recorded in the construction reference's positional/named rule per §4.1). The proposal: extend `::` (the scope operator) to admit an integer literal as its rhs, so `Bar::1` denotes the first positional field of a `Bar`-typed aggregate, `Bar::2` the second, and so on. The relaxation would make any aggregate accessible as a tuple without giving up the safety the typed-LHS-fixes-order rule provides.

**Open coherence questions.**

- **Lexer / parser.** The rhs of `::` would need to admit numeric literals; today it admits identifiers. The grammar production for `::` widens.
- **Subsumption interaction.** Records subsume to `[N]` (their buffer-backed parent), where positional access is array indexing, not `::`. So `record::1` and `(record as [N])[0]` are different surface forms despite touching the same byte. Acceptable, but worth recording explicitly.
- **Range checking.** `Bar::N` where N exceeds Bar's field count is a compile-time error (the typechecker knows Bar's declared field count).
- **Scope of relaxation.** Does the access form apply to all aggregate-typed values (every record, every variant with positional candidates), or only to types explicitly marked as tuple-shaped? Records already have positional layout per CP006; restricting `::N` access to a tuple-marked subset would be artificial.
- **Interaction with `.inline` records.** A flattened-into-parent inline record's positional indices either continue the parent's numbering, or remain local to the inline. The choice affects `Bar::N` semantics for records with inline children.

**Status.** Resolution deferred. The construction reference's open-questions section records the proposal; resolution will require a separate design dialog (or fold-in to a future related reference).

---

## 11. Operating-Principles Updates Pending

The 2026-05-03 dialog produced three pending updates to `project-operating-principles.md`. Per that document's §5 protocol (rare, deliberate updates), they are batched here and applied in a single future pass.

### 11.1 New §3.9 — Mainstream-Language Syntactic-Shape Drift

The operating-principles document's §3 catalogues recurring drift patterns. The existing entries (§3.1 through §3.8) cover semantic drift toward mainstream-language reasoning (return values, NULL, frame-ownership, etc.). The dialog surfaced two recurring *syntactic* drift patterns not yet codified:

1. **Type-annotation order.** Basis is `Type name`, not `name : Type`. The colon-with-trailing-type form is Scala / TypeScript / Rust / Python-typing syntax and does not appear in Basis. Parameter declarations, field declarations, and structural type forms (e.g., `${Decimal x, Decimal y}`, not `${x : Decimal, y : Decimal}`) all read type-first. The slip recurred multiple times during the dialog despite mid-dialog corrections.

2. **Ternary-shaped construction.** Basis has no ternary operator. The absence extends to ternary-shaped *constructions* — chains of `?-` + `|` that visually resemble `cond ? a : b ? c : d` are not valid Basis even when each individual primitive is. The `?:` block-marker is the proper Basis primitive for n-ary alternation; failure-recovery `|` is for failing paths only. This drift surfaced during the pattern-matching discussion (§6.6).

The §3.9 amendment will codify both, encouraging future sessions to recognize and avoid the slips.

### 11.2 New §2.X — Standard-Terminology Guideline

When a term in user-supplied phrasing diverges from the standard meaning, the divergence is itself a likely signal: either the user's terminology is off, or there is an inconsistent understanding between user and Claude. Either is worth verifying. The guideline: **use standard terminology; surface suspected nonstandard usage as a question rather than translating silently.**

The principle surfaced when the user described the union → candidate-type-byte-reinterpretation as "subsumption." Standard "subsumption" preserves Liskov substitutability (a value of the subtype can be substituted for a value of the supertype with semantics preserved). The union → candidate reinterpretation does not — the bytes are the same, but their *meaning* depends on which candidate is currently active, which the language doesn't track. Flagging the divergence rather than silently translating allowed the user to confirm which meaning was intended (the C-style byte-reinterpretation, with the user-responsibility caveat) and to refine the construction reference's terminology accordingly. The user's standing instruction in response: "Always, please always use standard terminology and suggest corrections like this where likely."

### 11.3 §1.5 Refinement — Union Byte-Reinterpretation

Operating principles §1.5 currently describes uniform upward subsumption for buffer-backed types (every buffer-backed type subsumes up its parent chain). The principle implies one-directional subsumption. Unions, under the byte-reinterpretation rule (§5), additionally admit downward reinterpretation to any type on at-least-one declared candidate's chain. The §1.5 refinement adds a sentence acknowledging this:

> Unions additionally admit downward reinterpretation to any buffer-backed type appearing on the standard subsumption chain of at-least-one declared candidate — the **byte-reinterpretation** relation, distinct from the Liskov-preserving upward subsumption. The reinterpretation is implicit and language-infallible; semantic validity is the user's responsibility. See construction reference §10.

---

## 12. Summary of Changes from Prior Checkpoints

| Topic | Prior State | This Checkpoint |
|---|---|---|
| Aggregate/Sequence literal fence tokens | CP009 §3 / §8: `{- -}` and `[- -]` with deferred lexer disambiguation (OQ-23) | **Revised**: `${ }` and `$[ ]` (§2). OQ-23's lexer-disambiguation half closed. |
| `-<` operator surface | Type-system §3.5: `'narrow -< v` for variants; object/pointer cases | **Extended**: `v -< _` (always-succeeds clear) and `_ -< v` (may-fail non-absent test) added (§3) |
| Aggregate literal positional form | CP009 §3.3: named-only | **Refined**: positional admitted when target type is contextually explicit; named otherwise (§4.1) |
| Variant fields in Aggregate literals | CP009 §3.3: a field is required unless the type provides a default | **Refined**: variant fields are *always* required, with `_` as the absent value (§4.2) |
| Union candidate access surface | Type-system §2.6: "interpretive cast operator" forwarded to construction reference | **Replaced**: implicit byte-reinterpretation subsumption to candidate-or-parent types; no operator (§5) |
| Pattern-matching | Implicit (compose existing primitives); no checkpoint commitment | **Confirmed**: composition via `?:` chains + `-<`; no new keyword surface (§6) |
| OQ-22 (parameterized `.implicit` types) | Open; CP009 §4.3 placeholder syntax | **Resolved**: `${Type field, ...}` and `$[N]T` mirroring value forms (§7) |
| OQ-13 (context params and init) | Open; forwarded jointly to construction + class-system | **Resolved**: construction-side fully here; class-system uninvolved; `.implicit` excludes `.context` (§8) |
| OQ-24 (`.implicit` phasing) | Open; CP009 §4.3 noted as deferred | **Resolved**: phasing is implementation latitude (§9) |
| **OQ-30** | Did not exist | **Newly registered**: language-level heap allocation mechanism. Deferred. (§10.1) |
| **OQ-31** | Did not exist | **Newly registered**: tuple-style positional access via `Bar::N`. Deferred. (§10.2) |
| Operating principles updates | n/a | Three pending: §3.9 (new), §2.X (new), §1.5 (refinement). Batched. (§11) |

---

## 13. Provenance

**Authored:** Distilled from the intent dialog between JimDesu and Claude (Opus 4.7) on 2026-05-03, in the construction-reference scope-confirmation thread following the 2026-05-02 completion of the type-system reference.

**Source materials read for this checkpoint:** `project-operating-principles.md` (lenses); `project-work-plan.md` (§3.4 scope-confirmation questions, §5 OQ registry, §6 pending consolidations); `intent-checkpoint-009.md` (densest source for the construction surface — `<-` polymorphic RHS, Aggregate/Sequence literals, `.implicit`, `=` defaults, OQ-10, OQ-22 placeholder, OQ-23 origin); `reference-failure-system.md` §§2 (block-marker constructs and failure-handling primitives, in service of the pattern-matching idiom verification); `reference-type-system-and-modes.md` §§2.6 (union OQ-1 resolution and interpretive-cast forwarding, since reframed), 3.4 (variant 3-word slot), 3.5 (`-<` operator declaration, since extended), 7.12 (OQ-23 forwarding); `reference-operational-semantics.md` §5.5 (implicit context parameter resolution mechanism); `intent-checkpoint-014.md` (format conventions for this checkpoint).

**Grammar changes implied:**

- New literal-fence tokens `${`, `}`, `$[`, `]` (with `}` and `]` already-existing tokens; the `$`-prefix forms are the new openers).
- Positional Aggregate-literal form `${value, ...}` admitted under the contextual-clarity rule.
- `_` admitted as a discard/empty marker on either side of `-<` for variant operands.
- `_` admitted as the absent value for variant-typed fields in Aggregate literals.
- `${Type field, ...}` and `$[N]T` admitted as type forms in `.implicit` constructor parameter slots and other structural-type positions.
- Grammar prohibits `.context` parameters on `.implicit` constructor declarations.

**Reference-document impact:**

- The construction reference (forthcoming) encodes all eight amendments and both new OQs from inception, with reconciliation markers citing this checkpoint.
- The type-system reference §§2.6 and 3 will be revised in a future consolidated pass to fold in the byte-reinterpretation reframe (§5.7).
- The op-sem reference §5.5 will be revised to redirect OQ-13's class-system forwarding to the construction reference (§8.5).
- The work plan's §3.5 / §3.6 record the same commitments; this checkpoint is the dialog-record companion to that authoritative tracking.

**Recommended next step:** Resume topic-organized reference work in a fresh session with full context budget. The next session drafts the **Construction and Initialization** reference per the section structure recorded in work plan §3.4, encoding the eight amendments (§§2-9 of this checkpoint) and the two new OQs (§10) from inception. The drafting session should also keep §11 (pending OPS updates) visible — those updates, along with the cross-document consolidations recorded in work plan §6, are batched for application in future passes rather than this one.
