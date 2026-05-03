# Basis Project — Operating Principles

**Purpose.** This document captures the *how-to-think-about-Basis* and *how-to-work-on-Basis-with-Claude* principles that have been settled across the design dialog with JimDesu. A fresh Claude session reading this — alongside the current work plan and the completed references — should be able to continue topic-organized reference work without re-discovering these principles by trial and error.

**This document is stable.** It updates only when a new working principle emerges, or when a recurring miscalibration shows up across multiple sessions and warrants explicit codification. The current work plan (separate document) updates after every reference.

---

## 1. Design Principles That Govern Reasoning About Basis

These are the lenses through which every type-system, semantic, or surface-form question must be considered. Reasoning that does not pass through these lenses produces wrong answers — not just suboptimal ones, but answers that contradict the language's load-bearing structure.

### 1.1 Frame-Ownership

Every slot is owned by *some* frame. "Return values" in the conventional sense do not exist in Basis: output flows through writeable parameter slots that the **caller** owns, written to by copy-restore on success.

When reasoning about value semantics, mode contracts, aliasing, or lifetime ceilings, the question to ask is: **which frame owns this slot?** Not: "what does this function return." The latter framing, dragged in from C++/Java/Rust/Go, produces wrong answers about Basis specifically.

This is the single most important lens. Multiple Claude instances have failed to internalize it; expect to be corrected on it if you're drafting design content that talks about call boundaries, output flow, or parameter contracts.

### 1.2 Each Frame's Static Analysis Is Local

Initialization tracking, failure-mode tracking, and access-path taint are each properties of a *single command body's* analysis on its *own* parameters and locals. **There is no cross-frame propagation** of these properties.

A PRODUCE or REFERENCE output of a downstream call is, in the caller's frame, a freshly-bound local — its access path is rooted at the caller's local, not at any of the caller's parameters. The caller's analysis sees the fresh local with whatever permissions the caller's own bindings give it, full stop.

Common error mode: introducing rules of the form "the caller's analysis catches X because the callee took Y as READ." If the rule talks about cross-frame propagation, it is wrong. The discipline migrates to the frame that has the right context — the language does not propagate.

### 1.3 Access Paths, Not Storage

For the read-only-taint discipline (the transitive READ contract): **the taint is on access paths, not on storage**. The same storage location may be reached at runtime through multiple access paths simultaneously — say, through a READ parameter and a separately-passed REFERENCE parameter that happen to alias. Writes through the READ-rooted path are forbidden; writes through the REFERENCE-rooted path are permitted. The language does not attempt to detect aliasing.

The phrase "storage **reached** from the parameter" is the right phrase. "Storage **reachable** from the parameter" is the wrong phrase — it pulls in storage-tainting connotations.

### 1.4 Buffer-Backed Containment

A buffer-backed type may contain *only* buffer-backed types. Records cannot contain pointers, command-typed values, objects, or variants. Unions cannot either. Typed buffers `[N]T` require T buffer-backed.

This rule is load-bearing in ways that recur: it eliminates whole categories of static-analysis questions. "What if a record contains a pointer?" — the case does not exist. "What if a buffer-element type carries references?" — the case does not exist. If a question presupposes such a structure, the question is its own answer: that structure isn't a Basis structure.

### 1.5 Buffer-Backed Subsumption Is Uniform and Broad

Every buffer-backed type — record, union, named domain, bracket form — subsumes up its parent chain to `[N]` or `[]`. A `Point` (record over `[8]`) is implicitly acceptable where `[8]` is expected, and where `[]` is expected. Two distinct records over `[8]` are *siblings* (analogous to `Inches` and `Centimeters` under `Int32`); they subsume separately to `[8]` but do not implicitly convert peer-to-peer.

This is essential. Without it, "passing a record as a buffer" — an everyday move — would be impossible. Resist any framing that limits the buffer-backed parent-to-child upcast to narrow-sense `domain` declarations only.

### 1.6 Variants Are the Only "May-Be-Absent" Type

Every variant slot inherently admits an **absent state** (no candidate present), in addition to any of its declared candidate states. A single-candidate variant is an optional. The 3-word slot's zero-fill (null candidate-pointer) represents the absent state, so bare `# x : SomeVariant` is well-formed and produces an absent-state slot.

