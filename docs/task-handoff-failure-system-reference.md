# Task Handoff: Draft the Failure System Topic Reference

**For:** The next Claude instance picking up the Basis Language project in a fresh session.
**From:** Claude (Opus 4.7), 2026-04-29, end of the OQ-26 typed-failures dialog.
**Cite this file at the start of the new session to load the task.**

---

## 1. The Task in One Line

Draft `reference-failure-system.md` — a topic-organized authoritative reference consolidating the failure-handling design across CP002, CP003, CP011, and CP012, with per-decision citations back to source checkpoints.

## 2. Why This Is Happening

The Basis project has accumulated twelve intent checkpoints over several months of design dialog. Reading and reasoning across them is increasingly error-prone — one near-miss in CP012 was caught only because the user spotted a bug in my draft of the at-stack-handler discipline. The user and I agreed (final exchanges before this handoff) to begin a phased consolidation: topic-organized reference documents that integrate across the checkpoint set and serve as the new authoritative source on each topic, with the checkpoints retained as historical record.

The user's mental furniture is topic-clustered, not chronological; the references match how they actually think about the language. They also reduce my retrieval-error surface by giving me coherent topic-shaped material instead of fragments scattered across twelve documents.

The Failure System is being consolidated first because it has the most-developed material, is the freshest in both our heads (CP012 just landed), and will be the typechecker-implementation thread's primary input.

## 3. Deliverable

- **File:** `reference-failure-system.md`
- **Location:** Alongside the checkpoints (same project knowledge, different filename convention to distinguish from dated checkpoints).
- **Estimated size:** 700–900 lines.
- **Scope:** Design specification — the rules, rationales for non-obvious choices, and open questions. **Not** pedagogical scaffolding (definitions of CFG, lattice, etc., which appear in CP011 §2 — these stay in their source checkpoints; the reference can point to them but should not duplicate them).
- **Working rhythm:** One drafting pass, present to user for review, revisions in place from there.

## 4. Section Structure

Seven sections, agreed with the user:

1. **Failure modes and marks** — the `:` / `?` / `!` distinction, subsumption rules, parameter-mode invariance, never-fails-must-handle as a corollary of conformance.
2. **Block markers for failure handling** — operational rules for `?`, `?-`, `?:`, `^`, `|`, `|`-with-spec, `??`, `-`, `%`, `@`, `@!` as they pertain to failure flow. The `%`-as-compound-guard refinement from CP003 lives here. The `^`-body-optional refinement from CP011 §4.6 lives here.
3. **Failure-mode static analysis** — the six-state lattice, transfer functions, body conformance rules, integration with the initialization analysis. From CP011 §§3–10 primarily; refined under §9 of this reference to anticipate the lattice extension under typed failures.
4. **Typed failures** — hierarchical tags, class-bound payloads (Haskell-style classes, not concrete types — this is load-bearing), structural set inclusion, signature surface, the no-Top forest, cross-module hierarchy treatment. From CP012 §§2–5.
5. **Payload lifecycle implementation** — the three-word slot (tag + payload-pointer + class-witness), the **holding-frame** model (the value lives in the holding frame and follows it through binding-as-move and re-fail-as-move), propagation by slot-copy without value-move, consumption-as-move into the recovery frame, the at-stack discipline (handlers fire at the value's actual lifetime end, never at intermediate moves). From CP012 §6 in full. **This is the section most likely to be misdrafted; see §6 of this handoff for what to watch for.**
6. **Top-level handling** — what's settled (`.program` and `.test` are the lattice's top frame; failures may propagate up; recovery within their bodies works normally), what's open as OQ-27 (the precise terminal-handling semantics).
7. **Open questions** — OQ-26.1 (implicit failure sets), OQ-26.2 (surface syntax), OQ-26.3 (recovery narrowing), OQ-26.4 (higher-order propagation form, paired with OQ-7), OQ-27 (top-level handling), OQ-28 (cross-module hierarchy extension), OQ-1 in its constrained form (failure-tag discriminator subcase is settled; general union-discriminator question remains).

