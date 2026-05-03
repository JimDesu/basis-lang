# Basis Language — Intent Checkpoint 007

**Status:** Working draft. Builds on intent-checkpoint-001.md through intent-checkpoint-006.md; supersedes them where they conflict. Notable supersedure: the no-non-local-state principle is reframed in §1, which sharpens (rather than overturns) checkpoint 001's articulation of the principle.
**Date of this checkpoint:** April 2026
**Provenance:** Continuation of the intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-28. This checkpoint resolves a long-standing tangle around closure semantics by establishing two distinct mechanisms — lambdas (as objects with implicit fields) and fexprs (as slot-reference-bearing values with lifetime ceilings) — and by sharpening the no-non-local-state principle into a provision-chain rule that licenses on-stack reach without permitting unrestricted access. The checkpoint also pins down the literal-as-untyped-buffer rule and adds three new open questions.

**Authority statement.** Where this document differs from earlier checkpoints, this document is authoritative. Where this document and the source code differ on syntactic matters, the code is authoritative; where they differ on semantic intent, this document is authoritative. The README and `language_notes.txt` remain non-authoritative.

---

## 1. The No-Non-Local-State Principle, Sharpened

The no-non-local-state principle has been articulated, in checkpoint 001 and subsequently, in language that conflated two distinct restrictions. This checkpoint disentangles them and restates the principle in its load-bearing form.

### 1.1 The Reframing

The principle is *not* that a command can only access state it has been passed as a parameter. It *is* that a command cannot reach for state that lies off the call stack — module-level mutable globals, ambient singletons, hidden caches, thread-local storage, and the like. State that physically resides on the call stack remains in principle reachable, with the further restriction that such reach must be licensed by an explicit provision chain (see §1.2).

The earlier articulation — "a command's parameter list is a complete inventory of its potential effects on the universe" — is a *useful description of the typical case*, where the typical case is a regular command whose effects flow exclusively through its parameters. It is not the universal rule. Mechanisms exist (and will exist) where a value carries with it a reference to state in a frame above the current one, and those references are themselves a form of provision.

### 1.2 The Provision Chain Rule

State reachable from inside a command body must arrive there by an *explicit chain of provision*. There are three kinds of provision:

- **Parameters and receivers.** A frame's caller passes state as named arguments at the call site. The state is locally bound to the parameter (or receiver) names within the body.
- **Context parameters.** The Scala-implicit-style mechanism (checkpoint 001 §3.5): a frame's caller has a value of matching type in lexical scope, which is automatically supplied at the call site. The provision is implicit at the syntactic level but not at the semantic level — the value must actually exist in the caller's lexical scope.
- **Captured-and-traveling state.** A value (such as a fexpr) carries with it references to slots in some frame, established at the moment the value was constructed. When the value is later invoked, those references are part of the value's identity. The provision was made at construction time, not at invocation time.

Absence of any of these means absence of reach. A frame six levels above the current one is on the call stack, but if no explicit provision chain reaches up to it, its slots are unreachable from the current command. The principle applies symmetrically to off-stack state (which can never be reached because no provision chain can be constructed) and to unprovisioned on-stack state (which happens to be physically present but is excluded by the same rule).

### 1.3 What This Means For The Existing Mechanisms

Every existing mechanism in the language is consistent with the sharpened principle:

- **Regular commands** receive their state via parameters and receivers; provision is at every call site.
- **Class dispatch** carries the receiver as a parameter, with the dictionary supplied as a hidden parameter; provision is implicit but explicit at the type level.
- **At-stack methods** are invoked on objects that are reachable in the registering frame at frame-exit time; provision is via the object's identity, established at object-introduction time.
- **Frame-exit hooks (`@`, `@!`)** are syntactic forms whose body executes in the frame where the hook was registered; provision is the registering frame's lexical scope, frozen at registration time.