Every variant analysis must handle the absent case alongside any candidate cases. **No other type has this property.** Pointers, objects, command-typed values, records, unions, named domains all contain what their type declaration says they contain. Variants alone admit an inherent "nothing here" state — and that's how Basis gets null-pointer-inclusive data structures without admitting NULL into the type system.

### 1.7 No `::`-Only-As-Class-Resolution Framing

`::` is the **scope operator**. It serves multiple roles: class-method resolution on a receiver, field-member access on aggregates, namespace and module resolution, partial-application bake-in. The unifying reading is *scope-into-a-namespace* — the surrounding context determines which role applies.

Don't call it the "class-resolution operator" (too narrow). Use "scope operator" with the role explicit if needed.

### 1.8 Orthogonality of Language and Standard Library

The user prefers orthogonal mechanism solutions over special-case intrinsics. Concrete cases:

- The `-<` dynamic-narrowing operator (resolves OQ-15) uses existing failure-handling machinery instead of introducing a downcast intrinsic. The standard library is not asked to provide downcast helpers.
- Variant pattern-matching surface forms are construction-side mechanisms using existing failure-handling, not bespoke intrinsics.

Default: when designing a new mechanism, first ask whether existing language facilities (`<-`, `-<`, the failure system, the scope operator, class dispatch) compose to do the work. Only reach for a new intrinsic if no orthogonal alternative exists.

### 1.9 Liskov Substitution as a Design Tool

Where the language admits a subtyping relation, the design should preserve Liskov substitutability. Where Liskov holds, hierarchies can be open; where it doesn't, they must be closed.

The buffer-backed hierarchy preserves Liskov by construction (subsumption is one-directional and value-rewriting is absent). The failure-tag hierarchy is currently closed (CP012 §2.4) but OQ-29 proposes opening it under a payload-covariance constraint that preserves Liskov.

### 1.10 Region-Style Memory Reclamation Is a Latent Constraint

Region-based reclamation feasibility is a downstream goal — the user wants the option preserved. Frame-ownership and the buffer-backed containment rule together preserve this option: buffer-backed values do not own non-local storage; non-buffer types are confined to positions where ownership and lifetime are explicit.

The reference-writing work does not argue for region reclamation but is careful to not preclude it. If a design decision would punch a hole in region feasibility, flag it.

---

## 2. Workflow Conventions

These are the working norms for drafting and revising the topic-organized references.

### 2.1 Scope Confirmation Before Drafting

Before writing a substantive document, confirm scope with the user via numbered questions covering:

- Section structure
- Boundaries against neighboring references (what stays here, what forwards)
- Status of relevant open questions (resolve, sharpen, forward)
- Recent design decisions that bear on the work

Don't draft against assumed scope. The five-point scope confirmations have been a load-bearing pattern.

### 2.2 Reconciliation Markers

Where a reference makes a choice not directly determined by source checkpoints — resolving an ambiguity, recording a post-checkpoint correction, filling a gap, or carving in a refinement after a user dialog — attach an inline *[Reconciliation: ...]* marker. This makes the provenance auditable and helps future readers (Claude or human) distinguish "what the checkpoints said" from "what the reference inferred or refined."

### 2.3 Citation Format

- Decision-level rules: `*[CPnnn §x.y]*` inline
- Cross-references to other completed references: `*[failure-system §x]*`, `*[op-sem §x]*`, etc.
- Forwarded items: `*Open question: OQ-N.* See §7.` with full treatment in the open-questions section
- Section-end footers: `*Sources for §N: ...*` summarizing the checkpoint spans drawn upon

### 2.4 Authority Statements

Each reference declares its authority relative to the source checkpoints (typically: "this reference is authoritative on a covered topic; the source checkpoints remain useful as historical record") and relative to the source code (typically: "code is authoritative on syntactic matters; this reference is authoritative on semantic intent"). Where references overlap, the later reference is authoritative on its domain and bridge notes flag the boundary.

### 2.5 Section Structure Discipline

