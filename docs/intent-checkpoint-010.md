# Basis Language — Intent Checkpoint 010

**Status:** Working draft. Builds on intent-checkpoint-001.md through intent-checkpoint-009.md; supersedes them where they conflict.
**Date of this checkpoint:** April 2026
**Provenance:** Continuation of the intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-28. This checkpoint resolves OQ-7 (fexpr design) by establishing implicit-capture-by-free-name as the capture model, introducing the per-invocation invocation frame for body-local state, formalizing the **locality rule** that constrains fexpr-typed slots to local construction and IN-parameter passing only, and committing to direct (non-copy-restore) semantics for captured-slot access. The checkpoint also pins down statement separation, re-entry, nesting, reference-chain flattening, failure-mode integration, and aliasing.

**Authority statement.** Where this document differs from earlier checkpoints, this document is authoritative. Where this document and the source code differ on syntactic matters, the code is authoritative; where they differ on semantic intent, this document is authoritative. The README and `language_notes.txt` remain non-authoritative.

---

## 1. Decision Summary

This checkpoint resolves OQ-7 with the following decisions:

1. **Captures are implicit by free name.** A fexpr body's free names that resolve to slots in the constructor's lexical scope are captured automatically. There is no explicit capture list. This contrasts deliberately with lambda captures (per checkpoint 008 §4), reflecting fexprs' narrower role and structurally-local lifecycle.

2. **Each invocation has its own invocation frame.** Locals introduced via `#` in the body live in a fresh per-invocation frame, distinct from both the capture frame and the invoker's frame. The invocation frame is the introducing frame for any objects, lambdas, or sub-fexprs constructed by the body; their lifecycles are bounded by the invocation.

3. **The locality rule constrains fexpr-typed slots.** A fexpr value may inhabit only a `#`-introduced local in the capture frame (with construction as its sole assignment) or an IN parameter slot of a command. Productive and reference fexpr-typed parameters are forbidden; pointers to fexpr-typed slots are forbidden; fexpr-typed object fields and variant candidates are forbidden; bare-identifier copy of fexpr values is forbidden. Together, these restrictions guarantee that fexprs cannot escape upward.

4. **Captured-slot access is direct, not copy-restore.** When a fexpr body reads or writes a captured slot, it operates on the capture-frame slot directly. Failures mid-body do not roll back captured-slot writes. This is a deliberate and meaningful difference from lambda reference captures (checkpoint 008 §5.4), motivated by the use case for fexprs: control-flow combinators where partial-mutation-on-failure is sometimes the desired semantics.

5. **Captures must be initialized at construction time.** A fexpr's captured slots must be in the `init` state at fexpr-construction. The body may freely read and write them. The body's analysis treats captured slots as initialized at every body entry.

6. **Body statements are separated by indentation.** Fexpr (and lambda) bodies use the same whitespace-significant statement separation as the rest of the language. There is no semicolon syntax for inline statement sequencing.

7. **Re-entry and nesting are permitted.** A fexpr may invoke itself recursively (each invocation gets its own invocation frame); a fexpr body may construct other fexprs, lambdas, and objects (each with its own lifecycle). The mechanisms compose naturally.

8. **Reference-chain flattening applies to captures.** A captured `&y` where `y` is a reference parameter of the constructor's frame flattens to the slot at the chain's origin, paralleling checkpoint 008 §5.1. The locality rule still applies to the flattened result.

9. **Failure-mode propagation follows the standard never-fails-must-handle rule.** A fexpr's marker (`:`, `?`, `!`) constrains its body's failure semantics; an invoking command must consume invoked-fexpr failures per the standard rule (checkpoint 002 §1.4).

10. **Aliasing is detected where statically possible.** A fexpr that captures slot `X` and is invoked from a context where `X` is also being passed as a writeable argument creates an aliasing concern; the typechecker flags such conflicts where it can detect them.

The locality rule resolves the typechecker's job for fexprs cleanly: ceiling-tracking reduces to a slot-usage check rather than the more elaborate cross-frame analysis needed for ceiling-tracked lambdas. The simpler rule fits fexprs' structural role.

The checkpoint also introduces one small open sub-question (shadowing of captured names by body-introduced locals) and resolves OQ-7.

---

## 2. The Capture Model: Implicit by Free Name

### 2.1 The Mechanism

A fexpr body is a sequence of statements that may freely reference names in the surrounding lexical scope. The typechecker computes the body's **free-name set** — names referenced in the body that are not bound by body-introduced `#` locals — and partitions it:

