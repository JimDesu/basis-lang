# Basis Project — Current Work Plan

**Purpose.** This document tracks the state of the topic-organized reference consolidation: what's done, what's pending, what's next, with reading lists and recommended approaches for the next item. It updates after every reference and after every substantive design dialog.

**Companion document:** `project-operating-principles.md` (stable working principles). Read that first; it sets the lenses through which every design question must be considered.

**Status as of 2026-05-02.** Three of the planned topic-organized references are complete. The fourth (Construction and Initialization) is the recommended next item, with notes on splitting it across two sessions.

---

## 1. The Topic-Organized Reference Consolidation

### Background

The Basis design dialog has accumulated 14 intent checkpoints (CP001 through CP014). Each checkpoint records the state of the design at one point in time, supersedes earlier checkpoints where they conflict, and registers new open questions. The checkpoint stream is ground truth but is not directly usable as a design reference: a topic of interest is typically scattered across multiple checkpoints with refinements, supersessions, and reframings layered through.

The consolidation produces topic-organized authoritative references that:

- Cover one design topic comprehensively
- Resolve cross-checkpoint conflicts in favor of the latest decision (with reconciliation markers)
- Carve in late-breaking design dialog decisions from inception
- Cross-reference but do not duplicate material owned by other references
- Maintain an open-questions section that catalogs what's resolved here, forwarded elsewhere, or genuinely open

### Reference Inventory

The agreed sequence of topic-organized references:

| # | Reference | Status | Date |
|---|-----------|--------|------|
| 1 | Failure System | Complete | 2026-04-29 |
| 2 | Operational Semantics | Complete | 2026-04-29 |
| 3 | Type System and Modes | Complete | 2026-05-02 |
| 4 | Construction and Initialization | **Next** | — |
| 5 | Lambda and Fexpr | Future | — |
| 6 | Class System | Future | — |

Possible additional references (not yet committed):