The mechanism that motivates the sharpening — fexprs — fits cleanly: a fexpr captures slot references at construction time, those references are part of the fexpr's identity, and when the fexpr is later invoked from a different frame, the captured references travel with it. The principle is preserved because the provision was made at fexpr-construction time, not at invocation time.

### 1.4 The Lambda Mechanism Also Fits

Lambdas (introduced in §3 below) capture *values* by-copy at construction time. Those captured values become hidden fields of the lambda object. When the lambda is later invoked, its body accesses the hidden fields the same way any object's methods access their own fields — through the `self`-equivalent provision chain that all object methods enjoy. The captured *values* are part of the lambda's identity, but no slot references are involved.

Both lambdas and fexprs preserve the provision-chain rule. They differ in *what* they carry (values vs. slot references) and consequently *what lifecycle constraints* they bear (none beyond the normal object ceiling vs. a tracked capture-frame ceiling). The principle does not bend for either; both pass through the same gate.

---

## 2. The Two Brace-Quote Mechanisms

The brace-quote forms in the grammar are not a single syntactic family with one set of semantics. They split into two distinct mechanisms with sharply different semantics, distinguishable at the syntactic level by the presence or absence of a failure-mode marker.

### 2.1 Syntactic Distinction

The split is:

- **Lambda forms** — bare `{...}`, no failure-mode prefix marker.
- **Fexpr forms** — `:{...}`, `?{...}`, `!{...}`, with the failure-mode marker as prefix.

The user reading source can distinguish at sight which mechanism is being used. A bare `{...}` is a lambda. A failure-marked `:{...}`, `?{...}`, or `!{...}` is a fexpr.

This is a sharper distinction than prior checkpoints had committed to. Earlier discussion (especially OQ-7 in checkpoints 001 and 002) had treated all brace-quote forms as a single family with possible fexpr/macro semantics applying to all. This checkpoint refines that picture: only the failure-marked forms carry fexpr semantics; the bare form is a lambda.

The command-literal forms `:<...>{...}`, `?<...>{...}`, `!<...>{...}` (with explicit parameter lists in the angle brackets) remain a separate matter — they are eagerly-evaluated command values, not subject to the lambda/fexpr distinction. Their treatment is unchanged by this checkpoint.

### 2.2 The Two Capture Models At A Glance

| Aspect | Lambda (`{...}`) | Fexpr (`:{...}`, `?{...}`, `!{...}`) |
| --- | --- | --- |
| What it captures | Values (by copy) | Slot references |
| Storage of captures | Hidden fields of an implicit object | References traveling with the value |
| Lifecycle | Normal object lifecycle (introducing-frame ceiling) | Capture-frame ceiling (must not escape above it) |
| Type-system tracking required | None beyond ordinary object lifecycle | Dedicated ceiling-tracking pass |
| Can mutate captured state | No (captures are copies) | Yes (captured slot references support reads and writes) |
| Failure-mode of the value | Determined by the body's invocation | Marked explicitly by the prefix |
| Typical use | Partial application, callbacks, dispatch-capture | User-defined control-flow combinators |

### 2.3 Why Two Mechanisms

A single mechanism unified across both kinds of capture would have to choose: either (a) all brace-quote forms capture by value, in which case user-defined control-flow combinators that want to mutate calling-frame state become impossible to express, or (b) all brace-quote forms capture by reference, in which case every brace-quote value carries a lifetime ceiling, the type-system tracking burden is universal, and the simpler "I just want to capture this dispatch lookup" use case (the `{foo :: do}` tight-loop optimization) gets dragged into ceiling-tracking it doesn't need.

Splitting the mechanism along the syntactic line of failure-mode marking lets each case have the tight semantics that fit it. Lambdas are by-value and pay no ceiling-tracking cost. Fexprs are by-reference and carry the cost where it's actually paying for something. The user picks the mechanism by what they write — and the syntactic signal (failure marker present or absent) is visible at every use site, so there's no ambiguity at the reader's eye.

