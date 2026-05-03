# Basis Language — Intent Checkpoint 006

**Status:** Working draft. Builds on intent-checkpoint-001.md through intent-checkpoint-005.md; supersedes them where they conflict.
**Date of this checkpoint:** April 2026
**Provenance:** Continuation of the intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-27. This checkpoint elevates "the fundamental datatype is a buffer" from a guiding principle into a structural commitment with explicit type-system consequences, unifies records/domains/unions under the umbrella of buffer-backed types, resolves OQ-9 in favor of whole-slot initialization tracking, and establishes the construction pattern "compute then atomic-initialize" that follows from these decisions. OQ-10 (composite initializers) is now load-bearing for construction and is the natural next thread.

**Authority statement.** Where this document differs from earlier checkpoints, this document is authoritative. Where this document and the source code differ on syntactic matters, the code is authoritative; where they differ on semantic intent, this document is authoritative. The README and `language_notes.txt` remain non-authoritative.

---

## 1. The Buffer-Backed Principle, Made Structural

The README's design principle "the fundamental datatype is a buffer" has been treated, until this checkpoint, as a guiding commitment rather than a structural one. This checkpoint elevates it: the principle is now a **fundamental, load-bearing constraint on the type system**, with explicit consequences for which types may appear in which positions.

### 1.1 The Two-Layer Split

Every type in Basis falls into exactly one of two categories:

**Buffer-backed types.** Types whose representation reduces — transitively — to bytes. They are value-like, byte-copyable, can be sliced and laid out as bytes, and can sit inside other byte-aggregates. They include:

- Bracket forms `[N]` and `[]` (the buffer primitive — fixed-size and unbounded byte storage).
- Bracket forms `[N]T` and `[]T` (typed views — buffers of T-values, where T is itself buffer-backed).
- Plain domains (named refinements over a buffer or another domain).
- Records (named refinements over a buffer with named field-offset structure).
- Unions (byte-overlays with a discriminator).

**Non-buffer types.** Types whose representation includes references, identity, dispatch information, or other non-byte semantics. They are reference-like or higher-order. They include:

- Pointers (`^T` for any T) — references to slots; carry target identity, not byte aggregate.
- Command-typed values (`:<...>`, `?<...>`, `!<...>`) — first-class commands carrying dispatch and (for block-quotes) capture information.
- Objects — identity-bearing aggregates with potentially non-contiguous storage; may transitively contain non-buffer fields.
- Variants — reference-semantics tagged sums whose candidates may be arbitrary types.

This split is fundamental. It governs where each kind of type may appear, how each kind of type is initialized, and how each kind of type interacts with the storage model.

### 1.2 The Containment Rule

The buffer-backed principle imposes a strict containment rule:

**A buffer-backed type may contain only buffer-backed types.**

Specifically:

- A record's fields must all be buffer-backed types. A record may not contain a pointer, a command-typed value, an object, or a variant.
- A union's candidates must all be buffer-backed types.
- A typed buffer `[N]T` requires T to be buffer-backed.
- A domain's parent must be buffer-backed (already required: the parent must reduce to a buffer or another domain).

Non-buffer types may appear only at top-level positions or as fields of objects:

- A top-level slot (introduced via `#` or as a parameter or receiver) may hold a buffer-backed type or a non-buffer type.
- An object's fields may hold a buffer-backed type or a non-buffer type.

This is the rule that makes the buffer-backed principle actually mean something. Without it, a record-containing-a-pointer would punch a hole in the byte-aggregate semantics; with it, every record's bytes are unambiguously a byte-aggregate, and any non-byte semantics is confined to top-level slots and object fields.

If a record needs to refer to other storage indirectly, it does so by **storing offsets, not pointers.** An offset is just bytes; a pointer is a reference. The two have different meanings; only the former is permitted in records.

### 1.3 The Grammar Already Enforces This

Direct inspection of `Grammar2.cpp` (master branch) confirms that the buffer-backed containment rule is already enforced at the grammar level:

- `DEF_RECORD_FIELD` uses `TYPE_EXPR_DOMAIN` for field types.
- `DEF_UNION_CANDIDATE` uses `TYPE_EXPR_DOMAIN` for candidate types.
- `TYPE_EXPR_DOMAIN` admits only `DEF_INLINE_RECORD`, `DEF_INLINE_UNION`, `TYPE_NAME_Q` (named domain or record), and `TYPE_EXPR_VECTOR_FIXED` followed optionally by another `TYPE_EXPR_DOMAIN`.
- `TYPE_EXPR_DOMAIN` does **not** admit `TYPE_EXPR_PTR`, `TYPE_EXPR_CMD`, `DEF_INLINE_OBJECT`, or `DEF_INLINE_VARIANT`.

