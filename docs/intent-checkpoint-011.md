# Basis Language — Intent Checkpoint 011

**Status:** Working draft. Builds on intent-checkpoint-001.md through intent-checkpoint-010.md; supersedes them where they conflict. This checkpoint integrates the failure-mode-and-typechecker thread first noted as the natural next major thread in checkpoint 002 §6 and confirmed as such by checkpoints 009 and 010.
**Date of this checkpoint:** April 2026
**Provenance:** Continuation of the intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-29. This checkpoint specifies the static failure-mode analysis the Basis typechecker performs: the abstract domain (a six-state lattice over each program point), transfer functions for all eleven block markers and for command invocation, body-conformance rules for the three failure marks, integration with the initialization analysis from checkpoint 004, and the never-fails-must-handle rule from checkpoint 002 §1.4 as a corollary of conformance. Subsumption rules for command-typed values are pinned down. The `.fail` keyword's static semantics are specified. A small refinement to `^` (DO_REWIND) from checkpoint 002 §2.6 — the body is optional, the construct requires a preceding sibling — is recorded. Worked examples walk through the canonical loop in both bodied and bodiless variants and through several other patterns. One substantial open question (typed failures) is introduced and flagged as the natural next major thread before implementation proceeds.

**Authority statement.** Where this document differs from earlier checkpoints, this document is authoritative. Where this document and the source code differ on syntactic matters, the code is authoritative; where they differ on semantic intent, this document is authoritative. The README and `language_notes.txt` remain non-authoritative.

---

## 1. Decision Summary

This checkpoint lands the following decisions:

1. **The typechecker performs a forward-flow analysis over the control-flow graph of each command body.** At each program point the analysis maintains a state with two components: a failure-state lattice value and a per-slot initialization-state value (the latter from checkpoint 004 §3, unchanged in mechanism). The two components share the CFG walk and the same join points, but join independently and interact only at transfer functions.

2. **The failure-state lattice has six states.** `clear` (no propagating failure on entry), `failing(?)` (a `?`-mode failure is propagating), `failing(!)` (a `!`-mode failure is propagating), `mixed(?)` (some incoming paths are clear, some carry a `?`-mode failure), `mixed(!)` (some incoming paths are clear, some carry a `!`-mode failure), and `unreachable` (no incoming path is reachable). At joins, `failing(!)` widens to `failing(?)` when joined with `failing(?)`, and `mixed(!)` widens analogously. `unreachable` is the lattice's bottom and the identity element for join.

3. **Transfer functions for the eleven block markers.** Each of `?`, `?-`, `?:`, `-`, `%`, `^`, `|`, `|`-with-spec, `??`, `@`, and `@!` has a transfer function expressed in terms of the lattice. The functions translate the operational rules from checkpoint 002 §2 (and the refinements from checkpoint 003) into static state transitions over the lattice.

4. **Transfer functions for call sites.** A call site invokes a command-typed value (named command, slot of command type, class-dispatched method, fexpr). The post-call state is determined by the *declared* mark of the value being invoked, not by what the value happens to be at runtime. A `:`-invocation produces a single success edge with state `clear`. A `?`-invocation produces two edges: success with `clear`, failure with `failing(?)`. A `!`-invocation produces a single failure edge with `failing(!)`.

5. **Body-conformance rules for the three marks.** A `:`-marked body is well-formed when every exit edge has state `clear` and at least one exit edge is reachable. A `?`-marked body has no constraint on exit-edge states beyond reachability. A `!`-marked body is well-formed when every exit edge has state `failing(?)` or `failing(!)` (no `clear` and no `mixed(M)` exits) and at least one exit edge is reachable. The asymmetry between `:` and `!` is exact: they are dual constraints, with `?` as the unconstrained middle.

6. **Subsumption is on the failure mark only.** `: <: ?` and `! <: ?`; `:` and `!` are mutually incomparable peers. Parameter modes (`'`, `&`, IN) and parameter types are invariant under subsumption — a `:<Int32'>` is not interchangeable with `:<Int32>` or `:<Int32&>`.

7. **The never-fails-must-handle rule is a corollary, not a separate stipulation.** A `:`-marked body cannot have any propagating failure reach an exit; therefore every `?`- or `!`-call inside must have its failure consumed by some recovery context within the body. Likewise, an `!`-marked body cannot have any successful path reach an exit; therefore every reachable path must produce a propagating failure (whether via a `!`-call, an unrecovered `?`-call, or a `.fail` operation).

8. **The `.fail` keyword is a `!`-call producing a failure value.** Its post-call state is `failing(!)`. Its operand is the failure value (the `Φ`-component of the operational semantics tuple from checkpoint 001 §1).

9. **Refinement to checkpoint 002 §2.6 (`^` / DO_REWIND).** The `^` body is optional. A bodiless `^` is treated as if its body had succeeded, producing an unconditional rewind to the preceding sibling. `^` requires a preceding sibling at the same indentation level; absence of one is a static semantic error.

10. **An open question is introduced about typed failures.** The current design has no mechanism for command signatures to advertise what *types* of failure values they may emit; signatures carry only the failure-mode mark. This is a meaningful ergonomic hole: `|`-with-spec recovery cannot be statically verified for exhaustiveness, and bare `|` becomes the only fully-general recovery mechanism. The design space and considerations are recorded as OQ-26; resolution should occur before the typechecker integration described in this checkpoint reaches full implementation.

---

## 2. Background: The Control-Flow Graph and Forward-Flow Analysis

This section introduces vocabulary used throughout the checkpoint. Readers familiar with compiler-construction terminology may skip it; it is included because the checkpoint is intended to be self-contained for implementation.

### 2.1 The Control-Flow Graph

A **control-flow graph** (CFG) is a directed graph in which each node is a program point — usually a single statement, or a "basic block" of statements with no internal branches — and each edge represents a possible flow of control from one point to the next. A linear sequence of statements with no branching corresponds to a chain of nodes connected by single edges. Constructs that branch (such as `?`-blocks, `?:` chains, `^` rewinds, `|` recoveries) introduce nodes with multiple outgoing edges, and constructs that join (the program points immediately *after* such branching constructs) have multiple incoming edges.

The CFG of a command body is finite (one node per syntactic statement, plus a synthetic exit node). It is typically constructed by a traversal of the AST, with edges added per the structure of each block-marker construct.

### 2.2 Forward-Flow Analysis

A **forward-flow analysis** is a static analysis that propagates a state value forward through the CFG, edge by edge, until the state at every node stabilizes. At each node, the analysis:

- Computes the per-incoming-edge contribution to the node's state (each predecessor contributes the state on its outgoing edge to this node).
- Combines the per-incoming-edge contributions via a **join operation** to produce the node's incoming state.
- Applies the node's **transfer function** to the incoming state, producing the outgoing state for each of the node's outgoing edges.

The join operation is required to be associative, commutative, and conservative (the joined value must be at least as permissive as any of its inputs — the analysis must not claim to know more than it does). The transfer functions are required to be monotone with respect to the join's induced partial order. Together these properties ensure the analysis terminates at a fixed point.

### 2.3 Why the Lattice

The **abstract domain** of a forward-flow analysis is a lattice — a partially-ordered set with well-defined join (least upper bound) and meet operations. For our analysis, the abstract domain has two components: the failure-state lattice (specified in §3 below) and the per-slot initialization-state lattice from checkpoint 004 §3 (unchanged here). The two components compose as a product lattice: the joint state at a program point is a pair (failure-state, per-slot-init-state-vector), and the joint join operation joins each component independently.

The two components are not coupled at the join level — they join independently. They *are* coupled at the transfer-function level: the failure-state determines which edges are reachable, and the init-state transitions happen on those reachable edges. The init-state component does not feed back into the failure-state analysis; the dependence is one-directional.

This is the load-bearing efficiency claim. A single CFG walk produces both analyses; the implementation maintains one work-list, one state-vector per node, and applies the joint transfer function at each visit. The conformance check at the body's exit uses both components together.

---

## 3. The Failure-State Lattice

### 3.1 The States

The failure-state lattice has six elements:

| State | Meaning |
| --- | --- |
| `clear` | No failure is propagating at this program point. Ordinary execution reaches here. |
| `failing(?)` | A failure is propagating at this program point, originating from a `?`-mode operation. |
| `failing(!)` | A failure is propagating at this program point, originating from a `!`-mode operation. |
| `mixed(?)` | Some incoming paths reach this point with state `clear`, others with `failing(?)` (possibly widened from `failing(!)`). |
| `mixed(!)` | Some incoming paths reach this point with state `clear`, others with `failing(!)`. |
| `unreachable` | No incoming path reaches this program point. The node is statically unreachable. |

The states are mutually exclusive: at any program point under the analysis, the state is exactly one of these six. They capture all the situations the typechecker needs to reason about for failure handling.

### 3.2 The Partial Order

The lattice's partial order, with `unreachable` as bottom and progressively more-permissive states above:

