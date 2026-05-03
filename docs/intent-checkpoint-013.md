# Basis Language — Intent Checkpoint 013

**Status:** Working draft. Builds on intent-checkpoint-001.md through intent-checkpoint-012.md; supersedes them where they conflict. This checkpoint partially resolves OQ-2 (implementation latitude for IN parameter passing, originally posed in CP001 §6) by reframing the question: what was previously taken to be a question about call-boundary copy semantics is settled here as a question about a *transitive read-only contract* on the parameter mode formerly called IN. The checkpoint also revises the parameter-mode vocabulary: IN / productive / reference become READ / PRODUCE / REFERENCE, with the markers (`'` and `&`) unchanged. Two sub-questions remain open and are tracked as OQ-2.1 (per-slot taint state model) and OQ-2.2 (value-disjoint vs. slot-view discrimination); the legacy OQ-2 sub-questions (a)–(c) from CP001 §6 are revisited and partially superseded under the new framing (see §7).

**Date of this checkpoint:** April 2026

**Provenance:** Continuation of the intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-30, opened from the operational-semantics reference's review of OQ-2.

**Authority statement.** Where this document differs from earlier checkpoints, this document is authoritative. Where this document and the source code differ on syntactic matters, the code is authoritative; where they differ on semantic intent, this document is authoritative. The README and `language_notes.txt` remain non-authoritative.

---

## 1. Decision Summary

This checkpoint lands the following decisions:

1. **The parameter-mode names change.** What was IN becomes **READ**; what was productive becomes **PRODUCE**; what was reference becomes **REFERENCE**. The marker syntax (`'` for PRODUCE, `&` for REFERENCE, no marker for READ) is unchanged. The new names are intended to make the *permissions/obligations* character of each mode explicit and to retire the calling-convention vocabulary that the old names implied.

2. **READ carries a transitive read-only contract.** A READ parameter promises that the callee will not write through *any* storage reachable from the parameter — not just the parameter's own slot. This strengthens the previous semantics (which only confined the callee's mutations to its own copy of the parameter slot) to also forbid mutations through pointers, references, and object-graph edges reachable from the parameter.

3. **PRODUCE retains its must-write-on-success obligation.** The choice of "PRODUCE" (rather than "WRITE") preserves the existing CP004 §2.2 contract: a PRODUCE parameter must be written exactly once on every successful return path. The rename is naming-only; no semantic change.

4. **REFERENCE is the may-read / may-write / may-neither mode.** Unchanged from the previous "reference" mode (CP004 §1). Caller-side: the slot must be initialized at the call. Callee-side: no obligation either way.

5. **READ-tainted values cannot flow into a writeable parameter slot.** This is the operational form of the transitive read-only contract: the typechecker rejects any program in which a value derived from (or aliasing storage reachable from) a READ parameter is passed as the argument to a PRODUCE or REFERENCE position. This is the typechecker rule that gives the contract its teeth.

6. **The "return values aliasing READ inputs" question collapses.** Because Basis has no return values per se — productive-parameter writes are the analog — and because READ-tainted values cannot flow into PRODUCE slots by rule (5), no signature annotation is needed for "this PRODUCE output may carry taint from READ inputs." It cannot. Productive-parameter post-states are structurally taint-free. (See §5.)

7. **OQ-2 is partially resolved.** The original OQ-2 sub-questions (a)–(c) from CP001 §6 are revisited under the new framing in §7. (a) is largely settled by the READ contract — the implementation may freely pass READ parameters by reference because the type system precludes the mutations that would make by-reference observable. (b) and (c) remain open in modified form.

8. **Two new sub-questions are opened as OQ-2.1 and OQ-2.2** (see §7), tracking remaining work in this thread: the model for per-slot taint as flow-sensitive state, and the rule discriminating value-disjoint operations from slot-view operations under taint propagation.

---

## 2. The Object-Graph Concern (Genesis)