- Names resolving to **slots** in the surrounding lexical scope (locals, parameters, receivers) are **captured**. The typechecker emits, at fexpr-construction, a slot reference for each captured name.
- Names resolving to **top-level definitions** (commands, types, classes, instances) are **not captured**. They are referenced via the language's normal name-resolution and are not part of the fexpr's identity.

The capture set is fully determined by the body's text. The user does not write a capture list; the typechecker computes it.

### 2.2 Why Implicit, Departing From Lambdas

Checkpoint 008 §4.2 introduced explicit capture lists for lambdas, motivated by refactor safety and the visibility of mutating effects. Fexprs depart from this convention deliberately:

- **Locality of use.** Fexprs are constrained by the locality rule (§4) to live within their capture frame, used immediately by surrounding commands. They do not travel across construction boundaries the way lambdas can.
- **Surface lightness for the canonical use case.** The motivating use of fexprs is control-flow combinators with the body written inline at the call site (`while: ?{lessThan: i, 10}, ?{...}`). An explicit capture list (`?{ / i / lessThan: i, 10 }`) would add noise to short bodies for limited refactor-safety gain.
- **Uniform capture mode.** Lambdas have IN and reference capture modes, with mode markers in the slash list disambiguating them; fexprs have only one capture mode (slot-reference, with direct access), so a list would be just names. The mode-disambiguation argument for explicit lists doesn't apply.

The asymmetry between lambdas and fexprs is itself part of the language's design vocabulary: explicit captures signal "this value travels"; implicit captures signal "this value stays local." A user reading `:<args / captures>{body}` knows immediately the value may travel; a user reading `?{body}` knows it doesn't.

### 2.3 What the Typechecker Reports

The typechecker's diagnostic output for fexprs makes the (otherwise invisible) capture set visible when relevant:

- At a fexpr-construction site, on hover or via tooling, the inferred capture set is available.
- In error messages — particularly locality-rule violations and ceiling-related rejections — the offending capture is named explicitly. The user does not have to scan the body to find what's pinning the lifecycle.

This tooling is implementation-defined; the language commits to making the information available, not to a specific surface.

### 2.4 Shadowing of Captured Names

A body-introduced local via `# r` may share a name with a slot in the capture frame. The natural rule is **shadowing**: within the body, `r` refers to the local; the outer `r` is inaccessible. This parallels how parameter names shadow enclosing-scope names in regular commands.

The stricter alternative is to make this a compile-time error, paralleling checkpoint 005 §1.2's same-scope rule for `&x` and `x`. The argument for the stricter rule is consistency; the argument against is that body-locals are already in a different (invocation) frame, so there is no actual collision — the shadowing is structural, not coincidental.

This checkpoint commits to **shadowing permitted**. A body-introduced local with the same name as a captured slot shadows the capture; the captured slot is inaccessible from inside the body for the duration of the local's scope. This is **OQ-25**, flagged as small and resolvable either way; the choice here is the conventional one.

---

## 3. The Invocation Frame

### 3.1 Per-Invocation Sub-Frame

Each invocation of a fexpr establishes a fresh **invocation frame**. This frame:

- Is created at the fexpr's invocation entry; destroyed at invocation exit.
- Is structurally a sub-frame of the **invoker's** frame (pushed onto the call stack when the invocation begins, popped at exit).
- Provides storage for body-introduced locals (`# x` declarations in the body).
- Is the **introducing frame** for any objects, lambdas, sub-fexprs, or other ceiling-bearing values constructed in the body.
- Has its own at-stack registration list for cleanup of body-introduced objects.

The invocation frame is distinct from both the capture frame and the invoker's frame. It is hygienic in the sense that body-locals do not escape, do not leak between invocations, and do not interfere with the invoker's locals.

### 3.2 Captured-Slot Access from Within the Invocation Frame

The invocation frame provides access to capture-frame slots via the fexpr's stored slot references. From the body's perspective:

- Reading a captured name reads the current contents of the capture-frame slot.
- Writing a captured name writes to the capture-frame slot.
- The reads and writes are **direct** — not copy-restore. The body operates on the capture-frame slot as if it were a local; failures mid-body do not roll back captured-slot writes.

This contrasts with lambda reference captures (checkpoint 008 §5.4), which use per-invocation copy-restore. The difference is deliberate (§5 below).

### 3.3 Cleanup and At-Stack Behavior

When the invocation frame exits, its at-stack registrations fire. Any objects, lambdas, or sub-fexprs constructed by the body during this invocation are cleaned up here. Failure during the body is handled at the invocation frame's exit coda; the failure (if any) propagates to the invoking frame after cleanup.