Object fields use `TYPE_EXPR` (which admits everything), and variant candidates use `TYPE_EXPR` for the same reason. The grammar's two-tier `TYPE_EXPR_DOMAIN` / `TYPE_EXPR` distinction corresponds exactly to the buffer-backed / non-buffer split.

No grammar changes are needed to enforce the buffer-backed principle. The grammar is correct as-is on this point.

### 1.4 Records Are A Form Of Domain

A consequence of the buffer-backed principle: **records and domains are the same fundamental concept, surfaced under different declaration forms**. A domain is "a value-like nominal type over a buffer." A record is "a value-like nominal type over a buffer, with named field-offset structure." The structural relationship is parent-child within the same conceptual layer, not two distinct layers.

This was not articulated in checkpoint 001 but is fundamental to the language. The grammar's separate `DOMAIN`, `RECORD`, and `UNION` declarations are surface conveniences for declaring different shapes of buffer-backed types:

- `DOMAIN` declares a buffer-backed type with no field structure (or whose structure is whatever the parent provides).
- `RECORD` declares a buffer-backed type with named field-offset structure.
- `UNION` declares a buffer-backed type as a discriminated overlay.

All three produce buffer-backed types. All three may serve as record fields, union candidates, or domain parents. All three follow the same nominal-typing rules (per checkpoint 003 §1, all three are nominally typed). The conceptual unity is the load-bearing fact; the surface declarations are conveniences.

Going forward, this document uses **"buffer-backed types"** as the umbrella term and the specific declaration form (domain, record, union) where the surface distinction matters.

---

## 2. Resolution of OQ-9: Whole-Slot Initialization Tracking

OQ-9 asked whether the typechecker should track initialization at the field level (per-field state for compounds) or at the whole-slot level (one bit per slot, compounds initialized atomically).

This checkpoint resolves OQ-9 in favor of **whole-slot tracking only**, with atomic initialization as the construction mechanism.

### 2.1 The Decision

Every slot has a single initialization state: `init` or `uninit` (with `uncertain` at control-flow joins, treated as `uninit` for read-checking). Compounds (records, objects, unions, variants) are slots like any other. There is no per-field state for compounds.

The lattice from checkpoint 004 §3 is unchanged. The CFG-walking analysis is unchanged. The integration with failure-mode tracking is unchanged. Field-level tracking is simply not introduced.

### 2.2 Why Whole-Slot Tracking Suffices

The buffer-backed principle and the at-stack lifecycle model already commit Basis to the discipline that compound construction is atomic. A record-as-domain is bytes; either all the bytes are validly populated, or they are not. An object is an identity-bearing aggregate; either it has been constructed, or it has not. There is no coherent notion of "this record is half-initialized" or "this object exists but only some fields are valid" — both would violate the language's broader principles.