The previous specification described IN parameters as "passed by value" with the callee receiving a copy. For value-like types (records of buffers, domains, unions of value-like candidates) this was a clean and isolating contract: the callee's copy was independent of the caller's slot, and any mutations the callee performed were confined to its frame.

The contract leaked for types whose values transitively reach external storage. Specifically:

- A pointer-typed `^T` IN parameter was explicitly permitted to be read-and-written-through during the call, even though the parameter's own slot could not be reassigned. This is recorded in CP001 §2.6 ("Reading from `^T` and writing to `^T` do not violate the no-non-local-state principle") and made operational in the operational-semantics reference §5.2 ("a non-writeable `^T` does not [permit pointer-swap] though the callee can still read from and write through the pointed-to storage during the call").

- An Object-typed IN parameter, by similar logic, presented a fat-pointer to a non-contiguous heap-resident value. The "copy" passed to the callee was a copy of the access mechanism, not of the object's reachable state. Mutations the callee performed through the object's pointer fields would propagate to anything else aliasing the same targets.

- More generally, any IN parameter of a type containing pointer fields, object fields, or variants over object-bearing candidates was subject to the same shallow-copy leakage.

CP010 §8.3 acknowledged this class of issues by attribution: "Aliasing through pointers, through indirect chains of calls, or through other dynamic mechanisms is not statically detectable. The user is responsible for these cases." This was a discipline-not-guarantee posture.

The user's review of OQ-2, in the operational-semantics reference's open-questions section, identified that the discipline-not-guarantee posture is in tension with the language's primary design goal of making programs easy to reason about. A programmer reading `IN x: SomeObject` should not have to reconstruct the object's reachable graph to know what the call may modify. The strong contract the name implies should hold.

This checkpoint settles the resulting design choice.

---

## 3. The Transitive READ Contract

### 3.1 Statement of the Rule

A READ parameter introduces a contract: the callee may not write, through any path, to storage reachable from that parameter. Reachability is transitive through pointer dereference, field access, indexing, and any other operation that yields a slot-view onto storage that the parameter aliases.

The contract is a callee-side promise verified at the type-checker level. A callee whose body would write through such storage fails to type-check, regardless of whether that write would be "observable" at any particular call site.

### 3.2 Operational Form: Taint Flow

The rule is enforced via a flow-sensitive *taint* property on slots and slot-views:

- A READ parameter introduces a tainted slot at the callee's frame entry.
- Operations that yield slot-views onto storage reachable from a tainted slot produce tainted slot-views (subject to the value-disjoint vs. slot-view discrimination — see OQ-2.2 in §7).
- A `<-` assignment from a tainted source taints the destination slot.
- An attempt to pass a tainted value or slot-view as the argument to a PRODUCE or REFERENCE position is a compile-time error.

Taint is local to the frame in which it originates. Because (5) of §1 prevents tainted values from flowing into writeable parameter slots, no taint crosses frame boundaries except as the originating-side of a fresh READ parameter at the next callee — which re-introduces taint freshly under that callee's frame, not by inheritance.

### 3.3 What the Rule Does Not Forbid

The rule forbids writes through reachable storage. It does not forbid:

- **Reading.** A callee may freely read through any path reachable from a READ parameter.
- **Local computation on read values.** A callee may compute fresh (untainted) values from reads; arithmetic on `Int32` values read through a READ parameter produces ordinary `Int32` results that are not themselves tainted.
- **Calling other commands with READ parameters that flow from the current READ parameter.** Passing a READ-derived value as an argument to another command's READ position is fine; the contract composes.

The rule is permission-restrictive, not data-flow-restrictive. The callee may inspect the data to its heart's content; what it may not do is write back.

### 3.4 Pointer-Field Semantics Under READ

A pointer-typed field of a READ parameter is itself read-only-through-this-access for the duration of the call. The callee may dereference it (a read), inspect what it points at (a read), follow chains of pointers (further reads), but may not write through it nor pass it to any callee in a writeable parameter position.