```
unreachable
    |
  clear, failing(!), failing(?)        (these three are pairwise incomparable
                                        as direct successors of unreachable
                                        — the join of any two gives a different
                                        higher state)
    |
  mixed(!)
    |
  mixed(?)
```

More precisely: the join (least upper bound) is computed by table (§3.3 below). Reading the order off the table:

- `unreachable` is below everything.
- `clear`, `failing(!)`, and `failing(?)` are incomparable peers, all above `unreachable`.
- `mixed(!)` is above `clear` and above `failing(!)` (it's the join of those two).
- `mixed(?)` is above `mixed(!)`, above `clear`, above `failing(?)`, and above `failing(!)` (it's the supremum of every pair-of-states that includes a `?`-flavored failure or that has a propagating-failure arm and a clear arm).

There is no element above `mixed(?)`; it is the lattice's top.

### 3.3 The Join Table

For program points with multiple incoming edges, the joined state is the join of the per-edge states. The full table:

| ∨ | `unreach` | `clear` | `failing(!)` | `failing(?)` | `mixed(!)` | `mixed(?)` |
| --- | --- | --- | --- | --- | --- | --- |
| `unreach` | `unreach` | `clear` | `failing(!)` | `failing(?)` | `mixed(!)` | `mixed(?)` |
| `clear` | `clear` | `clear` | `mixed(!)` | `mixed(?)` | `mixed(!)` | `mixed(?)` |
| `failing(!)` | `failing(!)` | `mixed(!)` | `failing(!)` | `failing(?)` | `mixed(!)` | `mixed(?)` |
| `failing(?)` | `failing(?)` | `mixed(?)` | `failing(?)` | `failing(?)` | `mixed(?)` | `mixed(?)` |
| `mixed(!)` | `mixed(!)` | `mixed(!)` | `mixed(!)` | `mixed(?)` | `mixed(!)` | `mixed(?)` |
| `mixed(?)` | `mixed(?)` | `mixed(?)` | `mixed(?)` | `mixed(?)` | `mixed(?)` | `mixed(?)` |

Two reading rules summarize the table:

- **Widening of `!` to `?`.** Whenever a `!`-flavored state is joined with a `?`-flavored state, the result is `?`-flavored. The `?` mark is the supremum: a context that joins a `!`-failure path with a `?`-failure path can only conservatively claim "a `?`-mode failure may be propagating" — it has lost the precision of "definitely `!`."
- **`clear` plus failing produces `mixed`.** A node with one incoming clear path and one incoming failing path is reachable on both, and downstream code must consider both possibilities. The `mixed(M)` state names this exactly. The `M` is the more-precise of the failure modes (or widens per the rule above if multiple modes contribute).

`unreachable` is the identity for join: joining anything with `unreachable` yields the other operand. This is the standard rule for "no incoming edge is reachable, so the joined state contributes nothing."

### 3.4 Why This Lattice Shape

The shape was chosen to capture exactly the distinctions the typechecker needs to make and no more.

- **Why distinguish `failing(?)` from `failing(!)`.** A `!`-marked body is conformant only when every exit is propagating-failure. If the lattice collapsed both into a single `failing` state, the typechecker would lose the ability to verify that the failure originated from a known-must-fail operation versus a may-fail operation that happened to fail. The distinction is also useful for diagnostics: error messages can name the specific call that produced the failure mode.
- **Why distinguish `mixed(M)` from `failing(M)`.** A `mixed(M)` state means downstream code must handle both "may be executing normally" and "may be propagating a failure." A pure `failing(M)` state means downstream code is *only* propagating failure (the next ordinary statement isn't executed unless caught). The distinction matters for the conformance check: a `:`-conformant body cannot have any exit in `failing(M)` *or* `mixed(M)`, because both contain a propagating-failure arm.
- **Why the widening rule rather than tracking `!` precisely through joins.** Tracking `!` precisely would require additional states (e.g., `failing(? ∨ !)` to mean "either kind of failure is propagating"), with corresponding precision in `mixed` states. The cost is more lattice elements and more transfer-function cases for limited benefit: the only place `failing(!)` provides actionable static information is conformance of `!`-marked bodies, where the widening to `failing(?)` doesn't cause information loss because both states are accepted by `!`-conformance. The user explicitly endorsed this trade-off; the lattice is intentionally compact.

### 3.5 Reachability and `unreachable` Propagation

The `unreachable` state propagates through transfer functions: if a node's incoming state is `unreachable`, every outgoing edge has `unreachable` as its outgoing state. This handles dead-code regions cleanly — code reachable only after a `!`-call's success edge (which doesn't exist) is `unreachable`, and the analysis correctly identifies it as such.

A body whose synthetic exit node has state `unreachable` on every incoming edge has no reachable exit. This is dead-code-equivalent and warning-worthy (per checkpoint 008 §3.4's note about `!`-marked lambdas with productive parameters), but is not a typecheck failure unless it conflicts with a conformance rule. A `!`-conformant body whose exits are all `unreachable` fails the "at least one exit edge is reachable" part of `!`-conformance — see §5.

---

## 4. Transfer Functions for the Eleven Block Markers

This section walks each of the block-marker constructs from checkpoint 002 §2, with refinements from checkpoint 003 §1, and specifies the transfer function for each. Where applicable, the section also identifies which incoming-state cases are statically impossible (so the typechecker can flag them as internal-consistency errors if encountered).

### 4.1 `?` (DO_WHEN) — Guard Block

**Operational behavior** (from CP2 §2.2): a `?`-block has a guard (the first statement) and a body (statements 2..N). Run the guard. If it succeeds, run the body; if it fails, skip the body (the guard's failure is consumed by the construct). Either way, control proceeds to the statement following the `?`-block.

**CFG structure.** The `?`-block introduces a node for the guard, a node for the body's entry, and a join node at the post-`?`-block program point. The guard's outgoing edges are: the success edge (to body entry) and the failure edge (to the post-`?`-block join, with the guard's failure consumed).

**Transfer function.** Let `S` be the lattice state at the `?`-block's entry. The state at body entry, on the success edge from the guard, is `clear` (the guard executed successfully on a path that was itself reachable at state `S`, where `S` was `clear` or `mixed(M)` — see "preconditions" below). The state at the post-`?`-block join receives two contributions: the body's exit (whatever state the body's last statement produced) and the guard-failure edge (state `clear` — the failure is consumed by the construct, not propagated). The post-block join state is the join of these two contributions.

**Preconditions on entry state.** The `?`-block's entry should be reached on a non-failing path, because a propagating-failure path would have been failure-skipped past the guard rather than entering it. Concretely, well-formed programs have `S ∈ {clear, mixed(M)}` at the `?`-block entry; a state of `failing(M)` on entry would mean a failure was already propagating and the `?`-block was somehow entered anyway, which the failure-skip mechanism prevents. The typechecker may treat `failing(M)` on entry as `unreachable` at the body entry, since the only way to reach the guard is on a non-failing path.

A practical formulation: the guard executes on the `clear` arm of `S`. If `S` is `mixed(M)`, the failing arm of `S` failure-skips past the entire `?`-block, contributing to the post-block join. The clear arm of `S` becomes the guard's incoming state. After the guard, body entry receives `clear` (on guard success) or the failure-consumed `clear` (on guard failure), and the post-block join receives the body's exit on the success branch plus the guard-consumed `clear` on the failure branch plus the failure-skipped `mixed(M)`-failing arm of `S`. The combined post-block state is the join.

**Body-statement failures.** Statements 2..N of the body have their `?`-marks treated under the surrounding context's rules. Their failures are not consumed by the `?`-block; they propagate per the surrounding-block's recovery contexts. This is consistent with CP2 §2.2's "guard-only recovery" categorization.

### 4.2 `?-` (DO_WHEN_FAIL) — Inverse Guard Block

**Operational behavior** (from CP2 §2.3): mirror of `?`. The first statement is the guard; the body runs iff the guard *fails*. The guard's failure is consumed regardless of which path engages.

**CFG structure.** The guard's outgoing edges are: the success edge (to the post-`?-`-block join, with the guard's success producing `clear`) and the failure edge (to body entry, with the guard's failure consumed).

**Transfer function.** Same shape as `?`, with the body and post-join roles of success/failure exchanged. State at body entry on the failure-engagement edge is `clear` (failure is consumed; body executes from clear). State at post-block join receives: body's exit state (on the engagement path) and the guard's success edge (`clear`).

**The failure value is not surfaced to the body.** CP2 §2.3 is explicit that `?-` consumes the *fact* of failure, not the *value*. The body has no access to the propagating-failure-value. Programs that need the value should use `|`-with-spec instead.

### 4.3 `?:` (DO_WHEN_SELECT) — Case-When Chain

**Operational behavior** (from CP2 §2.4): each `?:` block in the chain is a guard-block (similar to `?`). Consecutive `?:` siblings at the same indentation level form a chain. Each guard is tried in order; the first guard to succeed engages its body and skips the rest. If all guards fail, the non-`?:` siblings following the chain serve as the default arm.

**CFG structure.** The chain has a sequence of guard nodes, each with a success edge (to that guard's body entry) and a failure edge (to the next guard, or to the default arm if no more `?:` follow). Each engaged body's exit edge flows to the post-chain join. The default arm's exit (if the chain is exhausted) also flows to the post-chain join.

**Transfer function.** Each guard follows the `?` pattern: success edge transitions to body entry with state `clear`; failure edge consumes the failure and transitions to the next guard or to the default arm with state `clear`. The post-chain join receives all the engaged-body exit edges plus (if the chain is exhausted with no default arm) a `clear` contribution from "all guards failed and no body engaged." If a default arm is present, its exit contributes to the join in place of the bare-exhaustion path.

**No exhaustion-as-failure.** A `?:` chain whose guards all fail does not propagate a failure; it falls through to the default arm (or to the post-chain join with state `clear` if no default arm). This is consistent with CP2 §2.4's explicit rule.

**Body-statement failures.** As with `?`, statements 2..N of an engaged `?:`-body have their failures propagate per the surrounding context, not consumed by the chain.

### 4.4 `-` (DO_ELSE) — Else Branch

**Operational behavior** (from CP2 §2.7): attaches to a preceding `?` or `?-` at the same indentation level. Provides the alternative branch when the predecessor's body did not engage.

**CFG structure.** The `-` body's incoming edge is the predecessor's "did not engage" outcome — for `? -`, that's the guard-failure path (after consumption); for `?- -`, that's the guard-success path. The `-` body's exit edge flows to the post-pair join.

**Transfer function.** State at `-`-body entry is `clear` (whether the predecessor's path that engaged `-` was guard-failure-then-consumed or guard-success-then-no-body, both produce `clear`). State at post-pair join is the join of the `-`-body's exit and the predecessor's body's exit (whichever engaged in any given execution).

**Body-statement failures.** The `-`-body is *not* a recovery context. Failures inside `-` propagate per the surrounding rules.

### 4.5 `%` (DO_BLOCK) — Plain Grouping

**Operational behavior** (from CP2 §2.8 and CP3 §1): plain grouping. No control-flow effect, no recovery semantics. Body executes as ordinary sequential statements. When `%` stands in guard position of a `?`-family construct (per CP3 §1), the `%`-body's success-or-failure becomes the guard's success-or-failure: all statements succeed (guard succeeds) or the first failing statement's failure escapes (guard fails).

**CFG structure.** Linear chain of body statements; entry edge from the `%`-block's predecessor, exit edge to the post-`%` program point.

**Transfer function.** Identity: state propagates statement-by-statement through the body per ordinary CFG flow. The `%`-block adds nothing to the analysis beyond what the body's individual statements contribute.

### 4.6 `^` (DO_REWIND) — Loop Block

**Refinement to checkpoint 002 §2.6.** The `^` body is **optional**. The construct's full grammar is `^ body?`, where `body` is zero or more statements. A bodiless `^` is treated as if the body had succeeded — control unconditionally rewinds to the preceding sibling. A `^` without a preceding sibling at the same indentation level is a **static semantic error**.

**Operational behavior — with body** (from CP2 §2.6): full-body recovery. Body executes; any failure in the body terminates the body and consumes the failure; on body success, control jumps back to the preceding sibling (loop continues); on body failure (which has been consumed), control falls through to the post-`^` program point (loop terminates).

**Operational behavior — without body**: control unconditionally jumps back to the preceding sibling. The post-`^` program point is reachable from `^` itself only via the unbody'd "body succeeded" route — which is identical to the always-rewind route — meaning the post-`^` program point has no incoming edge from the `^`-block on this case alone. (The post-`^` program point may still be reachable via other routes, such as the `??`-rewired guard-failure path in the canonical loop pattern.)

**CFG structure — with body.** The `^` introduces a body region with full-body recovery: every statement in the body has its failure-edge routed back into the `^`-block's "body failed" outcome. Outgoing edges from `^`: the rewind edge (to the preceding sibling) and the post-`^` edge.

**CFG structure — without body.** The `^` has only one outgoing edge: the rewind edge. There is no post-`^` edge from the `^`-block itself.

**Transfer function — with body.** Let `S` be the entry state. The body executes from `S`, with the modification that every statement's failure edge is consumed. The outgoing rewind edge has state `clear` (corresponding to the body's success, which by full-body recovery means "no failure occurred during the body"). The outgoing post-`^` edge has state `clear` (corresponding to the body's failure, which has been consumed). Both edges produce `clear` at their respective targets; the difference is the target. The CFG fixed-point for the loop converges per the standard work-list algorithm.

**Transfer function — without body.** The outgoing rewind edge has state `S` (the entry state passes through unchanged because there is no body to execute). There is no post-`^` outgoing edge.

**Loop fixed-point.** The fixed-point analysis from CP4 §3.6 applies: the joined state at the preceding-sibling node (its incoming state, which now includes the rewind edge from `^`) must be at least as permissive as the state on the original entry edge into the preceding sibling. The standard work-list iteration handles this; convergence is guaranteed because the lattice is finite-height.

**Diagnostic note.** A `^`-block with no preceding sibling is a static semantic error reported at parse time or AST-construction time. The error message should identify the offending `^` and explain that `^` requires something to rewind to.

### 4.7 `|` (DO_RECOVER) — Bare Recovery

**Operational behavior** (from CP2 §2.9): `|` engages on a propagating failure from preceding siblings at the same indentation level. The failure is consumed on engagement; the body executes from no-active-failure. The body's own failures propagate per the surrounding rules.

**CFG structure.** The `|`-block's incoming edge is the program point at the same level following the preceding siblings. There are two sub-cases depending on the incoming state:
- Incoming state has a propagating-failure component (`failing(M)` or `mixed(M)`): the `|`-block engages. The failure is consumed; body executes from `clear`. The body's exit edge flows to the post-`|` program point.
- Incoming state is purely `clear`: the `|`-block does not engage. Control flows past the `|` to the post-`|` program point with state `clear`.

The `|`-block always has an outgoing edge to the post-`|` program point, regardless of which sub-case. The state on that outgoing edge depends on which sub-case engaged.

**Transfer function.** Let `S` be the entry state.
- If `S = clear`: the `|`-block does not engage. Outgoing state is `clear`.
- If `S = failing(M)`: the `|`-block engages. Body executes from `clear`. Outgoing state is the body's exit state (whatever the body produced).
- If `S = mixed(M)`: the `|`-block engages on the failing arm of `S` (consuming the failure, body executes from `clear`) and is skipped on the clear arm of `S` (state passes through as `clear`). The two paths join at the post-`|` program point. Outgoing state is the join of: the body's exit state (from the engaged path) and `clear` (from the skipped path).
- If `S = unreachable`: the `|`-block is unreachable. Outgoing state is `unreachable`.

**Bare `|` does not bind the failure value.** CP2 §2.9: the body has no access to the failure value. Programs that need the value should use `|`-with-spec.

### 4.8 `|`-with-spec (DO_RECOVER_SPEC) — Filtered/Binding Recovery

**Operational behavior** (from CP2 §2.10): same as `|` but the spec acts as a predicate on the failure value. The block engages only if the failure matches the spec. Non-matching failures continue propagating as if the `|`-with-spec block were not there. When the spec matches, the spec's identifier is bound to the failure value within the body.

**CFG structure.** Like `|`, but the engagement edge is conditional on the spec matching the propagating failure. Non-matching failures are not consumed; they continue to flow to whatever sibling or surrounding context picks them up.

**Transfer function.** Let `S` be the entry state.
- If `S = clear`: the block does not engage. Outgoing state is `clear`.
- If `S = failing(M)`: the block *may* engage, depending on whether the failure matches the spec. The static analysis cannot in general know; it must conservatively model both possibilities. Outgoing state on the engagement path: the body's exit state. Outgoing state on the non-engagement path: `failing(M)` continues to propagate. The combined outgoing state is the join.
- If `S = mixed(M)`: similar treatment with the clear arm and the failing arm handled separately.
- If `S = unreachable`: outgoing state is `unreachable`.

**The conservative treatment without typed failures.** Without typed failures (per OQ-26), the typechecker generally cannot statically determine whether a given `|`-with-spec engages on a given failure. The conservative analysis assumes both possibilities. With typed failures, a chain of `|`-with-spec blocks could be statically verified for exhaustiveness, and engagement could be statically determined when the failure type is known. This is one of the major motivations for OQ-26.

**Cascade among `|`-blocks.** Per CP2 §2.10, each `|` (with or without spec) is a recovery destination for preceding `|`-bodies as well: if `|` block N engages and its body itself fails, block N+1 (if present) can catch it. The CFG models this naturally — block N's body's failure edge flows to the program point that block N+1 sees as its incoming.

### 4.9 `??` (DO_WHEN_MULTI) — Escape Elevator

**Operational behavior** (from CP2 §2.5): `??` modifies the structural destination at which a paired inner construct's guard-failure recovery resumes. Without `??`, an inner `?` (or `?-`) block's guard-failure consumed-failure resumes at the next sibling of the inner block. With `??`, that resumption point is moved to the next sibling of the `??`-block — one indentation level higher.

**CFG structure.** `??`'s effect is a topological rewiring of the CFG, not a state transition. The inner `?`-block's guard-failure edge, which would otherwise route to the inner block's next sibling, is rewired to route to the `??`-block's next sibling. Other than this rewiring, the inner `?`-block executes normally; its body, its body's exit, and its post-block join are all as if no `??` were present, except that the rewired guard-failure edge bypasses the post-block join.

**Transfer function.** No new state transition. The `??`-block contributes to the CFG by rewiring an edge from the inner construct.

**Crucial corollary** (from CP2 §2.5): a `|` placed at the elevated level (next sibling after `??`) does *not* engage on the inner guard's failure, because that failure has already been consumed by `??`. Such a `|` engages only on failures from non-guard operations within the `??`-block (body statements after a successful guard, or other propagated failures from inside the construct).

**Pairing constraints.** `??` pairs with `?` and `?-` only. It does not combine with `?:` or `%` (per CP2 §2.5 and `^` is a sibling rather than an inner construct, so the question of `??` pairing with `^` doesn't arise — the canonical loop pattern is `?? ?` plus a sibling `^`, not `?? ^`). Bare `??` (with no inner `?` or `?-`) is a static semantic error.

### 4.10 `@` (DO_ON_EXIT) — Frame-Exit Hook (Always)

**Operational behavior** (from CP2 §3.6, CP3 §IN-1a): registers cleanup that runs at frame exit, regardless of success or failure. Does not see or consume the propagating failure (if any) when running.

**CFG structure.** The `@`-block contributes nothing to the body's normal CFG beyond the registration side-effect. The cleanup itself executes in a separate flow at frame exit, with its own CFG (which is, schematically, a synthetic frame-exit-cleanup region executed after the body's exit edge but before the actual frame-exit).

**Transfer function in the body's normal CFG.** Identity. The `@`-block's presence in the body affects what runs at frame exit but does not affect the body's per-program-point state during normal execution.

**Transfer function in the cleanup flow.** The cleanup body executes from a synthetic `clear` state (it does not see the propagating failure if any). Its internal failures, if any, propagate within the cleanup flow but do not return to the body's CFG.

### 4.11 `@!` (DO_ON_EXIT_FAIL) — Frame-Exit Hook (Failure Only)

**Operational behavior** (from CP2 §3.6, CP3 §IN-1b): registers cleanup that runs only on failure-exit from the frame. Like `@`, does not see or consume the propagating failure.

**CFG structure and transfer functions.** Same as `@` in structure: identity in the body's normal CFG, separate cleanup flow with its own CFG. The cleanup runs only on the body's failure-exit (i.e., only when the body's exit state has a propagating-failure component).

---

## 5. Transfer Functions for Call Sites

### 5.1 The Three Cases by Mark

A call site invokes a value of command type. The mark of that value (`:` / `?` / `!`) — *as declared at the call site*, not as the value happens to be at runtime — determines the transfer function.

**Invocation of a `:`-marked value.** The call site has one outgoing edge: the success edge with state `clear`. There is no failure edge — `:`-marked values never produce a propagating failure. If the entry state at the call site is `S`, the success-edge state is the result of propagating `S` through the call's success semantics: typically `clear` if `S = clear`, with productive parameter discharge per CP4 §3.5 occurring on the success edge.

**Invocation of a `?`-marked value.** The call site has two outgoing edges: the success edge with state `clear` and the failure edge with state `failing(?)`. Downstream code sees both edges; the lattice tracks them per-path. Productive parameter discharge happens only on the success edge; on the failure edge, productive parameter slots retain their pre-call init-state.

**Invocation of a `!`-marked value.** The call site has one outgoing edge: the failure edge with state `failing(!)`. There is no success edge — `!`-marked values never produce a successful return. The next ordinary statement in the body is reachable from the call only via the failure edge, which by failure-skip semantics means the next ordinary statement is not executed unless caught by a recovery context. Productive parameter slots passed to a `!`-call retain their pre-call init-state on the failure edge.

### 5.2 Subsumption at the Call Site Reads the Slot's Mark

A slot of type `?<...>` accepts values whose actual mark is `:`, `?`, or `!` (by the subsumption rules `: <: ?` and `! <: ?`). At a call site invoking such a slot, the static analysis uses the *slot's declared mark* — `?` — for the transfer function, regardless of what the runtime value is. This means the call site produces the two-edge `?`-pattern even if the runtime value is statically known (perhaps by some other analysis) to be a `:` value that never takes the failure edge.

This is sound: the analysis is conservative, and downstream code is prepared to handle failure edges that may at runtime be unreachable. Programs that need tighter analysis should declare the slot more tightly (e.g., as `:<...>` if the value is always `:`-marked).

The corollary: signatures opaquely upcast. A `?`-marked command whose body internally produces only `failing(!)` exits will at the call site present as `failing(?)` per the `?`-invocation rule. The caller has no static information about the internal `!`-precision.

### 5.3 Productive Parameter Discharge

For each productive `'r` argument at a call site, the productive obligation discharges on the success edge of the call. The discharge transitions `r`'s init-state from `uninit` (or whatever its pre-call state was, since productive parameters do not require pre-call initialization) to `init`. On the failure edge, `r`'s init-state is unchanged from its pre-call value.

For a `:`-call, only the success edge exists, so the productive obligation always discharges. For a `?`-call, the discharge happens conditionally on the success edge; downstream code must reach a join where `r` is initialized on every reaching path, or use `r` only on success-reachable paths. For a `!`-call, the success edge does not exist, so the productive obligation never discharges along any reachable path. This is the dead-code-equivalent case from CP8 §3.4: the productive parameter on a `!`-call is vacuously satisfied (no successful path exists, so the "must write on success" rule is trivially honored), but warning-worthy because the productive slot will never be written.

### 5.4 Reference Parameter Requirements

For each reference `&r` argument at a call site, the call site requires `r` to be in `init` state at the call site. This is true on the entry to the call, before either edge is considered. The check is on the program point of the call, using the joined incoming state. If `r` is not in `init` state at the call site, the program is rejected.

Reference parameters do not transition `r`'s init-state across the call (the slot remains `init` on both success and failure edges, since the callee reads or writes via the slot reference and the slot must remain meaningful throughout).

### 5.5 IN Parameter Behavior

For each IN argument at a call site, the call site requires the argument to be in `init` state at the call site (the value is read into the callee's local copy). IN parameters do not transition the caller's slot across the call; the caller's state is preserved on both edges (success and failure), as call-by-value semantics dictate.

### 5.6 Class Dispatch

A class-dispatched call invokes a value resolved through dispatch (a method-bearing value held in the class's dictionary for the receiver's type). The mark of the resolved value comes from the class's *declaration* of that method, not from the specific instance's implementation. A class declaring `Loggable :: log : String` (with the `:` mark) requires every instance to implement `log` as `:`-conformant; instance bodies are individually checked for class-conformance. The call site uses the class's mark — `:` — for the transfer function.

This is consistent with CP4 §3 (Haskell-style dictionary passing): the dictionary holds command-typed values whose marks match the class's declarations. The typechecker enforces that instance implementations conform.

### 5.7 V-Commands

A v-command's mark is declared on the v-command itself; the receivers' parameter modes (productive, reference, IN, per CP5 §2) are orthogonal to the failure-mode mark. A `:`-marked v-command must be `:`-conformant under the failure analysis; its receivers carry their CP5 R1+R2 obligations independently.

V-command dispatch composes single-class dispatches across the receivers' types (CP1 §4.4). Each receiver's class-method invocation contributes per the class-dispatch rule (§5.6 above). The v-command's overall failure-mode behavior is determined by composing those per-receiver invocations within the v-command's body.

### 5.8 First-Class Command-Typed Values

A slot of type `?<...>` (or `:<...>` or `!<...>`) holds a command-typed value. Invoking the slot is a call site whose declared mark is the slot's mark. The transfer function uses the slot's mark per §5.1.

The same applies to lambda values (per CP7 §3.5: lambdas have a visible signature that includes the failure-mode mark; the call site uses that mark) and to fexpr values (per CP10 §7: fexpr's failure-mode marker is part of its construction syntax; the invocation site uses that marker).

### 5.9 The `.fail` Keyword

The `.fail` operation takes one argument — a value, the failure value, the `Φ`-component of the operational semantics tuple from CP1 §1. Its static semantics:

- `.fail value` is a `!`-mode operation. It always fails; never produces a successful return.
- The transfer function is the same as a `!`-call: one outgoing edge, the failure edge with state `failing(!)`. There is no success edge.
- The post-`.fail` program point is reachable only via the failure-skip mechanism (i.e., not via ordinary control flow).
- The argument's type determines the failure value's type. (See OQ-26 for the broader question of how failure types are represented in signatures and how `.fail`'s argument type interacts with the surrounding command's signature.)

`.fail` is a keyword (per the convention that all Basis keywords begin with a period), parallel to `.cmd`, `.test`, `.program`, `.domain`, `.record`, `.union`, `.variant`, `.implicit`, etc. It is the primitive way to produce a failure with a programmer-specified value, distinct from a `?`- or `!`-call whose failure originates from within that call's body.

---

## 6. Body-Conformance Rules

A command body is **conformant** with its declared mark when its CFG analysis exit edges satisfy the rule for that mark. Conformance is the typechecker's verification that the body's static behavior matches the declared signature.

### 6.1 The Three Rules

**`:`-conformance.** A `:`-marked command body is well-formed when:
- Every reachable exit edge has state `clear`.
- At least one exit edge is reachable (i.e., not every exit edge is in state `unreachable`).

A `:`-marked body that has any exit in state `failing(M)` or `mixed(M)` fails conformance — a propagating failure has reached the body's exit, contradicting the `:` declaration. The error message should identify which incoming edges to the exit carry the propagating-failure contribution.

**`?`-conformance.** A `?`-marked command body is well-formed when:
- At least one exit edge is reachable.

There is no constraint on the *state* of the exit edges: they may be `clear`, `failing(?)`, `failing(!)`, `mixed(?)`, or `mixed(!)`. The `?` declaration is permissive — the body may succeed or propagate any failure mode.

**`!`-conformance.** A `!`-marked command body is well-formed when:
- Every reachable exit edge has state `failing(?)` or `failing(!)`.
- At least one exit edge is reachable.

A `!`-marked body that has any exit in state `clear` or `mixed(M)` fails conformance — a successful path has reached the body's exit, contradicting the `!` declaration. Both `clear` and `mixed(M)` are rejected: `clear` because the path is purely successful; `mixed(M)` because some path (the clear arm of the mixed state) is successful, even though other paths are failing.

### 6.2 The Asymmetry as Dual

The rules for `:` and `!` are exact duals:

- `:` rejects every exit-edge state containing a propagating-failure contribution: `failing(M)`, `mixed(M)`. Accepts only `clear`.
- `!` rejects every exit-edge state containing a successful-path contribution: `clear`, `mixed(M)`. Accepts only `failing(M)`.

The `?` middle accepts everything (with the reachability rider).

This duality is the principle behind the lattice's shape. The lattice was designed such that the conformance check for `:` and `!` is a simple state-membership test: `:` requires `clear`; `!` requires `failing(?)` or `failing(!)`; `?` requires reachability only. The mixed and unreachable states are precisely the cases where the check needs to pay attention to "some paths but not all" or "no paths at all."

### 6.3 The Reachability Rider

A body whose every exit edge is in `unreachable` state has no reachable exit. This is dead-code-equivalent: the body has no way to reach its end. The reachability rider in each conformance rule rejects this case as a typecheck failure (rather than just a warning), because a body that cannot complete is not a well-defined command — it has no defined behavior at all.

The typical case where this rider engages is a body that ends with a `!`-call (or `.fail`) on every path, with no recovery context. The body never reaches its exit because every path terminates in a propagating failure that is not consumed within the body. For a `!`-marked body, this is fine — the propagating failure *is* the body's exit (the failure edge from the `!`-call is the exit edge, not unreachable). For a `:`-marked body, this should be a conformance failure (the failure cannot reach the exit and remain `clear`-conformant). For a `?`-marked body, similarly — the failure reaches the exit per `?`-conformance, which permits failing exits.

The distinction between "exit edge is reachable in state `failing(M)`" (well-formed for `?` and `!`) and "exit edge is in `unreachable` state because no incoming edge to the exit was reachable at all" is subtle but important. In the former, control reaches the exit, just along a failing path. In the latter, control does not reach the exit. The former corresponds to the body fulfilling its (failing) contract; the latter corresponds to dead-code-equivalence.

### 6.4 Diagnostic Quality

The typechecker should produce diagnostics that name the offending exit-edge contribution and the path through the CFG that produced the conformance violation. For `:`-conformance failures: "this `?call` may fail, and the failure escapes the body, but the command is declared `:`." For `!`-conformance failures: "this path through the body produces a successful exit, but the command is declared `!`." For reachability failures: "no path through this body reaches the exit; the body is dead-code-equivalent."

---

## 7. Integration With the Initialization Analysis

### 7.1 The Product Lattice

The full state at each program point is a pair (failure-state, init-state-vector), where:

- The failure-state is one of the six values from §3.
- The init-state-vector assigns, to each slot in scope at the program point, one of `init` / `uninit` / `uncertain` (per CP4 §3).

The product lattice's join operation is component-wise: the joined failure-state is the failure-state-join (per §3.3) of the per-incoming-edge failure-states, and the joined init-state of each slot is the per-slot-init-join (per CP4 §3.5) of the per-incoming-edge slot-init-states. The two components do not interact at the join level.

### 7.2 The Shared CFG Walk

The typechecker walks each command body's CFG once, maintaining at each program point the product-lattice state. At each node:
- The incoming state is the join of predecessors' contributions (component-wise).
- The transfer function is applied to produce the outgoing state for each successor edge.

The transfer functions interact between components: the failure-state determines which edges are reachable, and the init-state transitions happen on those reachable edges. Concretely:
- A productive `'r` argument at a call site updates `r`'s init-state on the success edge of the call. The success edge's failure-state is `clear` (for any call mark that has a success edge); the success edge's init-state has `r` transitioned to `init`.
- The failure edge of a call (when one exists) does not transition any productive parameter slots; their init-states pass through unchanged.
- A reference `&r` argument at a call site requires `r` to be `init` at the call site (an init-state precondition), regardless of the call's mark or the call's edges.

### 7.3 Loop Fixed-Points

The loop fixed-point analysis from CP4 §3.6 applies to the joint product-lattice. Both components must reach a fixed point under iteration. The standard work-list algorithm handles this: when a node's joined incoming state changes (in either component), the node's outgoing transfer is re-applied, and any successors whose incoming states change are re-queued.

Because both component lattices are finite-height (six states for failure, three states for init per slot, with a finite number of slots in scope), the joint lattice is finite-height and the work-list algorithm terminates.

### 7.4 Recovery Contexts and Init-State

When a `|` or `|`-with-spec block engages on a propagating failure from preceding siblings, the body's init-state-vector is the per-slot-init-join of the failure-edge contributions from those siblings — i.e., the slot states as they were on the failing paths (not on the successful paths). This is consistent with CP4 §3.7 and falls out of the CFG join naturally: the `|`-block's incoming edge is the program point where the failure was active, and the init-state at that program point reflects the failure-path slot states.

For `?` and `?-` blocks, the body executes from the success edge (on `?`) or failure edge (on `?-`) of the guard. The init-state on the engaged edge propagates to the body's entry per the standard rule.

For `^`-blocks with full-body recovery, the post-`^` program point's incoming edges include both the body's success-paths' joins and the consumed-failure paths' joins. The init-state at the post-`^` program point is the per-slot intersection across all those paths.

### 7.5 Implementation Strategy

The implementation walks the CFG with a work-list algorithm. Each node has a state slot (initially bottom: failure-state `unreachable`, init-state-vector all-uninit). The algorithm seeds the entry node with the body's initial state (failure-state `clear`; parameters in `init` state, locals not yet introduced). It processes the work-list, applying transfer functions and updating successor states; if a successor's state changes, it re-queues. Convergence is when no node's state changes. Conformance is then checked at the body's exit edges per §6.

The single CFG walk produces both analyses: failure-mode conformance and initialization correctness. The two analyses share the work-list, the state representation, the join points, and the transfer-function evaluation. They differ only in which component of the state they update — a difference that is local to each transfer function's implementation.

---

## 8. The Never-Fails-Must-Handle Rule as Corollary

The "never-fails-must-handle" rule from CP2 §1.4 stated that a `:`-marked command body must handle every internal `?`-call's possible failure. Under the conformance machinery from §6, this rule is no longer a separate stipulation — it falls out of `:`-conformance.

### 8.1 Reading the Rule from `:`-Conformance

A `:`-marked body must have every exit edge in state `clear` (per §6.1). Every internal `?`-call produces a failure edge with state `failing(?)`. For that `failing(?)` to not contribute to an exit-edge state of `failing(M)` or `mixed(M)`, it must be consumed by some recovery context within the body — `?`'s guard recovery, `?-`'s guard recovery, `?:`'s chain recoveries, `^`'s full-body recovery, `|` (with or without spec) on a propagating-failure path, or `??`'s rewiring (which consumes the inner-construct's guard failure when `??` engages).

If no recovery context catches the failure, the failure edge propagates to the body's exit, the exit state contains a propagating-failure contribution (`failing(M)` or `mixed(M)`), and `:`-conformance fails. The user gets the must-handle error at the call site of the unrecovered `?`-call, with a diagnostic identifying the path through the CFG.

The rule is therefore not a separate check; it is what `:`-conformance demands.

### 8.2 The Dual Rule for `!`-Bodies

By the duality from §6.2, the analogous rule for `!`-marked bodies is: every reachable path must produce a propagating failure that reaches the exit, and no recovery context within the body may fully recover all of them.

Concretely, an `!`-marked body that contains a `?`-call whose failure is fully recovered by a `|` block is in state `clear` after the `|`. If the body's exit follows that `|` with no further failing operation, the exit state is `clear` and `!`-conformance fails. The diagnostic: "this path through the body produces a successful exit, but the command is declared `!`." The fix is to add a `.fail` operation at the end of that path, or to restructure the body so the `?`-call's failure isn't fully recovered.

This dual rule is the manifestation of `!`-conformance in the presence of recovery contexts. Like the must-handle rule for `:`, it is not a separate stipulation — it follows from §6.1.

### 8.3 Composition Across Mechanisms

The conformance check composes naturally across the language's various command-bearing mechanisms:

- **Class methods.** Per §5.6, the call site uses the class's declared mark; instances are individually checked for class-conformance against their methods' implementations.
- **V-commands.** Per §5.7, the v-command's body is conformant against its declared mark, with receiver-class invocations contributing per §5.6.
- **First-class command values.** Per §5.8, the slot's declared mark is used at the call site.
- **Lambdas.** Per CP7 §3.5 and CP8 §3, the lambda's failure-mode mark constrains its body. The body is conformant against its mark; the call site uses the mark from the visible signature.
- **Fexprs.** Per CP10 §7, the fexpr's marker constrains its body. The invocation site uses the marker.

In all cases, the conformance check is local to each command body or lambda body or fexpr body; the composition is via the call-site transfer function reading the declared mark of the value being invoked.

---

## 9. Worked Examples

This section walks several CFG analyses in detail. The examples are arranged in increasing complexity and exercise different combinations of constructs.

### 9.1 Trivial Linear Body

```
.cmd doNothing : =
    log: "hello"
```

Assume `log: String` is declared `:` (never fails).

**CFG.** Single node for `log: "hello"`, plus entry and exit synthetic nodes.

**Analysis.** Entry state `clear`. The `log` call is a `:`-invocation: one outgoing edge, success, state `clear`. The body's exit-edge state is `clear`.

**Conformance.** `doNothing` is declared `:`. Every exit edge has state `clear`; the exit is reachable. `:`-conformant. ✓

### 9.2 `:`-Body With Stray `?call`

```
.cmd brittle : Int32 input =
    # parsed <- (?parseFloat32: input)         ; ?-call, failure not handled
    log: parsed
```

`?parseFloat32: input` is declared `?` (may fail).

**CFG.** Sequence: entry → `?parseFloat32` call → `log` call → exit.

**Analysis.** Entry state `clear`. The `?parseFloat32` call: success edge with state `clear`, failure edge with state `failing(?)`. The next node, `log: parsed`, has the success edge as its only incoming non-failing predecessor; the failure edge skips past `log` and reaches the body's exit (per failure-skip semantics). Post-`log`-call program point: state is the join of `log`'s exit (state `clear` from the success-path; `log` is `:`) and the failure-skipped state from `?parseFloat32`'s failure edge (state `failing(?)`). Joined: `mixed(?)`.

The body's exit edge has state `mixed(?)`.

**Conformance.** `brittle` is declared `:` (no mark). `:`-conformance requires every exit edge to be `clear`; this exit is `mixed(?)`. Conformance fails. Diagnostic: "the call to `?parseFloat32: input` may fail, and the failure escapes the body, but `brittle` is declared `:`. Add a recovery context (e.g., `?` guard, `|` recovery) or declare `brittle` as `?`."

### 9.3 `:`-Body With `|` Recovery

```
.cmd robust : Int32 input, Float32 'result =
    # parsed <- (?parseFloat32: input)
    | parsed <- (Float32: 0)                   ; recovery: default to zero
    'result <- parsed
```

**CFG.** Entry → `?parseFloat32` call → (success: post-call program point) | (failure: skip to `|`-block) → `|`-block body (`parsed <- (Float32: 0)`) → post-`|` program point → `'result <- parsed` → exit.

**Analysis.** Entry state `clear`, `parsed` uninit. After `?parseFloat32` call: success edge has state `clear` and `parsed` init; failure edge has state `failing(?)` and `parsed` uninit. At the post-call program point (where `|` engages), incoming state is `mixed(?)` with `parsed` `uncertain` (init on success-arm, uninit on failure-arm). The `|`-block engages on the failing-arm: failure consumed, body executes from `clear`. The body's `parsed <- (Float32: 0)` is a productive write — `parsed` becomes init on this path. Post-`|` program point's state is the join of: (clear-arm of the `mixed(?)`, where `parsed` is init) and (engaged `|`-body's exit, where `parsed` is also init). Joined: `clear`, with `parsed` init.

After `'result <- parsed`: `'result` is productive; the bare-identifier-`<-` form copies `parsed`'s value. Success edge: state `clear`, `'result` init.

The body's exit edge has state `clear`, `'result` init.

**Conformance.** `robust` is declared `:`. Exit edge is `clear`. `:`-conformant. ✓

The productive `'result` parameter has init state `init` on the success edge of the body's exit; the productive obligation is discharged. ✓

### 9.4 The Canonical Loop, Bodied `^`

```
.cmd countDown : Int32 'count =
    'count <- (Int32: 10)
    ?? ? lessThan: count, 1
            log: count
       ^ subtract: 'count, count, 1
```

`lessThan: Int32, Int32` is declared `?` (succeeds when first arg less than second; fails otherwise). `subtract: Int32', Int32, Int32` is declared `?` (productive output, may fail on overflow). `log` is `:`.

Note: this example assumes `lessThan: a, b` succeeds when `a < b`; the loop counts up by failing the guard once `count` reaches 1. The structure illustrates the `??/?/^` pattern; the exact semantics may need a different test for downward counting, but the CFG structure is what matters here.

**CFG.** Entry → `'count <- (Int32: 10)` → `??`-block → exit. Inside `??`-block: `?`-block (guard: `lessThan`, body: `log: count`) and sibling `^`-block (body: `subtract`).

**Edge structure.** The `?`-block's guard-failure edge is rewired by `??` to bypass the `^`-block and route to the post-`??`-block program point. The `?`-block's body-exit (after `log`) flows to the `^`-block. The `^`-block's body-success edge rewinds to the `?`-block's guard. The `^`-block's body-failure edge (after consumed-failure) flows to post-`??`.

**Analysis.** Entry state `clear`, `'count` uninit. After `'count <- (Int32: 10)`: state `clear`, `'count` init. `'count` discharged. Note the productive write happened once at the body's start; the loop must not write `'count` again.

Wait — but the loop's `subtract: 'count, count, 1` is a productive write! That violates the exactly-once rule from CP9 §2 if the loop iterates more than once.

Let me revise the example:

```
.cmd countAndReport : Int32 'finalCount =
    # i : Int32 <- (Int32: 0)
    ?? ? lessThan: i, 10
            log: i
       ^ add: 'newI, i, 1
         i <- newI                            ; rebind i for next iteration
    'finalCount <- i
```

Hmm, this is getting complicated with the productive-once rule. Let me use a cleaner example.

```
.cmd sumToTen : Int32 'total =
    'total <- (Int32: 0)
    # i : Int32 <- (Int32: 0)
    ?? ? lessThan: i, 10
            # newTotal <- (add: total, i)     ; ?-call inside loop
            total <- newTotal                  ; not productive; ordinary local rebind
            # newI <- (add: i, 1)
            i <- newI
       ^                                       ; bodiless ^
```

Wait, but `'total` is productive and must be written exactly-once. The `total <- newTotal` inside the loop is a non-productive (ordinary) bare-identifier copy — but we initialized `total` with the productive `'total <- (Int32: 0)` write, which discharged the productive obligation. The subsequent `total <- newTotal` writes are ordinary writes to the local view of the parameter slot, not productive writes — they update `total`'s value but do not re-discharge the productive obligation (the obligation already discharged).

Actually, looking at this more carefully: the productive marker `'` constrains the *callee* (this command) to write the slot exactly once on every successful path. The subsequent writes inside the loop are *additional* writes, which under exactly-once is a violation. So the loop must not contain any productive write to `'total`.

The cleaner pattern: introduce `# total : Int32 <- (Int32: 0)` as a local, accumulate within it, and discharge `'total` with a single bare-identifier copy at the end:

```
.cmd sumToTen : Int32 'total =
    # i : Int32 <- (Int32: 0)
    # accum : Int32 <- (Int32: 0)
    ?? ? lessThan: i, 10
            # newAccum <- (add: accum, i)
            accum <- newAccum
            # newI <- (add: i, 1)
            i <- newI
       ^
    'total <- accum                            ; productive discharge, exactly once
```

OK, this should work. Let me walk it.

**CFG.** Entry → `# i <- ...` → `# accum <- ...` → `??`-block → `'total <- accum` → exit. Inside `??`: `?`-block with guard `lessThan: i, 10` and body (the four-statement accumulation), and a bodiless sibling `^`. The `^` rewinds to the `?`-block's guard; the bodiless `^` always rewinds (treated as if its body succeeded).

**Edge structure.** The `?`-block's guard-failure edge is rewired by `??` to the post-`??` program point. The `?`-block's body-exit edge flows to the bodiless `^`. The bodiless `^`'s only outgoing edge is the rewind to the `?`-block's guard — there is no post-`^` edge from the `^`-block itself. The post-`??` program point is reached only via the rewired guard-failure edge.

**Analysis (sketched).** Initial state: entry `clear`, all slots uninit. After `# i <- ...` and `# accum <- ...`: `clear`, both `i` and `accum` init.

Entering `??`. Inside, the `?`-block guard `lessThan: i, 10`: `?`-call with success edge (`clear`) and failure edge (`failing(?)`). The failure edge is rewired by `??` to bypass the `^`-block and reach post-`??` with state `clear` (failure consumed by `??`).

On guard success: body of `?` runs. Each `add` is `?` (overflow possible). Without going step-by-step through every `?`-call's failure handling, observe that each unhandled `?`-failure inside the body would propagate. For this example to be well-formed in a `:`-marked enclosing command, every `add` in the body must have its failure either consumed or restated.

For brevity, assume the surrounding command is `?`-marked or has appropriate handling; the CFG analysis would track each `?`-call's failure edge, and the body's exit (flowing to the bodiless `^`) would have a state determined by which failures were consumed.

If body completes successfully: exit edge to bodiless `^`. Bodiless `^` rewinds unconditionally to guard.

Loop fixed-point: the analysis iterates until states stabilize. The `i` and `accum` slots remain `init` throughout; the failure-state at the guard's incoming edge is `clear` (from the rewind path).

After `??`: post-`??` state is `clear` (from the rewired guard-failure edge). `accum` is init.

After `'total <- accum`: productive bare-identifier copy. `'total` becomes init. Exit edge: `clear`, `'total` init.

**Conformance.** If `sumToTen` is `:`-marked and all internal `?`-failures inside the loop are handled, exit is `clear`; `:`-conformant. Productive `'total` has init `init` on the success exit; obligation discharged exactly once (at the `'total <- accum` line).

### 9.5 The Canonical Loop, Bodiless `^` (Pure Form)

```
.cmd readUntilEOF : =
    ?? ? readLine
       ^
```

Assume `readLine` is `?` (fails on EOF).

**CFG.** Entry → `??`-block → exit. Inside `??`: `?`-block (guard `readLine`, no body) and bodiless `^`.

**Edge structure.** `?`-block guard-failure rewired to post-`??`. `?`-block guard-success flows to the bodiless `^`. Bodiless `^` rewinds to guard.

**Analysis.** Entry `clear`. Guard succeeds (read successful): body of `?` (empty) → bodiless `^` → rewind. Guard fails (EOF): rewired to post-`??` with state `clear`. Loop fixed-point: state stable at `clear` everywhere. Post-`??`: `clear`. Exit: `clear`.

**Conformance.** `readUntilEOF` is `:`-marked. Exit is `clear`. `:`-conformant. ✓

This is the cleanest illustration of the bodiless `^` idiom: "loop until the guard fails."

### 9.6 `!`-Marked Body Using `.fail`

```
.cmd alwaysFail : String reason =
    log: reason
    .fail: reason
```

**CFG.** Entry → `log: reason` → `.fail: reason` → exit.

**Analysis.** Entry `clear`. After `log` (which is `:`): success edge `clear`. After `.fail: reason`: failure edge `failing(!)`. Post-`.fail` program point reachable only via failure-skip — the failure edge goes to the body's exit. Exit edge state: `failing(!)`.

**Conformance.** `alwaysFail` is `!`-marked. Exit is `failing(!)`. `!`-conformant. ✓

### 9.7 `!`-Marked Body, Conformance Failure

```
.cmd brokenMustFail : String reason =
    log: reason
    ; (forgot to .fail)
```

**CFG.** Entry → `log: reason` → exit.

**Analysis.** Entry `clear`. After `log`: `clear`. Exit: `clear`.

**Conformance.** `brokenMustFail` is `!`-marked. Exit is `clear`; `!`-conformance rejects `clear`. Diagnostic: "this path through the body produces a successful exit, but the command is declared `!`. Add a `.fail` or restructure so that every path produces a propagating failure."

### 9.8 Cascade of `|`-with-spec Recovery (Without Typed Failures)

```
.cmd attempt : Int32 input =
    # result <- (?parseAndProcess: input)
    | parseError                               ; recovery 1
        log: "parse failed"
    | processError                             ; recovery 2
        log: "process failed"
    | log: "unknown failure"                   ; bare | as final fallback
    log: result
```

Without typed failures (per OQ-26), the typechecker cannot statically determine whether `parseError` matches the propagating failure, or whether `processError` matches. The conservative analysis: each `|`-with-spec block may or may not engage; non-engagement allows the failure to continue propagating.

**Analysis (conservative, without typed failures).** After `?parseAndProcess`: success edge `clear` with `result` init; failure edge `failing(?)` with `result` uninit.

`|` block 1 (`parseError`): may engage. If engages: body executes from `clear`, exits with `clear`. If doesn't engage: failure continues propagating, state remains `failing(?)`.

`|` block 2 (`processError`): may engage on whatever failure was not caught by block 1. Same conservative treatment.

Bare `|` block 3: engages on any propagating failure. After this block, no failure propagates (bare `|` catches everything).

Joined post-recovery state: `clear`, but `result` is `uncertain` (init on success-path, uninit on the recovery-paths). The subsequent `log: result` reads `result`, which requires `init`; the read fails the init-check.

**Conformance.** With typed failures (and OQ-26 resolved), the typechecker could determine that `parseError` and `processError` together exhaust all possible failure types from `?parseAndProcess`, eliminating the need for the bare `|`. Without typed failures, the bare `|` is required for full coverage, and the recovery bodies should themselves write `result` to satisfy the init-check at the subsequent `log: result`.

The example illustrates an ergonomic gap: without typed failures, the user cannot statically prove that their `|`-with-spec chain is exhaustive, so they must include the bare `|` defensively, and the typechecker cannot help them refine when refactoring changes the failure types.

### 9.9 Class Dispatch Through a `:`-Method

```
.class Loggable :
    declare log : String

.cmd processItem : Loggable item, String message =
    item :: log: message
    log: "processed"
```

**Analysis.** The class declares `log` as `:` (no mark). At the dispatch site `item :: log: message`, the call uses the class's mark `:`. Transfer function: success edge `clear`, no failure edge.

After `item :: log: message`: state `clear`. After `log: "processed"`: `clear`. Exit: `clear`.

**Conformance.** `processItem` is `:`. Exit is `clear`. ✓

If an instance of `Loggable` for some type tried to implement `log` with internal failures and let them propagate, that instance's body would fail `:`-conformance. Class-conformance is checked instance-by-instance per CP1 §4 (Haskell-style dictionary passing).

---

## 10. Refinements to Earlier Checkpoints

### 10.1 `^` (DO_REWIND) Body Is Optional; Preceding Sibling Required

Per the user's note during this checkpoint's intent dialog, the `^` construct's body is optional. CP2 §2.6 implicitly assumed the body was always present; this checkpoint refines that. The full grammar of `^` is `^ body?`, where `body` is zero or more statements. A bodiless `^` is treated as if its body had succeeded — control unconditionally rewinds to the preceding sibling.

A `^` without a preceding sibling at the same indentation level is a static semantic error. The construct requires something to rewind to.

This refinement does not change the operational semantics of the bodied case (CP2 §2.6) and does not change any other checkpoint's reasoning. It updates CP2 §2.6 in place.

### 10.2 Dead-Code-Equivalent Cases Are Warning-Worthy

Per CP8 §3.4 and consolidated here: a productive parameter on a `!`-call discharges vacuously (no successful return path exists). The discharge is warning-worthy because the productive slot will never be written. Similarly, a body whose every exit edge is `unreachable` (per §6.3) is dead-code-equivalent and should produce a warning.

These are not new rules; they consolidate observations from CP4 and CP8 in light of the conformance machinery from §6. Implementations should produce diagnostics that name the specific path or call site that triggers the warning.

---

## 11. Implementation Notes

### IN-19: Work-List Algorithm Details

The forward-flow analysis is a standard work-list algorithm:

1. Construct the CFG of the command body. Each statement is a node; control-flow constructs introduce branching nodes and join nodes per the structure described in §4.
2. Initialize each node's state to bottom (failure-state `unreachable`, init-state-vector all-uninit).
3. Seed the entry node with the body's initial state: failure-state `clear`; parameters in `init` state (per their CP5 R1+R2 obligations); locals not yet introduced (uninit).
4. Add the entry node to the work-list.
5. While the work-list is non-empty: pop a node, compute its joined incoming state (per §3.3 and CP4 §3.5), apply its transfer function (per §4 and §5), and update each successor's state. If a successor's state changes, add the successor to the work-list.
6. Continue until the work-list is empty (no node's state changes).
7. Check conformance at the body's exit edges per §6.

The lattice is finite-height (six failure-states; three init-states per slot times finite slots in scope), so the algorithm terminates.

### IN-20: Lattice Representation

The failure-state lattice has six elements; a 3-bit representation suffices (with one bit-pattern unused). The init-state lattice has three states per slot; 2 bits per slot. The combined per-program-point state is a small struct: 3 bits for failure-state, plus 2 bits per in-scope slot.

### IN-21: Diagnostic Quality

The typechecker should produce diagnostics that:
- Name the specific call site or block-marker construct that triggered the conformance failure.
- Identify the path through the CFG that produced the offending state contribution. This requires the analysis to track *which* incoming edges contributed to a given joined state, which can be done by maintaining, per node, the set of contributing predecessors with their per-edge states.
- For init-state failures, identify the slot whose state is uninit/uncertain at the use site.
- For dead-code-equivalent warnings, identify the path that makes the warning case applicable.

This level of diagnostic quality is implementation work, not language-design work, but is worth flagging as part of the typechecker's contract with the user.

### IN-22: Relationship to `Φ`-Tracking

The failure-state lattice tracks *whether* and *with what mode* a failure may be propagating, not the failure value `Φ` itself. The value is a runtime quantity carried by the failure as it propagates; the static analysis does not generally know its identity. Per OQ-26, the introduction of typed failures would extend the static analysis to track the failure value's *type* at each program point, refining the lattice to include type information. The current lattice is type-agnostic and is the floor on which OQ-26's resolution will build.

---

## 12. Grammar Status

This checkpoint introduces no new grammar productions or tokens. The lattice analysis and conformance check operate against the existing grammar's parser output. The `^`-body-optional refinement (per §10.1) is a correction to checkpoint 002 §2.6, not a grammar change — the grammar already accepts `^` with no body (as `DO_REWIND` followed by an empty body), per `Grammar2.cpp` master inspection during prior checkpoints.

The `.fail` keyword's grammar production is the standard period-prefixed-keyword form parallel to `.cmd`, `.test`, etc. The exact `Grammar2.cpp` production for `.fail` may need to be added if it is not already present; this is a small, mechanical change.

---

## 13. Open Questions Updated

| OQ | Status | Notes |
| --- | --- | --- |
| OQ-1 | Open | Union discriminator representation. |
| OQ-2 | Open | Implementation latitude for IN parameter passing. |
| OQ-3 | Mostly resolved (per checkpoint 004) | |
| OQ-4 | Resolved (per checkpoint 004) | |
| OQ-5 | Open | Single-class instance coherence. |
| OQ-6 | Substantially resolved (per checkpoint 008) | |
| OQ-7 | Resolved (per checkpoint 010) | |
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
| OQ-25 | Open | Shadowing of captured names by body-introduced locals in fexpr/lambda bodies. |
| OQ-26 | **New** | Typed failures: command signatures advertising what types of failure values they may emit. See §13.1 below. |

### 13.1 OQ-26: Typed Failures (New, Substantial)

**The problem.** Command signatures currently carry only the failure-mode mark (`:` / `?` / `!`); they do not carry information about what *types* of failure values may be produced by the command. This has several consequences:

- **`|`-with-spec recovery cannot be statically verified for exhaustiveness.** A chain of typed recovery handlers cannot be proven to cover all possible failure types because the typechecker has no static knowledge of which types may emerge from a given `?` or `!` invocation. The user must include a bare `|` as a defensive fallback to ensure full coverage.
- **Refactoring is unsafe.** Adding a new failure type to a command's body silently breaks callers' recovery logic — the new type may not be caught by existing `|`-with-spec handlers, and the typechecker cannot detect the gap.
- **Bare `|` is the only fully-general recovery.** This degrades the language's recovery story: the typed `|`-with-spec form, which was designed to enable type-driven discrimination of failures (CP2 §2.10), cannot be used in safety-critical patterns without falling back to bare `|` for the residual case.
- **Diagnostic quality is limited.** The typechecker cannot help the user understand which failures their code may need to handle, beyond "this `?`-call may fail" — without the failure type, it cannot list the specific failure types.

**The design space.** Several approaches are worth considering:

- **Single-failure-type signatures.** Each `?` or `!` mark in a signature carries one failure type, which may itself be a union or variant summing multiple cases. The signature looks like `?<FailureType, ParameterTypes...>` or similar. Subsumption must specify when one failure type is interchangeable with another.
- **Failure sets.** Each `?` or `!` carries a *set* of named failure types. Set union under composition: a command that calls multiple sub-commands with different failure types has a failure set that is the union. This is the Zig approach (error sets).
- **Implicit inference.** The typechecker infers each command's failure type from its body, similar to Zig's error set inference. This reduces the signature burden but couples signatures to bodies in ways that may complicate separate compilation.
- **Class-based.** Failures must implement a `Failable` class with operations for discrimination and inspection. The class mechanism handles dispatch on failure type uniformly.

**Considerations for resolution.** The chosen mechanism must:

- Integrate with the buffer-backed/non-buffer split (per CP6). Failures that propagate need to be efficiently representable; very large failure values incur copy costs as the failure propagates through frames.
- Handle the discriminator question from OQ-1. Failure values that are tagged sums must have well-defined discrimination; the choice of discriminator representation affects how `|`-with-spec implements its filter.
- Specify the failure-type signature surface (placement, syntax) without overburdening signatures with noise.
- Specify subsumption rules: when a callee's failure type is widened by the caller's signature.
- Address inference: whether and where automatic inference is permitted, and how it interacts with separate compilation.
- Integrate with class dispatch (a class method's failure type must be known across all instances; otherwise instances can produce unanticipated failure types that callers cannot prepare for).
- Specify how `.fail`'s argument type relates to the surrounding command's failure type signature.

**Recommended next step.** OQ-26 is the natural next major thread, separate from this checkpoint. Resolution should occur before the typechecker integration described in this checkpoint reaches full implementation, because the lattice analysis here assumes (per the conservative treatment of `|`-with-spec in §4.8) that engagement is possibly-not-statically-determinable. With typed failures, that engagement becomes statically determinable in many cases, and the lattice analysis can be refined to incorporate type information at the program-point state.

The mechanism, once chosen, will likely refine §3 (the lattice gains a type-of-propagating-failure component on the failing/mixed states), §4.8 (`|`-with-spec engagement becomes statically determinable for typed failures), §6 (conformance can be sharpened to "the body's exit failure types are a subset of the declared failure types"), and §9.8 (the cascade example becomes statically verifiable for exhaustiveness).

---

## 14. Summary of Changes from Prior Checkpoints

| Topic | Prior State | This Checkpoint |
| --- | --- | --- |
| Failure-mode static analysis | Implicit (CP2 §6 noted the thread; not designed) | Six-state lattice with full transfer functions (§§3–5) |
| Body conformance | Implicit (must-handle rule from CP2 §1.4 stated operationally) | Three precise rules: `:`, `?`, `!`-conformance (§6) |
| Subsumption | Implicit | `: <: ?`, `! <: ?`; parameter modes invariant (§5.2) |
| Initialization analysis integration | Stated as "shared CFG walk" (CP4, CP9) | Specified as product lattice with independent join, interacting transfer functions (§7) |
| Must-handle rule | Stated as separate rule (CP2 §1.4) | Corollary of `:`-conformance; dual rule for `!`-conformance (§8) |
| `.fail` keyword | Used in examples; semantics implicit | Specified as `!`-call with explicit failure value (§5.9) |
| `^` body optionality | Implicit assumption that body present (CP2 §2.6) | Body explicitly optional; no preceding sibling is static error (§10.1) |
| Typed failures | Not addressed | New OQ-26: substantial, flagged as next major thread (§13.1) |

---

## 15. Provenance

**Authored:** Distilled from continued intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-29.

**Source materials read for this checkpoint:** intent-checkpoint-001.md through intent-checkpoint-010.md.

**Grammar changes implied:** The `.fail` keyword may need explicit production if not already present in `Grammar2.cpp`. The `^`-body-optional clarification (per §10.1) is consistent with the existing grammar; it is a correction to CP2 §2.6's prose, not a grammar change. No other grammar changes are implied.

**Recommended next step:** Commit alongside the prior checkpoints (suggested path: `docs/intent-checkpoint-011.md`). The natural next major thread is **OQ-26 (typed failures)**, which is a substantial design thread that should land before full implementation of the typechecker analysis described here. After OQ-26, the smaller open questions (OQ-13 implicit context parameters, OQ-14 same-scope rule for `&x` and `x`, OQ-15 downcast intrinsic, OQ-16 overloading restriction, OQ-22 parameterized literal types, OQ-25 shadowing rule) can be resolved inline with implementation work or in small dedicated checkpoints. The implementation of the failure-mode-and-typechecker analysis itself is bounded by what OQ-26 settles, since the lattice will likely be refined to track failure types at the program-point state.