The split also clarifies the relationship to OQ-7. The original "block-quote macro/fexpr semantics" question was tangled because it was trying to design a single mechanism that did two different jobs. With the split, OQ-7 narrows to the fexpr-specific design (what fexpr bodies can do, the typechecker's ceiling-tracking rules, and so on); the lambda half is settled in §3 below.

---

## 3. Lambdas: Objects With Implicit Fields

Lambdas — including partially-applied commands — are implemented as objects with implicit (machine-generated, hidden) fields. The captured values are stored in those fields at construction time. The lambda inherits the existing object lifecycle in full.

### 3.1 The Model

Construction of a lambda allocates an object whose anonymous class has:

- One hidden field per captured value, holding a copy of that value.
- One method, the *invoke* method, whose signature matches the remaining (non-captured) parameters of the underlying command, and whose body forwards to the underlying command, supplying the captured fields' values for the captured-parameter positions and the invoke-method's arguments for the remaining positions.

The invoke method dispatches normally — it's a method on the lambda's anonymous class, accessed through the lambda's identity. From the rest of the language's perspective, the lambda is just an object, and invoking it is just calling a method.

A lambda whose underlying command has *no* remaining parameters after capture is a *thunk*: an object whose invoke method takes no arguments and produces the underlying command's effect. The grammar's `:<>{...}` form fits here in the command-literal direction; in the brace-quote direction, an over-fully-applied `{...}` produces a thunk. Both directions converge on the same runtime shape.

### 3.2 By-Value Capture

The captured fields hold *values*, not slot references. At construction time, each captured value is read from its source slot and copied into the lambda's hidden field. The lambda's later behavior is determined by its hidden fields' contents at the moment of construction; subsequent changes to the source slots do not affect the lambda.

This is the load-bearing decision. By-value capture is what gives lambdas the clean object-lifecycle story (no slot references means no dangling references means no ceiling beyond the ordinary object ceiling). It is also what creates the dividing line between lambdas and fexprs: the distinct cases that lambdas cannot handle (mutating captured state in the calling scope) are the cases fexprs are for.

### 3.3 Lifecycle Inheritance

The lambda is an object. It follows the object lifecycle rules from checkpoint 002 §3 unchanged:

- Its introducing-frame ceiling is the frame where the brace-quote expression was evaluated.
- It cannot escape above its introducing frame, by the same structural guarantee that prevents any object from doing so (no return values; only writeable parameters can move state upward, and writeable-parameter mechanics correctly handle ceiling migration on copy-restore — checkpoint 002 §3.3 and §3.4).
- At its frame-exit, its at-stack handling fires (with implications for captured-field cleanup discussed in IN-2 below).
- It is non-buffer-backed (per checkpoint 006 §1), so it may live in top-level slots, parameter slots, and object fields, but not in records, unions, or typed buffers.

No new lifecycle machinery is needed. The lambda *is* an object; the existing object machinery handles it.

### 3.4 Partial Application: IN-Mode Only

Partial application via the lambda mechanism — whether expressed via brace-quote syntax, a partial-application operator, or any other surface form — is restricted to **IN-mode parameters and IN-mode receivers**. Productive `'` and reference `&` positions cannot be partially applied.

The reasoning is structural. Partial application means baking a value into the lambda's captured-field set. For an IN parameter, this works directly: the captured value is a copy of the input that will be supplied to the underlying command at invocation time. For a productive `'` parameter, there is no value to bake in — the productive contract is that the *caller* supplies an output slot — and baking in a slot reference would create the fexpr-style ceiling problem in a place where it doesn't belong. For a reference `&` parameter, the same issue applies: a captured slot reference would have a lifetime ceiling at the partial-application site, and propagating the lambda above that ceiling would dangle the reference.

The IN-mode-only restriction is the clean line. A lambda's underlying command may have writeable parameters; those positions remain unbound after partial application and must be supplied at the invocation site. An attempt to partially apply on a writeable position is rejected at compile time.

This restriction does not significantly diminish partial application's utility. The common case — capture some input arguments, defer the rest — is fully supported. The disallowed case — capture some output slots — is structurally incoherent under the language's other rules and would produce ceiling problems even if the syntax permitted it.

### 3.5 Visible Signature, Hidden Representation

A lambda has a *visible signature* and a *hidden representation*.

The **visible signature** is what the programmer sees and what the type system surfaces in user-facing positions. It is a command-typed value with the failure-mode and remaining-argument shape of the lambda's invoke method:

- `:<RemainingArgs>` for a never-fails lambda.
- `?<RemainingArgs>` for a may-fail lambda.
- `!<RemainingArgs>` for a must-fail lambda.
- `:<>`, `?<>`, `!<>` for thunks (no remaining arguments).

This is the type the programmer assigns to a slot, declares as a parameter type, or stores in a field. From the surface language's perspective, a lambda is interchangeable with any other command-typed value at the same signature. It can be invoked, passed around, and stored using exactly the same mechanisms.

The **hidden representation** is the anonymous class with its captured-field structure. The typechecker reasons about the hidden representation for purposes of lifecycle, dispatch, and storage placement. Programmers cannot name the anonymous class, cannot access the hidden fields directly, and cannot subclass or otherwise specialize the anonymous class. The hidden representation exists for the language's own bookkeeping, not for the programmer's manipulation.

The split between visible signature and hidden representation is what makes the lambda mechanism *substitutable* with other command-typed values. A function that takes a `?<Int32>` parameter accepts a regular `?`-marked command, a `?<...>{...}` command-literal, *or* a lambda whose invoke signature is `?<Int32>`. The user's code can be polymorphic over these without caring which it gets. The hidden representation differs (a regular command has no captured fields; a lambda has some), but the visible signature is uniform and the invocation mechanism is uniform.

The exact form of the visible-signature type — whether it is structural (matched by signature shape) or whether it carries some marker indicating "this is a lambda" — is left for OQ-18.

### 3.6 The Dispatch-Capture Idiom As A Lambda

The dispatch-capture pattern from checkpoint 001 §3.4 — `# c <- {foo :: do}` for the tight-loop optimization — fits as a degenerate lambda case. The brace-quote captures the resolved method-bearing value (the result of the `::` lookup), producing a lambda whose only "capture" is that resolved dispatch and whose invoke method forwards directly to it. The lambda's invoke signature matches the underlying command's signature exactly, so subsequent invocations through `c` look identical to invocations through any other command-typed value at that signature.

This unifies what previously looked like separate mechanisms. The tight-loop optimization, partial application, and ordinary lambda capture are all instances of the same construction: build an object whose hidden fields hold the captures and whose invoke method is the deferred work.

---

## 4. Fexprs: Slot-Reference Capture With Ceilings

Fexprs are the language's mechanism for values that carry references to calling-frame slots and that license those references' use from inside other commands. They are not lambdas; they are not first-class command values in the ordinary sense; they are a distinct mechanism with its own lifecycle constraints.

### 4.1 The Model

A fexpr is constructed by the failure-marked brace-quote forms `:{...}`, `?{...}`, `!{...}`. The body of the form is captured as an unevaluated form, together with references to the slots it names from the surrounding lexical scope. The fexpr is a value of a fexpr-typed kind, distinct from command-typed values (a regular command-value has no slot-reference baggage; a fexpr does).

When the fexpr is invoked, its body executes against the captured slot references. Reads access the current contents of the referenced slots; writes modify them. The body executes in the *invoking* frame's control-flow context (failures propagate from the fexpr's body into the invoking frame's recovery contexts), but it operates on the *capture* frame's storage.

### 4.2 The Lifetime Ceiling

A fexpr's captured slot references are only meaningful while the capture frame is on the call stack. Once the capture frame has unwound, those slots no longer exist; an attempt to invoke the fexpr after that point would dereference dangling references.

The lifetime ceiling rule:

- A fexpr's ceiling is its capture frame.
- The fexpr may be passed *downward* freely — to commands called from the capture frame or any frame below it on the call stack at invocation time. The capture frame is still present, so the captured slots still exist.
- The fexpr may *not* be assigned to any slot whose ceiling is above the fexpr's ceiling. Such an assignment is rejected at compile time.

This is structurally identical to the object-lifetime-ceiling rule from checkpoint 002 §3.1, which also prevents upward escape. Both rules fall out of the same underlying observation: a value bound to stack-resident state has a ceiling at the frame where that state lives, and the type system must enforce that the value never reaches a slot above its ceiling.

The mechanism that enforces the rule may share machinery with object-ceiling enforcement. The two ceiling concepts are conceptually unified even if implemented separately. This is an implementation matter; what the language commits to is the rule, not the implementation.

### 4.3 Use Case: User-Defined Control-Flow Combinators

The motivating use case for fexprs is user-defined control-flow constructs that operate on calling-frame state without knowing what that state is. A user-defined while-loop is the canonical example: the loop construct receives a fexpr representing the loop body and another representing the continuation test, and on each iteration it invokes them. The fexprs read and write slots in the calling frame, doing whatever work the calling code wanted; the loop construct itself is generic over what.

This is the Kernel/newLISP family of fexpr semantics: the called value receives an unevaluated form bound to its capture environment, and the call-site licenses the reach into that environment by passing the fexpr in. The reach is provisioned at fexpr-construction time and travels with the fexpr; the called code (the loop construct) doesn't reach for state, it invokes a value that was given to it.

The provision-chain rule from §1.2 is preserved exactly. The loop construct never reaches for the calling frame's slots; it merely invokes a fexpr value that was passed to it. The fexpr's reach to the calling frame's slots was established at construction time, inside the calling frame, where reach was unambiguous.

### 4.4 What Remains To Design

This checkpoint establishes the mechanism's shape but leaves substantial detail to a future thread:

- The exact syntax for slot capture inside a fexpr body (which slots are referenced; whether capture is implicit-by-name or explicit).
- The typechecker's ceiling-tracking rules — the algorithm for proving that a fexpr never reaches a slot above its ceiling, and the program forms that suffice for such proofs.
- Whether fexprs can capture *other* fexprs (nesting), and how the ceilings compose when they do.
- Whether a fexpr can be invoked recursively, and what that means for re-entry into the capture frame's slot accesses.
- How fexpr invocation interacts with the failure-mode integration that is the natural next major thread.

These are all gathered under OQ-7 (refined in §7 below).

---

## 5. Literals as Untyped Buffers

Literals in Basis are bytes — untyped buffers — and serve as inputs to constructor commands. A decimal literal `42` is the bytes that decimal value would represent in some natural encoding, with no typing imposed; a string literal `"hello"` is the bytes of that text in the source-file encoding, with no typing imposed.

### 5.1 The Rule

A literal cannot be used as a value of any domain, record, or union directly. It can be supplied:

- As an argument to a constructor command that knows how to interpret the bytes as a value of its target type.
- As an argument to a buffer-typed parameter (`[N]` or `[]`), where the buffer-typed parameter accepts the literal's bytes verbatim.

Specifically, a `Float32`-typed slot cannot be initialized from a decimal literal directly: `# x : Float32 = 3.14` is rejected. The programmer must invoke a constructor: `# x : Float32 = (Float32: 3.14)` or whatever the constructor's surface form ends up being. The constructor takes the literal bytes and produces a `Float32` value, with whatever representation conversion that requires.

The rule preserves the buffer-backed principle's strict separation: domains have rules about which byte patterns are valid (a `Positive` excludes zero; a `Float32` has IEEE-754 layout requirements). A literal cannot satisfy those rules merely by being bytes; it must pass through a constructor that validates and (where necessary) reformats the bytes into the domain's expected layout.

The rule also resolves a class of subtle errors. Without it, the language would have to specify a default interpretation for every literal-shape-vs-target-type pair, and many such interpretations are arbitrary or ambiguous (does a hex literal `0x41` mean the integer 65, the character `A`, the byte 0x41, or something else?). With the rule, the constructor named at the call site picks the interpretation, and the literal is just bytes until it gets there.

### 5.2 Open Sub-Questions

The rule leaves a number of details for OQ-17 (introduced below):

- The exact syntactic form for "passing a literal to a constructor" — whether it uses ordinary call syntax `(Float32: 3.14)`, the OQ-10 composite-initializer form when that's settled, or something else.
- Whether bracket-form types `[N]` and `[]` accept literals directly without a constructor call, given that bracket-form types *are* the literal-bearing types in some sense (a `[16]` is sixteen bytes, full stop). The natural reading is yes — `[16]` is the type whose constructor is just "take the bytes" — but this should be confirmed.
- How implicit type conversions interact with literals. The current model has none (no implicit conversions exist except up the domain hierarchy), so a literal must always be paired with an explicit target. This is consistent with the no-hidden-control-flow principle but should be stated as a positive design commitment rather than a default.
- Whether the constructor invocation for a literal can be elided in obvious cases, e.g., `# x : Float32 = 3.14` as syntactic sugar for `# x : Float32 = (Float32: 3.14)`. Such sugar would aid ergonomics at the cost of one specific kind of hidden control flow (an implicit constructor call). The trade-off needs deliberate consideration.

---

## 6. Implementation Notes

### IN-2: Lambda Captured-Field Cleanup

A lambda is an object, and its captured fields are part of its identity from construction time onward. When the lambda's introducing frame exits and the lambda's at-stack handling fires, the lambda's captured fields are cleaned up as part of the lambda's destruction — including any captured object fields, whose at-stack-fail or at-stack methods (if any) fire transitively as the lambda is dismantled.

The mechanism reuses existing object infrastructure. From the registration system's perspective, the lambda is a normal object; its captured-object fields are normal contained-object fields. Transitive cleanup at frame exit handles the rest, exactly as it does for any user-declared object containing other objects.

The lambda itself does not need a custom at-stack method beyond what the existing object machinery provides. The compiler-generated anonymous class can have a default at-stack method that walks the hidden fields and triggers their cleanup, or it can rely on whatever default the object system provides for "destroy this object's fields."

### IN-3: Captured-Field Optimization For Buffer-Backed Captures

A lambda whose captured fields are all buffer-backed types — domains, records, unions, or typed buffers, none of which have at-stack methods — needs no destructor. The bytes of the captured fields are part of the lambda's storage and are reclaimed when the lambda's storage is reclaimed; no per-field cleanup work is needed.

The optimizer can detect this case statically and elide the lambda's at-stack method entirely. This is a significant optimization for the common case of lambdas that capture small primitive values (Int32, Float32, etc.) — the lambda becomes a trivial object with no destructor, and its construction and destruction reduce to plain field-copy and storage-reclamation operations.

Lambdas with mixed captures (some buffer-backed, some non-buffer) can have their at-stack method specialized to walk only the non-buffer captures. The buffer-backed ones contribute nothing to cleanup work.

### IN-4: Fexpr Cleanup Is Free

A fexpr captures slot references, not values. When the fexpr's capture frame exits, the slots that the fexpr's references pointed to are themselves cleaned up by the frame's normal exit machinery — those slots are just other slots in the same frame. The fexpr itself has no captured-value cleanup work to do.

Consequently, fexprs need no at-stack handling for their captures. The fexpr's storage is reclaimed when the capture frame exits; the captured references are abandoned (the slots they pointed to are simultaneously being reclaimed); no transitive cleanup is required.

The asymmetry between IN-2 and IN-4 is structural: lambdas own copies of their captures (and therefore own their cleanup); fexprs hold references to state owned elsewhere (and therefore have no cleanup obligation).

---

## 7. Open Questions Updated

| OQ | Status | Notes |
| --- | --- | --- |
| OQ-1 | Open | Union discriminator representation. |
| OQ-2 | Open | Implementation latitude for IN parameter passing. |
| OQ-3 | Mostly resolved (per checkpoint 004) | |
| OQ-4 | Resolved (per checkpoint 004) | |
| OQ-5 | Open | Single-class instance coherence. |
| OQ-6 | Open | Partial application beyond receiver-only — **substantially clarified by §3.4**: the mechanism for partial application is the lambda mechanism, restricted to IN-mode positions. The remaining sub-questions concern the surface syntax for partial application and its interaction with v-command receiver tuples. |
| OQ-7 | **Refined** | Block-quote macro/fexpr semantics — **the lambda half is settled in §3**. OQ-7 narrows to the fexpr design proper: capture syntax, ceiling-tracking algorithm, nesting and recursion semantics, and integration with the failure-mode story. |
| OQ-8 | Mostly resolved (per checkpoint 002) | |
| OQ-9 | Resolved (per checkpoint 006) | |
| OQ-10 | Open, load-bearing | Composite initializers (`=` vs. `<-`). Now also bears on §5 (the literal-passing-to-constructor story) and OQ-17 below. |
| OQ-11 | Resolved (per checkpoint 005) | |
| OQ-12 | Resolved (per checkpoint 005) | |
| OQ-13 | Open | Implicit context parameters and initialization. |
| OQ-14 | Open | Same-scope rule for `&x` and `x`. |
| OQ-15 | Open | Full design of the downcast intrinsic. |
| OQ-16 | **New** | Overloading restriction on dynamically-dispatched commands. The brace-quote dispatch-capture form `{foo :: do}` is unambiguous only if class members cannot be overloaded on argument shape — at most one command per class-member name. The candidate restriction is "class member names are unique within a class" (which would mean cross-class overloading is fine but within-class overloading is not). The exact form of the restriction, its scope (class members only, all dynamically-dispatched commands, or something else), and its costs in expressiveness are open. |
| OQ-17 | **New** | Compound literal syntax and literal-as-input rules. The §5 rule establishes that literals are untyped buffers fed into constructors; the surface syntax for that feeding (whether ordinary call form, OQ-10 composite-initializer form, or sugar over the constructor call) is open, as are sub-questions about bracket-form types accepting literals directly and whether implicit constructor-call sugar is permissible. |
| OQ-18 | **New** | Lambda anonymous-class type representation. A lambda has a visible signature (a command-typed value with the invoke method's shape) and a hidden representation (an anonymous class with captured fields). The exact type-system mechanism that connects them — whether the visible signature is a structural type matched by signature shape, whether it carries a marker distinguishing lambdas from other command-typed values, how subsumption between lambda types and other command-typed values works, and what the typechecker shows in error messages — is open. The user-facing intent is: the programmer sees only the remaining-arguments signature (or the thunk shape if no arguments remain); the rest is the language's bookkeeping. |

---

## 8. Provenance

**Authored:** Distilled from continued intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-28.

**Source materials read for this checkpoint:** intent-checkpoint-001.md through intent-checkpoint-006.md.

**No grammar changes implied by this checkpoint.** The brace-quote forms (bare and failure-marked) are already present in the grammar; the semantic split between them is the new material, and it is a typechecker matter, not a parser matter. The literal-as-untyped-buffer rule is consistent with the grammar's existing literal productions; the question of which constructor-call surface forms are accepted is part of OQ-17 and may imply later grammar changes, but no immediate change is implied.

**Recommended next step:** Commit alongside the prior checkpoints (suggested path: `docs/intent-checkpoint-007.md`). Per the prior plan, OQ-10 (composite initializers, `=` vs. `<-`) remains the load-bearing next thread for the construction story, with OQ-17 (compound literal syntax) joining it because the two are tightly linked. The fexpr design (refined OQ-7) is the natural thread after that, since it depends on the type-system tracking machinery and benefits from being approached after the construction story is settled. The full failure-mode-and-typechecker integration thread can then be picked up against a complete enough type-system foundation.