- Grammar and Lexer (collecting the implementation thread that OQ-23 forwards into)
- IR / Compilation Strategy (issue #24 territory; possibly post-frontend)

### Authority Layering

For any covered topic:

- The most recent applicable reference is authoritative.
- Source checkpoints remain useful as historical record but are non-authoritative where a reference has consolidated the topic.
- Source code is authoritative on syntactic matters; references are authoritative on semantic intent.
- Where two references overlap, the later reference is authoritative on its domain; bridge notes in the earlier reference flag the boundary.

---

## 2. Completed References

### 2.1 `reference-failure-system.md`

**Covers:** Failure-mode marks (`:`, `?`, `!`); the failure system's runtime mechanics; typed failures (CP012); the failure-tag hierarchy; recovery via `|: Tag`; conformance rules; the failure-tag class-witness slot structure.

**Key resolutions recorded:**
- OQ-26 family substantively resolved (typed-failure design from CP012)
- OQ-26.5 closed in favor of design I-a
- OQ-28 registered (closure of failure-tag hierarchy; refined later by OQ-29 in type-system reference §7.9)

**Pending consolidations:**
- §7.7 (OQ-1 in constrained form) was framed as remaining open for both variant and union halves; the type-system reference resolves both. A future consolidated pass may update §7.7 to point at type-system §3.4 (variant) and §2.6 (union).
- §7.6 (OQ-28) will need a touch-up note cross-referencing OQ-29 (type-system §7.9), which proposes a specific Liskov-style opening direction.

### 2.2 `reference-operational-semantics.md`

**Covers:** Execution model; surface structure (block markers); the two foundational principles (no-non-local-state, no-hidden-control-flow); commands and invocation (signatures, parameter passing operationally, first-class commands, implicit context parameters, `::` scope operator, multiple dispatch); frame-exit and lifecycle; command-literal vs. block-quote surface distinction; operational open questions.

**Key resolutions recorded:**
- Provision-chain reframe of no-non-local-state per CP007
- Block markers structurally per CP002
- Frame-locality principles (init analysis, failure-mode analysis)

**Pending consolidations:**
- §5.2 carries a bridge note flagging CP013 supersession; the bridge note's "aliasing materializes in the caller's frame and is the caller's responsibility" framing is canonical and aligns with type-system §4.6.
- §5.3 (`-> name` mechanism) and §8.2 (OQ-3) require revision to fold in CP014's resolution (already carved into type-system §4.9 from inception).
- §8.3 (OQ-15) will need a touch-up note cross-referencing the `-<` operator resolution recorded in type-system §3.5.

### 2.3 `reference-type-system-and-modes.md`

**Covers:** The buffer-backed principle (CP006 made structural); buffer-backed types (buffers, ranges, domains, aliases, records with `.inline` modifier, unions); non-buffer types (pointers, command-typed values, objects, variants with the absent state); the parameter-mode system (READ/PRODUCE/REFERENCE per CP013); type relations (nominal typing, buffer-backed hierarchy); the no-defaults discipline.

**Key resolutions recorded:**
- OQ-1 (variant half) — 3-word slot
- OQ-1 (union half) — untagged byte overlay
- OQ-14 broadened — same-scope rule for all mode-marker pairs, both bindings and parameter declarations
- OQ-15 — `-<` dynamic-narrowing operator
- OQ-2.1 closed without action (cross-frame taint flow does not occur)
- OQ-2.2 closed without action (buffer-backed containment rule eliminates the case)
- OQ-29 newly registered — Liskov-style opening of the failure-tag hierarchy under payload-covariance

**New material with no prior checkpoint coverage:**
- Single-candidate variants as optionals; absent state for all variants
- `::` named the scope operator
- `.inline` modifier for record-in-record composition
- Buffer-backed hierarchy uniformity across records, unions, named domains

**Pending consolidations:** none specific to this reference; it is fully aligned with op-sem §5.2 bridge note.

---

## 3. Next Reference: Construction and Initialization

### 3.1 Scope

Covers the construction-and-initialization surface of the language:

- The `<-` polymorphic-RHS mechanism (CP009 §3 — multiple RHS shapes)
- Aggregate literals (`{- ... -}`) and Sequence literals (`[- ... -]`)
- The `.implicit` mechanism (CP009 §4) — including OQ-22 (parameterized literal types in `.implicit`)
- The `=` defaults declaration form (CP009 §5)
- Atomic compound construction (per CP006 §2 / OQ-9 resolution)
- OQ-10 composite-initializer mechanism (resolved per CP009)
- The `-<` operator's full surface form (declared in type-system §3.5; full surface lives here)
- Variant construction including absent-state introduction
- Bare-identifier `<-` value-copy primitive (CP009 §3.7)
- Construction-side aspects of context parameters (OQ-13)
- Pattern-matching surface forms (variant case analysis with absent-case coverage)
- Interpretive casting for unions (declared in type-system §2.6; surface lives here)

### 3.2 Reading List

Before drafting, load:

**Operating principles:**
- `project-operating-principles.md` (this companion document)

**Completed references (cross-references will be needed):**
- `reference-failure-system.md` — for failure-system integration in `?-` patterns and `-<` failure semantics
- `reference-operational-semantics.md` — for parameter-passing mechanics and frame-exit lifecycle
- `reference-type-system-and-modes.md` — heavily; for type-side commitments that construction realizes (variant absent state, buffer-backed hierarchy, READ/PRODUCE/REFERENCE modes, `.inline` records, scope operator)

**Source checkpoints:**
- CP001 — foundational construction mechanics, expression-position calling convention
- CP004 — initialization analysis (the lattice that construction must satisfy)
- CP006 — buffer-backed principle and OQ-9 resolution (atomic compound construction)
- CP007 — provision-chain rule (which construction patterns are well-formed under no-non-local-state)
- CP009 — **densest checkpoint for this reference** — `<-` RHS shapes, Aggregate/Sequence literals, `.implicit`, `=` defaults, OQ-10 resolution
- CP010 (partially) — for fexpr-based construction patterns if relevant
- CP012 (partially) — for typed-failure construction if relevant
- CP014 — for `-> name` interpretation across modes (already carved into type-system §4.9; construction may cross-refer back)

### 3.3 Split Recommendation

The construction reference is structurally larger than any of the three completed references. CP009 alone is dense. A single-session draft is feasible for the first cut but iteration headroom may be tight.

**Recommended split:**

**Pass 1 — Construction Surfaces (one session):**
- The `<-` polymorphic-RHS family, all shapes
- Aggregate literals and Sequence literals
- `.implicit` mechanism (with OQ-22)
- Bare-identifier `<-` value-copy primitive
- Interpretive casting for unions

**Pass 2 — Defaults, Composites, and Variants (one session):**
- The `=` defaults declaration form
- OQ-10 composite initializers
- Atomic compound construction with CP006 §2 implications
- Variant construction including absent-state introduction
- `-<` operator's full surface (cross-referencing type-system §3.5)
- Pattern-matching surface forms
- Construction-side context parameters (OQ-13)

The split is justifiable thematically (surfaces vs. compounds-and-defaults) and gives each pass a fresh context budget.

**Alternative — single-pass:** If the user prefers a single document, a single session can attempt the full draft but should plan for tight iteration and may need a second session for substantive revisions.

### 3.4 Scope-Confirmation Questions to Ask the User

Before drafting (per Operating Principles §2.1), confirm:

1. Section structure for the chosen pass (or full document)
2. Whether OQ-22 resolves here or stays open and forwarded — material for resolution exists in CP009
3. Whether OQ-13 (implicit context parameters and init) resolves here, in the class-system reference, or jointly
4. Whether pattern-matching surface forms are owned by this reference or by a future "pattern-matching" reference
5. Treatment of variant-construction absent-state surface (introducing an absent variant; matching against it)
6. Confirmation that interpretive casting for unions lives here (the type-system reference declared the read-mechanism but forwarded surface)

---

## 4. Future References

### 4.1 Lambda and Fexpr (#5)

**Owns:** CP007–CP008–CP010 in full; OQ-7 history; OQ-18, OQ-21, OQ-25; lambda capture mechanics and ceiling-tracking; fexpr's locality rule and per-invocation invocation frame; partial-application beyond receiver-elision (OQ-6 partial owner).

**Notable forwarded items:**
- OQ-25 (capture-shadowing)
- OQ-21 (capture-list interaction with implicit context parameters)
- OQ-18 (lambda visible-signature representation)

### 4.2 Class System (#6)

**Owns:** Haskell-style dictionary passing details, instance coherence (OQ-5), partial application (OQ-6), overloading on dispatched commands (OQ-16), and the cross-module Liskov question for failure tags (OQ-29).

**Notable forwarded items from type-system reference §7:**
- OQ-5 sub-questions (instance coherence, orphan instances, domain-specific dispatch, more-specialized-module-wins)
- OQ-6 (partial application beyond receiver-only)
- OQ-13 (implicit context parameters and init) — jointly with construction
- OQ-16 (overloading restriction on dynamically-dispatched commands)
- OQ-29 (Liskov-style opening of the failure-tag hierarchy)

### 4.3 Possible Additional References

- **Grammar and Lexer.** Implementation-thread material collecting OQ-23 and the `.inline`-placement question, plus other minor lexer disambiguation matters that have accumulated. Not strictly a design reference but useful as a single document for the implementation work.
- **IR and Compilation Strategy.** Tracks issue #24. Probably post-frontend.

---

## 5. Open Questions Registry

State of all OQs across the project, current as of 2026-05-02.

### 5.1 Resolved

| OQ | Resolved by | Where |
|----|-------------|-------|
| OQ-1 (variant half) | type-system §3.4 | 3-word slot (tag/candidate-pointer/witness) |
| OQ-1 (union half) | type-system §2.6 | Untagged byte overlay |
| OQ-3 | CP014 / type-system §4.9 | `-> name` unified across READ/PRODUCE/REFERENCE |
| OQ-4 | CP004 | Default initialization story |
| OQ-7 | CP010 | Fexpr design |
| OQ-8 | CP002 / CP014 | At-stack mechanism |
| OQ-9 | CP006 | Whole-slot tracking |
| OQ-10 | CP009 | Composite initializers |
| OQ-11 | CP005 | Marker syntax (identifier-shape) |
| OQ-12 | CP005 | Receiver modes (per-shape tables) |
| OQ-14 | type-system §4.3 | Same-scope rule (broadened) |
| OQ-15 | type-system §3.5 | `-<` dynamic-narrowing operator |
| OQ-17 | CP009 | Compound literal syntax |
| OQ-19 | CP008 | Reference marker placement (`Type&` suffix) |
| OQ-26 family | failure-system §§4–5 | Typed-failure design |

### 5.2 Closed Without Action

| OQ | Reason |
|----|--------|
| OQ-2.1 | Cross-frame taint flow does not occur; presupposed-question dissolves |
| OQ-2.2 | Buffer-backed containment rule eliminates the case |

### 5.3 Forwarded (Open, Owned Elsewhere)

| OQ | Owner | Notes |
|----|-------|-------|
| OQ-2 (legacy parts) | typechecker-impl thread | Implementation latitude for IN passing |
| OQ-5 sub-questions | class-system reference | Instance coherence |
| OQ-6 | class-system reference | Partial application beyond receiver |
| OQ-13 | construction + class-system | Implicit context parameters and init |
| OQ-16 | class-system reference | Overloading on dispatched commands |
| OQ-18 | lambda-and-fexpr reference | Lambda visible-signature |
| OQ-20 | implementation thread | Slash-list grammar details |
| OQ-21 | lambda-and-fexpr reference | Capture-list and implicit context |
| OQ-22 | construction reference | Parameterized literal types in `.implicit` |
| OQ-23 | implementation thread | Lexer disambiguation; collects `.inline`-placement |
| OQ-24 | construction reference | Phasing of `.implicit` Aggregate/Sequence support |
| OQ-25 | lambda-and-fexpr reference | Capture-shadowing |
| OQ-28 | failure-system §7.6, refined by OQ-29 | Closure of failure-tag hierarchy |
| OQ-29 | class-system reference | Liskov-style opening with payload covariance |

### 5.4 Genuinely Open (Recorded but Awaiting Treatment)

| OQ | Where Recorded | Notes |
|----|----------------|-------|
| Record padding and layout | type-system §7.3 | Minor; implementation latitude vs. deterministic layout |

---

## 6. Pending Cross-Document Consolidations

These touch-up notes have accumulated as later references settled material that earlier references had as open. Each is a small revision; collected here for a future consolidated pass.

| Where | What |
|-------|------|
| failure-system §7.6 | Cross-reference OQ-29 (type-system §7.9); possible eventual collapse |
| failure-system §7.7 | Update OQ-1 entries to point at type-system §§2.6 (union) and 3.4 (variant) |
| op-sem §5.2 (bridge note) | Confirmed canonical; reading is "caller's responsibility, not language-enforced"; aligns with type-system §4.6 |
| op-sem §5.3 (`-> name`) | Fold in CP014's resolution (carved into type-system §4.9 from inception) |
| op-sem §8.2 (OQ-3) | Same as §5.3; mark resolved per CP014 |
| op-sem §8.3 (OQ-15) | Cross-reference `-<` operator resolution at type-system §3.5 |

---

## 7. Update Protocol

This document is updated when:

- A new reference is completed (move from "next" to "completed", add summary; update OQ registry)
- A substantive design dialog produces a new resolution, closure, or registered OQ (update §5)
- A pending consolidation is performed (remove from §6)
- The recommended next reference changes (update §3)
- A new reference category is added (update §1.2 inventory and §4)

Conservative additions; preserve the structure. If the document grows beyond ~400 lines, consider splitting the OQ registry into its own file.

The companion document `project-operating-principles.md` updates separately and rarely.