This supersedes the previous rule from CP001 §2.6 / operational-semantics reference §5.2 that permitted writing through a non-writeable `^T`. Under READ semantics, that write is forbidden.

The previous rule remains intact for pointers obtained outside the READ-reachable graph (e.g., a `^T` parameter passed in REFERENCE mode, or a `^T` constructed locally in the callee's frame from non-READ sources).

### 3.5 Composition with Captures

Lambda and fexpr captures of names that bind tainted slots inherit the taint at capture time:

- A lambda that captures a free name binding a READ parameter (or any value derived from one) carries the taint into its body. The body sees the captured value at READ semantics and cannot write through reachable storage.

- A fexpr (CP010) that captures a free name from an enclosing frame inherits taint at the point of capture. The fexpr's body, when invoked, observes the captured slot at the same taint level.

The aliasing analysis already specified in CP010 §8 for fexpr captures composes with taint propagation without further machinery; the taint flag is one more component of the per-slot state vector that the typechecker maintains.

### 3.6 Composition with Partial Application

CP008 §3.4 restricts partial application to IN-mode parameters and receivers. Under the rename, this becomes **READ-mode parameters and receivers**. The substantive semantics are unchanged: a partial-application captures a copy of the value supplied at the partial-application site, baking it into the lambda's hidden field set.

If the captured value is itself READ-tainted (because it was derived from a READ parameter at the partial-application site), the bake-in carries the taint into the lambda. The lambda's body sees the baked-in field at READ semantics. This is consistent with §3.5's general rule for captures.

If the captured value is untainted (a fresh value, an externally-supplied READ parameter at the partial-application site that has no enclosing READ context above it, etc.), the bake-in is untainted. The lambda's body sees the field at READ semantics either way (because the underlying parameter mode is READ); the taint distinction matters only if the lambda's body subsequently passes the field forward to another command's writeable parameter, which is forbidden in either case.

---

## 4. Mode-Naming Reframe

### 4.1 Old Names and Why They Are Retired

The names IN / productive / reference were inherited from a calling-convention frame: data-direction at the call boundary, with the callee treated as an opaque box that data flows into and out of. This frame fit when IN's contract was "the callee's local mutations are confined" — a property about what crosses the boundary.

Under the rule of §3, IN's contract is no longer about boundary-crossing. It is about *what the callee is permitted to do* with the parameter while inside its own frame. Specifically: the callee may read through reachable storage but may not write through it. This is a permissions property, not a data-direction property.

The old name IN does not capture this. The old name was also misleading in another direction: a programmer who knew IN as "input" might suppose that an IN object's reachable mutable state was fair game for mutation by the callee, on the grounds that the parameter is "going in" (accepted by the callee for the callee's use). This intuition is wrong under the new contract; the new name should not invite it.

### 4.2 New Names and What They Express

- **READ**: the parameter may be read by the callee, through any reachable path; it may not be written through any reachable path. The transitive read-only contract.

- **PRODUCE**: the callee is obligated to write the parameter's slot exactly once on every successful return path. Caller-side, the slot may be passed initialized or uninitialized; the callee will produce the value. The must-write-on-success obligation is what the name highlights.

- **REFERENCE**: the callee may read, may write, or may do neither; no obligation. Caller-side, the slot must be initialized at the call. The freest of the three modes from the callee's perspective; the most demanding of the caller in that it permits arbitrary mutations.

The three names are *roles*, not *directions*. Each describes what the callee is permitted/obligated to do; the caller's side is derivable from the callee's contract.

### 4.3 Markers Unchanged

The marker syntax remains:

- `'name` (and `Type'` in nameless type-expressions): PRODUCE.
- `&name` (and `Type&` in nameless type-expressions): REFERENCE.
- bare `name`: READ.

This is a vocabulary change at the document level, not a syntax change.

### 4.4 Triple-Duty Use of "REFERENCE" and Its Acceptance

The word "reference" already carries multiple meanings in language design and in this language's prior vocabulary:

- A parameter mode (the mode formerly called "reference," now uppercased as REFERENCE).
- A type — pointers `^T` are widely called "references" in informal usage.
- A general operational-machinery term for slot-views derived from non-local origins (e.g., the reference-chain flattening rule of CP008 §5.1).

The triple use is permitted because the three readings are not at odds: a REFERENCE parameter is precisely a slot-reference made first-class at the call boundary, and a `^T` value is precisely a reference-as-data. If this conflation later proves problematic, the parameter mode may be renamed (candidates discussed but not adopted: MODIFY, ALTER, READ-WRITE); for now, the conflation is accepted.

---

## 5. PRODUCE Outputs Under the Transitive Contract

This section addresses what was originally raised as the "return values aliasing READ inputs" question and explains why it collapses.

### 5.1 The Question Restated

In a calling-convention frame: if a callee has a READ parameter `x` and a PRODUCE parameter `'y`, may the callee write the value of `x` (or some value derived from `x`) into `'y`? If so, the productive output `'y` aliases storage reachable from `x`, and a caller using `'y` to mutate would inadvertently mutate state reachable from the original `x` — defeating READ.

### 5.2 The Resolution

Under the rule of §1.5: a READ-tainted value cannot flow into a writeable parameter slot. PRODUCE parameters are writeable parameter slots. Therefore, no callee body that satisfies the type-checker can write a READ-tainted value into a PRODUCE output. The PRODUCE output is structurally guaranteed to carry no taint inherited from the callee's READ inputs.

### 5.3 What This Means at the Caller Side

When a caller receives a PRODUCE output from a callee, the caller does not need to ask "is this output tainted by some READ input I supplied?" The answer is always no: the output is fresh from the callee's perspective with respect to taint. The caller may treat the output as untainted in its own frame.

This collapses what would otherwise have been a signature-annotation burden. No "may-be-derived-from-READ" annotation is needed at the signature level, because no PRODUCE output can be so derived. The contract holds modularly: each callee's body either type-checks under the rule or it does not, and the caller can rely on the rule's guarantee without inspecting the callee's body.

### 5.4 Forbidden Patterns and Why They Are Correctly Forbidden

Two patterns that the strict rule forbids and that programmers might initially expect to write:

**Field extraction returning a pointer.** A callee `getField: READ o: ^MyObj, 'fp: ^FieldT` that intends to set `'fp` to point at a field of `o`. The strict rule forbids this — and rightly: permitting it would let the caller use `'fp` to write into `o`'s field, defeating READ. The pattern is forbidden, the forbiddance is correct, no escape hatch is wanted.

**Identity pass-through.** A callee that takes a READ parameter and produces "the same value" through a PRODUCE output. The strict rule forbids this; the caller already has the value; no honest reason to round-trip exists. Also correctly forbidden.

The strict rule has no false positives that the language must accommodate. The patterns it forbids are patterns that *should* be forbidden.

---

## 6. Composition with Existing Mechanisms

### 6.1 Failure-Mode Analysis (CP011)

The failure-mode analysis is a forward-flow CFG walk over per-program-point state. The taint analysis introduced here is also a forward-flow CFG walk over per-program-point state. The two share machinery: the per-slot state vector that the typechecker already maintains for failure-mode and initialization tracking gains a taint flag. Walks remain single-pass.