## 5. Citation Format

Citations are at **decision level** for load-bearing rules, **section level** for explanatory connective tissue, with explicit **reconciliation markers** where the reference makes a choice not directly determined by the source checkpoints.

### Inline citation, decision-level:

> A `|`-with-spec block engages on a propagating failure whose tag is at-or-below the spec's tag in the relevant hierarchy. Failures whose tags are not at-or-below the spec continue propagating. *[CP002 §2.10; CP012 §3.4]*

### Section footer, where prose is mostly explanatory:

> *Sources: CP002 §§2.9–2.10, CP011 §§4.7–4.8, CP012 §3.4.*

### Reconciliation marker, where the reference resolves ambiguity or fills a gap:

> At-stack handlers on a payload value fire when the value's actual lifetime ends — at handler-body-exit if the body completes without re-failing, at the eventual final consumption of the chain if the body re-fails, or at program termination otherwise. *[CP012 §6.8; reconciles CP002 §3 (general at-stack rule) with CP012 §6.4 (binding-as-move). An earlier CP012 draft proposed at-stack-fire-at-binding; this was identified as inconsistent with re-fail and corrected. See CP012 §13 revision note.]*

The reconciliation markers are where the user's eyes will go on review. They are also where errors of mine are most likely to have impact. **Be deliberate about marking every place the reference makes a choice that the sources didn't directly determine** — even small choices.

## 6. Critical Things to Get Right

These are points where the design has subtle structure that draft-by-pattern-matching is likely to miss. Read CP012 carefully before drafting §5 of the reference.

- **A class in Basis is a Haskell-style typeclass — a constraint on types, not a concrete type with a layout.** The previous Claude (me) drifted into using "class" as if it meant "record type" partway through the OQ-26 dialog and the user caught it. The reference must use "class" correctly throughout: a class has no size; instances of types satisfying a class are the things that have layouts. The bound class on a failure tag is the *contract*; the *concrete type* of any particular payload value is per-`.fail`-site detail not visible to consumers.

- **The holding-frame model.** The payload value lives in a particular frame at any given time — its holding frame — and that frame *changes* across binding and re-fail. Initially the firing frame; on consumption-by-binding, the recovery frame; on re-fail, the new firing frame. Most error-prone failure mode: writing as if the payload stays in the original firing frame for the duration of the recovery, which leads to incoherent at-stack semantics under re-fail.

- **At-stack handlers fire when the value's *actual lifetime* ends, not at intermediate moves.** Specifically, *not* at the moment of binding. An earlier CP012 draft got this wrong; the corrected rule is in CP012 §§6.4–6.8. The user identified the bug via the re-fail counterexample: if the at-stack handler fires at binding, it might mutate the payload before the body can re-fail using it.

- **OQ-26.5 is resolved by CP012, not still open.** Move-at-binding (interpretation I-a) is the choice. Some old language in earlier checkpoints may still describe it as open.

- **The slot is fixed-size: tag + payload-pointer + class-witness, three words.** It is identity-of-failure, not contents-of-failure. Propagation copies the slot, not the payload.

- **Consumption ends the failure-in-flight; the *value* survives if the handler binds it.** "Consumption" and "end of value lifetime" are distinct events; this distinction is what makes re-fail semantics coherent.

- **`%`-blocks have no recovery semantics on their own** (CP002 §2.8) but can stand as guards (CP003 §1). The `.program`/`.test` body admits normal recovery constructs at its top level; OQ-27 concerns only what happens to failures that escape the *entire* body.

- **No language-imposed Top tag.** Each module declares its own roots; tags from different hierarchies are incomparable. Bare `|` provides catch-anything-without-binding; that's sufficient for catch-all patterns.

- **Cross-module hierarchy extension is asserted closed by CP012 §2.4 but flagged provisional (OQ-28).** The reference should reflect "closed by current decision; see OQ-28 for revisit."

## 7. Source Materials

Read in this order:

- **CP002** — operational semantics of all eleven block markers; failure-mode glossary; the must-handle rule (informal); rewind block (`^`); recovery block (`|`) and `|`-with-spec; re-fail pattern (`?-` followed by `.fail`); frame-exit hooks (`@`, `@!`).
- **CP003** — `%`-blocks as compound guards; disjunctions inside `%`-conjunctions via `|`-chains. Small but load-bearing.
- **CP011** — six-state failure-mode lattice; transfer functions for the eleven block markers and call sites; body-conformance rules; subsumption; never-fails-must-handle as corollary; `^`-body-optional refinement; integration with init-state analysis from CP004; lattice diagnostic considerations.
- **CP012** — typed failures: hierarchical tags, class-bound payloads, structural set inclusion, holding-frame implementation, the corrected at-stack discipline. **The CP012 §13 revision note is important: it documents where an earlier draft got at-stack discipline wrong and how the corrected version was reached.**

Secondary references (consult if needed but not the primary input):

- CP004 §3 (initialization analysis — the lattice referenced by CP011 §7)
- CP008 §10 / FI-1 (region-style cleanup — relevant background for at-stack-handler timing)

## 8. Anti-Patterns to Avoid

- **Don't pedagogically explain CFGs, lattices, transfer functions.** That material is in CP011 §2 and should stay there. The reference can say "the analysis is a forward-flow analysis over the body's CFG; see CP011 §2 for the framework definitions" and move on.
- **Don't reproduce CP012 §6 verbatim.** Consolidate it; cite it; refine its prose where the wider topic context suggests refinements; don't just paste.
- **Don't quietly resolve open questions.** If the source checkpoints leave something open, the reference must leave it open and cite the OQ. Resolving an OQ is design work, not consolidation work, and must be a separate dialog with the user.
- **Don't omit the reconciliation markers.** Every place the reference makes a choice not directly determined by the sources, mark it explicitly. This is non-negotiable; it's the user's primary review handle.
- **Don't drift into describing the typechecker implementation.** The reference is design specification. "The typechecker enforces this rule by ..." is implementation; "the rule is ..." is specification. The reference is the latter.

## 9. Working Rhythm With the User

The user (JimDesu) works at a parent's pace and prefers concentrated work over diffuse exchange. They will likely review the draft section-by-section rather than as one monolith. They are a careful reader and will catch errors of substance; they are tolerant of minor stylistic infelicities. They prefer prose to bullets where prose works. Their feedback style is direct and specific; engage substantively with corrections rather than apologizing reflexively.

The drafting pass should produce the full reference in one go. The user will review and indicate revisions; revisions happen in place in the same file.

## 10. After This Reference Is Done

The user has provisionally agreed to the topic-split:

1. Failure System (this task)
2. Operational Semantics and Block Markers (foundational, cited by most others)
3. Type System and Modes
4. Construction and Initialization Story
5. Lambda and Fexpr System
6. Class System (last; most material is open)

But these are downstream tasks. Don't start them in the same session as the Failure System reference unless the user explicitly directs. Each consolidation is a focused thread of its own.

The full-baseline consolidation (a single authoritative document covering the whole language design) is *not* the current goal; topic references are the agreed approach. Full baseline becomes the right move when typechecker implementation begins, not before.

---

## 11. How to Begin the New Session

User will likely say something like "continue with the Failure System reference" or cite this file directly. On receipt:

1. Read this handoff in full.
2. Use `project_knowledge_search` to load CP002, CP003, CP011, CP012 in that order.
3. Acknowledge the task back to the user briefly (one short paragraph confirming what's about to be drafted, the structure, the citation format) and ask if anything has changed since the handoff before drafting.
4. Draft the reference per §§3–6 of this handoff.
5. Save to `/mnt/user-data/outputs/reference-failure-system.md` and present to the user.

Do not begin drafting without the brief acknowledgment-and-confirmation step. The user values the chance to redirect before substantial work happens, and a fresh session may have context the handoff doesn't capture.