If the invocation propagates a failure, captured-slot writes performed before the failure remain in place (per the direct-access rule). This is how fexprs differ from copy-restore-bearing constructs: the failure does not roll back the captured-slot mutations, and the invoking frame sees both the propagated failure and the (partially) mutated capture-frame state.

### 3.4 Re-Entry Safety

Each invocation has its own frame; concurrent invocations (whether from re-entry within a single invocation, or from invocations interleaved through any other mechanism) have independent locals and at-stack registrations. The captured-slot references are shared across invocations, but the body-locals and lifecycle bookkeeping are not.

A fexpr that recursively invokes itself thus operates on a stack of independent invocation frames. Non-termination is the user's concern; the language imposes no recursion-depth limit beyond the host's stack capacity.

---

## 4. The Locality Rule

### 4.1 Statement of the Rule

A fexpr value may inhabit only the following positions:

- A `#`-introduced local slot in the capture frame, with construction as the slot's **sole assignment**. The construction takes the form `# f <- :{...}` or `# f <- ?{...}` or `# f <- !{...}`. The slot may not be subsequently overwritten, copied to another slot, captured by reference, or placed in a pointer.
- An IN parameter slot of a command, populated by argument-passing from a frame at-or-below the capture frame. The parameter is invoked by the receiving command but is otherwise opaque.

Beyond invocation, no other operation is permitted on a fexpr-typed slot.

### 4.2 The Auxiliary Restrictions

The locality rule's force comes from a set of auxiliary restrictions on the type system. Each closes off a distinct path by which a fexpr could leak above its capture frame:

**Restriction A: No productive or reference fexpr-typed parameters.** A parameter of fexpr type may carry only the IN mode marker (or no marker, equivalently). The productive `'` and reference `&` markers are forbidden on fexpr-typed parameters.

The reasoning: a productive fexpr-typed parameter would license the callee to write a fexpr value to the caller's slot, migrating a fexpr constructed in the callee's frame upward — exactly the leak the rule prevents. A reference fexpr-typed parameter would license the callee to write through the slot reference, with the same leak risk.

**Restriction B: No pointers to fexpr-typed slots.** The type `^Fexpr` (where `Fexpr` is any fexpr type) is forbidden. Pointers can outlive their target slots in the absence of explicit lifetime tracking, and the language does not track lifetimes of pointer targets at the granularity needed to ensure a fexpr-pointer is never dereferenced after its target slot has been destroyed.

**Restriction C: No fexpr fields in objects, variants, or records.** Object fields and variant candidates may not have fexpr type. (Records cannot have fexpr fields anyway, since records are buffer-backed and fexprs are not — but the prohibition is stated explicitly for clarity.)

The reasoning: a fexpr stored in an object field would have a lifecycle tied to the object's lifecycle, which is not bounded by the fexpr's capture frame. The fexpr could be reached after its capture frame has exited.

**Restriction D: No bare-identifier copy of fexpr values.** The bare-identifier `<-` form (`# f2 <- f1`) is forbidden when both slots have fexpr type. The construction site (`# f <- :{...}`) is the only valid assignment; any further migration of the value is forbidden.

The reasoning: a bare-identifier copy creates a second slot holding the same fexpr value. Even when both slots' lifetimes are bounded by the same frame (and so neither slot would dangle), the second-slot pattern admits patterns that the simpler rule rejects, and the simpler rule is what the typechecker enforces.

**Restriction E: No fexpr captures by lambdas.** A lambda may not capture a fexpr-typed slot (whether by IN copy or by `&` reference). IN capture would create a copy of the fexpr in the lambda's hidden field, violating Restriction D. Reference capture would require writing to the slot, which Restriction A's reasoning forbids.

The reasoning: lambdas can travel; the locality rule confines fexprs; a lambda that captured a fexpr would carry the fexpr beyond its capture frame.

### 4.3 What the Locality Rule Achieves

Together, the locality rule and its restrictions ensure that a fexpr value cannot reach any frame above its capture frame at any point in its lifetime. The proof is structural:

- The fexpr is constructed in the capture frame and assigned to a `#`-local there.
- The only further operations on the value are invocation and argument-passing as IN.
- IN argument-passing pushes the value into a sub-frame's parameter slot; that slot's lifetime is bounded by the sub-frame, which is below the capture frame.
- No other operation can place the value in a frame above the capture frame.

When the capture frame exits, all sub-frames have already returned (their fexpr-bearing parameter slots are destroyed); the fexpr's `#`-local is destroyed; the value ceases to exist. No reference to the fexpr can persist.