Each reference uses a consistent shape: prelude (status, date, provenance, authority statement, citation conventions, any standing principles), numbered sections covering the topic, an Open Questions section with resolved/forwarded/open status for each, and a Provenance section closing out (source materials, resolutions recorded, forwarded sub-questions, reference-document impact, recommended next step).

### 2.6 Read Source Materials Before Drafting

Don't pattern-match from contemporary-language reasoning to Basis. If a topic touches CP004, read CP004; don't infer from "what languages typically do." This has been the source of multiple corrections — the "predecessor instances keep falling into the wrong paradigm" comment from JimDesu was about exactly this.

### 2.7 Tight Mid-Draft Corrections

When the user corrects a draft mid-stream, the correction is often a recurring miscalibration (see §3 below). Don't argue for the original framing — the user has explained it before, often multiple times. Make the correction cleanly, note in the reconciliation marker that it's a 2026-MM-DD review correction, and move on.

### 2.8 Honesty About Capacity

When approaching context-budget limits for a session, say so candidly. Don't promise iteration headroom that isn't there. The user can decide whether to split the work, save and restart, or push through.

---

## 3. Recurring Miscalibrations to Avoid

Specific failure modes that have come up multiple times across sessions. A fresh Claude instance is likely to drift into these without explicit warning.

### 3.1 Cross-Frame Propagation Drift

The temptation: writing rules of the form "the caller's analysis is tainted because the callee took something at READ." Every time this has been written, it has been wrong. Each frame's analysis is local. If you find yourself writing "the caller now holds a tainted slot because of the call," stop and reread §1.2.

### 3.2 Storage-Tainting Drift

The temptation: writing rules that reference storage-reachability from the READ parameter, or that propagate taint based on aliasing. Every time this has been written, it has needed correction. The model is access-path-tainting; storage reachability is a runtime property the language doesn't track.

### 3.3 Records-Can-Have-Pointers Drift

The temptation: when discussing buffer-backed taint propagation, reaching for examples like "a record with embedded pointer fields." Records cannot have pointer fields. The buffer-backed containment rule (§1.4) eliminates the case. If a question's framing presupposes such a structure, the framing is wrong.

### 3.4 Records-and-Unions-Don't-Subsume Drift

The temptation: writing "records are not subsumable; two records are either the same nominal type or unrelated." Wrong — records subsume up their parent chain to `[N]` and `[]`. Two distinct records *at the same level* are siblings (don't peer-convert), but both subsume to the shared ancestor. Same for unions.

### 3.5 Special-Case-Intrinsic Drift

The temptation: when designing a new mechanism, reaching for a new intrinsic or new syntactic form. First ask: do existing language facilities compose? The user has consistently preferred orthogonal solutions (`-<` over a downcast intrinsic; auto-emitted accessors not needed because pattern-matching plus failure-handling does the work).

### 3.6 Variants-Are-Like-Objects Drift

The temptation: discussing variants by structural analogy with objects. Variants have a unique "absent" state property no other type has; they are tagged sums with their own slot shape; their construction surface is different. Variants are not a flavor of object.

### 3.7 `::`-As-Class-Resolution Drift

The temptation: calling `::` the "class-resolution operator." It is the scope operator; class-resolution is one role.

### 3.8 Drift Toward C++/Java/Rust/Go Reasoning

The temptation: reasoning by analogy to mainstream language semantics. Basis differs from these in load-bearing ways — no return values, no NULL, frame-ownership rather than function-result ownership, transitive read-only contracts, write-once productive parameters. Reasoning that imports mainstream-language assumptions reaches mainstream-language conclusions, which are often wrong here.

---

## 4. How to Use This Document

When starting a fresh session on Basis design work:

1. Read this document first.
2. Read the current work plan document next.
3. Read the relevant completed references for the work item identified in the plan.
4. Read the relevant checkpoints listed in the plan's reading list for that work item.
5. Confirm scope with the user before drafting (per §2.1).

When this document needs updating:

- A new design principle has emerged in user dialog and applies broadly.
- A new recurring miscalibration has been observed across multiple sessions.
- A workflow convention has been refined.

Update this document conservatively. Most updates belong in the work plan, not here.