Field-level tracking would make sense in a language where partial-construction states are valid intermediate forms (C++ constructors are like this; Rust's stack-resident structs are like this). Basis is not such a language. Mutation either succeeds fully or fails fully (per checkpoint 001); a partial state cannot exist by design.

Given this, the tracking mechanism that matches the design is the simpler one. Whole-slot tracking enforces the atomic-construction discipline by construction.

### 2.3 The Construction Story

Construction of a buffer-backed type or an object proceeds in two phases:

**Phase 1: Compute the constituent values.** The constructor body invokes whatever commands are needed to produce the values that will populate the new instance. Each computed value is held in some slot — typically a parameter, a local introduced via `#`, or the result of a may-fail command in expression-position. These slots are tracked individually under whole-slot rules; their initialization states are independent.

**Phase 2: Atomic initialization of the receiver.** The constructor body performs a single conceptual write to its productive receiver `'r`. This single write fills the entire `'r` slot. Before this write, `'r` is uninitialized; after this write, `'r` is initialized; there is no intermediate state.

The mechanism for the atomic write is the subject of OQ-10 (composite initializers, `=` vs. `<-`). Whatever OQ-10 settles on becomes the syntax that produces the right-hand-side of this single write.

### 2.4 What This Means For Constructors

A constructor `T 'r : Bar a, Baz b = ...` has a body whose final operation is the atomic initialization of `'r`. Prior operations compute values; the final operation applies them.

Schematically:

```
T 'r : Bar a, Baz b =
    ; Phase 1: compute constituent values.
    ; Each step uses ordinary slots (a, b, locals introduced via #, etc.).
    # someValue <- (someComputation: a)
    # otherValue <- (otherComputation: b, someValue)

    ; Phase 2: atomic initialization.
    'r <- (initializer-form: someValue, otherValue)        ; OQ-10 syntax
```

The body satisfies its definite-assignment obligation (per checkpoint 004 §3.3) by performing the atomic initialization on every successful return path. Failures during Phase 1 (in any may-fail computation) propagate naturally — failure-paths are exempt from the obligation, so a failure in computing `someValue` correctly leaves `'r` uninitialized in the caller, with copy-restore preserving the caller's pre-call state.

This pattern is uniform across all constructors. Construction is always "compute, then atomically initialize." There is no procedural field-by-field construction style; the language does not provide it.

### 2.5 Why This Pattern Is Desirable, Not Merely Forced

The "compute then atomic-initialize" pattern is not a workaround for whole-slot tracking. It is an instance of a deeper alignment between the language's design principles:

- **"Mutation either succeeds fully or fails fully."** Atomic initialization is the cleanest realization of this for construction. A partially-built compound cannot exist; therefore, a failure during construction cannot leave a half-built thing behind.
- **"No hidden control flow."** A constructor body is read top-to-bottom: compute values, then commit. No hidden reordering, no implicit zero-fill, no constructor-chain hidden in an initializer list. Each line says what it does.
- **Failure semantics integrate naturally.** A may-fail computation in Phase 1 either produces its value successfully (and Phase 2 proceeds) or propagates a failure (and Phase 2 never fires, leaving the caller's slot untouched per copy-restore). Recovery contexts apply normally.

Whole-slot tracking is therefore not a compromise; it is the tracking discipline that fits the language's design. Field-level tracking would have been, in a sense, a compromise — adding bookkeeping to support a pattern (procedural field-by-field construction) that the language's other principles were already discouraging.

---

## 3. Default Initialization Under R1

This section restates the default-initialization rules from checkpoint 004 in light of the buffer-backed principle and the whole-slot tracking decision. The position remains R1: where zero-fill is inappropriate, the programmer specifies what is appropriate.

### 3.1 Zero-Default Rules for Buffer-Backed Types

A buffer-backed type has a **zero-default** if and only if its bytes have a meaningful zero interpretation. Specifically:

- A plain domain over `[N]` with no declared invariant has a zero-default. The bytes are zeros; the domain has no rule excluding zero.
- A record whose every field has a zero-default has a zero-default. (Recursive: each field's buffer-backed type is checked.)
- A union with a designated candidate at discriminator-zero, where that candidate's bytes are also zero, has a zero-default.
- A buffer-backed type with any constituent that lacks a zero-default does **not** have a zero-default.
- A buffer-backed type with a declared invariant that excludes zero does **not** have a zero-default.

When a buffer-backed type has a zero-default, a `# x : T` introduction produces an `x` whose state is `init` (zero-filled bytes, valid by construction). When a buffer-backed type does not have a zero-default, the type author must declare an explicit default via the OQ-10 mechanism, or the type is not usable for bare introduction (`# x : T` is rejected; only `# x : T = ...` works).

### 3.2 Non-Buffer Types Have No Zero-Default

Pointers, command-typed values, objects, and variants do not have zero-defaults under any circumstances:

- **Pointers** have no zero-default because there is no null pointer in Basis (per checkpoint 004). A pointer is either a valid reference or it does not exist as an initialized slot.
- **Command-typed values** have no zero-default because there is no "zero command." A command-typed value is either a real command (with dispatch and possibly capture) or it does not exist.
- **Objects** have no zero-default because objects have identity, may contain non-buffer fields, and are constructed via constructor commands that do not have a zero-receiver convention.
- **Variants** have no zero-default because their candidates may be arbitrary types (including non-buffer types), and there is no general rule for which candidate the zero state would represent.

For all four, bare `# x : T` is rejected. Only `# x : T = ...` (using the OQ-10 mechanism) or `# x : T <- (constructor: ...)` is valid.

### 3.3 The R1 Mechanism Generalizes

R1 — "where zero-fill is inappropriate, the programmer specifies what is appropriate" — applies uniformly across all types:

- For buffer-backed types whose bytes have a meaningful zero interpretation: zero-fill is the default; no programmer specification needed.
- For buffer-backed types whose invariants or constituents prevent zero-fill: programmer declares an explicit default via OQ-10.
- For non-buffer types: no zero-default exists; programmer initializes at every use site via OQ-10 syntax or via constructor invocation.

The mechanism is the same in all cases — the OQ-10 syntax — applied at type-declaration time for invariant-bearing buffer-backed types, and at use-site time for non-buffer types.

This is one of several reasons OQ-10 is now load-bearing: it is the unified mechanism for non-zero initialization across the entire type system.

---

## 4. Implications and Cross-References

### 4.1 Refines Checkpoint 001

Checkpoint 001 §2 described the type system in terms that treated records, objects, unions, and variants as four distinct kinds, with a noted parallel between record/object and union/variant. This checkpoint refines that picture: the parallel was not coincidental but structural. Records and unions are **buffer-backed**; objects and variants are **non-buffer**. The parallel runs along this axis throughout the type system.

Checkpoint 001's separate sections on records, objects, unions, and variants remain accurate as descriptions of each surface form's specifics. What this checkpoint adds is the explicit unifying principle: records-and-unions sit on the buffer-backed side, objects-and-variants sit on the non-buffer side, and the containment rule (§1.2) governs which can appear inside which.

### 4.2 Refines Checkpoint 004

Checkpoint 004 §3 specified the typechecker's initialization analysis at the slot level, using a tri-state lattice (`init`, `uninit`, `uncertain`). This checkpoint confirms that the slot-level analysis is the only level — there is no field-level analysis. The lattice and the rules remain as written; they apply uniformly to all types, with compounds tracked as single slots.

Checkpoint 004 §8.2 (OQ-9) listed three sub-questions about field-level tracking. All three are now moot:

- (a) Field-level tracking is not supported. Construction uses single-shot atomic initialization (per OQ-10).
- (b) Nested field-level tracking is therefore also not supported.
- (c) Reference parameters to field positions of compounds are not supported (the reference must target the compound as a whole, and the compound must be fully initialized to be referenced).

### 4.3 Refines Checkpoint 005

Checkpoint 005 §2.3 specified constructor receivers as productive `'r`. This checkpoint adds the structural commitment that the constructor body's final operation is the atomic initialization of `'r`, with prior operations being value-computation only. The "compute then atomic-initialize" pattern is now the explicit convention.

Constructor bodies, under this convention, follow a uniform shape: a sequence of value-producing operations, ending in a single initializer-write to the receiver. This is a convention, not a syntactic requirement — the typechecker doesn't reject a body that happens to write `'r` earlier and then does additional work. But the natural and idiomatic shape is compute-then-commit.

### 4.4 Implications for OQ-10

OQ-10 was previously framed as a question about ergonomic syntax for compound initialization. After this checkpoint, OQ-10 is **load-bearing for the entire construction story**. Without OQ-10's mechanism:

- Objects cannot be constructed (no zero-default, no field-level tracking).
- Buffer-backed types with non-zero invariants cannot declare their defaults.
- Constructor bodies have no syntax for their final atomic-initialize step.

OQ-10 is therefore the natural next thread, and the design constraints on it have grown: it must be expressive enough to handle realistic construction patterns, must support potentially-failing computations of constituent values, and must produce a single conceptual write to the target slot.

---

## 5. Open Questions Updated

| OQ | Status | Notes |
| --- | --- | --- |
| OQ-1 | Open | Union discriminator representation. Connects to Note 2 and now also to §3.1 (zero-default for unions). |
| OQ-2 | Open | Implementation latitude for IN parameter passing. |
| OQ-3 | Mostly resolved (per checkpoint 004) | |
| OQ-4 | Resolved (per checkpoint 004) | |
| OQ-5 | Open | Single-class instance coherence. |
| OQ-6 | Open | Partial application beyond receiver-only. |
| OQ-7 | Open | Block-quote macro/fexpr semantics. |
| OQ-8 | Mostly resolved (per checkpoint 002) | |
| OQ-9 | **Resolved** | Whole-slot tracking only; atomic initialization via OQ-10 mechanism. |
| OQ-10 | Open, **now load-bearing** | Composite initializers (`=` vs. `<-`). The unified mechanism for non-zero initialization across the type system. Next thread. |
| OQ-11 | Resolved (per checkpoint 005) | |
| OQ-12 | Resolved (per checkpoint 005) | |
| OQ-13 | Open | Implicit context parameters and initialization. |
| OQ-14 | Open | Same-scope rule for `&x` and `x`. |
| OQ-15 | Open | Full design of the downcast intrinsic. |

---

## 6. Provenance

**Authored:** Distilled from continued intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-27.

**Source materials read for this checkpoint:** intent-checkpoint-001.md through intent-checkpoint-005.md, and the `Grammar2.cpp` (master branch) inspection performed for checkpoint 004 — which directly verified that `TYPE_EXPR_DOMAIN` already enforces the buffer-backed containment rule.

**No grammar changes implied by this checkpoint.** The grammar's existing two-tier distinction between `TYPE_EXPR_DOMAIN` (buffer-backed) and `TYPE_EXPR` (any type) corresponds exactly to the principle articulated here. The principle was implicit in the grammar; this checkpoint makes it explicit in the language's intent documentation.

**Recommended next step:** Commit alongside the prior checkpoints (suggested path: `docs/intent-checkpoint-006.md`). The natural next intent thread is OQ-10 (composite initializers, `=` vs. `<-`), now elevated to load-bearing status. After OQ-10 is settled, the construction story is complete and the failure-mode-and-typechecker thread can be taken up.