### 4.4 Why The Stricter Rule, Not Cross-Frame Ceiling Tracking

The ceiling-tracking machinery from checkpoint 008 (used for lambdas with reference captures) could in principle handle fexprs more flexibly, with reference-chain flattening enabling the helper-builds-fexpr-and-returns-it pattern. The locality rule deliberately rejects this flexibility for fexprs.

The reasoning is cost-benefit: the canonical fexpr use case (control-flow combinators with body-at-call-site) does not need the cross-frame flexibility, and the simpler rule is significantly easier to teach, parse, and check. Cross-frame fexpr lifetime tracking would also interact awkwardly with the direct-access semantics (a fexpr that survives across helper boundaries, with direct access to the helper's frame slots, would have to deal with the helper's frame having exited — which the simpler rule structurally forbids).

For the use cases that *would* benefit from the more flexible mechanism, ceiling-tracked lambdas (checkpoint 008) provide it. The two mechanisms — fexprs (strict locality, direct access, no ceiling tracking) and ceiling-tracked lambdas (cross-frame, copy-restore, ceiling tracking) — cover different parts of the design space.

### 4.5 Reference-Chain Flattening Within the Locality Rule

A fexpr's captured slot may be a reference parameter of the constructor's frame. The capture follows the reference chain to the slot's origin frame, paralleling checkpoint 008 §5.1.

This composes with the locality rule:

- The captured slot's origin frame may be at or above the constructor's frame.
- The fexpr is still confined to the constructor's frame (not to the origin frame) — the locality rule is about the fexpr value's residency, not the captured slot's origin.
- The body's reads and writes of the captured slot operate on the origin frame's slot via the chain.

This means a constructor that takes a reference parameter `&accum` and constructs a fexpr capturing `&accum` produces a fexpr that, when invoked from the constructor's frame or below, reads and writes the original `accum` slot at the chain's origin. The fexpr does not survive the constructor's return — but during the constructor's lifetime, it operates on origin-frame state.

---

## 5. Body Semantics

### 5.1 Direct Access to Captured Slots

Captured-slot reads and writes in a fexpr body are direct, not copy-restore. Mid-body failures do not roll back captured-slot writes performed earlier in the body.

This is a meaningful design choice with consequences:

- **Failure-atomicity is local to the captured-slot write, not to the body.** Each captured-slot write either succeeds or fails atomically (the write itself is atomic), but multiple writes within a body are not transactional with respect to each other.
- **A failed invocation may leave captured slots in a partially-mutated state.** The invoking frame, on receiving the propagated failure, sees both the failure and the partial mutations.
- **This is sometimes the desired semantics.** A recovery construct that wants to inspect the state at the moment of failure cannot use copy-restore (which would have rolled back the relevant state). Direct access preserves the state for inspection.

The contrast with lambda reference captures (checkpoint 008 §5.4) is intentional. Lambdas use copy-restore to preserve the broader language's mutation atomicity guarantee for their captured slots; fexprs trade that atomicity for the expressive flexibility of partial-mutation-on-failure semantics. The user picks which mechanism fits their use case.

### 5.2 Initialization Requirement for Captures

A fexpr's captured slots must be in the `init` state at fexpr-construction time. The typechecker verifies this at the construction site.

The body's analysis treats captured slots as `init` at every body entry. This is the conservative rule: even though a body's earlier invocations might have written-after-reading-uninit (in some hypothetical scenario), the typechecker doesn't track per-invocation state across invocations. Captured slots are `init` at body entry, the body reads and writes them freely, and the slot remains `init` after the body (writes don't un-initialize).

A consequence: a fexpr cannot be used to initialize an uninitialized slot in the capture frame. For that purpose, the user should use a regular productive command call. Fexprs are for control-flow combinators that operate on already-initialized state.

### 5.3 Body-Introduced Locals

A `# x` introduction in a fexpr body introduces a slot in the **invocation frame** (per §3). The slot's lifetime is bounded by the invocation. Subsequent invocations get fresh body-locals.

Body-locals interact with captured names per the shadowing rule (§2.4). Within the local's scope, the captured name (if any) is shadowed.

### 5.4 What Body Code Can Do

A fexpr body is ordinary command-body code, with the additional license to read and write captured slots. There are no specific restrictions on body content beyond what the language imposes generally:

- **Sub-commands:** the body can call any command in scope, with arguments drawn from captured slots, body-locals, or top-level definitions.
- **Sub-fexprs:** the body can construct other fexprs. The sub-fexpr's capture frame is the invocation frame (because that's where the construction happens).
- **Lambdas:** the body can construct lambdas. The lambda's introducing frame is the invocation frame.
- **Objects:** the body can construct objects. The introducing frame is the invocation frame; objects do not survive the invocation.
- **Productive writes to captured slots:** the body can perform productive writes to captured slots. Per §5.1, these are direct (not copy-restore).
- **Recursion:** the body can invoke fexprs (including itself, per §3.4 — but the invocation must be of a fexpr value reachable from the body, not a back-reference to the body's own structure).

Each of these is a normal use of the language, with the invocation frame providing the lifecycle bookkeeping.

### 5.5 Statement Separation

Statements within a fexpr body are separated by indentation, per the language's general layout rules. There is no semicolon syntax for inline statement sequencing.

This applies equally to lambda-with-body forms (per checkpoint 008 §3). Both kinds of body use the same convention; users do not have to remember different rules for different forms.

---

## 6. Re-Entry and Nesting

### 6.1 Re-Entry

A fexpr may be invoked while already on the call stack from a previous invocation. Each invocation establishes its own invocation frame; locals and at-stack registrations are per-invocation.

Re-entry can arise from:

- A fexpr invoking itself (directly or via a chain of calls that eventually invokes the same fexpr).
- A fexpr `f1` invoking another fexpr `f2`, where `f2`'s body does work that re-invokes `f1`.
- Any other call pattern that brings control back into a previously-entered fexpr body.

The captured-slot references are shared across invocations; the body-locals are per-invocation. A re-entrant fexpr that reads or writes captured slots sees the slot state as left by whatever operations have happened since this invocation's body began — including any operations performed by the re-entering invocations.

This is unrestricted: the language permits the pattern, and the user is responsible for the semantics. The hackery this enables (per the user's framing) is intentional flexibility.

### 6.2 Nesting

A fexpr body may construct other fexprs. The constructed fexpr's capture frame is the invocation frame of the outer fexpr (because that's where the construction occurs). The constructed fexpr's lifetime is therefore bounded by the outer fexpr's invocation; it cannot be returned, stored, or otherwise persisted beyond the outer body's completion.

The locality rule applies recursively: the inner fexpr can be assigned to a `#`-local in the invocation frame, passed as an IN argument to commands called from the body, and invoked. It cannot be returned via any mechanism, because no productive or reference fexpr-typed parameters exist (Restriction A) and no other escape route is permitted.

Ceilings compose naturally: each fexpr's lifetime is bounded by its own capture frame; nested fexprs have nested capture frames; the locality rule applies at each level.

---

## 7. Failure-Mode Integration

### 7.1 The Marker on the Form

A fexpr's failure-mode marker is part of its construction syntax: `:{...}` is never-fails, `?{...}` is may-fail, `!{...}` is must-fail. The marker constrains the body's failure semantics:

- A `:`-marked fexpr's body must never fail. Every statement in the body must be statically guaranteed to succeed (per the never-fails-must-handle rule), with all may-fail and must-fail sub-calls handled within the body.
- A `?`-marked fexpr's body may fail. Failures propagate to the invoker.
- A `!`-marked fexpr's body must fail. Every successful return path through the body would be a static error.

The typechecker verifies body conformance at the fexpr-construction site, exactly as for lambda-with-body forms (checkpoint 007 §3.5).

### 7.2 Invocation in a Surrounding Context

When a fexpr is invoked, the resulting failure (if any) propagates per the standard failure-mode rules. The invoking command must consume the failure if its own marker is more restrictive than the fexpr's:

- A `:`-marked command invoking a `?`-marked fexpr must consume the possible failure within the command's body. The standard recovery contexts (`?`, `?-`, `?:`, `^`, `|`, `??`) apply.
- A `?`-marked command invoking a `?`-marked fexpr may either consume or propagate; the standard rules apply.
- A `:`-marked command invoking a `:`-marked fexpr is fine; no failure to handle.
- A `!`-marked command invoking a `!`-marked fexpr is fine; the must-fail propagates.

This is the standard never-fails-must-handle rule (checkpoint 002 §1.4), applied to fexpr invocations exactly as it applies to any other command-typed value's invocation. No new rules are needed.

### 7.3 The Canonical While-Loop Pattern

The interaction is illustrated by the canonical user-defined while-loop pattern:

```
.cmd while : ?{} cond, ?{} body =
    ?? ?- cond
        body
```

`while` is `:`-marked (never-fails). It accepts two `?`-marked fexpr parameters. Inside its body:

- `?- cond` invokes `cond` as the guard of an inverse-guard block (checkpoint 002 §2.3). On `cond`'s failure, the block engages; on success, it doesn't. Either way, the failure is consumed by the construct.
- `body` is invoked unconditionally inside the block; its failures must be handled by the surrounding context. Wrapping the whole structure in `??` (per checkpoint 002 §2.5) elevates the recovery destination, producing the loop semantics.

The `?`-marked fexpr inputs make the failure semantics explicit at the type level. The typechecker verifies that `while`'s body consumes both possible failure paths, satisfying the never-fails-must-handle rule.

---

## 8. Aliasing

### 8.1 The Problem

A fexpr captures slot `X`. The fexpr is then invoked from a context where `X` (or an aliasing slot, e.g., a writeable parameter binding to `X`) is also being passed as an active writeable argument to some operation. The body's accesses to the captured `X` and the writeable parameter's accesses to `X` interleave; the aliasing creates a potential-for-confusion.

### 8.2 What the Typechecker Detects

Where the aliasing is statically detectable, the typechecker rejects the program:

- **Same-statement aliasing:** if a `<-` invocation has the form `cmd: ..., &X, ...` and the invocation also passes a fexpr that captures `X`, the typechecker detects the aliasing via the fexpr's capture set.
- **Same-construction aliasing:** if a fexpr is constructed and then immediately invoked in a context that passes `&X` to the same call, similar detection applies.

In both cases, the typechecker reports the aliasing and requires the user to disambiguate (e.g., by sequencing the operations so they don't overlap, or by restructuring the call).

### 8.3 What the Typechecker Cannot Detect

Aliasing through pointers, through indirect chains of calls, or through other dynamic mechanisms is not statically detectable. The user is responsible for these cases, consistent with the language's general stance on pointer aliasing (checkpoint 001 §2.6).

---

## 9. Worked Examples

### 9.1 The Canonical While-Loop

```
.cmd while : ?{} cond, ?{} body =
    ?? ?- cond
        body

.cmd useIt =
    # i : Int32 <- (Int32: 0)
    # total : Int32 <- (Int32: 0)
    while: ?{lessThan: i, 10},
           ?{
               total <- (add: total, i)
               i <- (add: i, 1)
           }
    log: total
```

Breakdown:

- `while`'s body uses `?- cond` to invert the condition's failure semantics (failure means stop) and `??` to elevate the loop-exit destination above the invocation point.
- The first fexpr `?{lessThan: i, 10}` captures `i` from `useIt`'s frame. Its body is `lessThan: i, 10` — a `?`-marked test that succeeds when `i < 10`, fails otherwise.
- The second fexpr `?{total <- ...; i <- ...}` captures both `total` and `i`. Its body performs two productive writes, both directly to the captured slots.
- After `while` returns, `total` and `i` reflect the loop's final state.

Failure-atomicity note: each individual `add` call uses copy-restore on its own writeable parameters. The fexpr body's two `<-` operations are individually atomic, but not transactional with respect to each other — if the second `<-` failed, `total` would have its updated value but `i` would not.

### 9.2 Reference-Chain Flattening Through a Helper

```
.cmd buildIncrementer : ?{} 'inc, &Counter c =
    'inc <- ?{ Counter::increment: &c }

.cmd outer =
    # state : Counter <- (Counter: 0)
    # incrementer
    buildIncrementer: 'incrementer, &state
    incrementer
    incrementer
    log: state                                  ; logs 2
```

The fexpr captures `&c` in `buildIncrementer`'s body. `c` is a reference parameter; the chain flattens to `state` in `outer`'s frame. The fexpr's capture frame is `buildIncrementer`'s frame.

Wait — but `buildIncrementer` exits before `incrementer` is invoked! Does the fexpr survive?

This is exactly the case where the locality rule constrains things tightly. The construction `'inc <- ?{ ... }` would migrate the fexpr value from `buildIncrementer`'s frame to `outer`'s frame via the productive `'inc` parameter. **This is forbidden by Restriction A** (productive fexpr-typed parameters are not permitted).

So this example is ill-typed. The fix is for `buildIncrementer` to not exist as written; `outer` must construct the fexpr itself:

```
.cmd outer =
    # state : Counter <- (Counter: 0)
    # incrementer <- ?{ Counter::increment: &state }
    incrementer
    incrementer
    log: state                                  ; logs 2
```

Here the fexpr is constructed in `outer`'s frame, captures `state` directly (no chain), and is invoked twice. The locality rule is satisfied.

The pattern that 9.2 attempted — a helper that builds a fexpr and returns it — is one of the patterns the locality rule deliberately rejects. Users who need this pattern should reach for ceiling-tracked lambdas (checkpoint 008) instead.

### 9.3 Re-Entry

```
.cmd useReentry =
    # depth : Int32 <- (Int32: 0)
    # f
    f <- ?{
            depth <- (add: depth, 1)
            ? lessThan: depth, 3
                f                                 ; re-invoke
            depth <- (subtract: depth, 1)
        }
    f
    log: depth                                   ; logs 0
```

The fexpr captures `depth` and `f` (a slot in the same frame holding the fexpr itself). On invocation:

- Increments `depth`.
- If `depth < 3`, recursively invokes itself (the captured `f`).
- Decrements `depth` after recursion returns.

Re-entry is sound: each invocation has its own invocation frame; the captured `depth` is shared, mutated by each invocation's increments and decrements. Final `depth` is 0 because every increment is paired with a decrement.

Note: the fexpr captures `f` (the slot it is itself stored in). This works because the construction `f <- ?{...}` initializes `f` before any invocation occurs; subsequent invocations read `f`'s value (the fexpr itself) from the captured slot.

### 9.4 Nested Fexpr Constructed in Body

```
.cmd dispatcher : ?{} pre, ?{} post, ?{} body =
    pre
    body
    post

.cmd useIt =
    # x : Int32 <- (Int32: 0)
    dispatcher: ?{x <- (add: x, 1)},
                ?{x <- (subtract: x, 1)},
                ?{
                    # localFexpr <- ?{
                        x <- (multiply: x, 2)
                    }
                    localFexpr
                    localFexpr
                }
    log: x                                      ; logs 0; (((0 + 1) * 2) * 2) - 1 = 3, wait...
```

(Actually the math: pre adds 1 → x=1; body invokes localFexpr twice — first multiplies by 2 → x=2, second multiplies by 2 → x=4; post subtracts 1 → x=3.)

`localFexpr` is constructed inside the body of the outer fexpr. Its capture frame is the outer fexpr's invocation frame. Its captured `x` is the same `x` (via the outer fexpr's capture chain) — `useIt`'s `x` slot.

The locality rule applies: `localFexpr` is constructed in a `#`-local in the invocation frame; it's invoked from the same body; it cannot be returned because no productive or reference fexpr-typed parameter exists. When the outer fexpr's invocation exits, `localFexpr`'s slot is destroyed.

---

## 10. Implementation Notes

### IN-14: Capture-Set Computation

At each fexpr-construction site, the typechecker computes the body's free-name set and partitions it into captures (slot references) and top-level references. This is standard closure analysis; the implementation does not require new machinery beyond what other capture-bearing forms already need.

The capture-set is recorded as part of the fexpr value's metadata for ceiling enforcement and aliasing detection.

### IN-15: Locality-Rule Enforcement

The locality rule's restrictions (Restrictions A through E in §4.2) are typechecker-enforced. They reduce to per-type checks on parameter modes, pointer-target types, field types, and assignment forms. The enforcement is mostly local to each declaration or use site; no global flow analysis is needed (in contrast to ceiling-tracked lambdas, which require it).

### IN-16: Invocation-Frame Setup

Each fexpr invocation establishes a fresh frame on the call stack. The frame's bookkeeping is the same as for any command frame: storage for body-locals, an at-stack registration list, and frame-exit cleanup. The implementation may share machinery with regular command-call frame setup.

### IN-17: Captured-Slot Direct Access

Captured-slot reads and writes compile to direct slot accesses through the stored slot reference. No copy-restore wrapping is needed. Failures mid-body do not trigger any rollback of captured-slot state; the failure simply propagates from the body to the invocation-frame exit, then to the invoking frame.

### IN-18: Reference-Chain Flattening at Construction

When a fexpr captures a name that resolves to a reference parameter, the typechecker computes the chain at construction time and emits a flattened slot reference (per checkpoint 008 §5.1's mechanism). The runtime stored reference is to the chain's origin, not the proximate slot.

The locality rule still applies based on the construction site's frame: even though the captured slot's origin may be in a frame above the constructor's, the fexpr value itself is confined to the constructor's frame.

---

## 11. Grammar Status

This checkpoint introduces no new grammar productions or tokens. The brace-quote forms (`{...}`, `:{...}`, `?{...}`, `!{...}`) and command-literal forms (`:<...>{...}`, etc.) are already present in the grammar.

The locality rule is enforced entirely by the typechecker, against existing parser output. No parser changes are needed.

The fexpr type expressions `:{}`, `?{}`, `!{}` follow the existing command-type-expression grammar with empty parameter lists (or in the lambda-with-body case, the `:<>{...}` form's type-level analog). Whether the parser already accepts these as types or whether a small extension is needed is an audit item, not a design question.

---

## 12. Open Questions Updated

| OQ | Status | Notes |
| --- | --- | --- |
| OQ-1 | Open | Union discriminator representation. |
| OQ-2 | Open | Implementation latitude for IN parameter passing. |
| OQ-3 | Mostly resolved (per checkpoint 004) | |
| OQ-4 | Resolved (per checkpoint 004) | |
| OQ-5 | Open | Single-class instance coherence. |
| OQ-6 | Substantially resolved (per checkpoint 008) | Surface syntax for partial application of v-command receiver tuples remains. |
| OQ-7 | **Resolved by this checkpoint** | Fexpr design: implicit capture, per-invocation invocation frame, locality rule, direct captured-slot access. See §§2–8. |
| OQ-8 | Mostly resolved (per checkpoint 002) | |
| OQ-9 | Resolved (per checkpoint 006) | |
| OQ-10 | Resolved (per checkpoint 009) | |
| OQ-11 | Resolved (per checkpoint 005) | |
| OQ-12 | Resolved (per checkpoint 005) | |
| OQ-13 | Open | Implicit context parameters and initialization. |
| OQ-14 | Open | Same-scope rule for `&x` and `x`. |
| OQ-15 | Open | Full design of the downcast intrinsic. |
| OQ-16 | Open | Overloading restriction on dynamically-dispatched commands. |
| OQ-17 | Substantially resolved (per checkpoint 009) | |
| OQ-18 | Open | Lambda visible-signature representation. |
| OQ-19 | Resolved (per checkpoint 008) | |
| OQ-20 | Open | Slash-list internal grammar details. |
| OQ-21 | Open | Capture-list interaction with implicit context parameters. |
| OQ-22 | Open | Surface form for parameterized literal types in `.implicit` declarations. |
| OQ-23 | Open | Lexer disambiguation rules for `{-`, `-}`, `[-`, `-]`, `{-}`, `[-]`. |
| OQ-24 | Open | The phasing of `.implicit` Aggregate/Sequence support. |
| OQ-25 | **New** | Shadowing of captured names by body-introduced locals in fexpr (and lambda) bodies. This checkpoint commits to permitted shadowing per §2.4, but the question of whether the stricter rule (compile-time error, paralleling checkpoint 005 §1.2 for `&x` and `x`) would be preferable is open and resolvable either way. The current commitment is the conventional choice; revisit if the stricter rule shows compelling benefit. |

---

## 13. Summary of Changes from Prior Checkpoints

| Topic | Prior State | This Checkpoint |
| --- | --- | --- |
| Fexpr capture | Open (OQ-7) | Implicit by free name |
| Fexpr body locals | Unspecified | Per-invocation invocation frame |
| Fexpr-typed slot usage | Unspecified | Locality rule (§4) |
| Captured-slot access | Unspecified | Direct (not copy-restore) |
| Capture initialization | Unspecified | Required at construction |
| Body statement separation | Unspecified | Indentation (same as elsewhere) |
| Re-entry | Unspecified | Permitted |
| Nesting | Unspecified | Permitted |
| Reference-chain flattening | Lambdas only (008) | Extended to fexpr captures |
| Aliasing detection | Unspecified | Statically detectable cases rejected |

---

## 14. Provenance

**Authored:** Distilled from continued intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-28.

**Source materials read for this checkpoint:** intent-checkpoint-001.md through intent-checkpoint-009.md.

**No grammar changes implied by this checkpoint.** The fexpr brace-quote forms (`:{...}`, `?{...}`, `!{...}`) are already in the grammar. The locality rule and its restrictions are typechecker-enforced against existing parser output. The fexpr type expressions follow the existing command-type-expression grammar.

**Recommended next step:** Commit alongside the prior checkpoints (suggested path: `docs/intent-checkpoint-010.md`). With OQ-7 resolved and the construction story complete (per checkpoint 009), the natural next major thread is the **failure-mode-and-typechecker integration thread** that has been pending since checkpoint 002 §6 — the static rules by which `?` / `!` / `:` markers propagate through type expressions, command compositions, recovery contexts, and command-typed-value invocations. Smaller open questions (OQ-13 implicit context parameters, OQ-14 same-scope rule, OQ-15 downcast intrinsic, OQ-16 overloading restriction) can be resolved inline with implementation work or in small dedicated checkpoints.
