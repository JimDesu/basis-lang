# Basis Language — Intent Checkpoint 014

**Status:** Working draft. Builds on intent-checkpoint-001.md through intent-checkpoint-013.md; supersedes them where they conflict. This checkpoint resolves OQ-3 (the `-> name` result designator's semantics on non-writeable parameters) by adopting CP001 §6 reading (a) for READ parameters, with a unified statement of `-> name`'s meaning across all three parameter modes. The checkpoint also acknowledges OQ-8's standing status (substantively settled by CP002 §4; remaining bookkeeping-representation sub-question forwarded to implementation, paralleling OQ-2.1 from CP013).

**Date of this checkpoint:** April 2026

**Provenance:** Continuation of the intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-30, in the operational-semantics review thread following CP013's closure.

**Authority statement.** Where this document differs from earlier checkpoints, this document is authoritative. Where this document and the source code differ on syntactic matters, the code is authoritative; where they differ on semantic intent, this document is authoritative. The README and `language_notes.txt` remain non-authoritative.

---

## 1. Decision Summary

This checkpoint lands the following decisions:

1. **OQ-3 is resolved.** The `-> name` result designator (CP001 §3.3) is permitted on parameters of all three modes — READ, PRODUCE, REFERENCE — with a unified meaning derived from the parameter's mode (see §2).

2. **For READ parameters, `-> name` evaluates to the *initial* value the caller passed in.** This is CP001 §6 reading (a) for the OQ-3 sub-questions: identity-shaped passthrough, with no back-door output channel for transformed values. The READ-as-permission framing established by CP013 is preserved — the only way to surface a transformed value as a result is via PRODUCE.

3. **The READ form is well-formed even when the command has no writeable parameters.** The predicate-with-passthrough idiom — `(validate: x -> x)`, where validation is a may-fail check whose success makes `x` available to the next stage in an expression chain — is a primary use case and is supported.

4. **For PRODUCE parameters, `-> 'name` evaluates to the post-write-back value from copy-restore** — the value the callee produced. (This is the existing meaning, recorded explicitly here for completeness.)

5. **For REFERENCE parameters, `-> &name` evaluates to the post-call value of the slot** — the callee's possibly-modified copy after copy-restore. This is the builder / fluent-interface case: chained operations on a receiver each surface the (possibly modified) receiver as the expression value for the next call in the chain.

6. **Failure behavior is uniform across all three modes.** If the call fails, the expression evaluation fails, and the pre-call state of `name` in the caller's frame is preserved (failure-atomicity unchanged from CP001 §3.2 / CP004 §1).

7. **OQ-8 is acknowledged as substantively settled by CP002 §4** — registration migration semantics, exactly-once cleanup discipline. The remaining sub-question is the precise representation of registration bookkeeping in the runtime (list vs. graph, layout within stack-frame data); this affects implementation but not user-visible semantics, and is forwarded to the implementation thread (paralleling OQ-2.1's forwarding from CP013).

---

## 2. The `-> name` Result Designator: Unified Meaning Across Modes

### 2.1 Background

The `-> name` clause in a command's signature designates which parameter (or receiver) becomes the expression-position result when the command is invoked in expression context (CP001 §3.3). The well-formedness rule requires `name` to be a parameter or receiver — full stop. It is *not* required to be writeable.

CP001 §6 raised the question of what `-> name` means when `name` designates a non-writeable parameter (now READ under CP013's rename). Three readings were proposed:

(a) The result is the *initial* value the caller passed in.

(b) The result is the value as observed inside the callee at the moment of return — the callee's local copy, possibly mutated locally during the body.

(c) Restricted to specific patterns yet to be specified.

CP004 §6.1 partially resolved this by ruling out productive semantics for READ: under the no-defaults model, a READ parameter's local doesn't write back, so the surface form, if meaningful at all, has to be (a) or (b).

This checkpoint settles on **(a)** and gives the unified statement across all three modes.

### 2.2 The Unified Statement

For a command invocation of the form `(cmd: args)` whose signature includes `-> name`, where `name` is a parameter (or receiver) of the command:

- **If `name` is a PRODUCE parameter (`'name`):** the expression value is the post-write-back value from copy-restore — the value the callee produced and committed via the productive contract. Failure of the call propagates as expression failure; no post-write-back occurred, so no value is lifted.

- **If `name` is a REFERENCE parameter (`&name`):** the expression value is the post-call value of the slot — the callee's possibly-modified copy after copy-restore. Failure of the call propagates as expression failure; no copy-restore occurred, so the caller's slot retains its pre-call value but the expression evaluation fails.

- **If `name` is a READ parameter (no marker):** the expression value is the *initial* value the caller passed in — the value at the call site, before the call begins. Failure of the call propagates as expression failure.

### 2.3 The READ Case in Detail

For READ, the expression value is determined *before* the call runs. This has three consequences worth surfacing:

**Pure-effect commands are well-formed.** A command with only READ parameters (no writeable parameters) and `-> READ_param` is well-formed. The `cmd -> x` form evaluates to whatever `x` was at the call site; the call itself contributes via its observable effects (success/failure outcome, any at-stack hook firings, any sub-call effects). The most common case is a may-fail check whose outcome gates further computation — the predicate-with-passthrough idiom.

```
(validate: x -> x)
```

If `validate` succeeds, the expression evaluates to `x`. If `validate` fails, the expression fails and the failure propagates (with whatever payload `validate` carries under typed failures, per the failure-system reference). The next stage in an expression chain receives `x` only if validation succeeded.

**No back-door output channel.** Reading (b) — where the callee's local-after-mutation surfaces as the expression value — would have created an output channel on a mode the language otherwise treats as input-only. Reading (a) preserves CP013's READ-as-permission framing: READ describes what the callee may *do*, not what the callee *produces*. To surface a transformed value as a result, the callee must use a PRODUCE parameter explicitly.

**Identity-shaped passthrough is the canonical pattern.** When the same name appears as both the input the caller threads through and as the `-> name` designator, the form effects "use this command for its effect; pass the same value forward." This is structurally similar to threading a value through a sequence of validations, conversions, or instrumentation calls without changing the value's identity.

### 2.4 The REFERENCE Case in Detail

For REFERENCE, the expression value is the callee's possibly-modified copy after copy-restore. This is the builder / fluent-interface case. Chained method calls on a receiver — common in Java's StringBuilder, Scala's Builder pattern, jQuery's chain syntax — work because each method surfaces the (possibly modified) receiver as the expression value for the next chained call.

Under Basis's parameter-mode framework, the receiver in such patterns is REFERENCE-mode (per the v-command receiver convention from CP005). A v-command method whose signature includes `-> &receiver` lifts the post-call receiver value as the expression result, allowing the chained-call idiom.

The unified statement applies: the expression value is the post-call slot value. For a method that did not modify the receiver, this is bit-identical to the pre-call state; for a method that did modify the receiver, the modifications are visible in the lifted value (and, equivalently, in the caller's slot via copy-restore).

### 2.5 The PRODUCE Case in Detail

For PRODUCE, the expression value is the post-write-back value from copy-restore. This is the existing meaning of `-> 'name` and is the ordinary case for "use this command in expression position to produce a value." Constructor-style commands are typically written this way, with `-> 'p` making the bare invocation form valid in expression context, lifting the produced value.

This case is unchanged from earlier semantics; it is recorded here for completeness alongside the other two modes.

### 2.6 Failure-Atomicity Preserved

In all three cases, failure of the call propagates as expression failure. The pre-call state of `name` in the caller's frame is preserved (CP001 §3.2 / CP004 §1 failure-atomicity, unchanged). For PRODUCE, no post-write-back occurred, so no value is lifted; for REFERENCE, no copy-restore occurred, so the caller's slot retains its pre-call value but the expression fails; for READ, the value was already lifted before the call, but the expression as a whole still fails because the call itself failed. The discipline is: if any sub-expression fails, the whole expression fails.

---

## 3. OQ-8 Status Acknowledgment

### 3.1 Substantive Settlement

CP002 §4 substantially settled OQ-8 (at-stack-exit method registration mechanics). The settled material covers:

- At-stack methods may have any failure mode (`:`, `?`, `!`).
- Manually invoked at-stack methods follow ordinary failure-handling rules.
- Automatically invoked at-stack methods have their failure status silently ignored.
- At-stack methods are virtual in the class-dispatch sense.
- On successful copy-restore involving a writeable `^Object` parameter that swaps to a new object, the new object's at-stack registration migrates from the callee's frame to the caller's frame.
- On failure during a call that allocated a new object, the new object remains registered with the failed callee's frame and is cleaned up by that frame's failure-exit mechanism.
- Every constructed object's at-stack methods fire exactly once, on some frame's exit.

### 3.2 The Remaining Open Sub-Question

OQ-8's remaining open sub-question (as recorded in CP002 §4) is the precise representation and bookkeeping of registration migration in the runtime: whether registrations are tracked as a list, a graph, embedded in stack-frame data, or by some other structure. This affects implementation but not user-visible semantics.

### 3.3 Forwarding

This sub-question is forwarded to the implementation thread, paralleling OQ-2.1's forwarding from CP013 §7.1. The implementation must guarantee that every constructed object's at-stack methods (if any) are eventually invoked by some frame's exit, exactly once; the precise representation that achieves this guarantee is the implementation's choice.

---

## 4. Summary of Changes from Prior Checkpoints

| Topic | Prior State | This Checkpoint |
| --- | --- | --- |
| OQ-3 (`-> name` semantics) | Open (mostly resolved by CP004 §6.1 — narrowed to readings (a) or (b)) | **Resolved**: reading (a) for READ; unified meaning across all three modes (§2) |
| `-> name` on READ parameters | Underspecified | Evaluates to caller-passed initial value; predicate-with-passthrough idiom supported (§2.3) |
| `-> 'name` on PRODUCE parameters | Implicit ("post-write-back value") | Stated explicitly (§2.5) |
| `-> &name` on REFERENCE parameters | Implicit (builder pattern by convention) | Stated explicitly; builder/fluent-interface idiom recorded (§2.4) |
| Failure behavior of `-> name` | Implicit | Uniform across all three modes; failure-atomicity preserved (§2.6) |
| OQ-8 (at-stack registration bookkeeping) | Substantively settled by CP002 §4; open sub-question on representation | Acknowledged; representation forwarded to implementation thread (§3) |

---

## 5. Provenance

**Authored:** Distilled from continued intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-30, immediately following CP013's closure during the operational-semantics review thread.

**Source materials read for this checkpoint:** intent-checkpoint-001.md (OQ-3 origin, `-> name` mechanism); intent-checkpoint-002.md (OQ-8 origin, at-stack mechanism); intent-checkpoint-004.md (CP004 §6.1's narrowing of OQ-3 to readings (a) or (b)); intent-checkpoint-005.md (receiver modes, marker syntax); intent-checkpoint-013.md (READ-as-permission framing); reference-operational-semantics.md (the operational-semantics reference whose §§5.3, 8.2, 8.4 prompted this resolution).

**Grammar changes implied:** None. The `-> name` clause grammar is unchanged; this checkpoint specifies its semantics across the three parameter modes.

**Reference-document impact:** The operational-semantics reference §5.3 and §8.2 will be revised in a future consolidated pass to fold this resolution into the body text. The §8.4 entry for OQ-8 (added during the same review thread) points at this checkpoint's content.

**Recommended next step:** Resume topic-organized reference work. Per the failure-system reference's handoff §10, the next reference is **Type System and Modes**, covering buffers/ranges/domains/aliases/records/objects/unions/variants/pointers/command-types as types and parameter-mode markers (READ, PRODUCE, REFERENCE) as type-level features, with the new naming carved into the authoritative material from inception.