No new control-flow primitives are introduced. Recovery contexts (`?-`, `?:`, `|`, `??`) preserve taint as they preserve initialization state — values bound through recovery handlers carry their taint, and a recovered failure value may itself be tainted if it originated from a READ parameter (though this requires a callee-side propagation of READ-derived data into a `.fail`, which is itself a write through reachable storage and therefore forbidden in most cases — the failure-system reference's typed-failures material applies).

### 6.2 Init-State Analysis (CP004 §3)

Same composition story. The init-state per-slot flag and the taint per-slot flag are independent dimensions of the per-slot state. No interaction other than co-location in the analysis.

### 6.3 Pointer Operations

Pointer dereference (`p^`) on a tainted `p` produces a tainted slot-view. Pointer construction (`x&`) from a tainted source produces a tainted pointer. Indexing (`b[i]`) on a tainted buffer produces a tainted slot-view if the element type contains references; produces a fresh untainted value if the element type is value-like. (See OQ-2.2 in §7 for the precise rule.)

### 6.4 Lambda and Fexpr Captures

Already covered in §3.5. The taint flag composes with the existing capture machinery.

### 6.5 Receivers

Receivers (CP005 §§2.3, 3) take parameter modes. Under the rename:

- Constructor receivers are PRODUCE (the constructor fills the receiver).
- V-command method receivers are typically REFERENCE.
- At-stack-exit method receivers are REFERENCE.
- A method that only inspects its receiver (a "pure query" pattern) declares its receiver READ.

The transitive contract applies to receivers exactly as it does to ordinary parameters. A READ receiver cannot have its reachable storage written through, including by the receiver's own methods called recursively.

---

## 7. Open Sub-Questions and Revisited Legacy Sub-Questions

This section catalogs the remaining work in the OQ-2 thread.

### 7.1 OQ-2.1: Per-Slot Taint as Flow-Sensitive State

The model of per-slot taint as a flow-sensitive flag tracked alongside init-state and failure-mode in the typechecker's CFG walk is sketched in §3.2 and §6.1. The full specification — including the precise transfer functions for each operation that introduces, propagates, or could be argued to clear taint; the treatment of taint at branch joins; the interaction of taint with the recovery-handler binding rule — is not pinned down here.

Tentative shape:

- Taint is a property of slots and slot-views.
- Taint is introduced at READ parameter binding and at slot-views derived from tainted slots.
- Taint propagates through `<-` assignment from tainted sources.
- Taint never clears (within a frame); a fresh untainted value is produced only by an operation that is value-disjoint with respect to its tainted inputs.
- At branch joins, taint is the disjunction over branches: a slot is tainted at the join if it is tainted on any incoming branch.
- A slot whose taint becomes irrelevant (because the slot is overwritten with a fresh untainted value) is re-flagged as untainted.

The full transfer-function specification is left for the typechecker-implementation thread.

### 7.2 OQ-2.2: Value-Disjoint vs. Slot-View Discrimination

When does an operation on a tainted source yield a tainted slot-view, and when does it yield a fresh untainted value? The rule sketched in §6.3 — "result type is value-like → untainted; result type contains references → tainted" — appears to fall out naturally, but several cases need precise treatment:

- Records with embedded pointer fields: byte-copying the record copies pointer bits but does not deep-copy. Reading a record from a tainted slot must produce... what? A tainted record view? A fresh-but-tainted value (because its pointer fields still alias READ-reachable storage)? The latter, presumably, because the pointer fields reach back into the original READ graph.

- Variants over object candidates: similar to records with pointer fields.

- Buffer indexing where the element type itself contains references: indexing into a `[Buffer ^T]` from a tainted buffer yields a tainted `^T`.

- Domain values derived from byte-buffers within a tainted record: the bytes are read and reinterpreted as a domain value (an `Int32`, say). The result is value-disjoint from the buffer's other state and is reasonably untainted.

The rule is approximately "if the operation yields a value whose representation includes any reference reaching storage reachable from the tainted source, the result is tainted; otherwise the result is untainted." Pinning this precisely requires walking through every type former and every operation.

### 7.3 Revisited Legacy OQ-2 Sub-Questions

The original OQ-2 sub-questions from CP001 §6:

**(a) Conditions under which large IN parameters may be passed by reference under the hood (no aliasing through other parameters, no escape of the parameter address, no observation of mutation through the reference).** Largely settled by the READ contract: the type system already precludes the mutations that would make by-reference passing observable, so the implementation may freely pass READ parameters by reference. The remaining question is whether the language *commits* to by-reference passing as the implementation strategy (which would inform programmer mental models for performance) or leaves it as latitude. The user's earlier preference was for a commitment to by-reference passing with a compiler diagnostic when a parameter's data is large enough that by-reference would be the obviously-better choice; this is recorded but not finalized.

**(b) Interaction with macro/fexpr-like semantics: the `:{...}` block-quote and `:<...>{...}` command-literal forms.** Still open. The taint discipline must extend to these forms; in particular, the block-quote `:{...}` (CP007 §2.1) eagerly evaluates its body and constructs a value, and the parameter-passing treatment of READ parameters within the eagerly-evaluated body needs specification. Forwarded to the lambda-and-fexpr reference.

**(c) Correctness criterion for "unobservable divergence" in a language with first-class commands and writeable parameters.** Reframed under the READ contract: the criterion is now "any implementation strategy is permitted whose observable effect coincides with that of the by-value semantics under the transitive READ contract." Because the READ contract already precludes the writes that would distinguish by-reference from by-value, the criterion is automatically satisfied for READ parameters. PRODUCE and REFERENCE remain at copy-restore semantics; their criterion is unchanged.

---

## 8. Summary of Changes from Prior Checkpoints

| Topic | Prior State | This Checkpoint |
| --- | --- | --- |
| Parameter mode names | IN / productive / reference | READ / PRODUCE / REFERENCE (§§1.1, 4) |
| Marker syntax | `'` and `&` | Unchanged (§1.1, §4.3) |
| IN/READ contract | Callee's mutations to its copy are confined | Callee may not write through any reachable storage (§§1.2, 3) |
| Taint analysis | Not present | Per-slot flow-sensitive flag tracked alongside init-state and failure-mode (§§3.2, 6.1) |
| Pointer-field writes through non-writeable `^T` | Permitted (CP001 §2.6) | Forbidden when reached from a READ parameter (§3.4) |
| PRODUCE outputs derived from READ inputs | Unspecified concern | Structurally forbidden by taint rule; no signature annotation needed (§§1.6, 5) |
| Pointer aliasing posture | Discipline-not-guarantee (CP010 §8.3) | Compiler-enforced for the READ-reachable case (§3) |
| Partial-application restriction | IN-mode only (CP008 §3.4) | READ-mode only; same restriction under new name (§3.6) |
| OQ-2 (a), (b), (c) | Open | (a) largely settled; (b) reframed and forwarded; (c) reframed and largely settled (§7.3) |
| OQ-2.1, OQ-2.2 | N/A | New sub-questions opened (§§7.1, 7.2) |

---

## 9. Provenance

**Authored:** Distilled from continued intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-30.

**Source materials read for this checkpoint:** intent-checkpoint-001.md, intent-checkpoint-002.md, intent-checkpoint-004.md, intent-checkpoint-005.md, intent-checkpoint-007.md, intent-checkpoint-008.md, intent-checkpoint-010.md, intent-checkpoint-011.md, intent-checkpoint-012.md; reference-failure-system.md; reference-operational-semantics.md (in-progress).

**Grammar changes implied:** None at the syntactic level; markers and surface forms are unchanged. Documentation conventions update (rename of mode terms throughout future references and grammar comments).

**Reference-document impact:** The operational-semantics reference §5.2 ("Parameter Modes (Operational Behavior)") and §8.1 (OQ-2 cataloging) require revision. Recommended approach: revise once after the OQ-2 thread fully closes (after OQ-2.1 and OQ-2.2 are resolved), rather than churning the reference twice on the same OQ. The planned type-and-modes reference will use the new names from inception. The failure-system reference is unaffected by this checkpoint.

**Recommended next step:** Continue the OQ-2 thread with OQ-2.1 (per-slot taint state model), then OQ-2.2 (value-disjoint vs. slot-view discrimination). After both close, revise the operational-semantics reference §§5.2 and 8.1 in a single consolidated edit, and proceed to the type-and-modes reference (next per the agreed topic split, where the new mode names will be carved into the authoritative material).
