# Basis Language — Intent Checkpoint 008

**Status:** Working draft. Builds on intent-checkpoint-001.md through intent-checkpoint-007.md; supersedes them where they conflict.
**Date of this checkpoint:** April 2026
**Provenance:** Continuation of the intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-28. This checkpoint extends the lambda mechanism in two related directions: (1) it confirms that lambda invoke-method parameters can carry any mode marker (IN, productive `'`, reference `&`), with body obligations inherited unchanged from checkpoint 004, and (2) it introduces an explicit-capture-list mechanism for the lambda-with-body form, repurposing the slash delimiter to separate invoke-method parameters from captures. Captures may be IN or reference; productive captures are forbidden. The checkpoint also introduces ceiling tracking for reference captures (with reference-chain flattening), per-invocation copy-restore semantics, a closure rule eliminating accidental captures, a consolidated rollup of grammar enhancements implied by checkpoints 001–008, and a future-intent placeholder for region-style cleanup compatibility.

**Authority statement.** Where this document differs from earlier checkpoints, this document is authoritative. Where this document and the source code differ on syntactic matters, the code is authoritative; where they differ on semantic intent, this document is authoritative. The README and `language_notes.txt` remain non-authoritative.

---

## 1. Decision Summary

This checkpoint lands six interlocking decisions about the lambda mechanism:

1. **Lambda invoke-method parameters carry any mode marker.** The angle-bracket parameter list of a lambda-with-body form (`:<...>{...}`, `?<...>{...}`, `!<...>{...}`) accepts IN, productive `'`, and reference `&` parameters, with the same semantics those modes carry on ordinary command parameters. The body is held to the obligations from checkpoint 004 §3.3–3.4. The bare brace-quote form (dispatch capture, partial application) already permits writeable invoke-method parameters per checkpoint 007 §3.4 and is unchanged.

2. **Captures in the lambda-with-body form are explicit.** A lambda-with-body uses the slash delimiter inside its angle brackets to separate invoke-method parameters from captures. Names from the surrounding lexical scope that the body uses must appear in the slash list. A body referencing a free name not in the parameter list and not in the slash list is a compile-time error.

3. **Captures are IN or reference; productive captures are forbidden.** The slash list permits plain names (IN captures) and `&`-marked names (reference captures). The productive marker `'` is forbidden in capture position. The reasoning is that productive contracts presuppose initialization-of-uninitialized-slots, which doesn't compose with multi-invocation lambda semantics.

4. **Reference captures pin the lambda's ceiling.** A lambda with one or more `&` captures has its ceiling computed as the minimum of the captured slots' origin frames, after reference-chain flattening. A lambda with no `&` captures has no ceiling beyond the ordinary object lifecycle and remains lightweight per checkpoint 007 §3.3.

5. **Reference captures use per-invocation copy-restore.** Each invocation of a lambda with `&` captures performs standard copy-restore against the captured slot: read at invocation entry, write back to the captured slot on successful return, no write-back on failure. This preserves failure-atomicity per invocation and matches the rest of the language's writeable-parameter mechanics.

6. **The bare brace-quote form retains its prior treatment.** Captures in the bare form are positional (determined by the underlying command's signature and the syntactic placement of values vs. holes) rather than nominal, and capture modes are inherited from the underlying command's parameter modes. The bare form does not need a slash list because its captures are explicit by virtue of being written rather than `_`-elided.

The checkpoint also introduces:

- A consolidated **Grammar Enhancements** section (§9) that rolls up the grammar work implied by checkpoints 001–008.
- A **Future Intents** section (§10) noting region-style cleanup compatibility as a desideratum (not a current design priority).
- Three new open questions and updates to several existing ones (§11).

---

## 2. Background and Motivation

The lambda mechanism as it stood after checkpoint 007 was structurally clean but expressively narrow. Checkpoint 007 §3.4 restricted partial application to IN-mode positions for sound structural reasons: a captured slot reference would carry a lifetime ceiling at the construction site, and propagating the lambda above that ceiling would dangle the reference. The restriction excluded both productive `'` and reference `&` captures.

Two distinct motivations pressed against this restriction. First, the **invoke-method-parameter** question: if the bare form permits writeable invoke-method parameters (which §3.4 implicitly affirms by saying "those positions remain unbound after partial application and must be supplied at the invocation site"), the same should be true for the lambda-with-body form, which currently has no syntactic provision for mode markers in its angle brackets. This is a small extension and largely a confirmation.

Second, the **capture** question: the IN-only capture rule prevents the consumer-style abstraction where a lambda mediates between a generic operation (which doesn't know about the state) and the state itself. Concretely, a callback registered with a subsystem, an event handler that updates external state, a lazy-initialized iterator with mutable cursor — these patterns either need writeable captures or have to be expressed via fexprs (which carry direct slot access semantics that bypass copy-restore and break the failure-atomicity story).

The dialog explored two relaxation paths. The first (writeable invoke-method parameters) is essentially confirmation of what §3.4 already permits, plus a grammar extension. The second (writeable captures) is more substantial: it requires ceiling tracking, reference-chain flattening, and per-invocation copy-restore semantics. Both are addressed in this checkpoint.

A separate but related concern motivated the slash-list reframe. Implicit captures — body free names being silently bound to surrounding-scope slots — are a refactor hazard inconsistent with the rest of the language's commitments to no-hidden-control-flow and explicit provision (checkpoint 007 §1.2). Making captures explicit at the syntactic level eliminates the refactor hazard and gives the typechecker a clean syntactic locus for ceiling computation and capture-mode declaration. The slash repurposing draws on the existing context-parameter convention (`:` separates parameters from implicit context parameters in regular command signatures) and uses the same syntactic shape for an analogous purpose.

---

## 3. Lambda Invoke-Method Parameters Carry Any Mode

### 3.1 The Affirmation

A lambda-with-body form's angle-bracket parameter list may include IN, productive `'`, and reference `&` parameters. The bodies of such lambdas are held to the standard obligations from checkpoint 004:

- **IN parameters** (no marker): the body may read; writes are local to the invocation and do not propagate to the caller.
- **Productive `'` parameters**: the body must definite-assign on every successful return path. The caller may pass an uninitialized slot at the invocation site.
- **Reference `&` parameters**: the body may read, write, or neither. The caller must pass an initialized slot at the invocation site.

These obligations apply at the body of the lambda, not at the lambda-construction site. The construction site sees only the lambda's visible signature (per checkpoint 007 §3.5), which carries the mode information for purposes of typechecking the eventual invocation.

### 3.2 No New Safety Question

The question "is it safe for a lambda invoke method to have writeable parameters" was framed in earlier discussion as if it required new analysis. It does not. Invoke-method parameters are not captures: they are supplied fresh at each invocation, in the invoker's frame, and live exactly as long as the call lives. Their copy-restore mechanics are the standard ones from checkpoint 001 §3.2 and checkpoint 002 §3.3–3.4. There is no new ceiling question because:

- The lambda has its own ceiling (the introducing frame, or — per §5 below — the min of `&`-captured slots' origin frames), and cannot escape upward beyond that ceiling.
- The invocation happens at or below the lambda's ceiling, since the lambda is reachable only at or below its ceiling.
- The writeable parameter's slot is in the invoker's frame, and copy-restore operates between the invoker and the invoke-method's local copy. No cross-frame slot reference enters the picture.

In short: invoke-method writeable parameters reuse the writeable-parameter machinery that already works for ordinary commands. The relaxation is a confirmation that existing rules apply unchanged.

### 3.3 The Bare Form

The bare form already permits writeable invoke-method parameters per checkpoint 007 §3.4. Nothing in this checkpoint changes the bare form's treatment.

### 3.4 Body-Side Failure-Mode Conformance

A lambda's failure-mode marker (`:`, `?`, or `!`) constrains what the body can do (per checkpoint 007 §3.5):

- A `:`-marked lambda's body must never fail.
- A `?`-marked lambda's body may fail.
- A `!`-marked lambda's body must fail.

These constraints interact with the productive obligation as follows: a productive parameter requires definite-assignment on every successful return path. A `!`-marked lambda has no successful paths, so the obligation is vacuously satisfied — though a `!`-marked lambda with a productive parameter is dead-code-equivalent and the typechecker may warn. A `:`-marked lambda has only successful paths, so the productive obligation applies universally. A `?`-marked lambda's productive obligations apply only to the success-paths.

---

## 4. Captures in the Lambda-with-Body Form Are Explicit

### 4.1 The Slash List

The lambda-with-body form's angle-bracket content is extended to support a slash delimiter:

```
:<invoke-params / capture-list>{body}
?<invoke-params / capture-list>{body}
!<invoke-params / capture-list>{body}
```

The slash delimiter is the same character used in regular command signatures to separate parameters from implicit context parameters (per checkpoint 001 §3.1). The repurposing is intentional: in both contexts, the slash separates "what the caller supplies at the call/invocation site" from "what comes from elsewhere." The semantic mechanism differs (context parameters are resolved by type at call sites; captures are resolved by name at construction time), but the syntactic position carries a consistent meaning at the right level of abstraction.

The slash and capture list are optional. A lambda-with-body with no captures uses the same syntax as before (`:<args>{body}`); no slash, no capture list. A lambda-with-body with captures includes the slash and capture list (`:<args / capture-list>{body}`). A thunk with captures (no invoke-method parameters but some captures) writes the slash with an empty left side: `:< / capture-list>{body}`. A pure thunk with neither parameters nor captures remains `:<>{body}`.

### 4.2 The Closure Rule

A lambda's body may freely use:

- Names declared in its invoke-method parameter list.
- Names declared in its capture list.
- Top-level definitions (commands, types, classes — these are not slots and not captures).

A body referencing any other free name is a compile-time error.

This is the **closure rule**: every name in the body that resolves to a slot in the surrounding scope must appear explicitly in the capture list. Implicit captures (body silently binds to a surrounding-scope name) are forbidden.

The closure rule has three observable benefits:

- **No accidental captures.** A typo or a refactor that introduces a same-named slot in a new scope cannot silently rebind a body's free name. The typechecker either finds the name in the parameter list, the capture list, or the top-level definitions; if not, the body is rejected.
- **Refactor safety.** Cutting and pasting a lambda from one context to another carries the capture list with it. The typechecker checks that the captured names exist with compatible modes in the new context, surfacing exactly the work required.
- **Algorithmic ceiling computation.** The typechecker computes the lambda's ceiling from the capture list directly, without scanning the body for free names. Error messages can point at the specific capture entry that's pinning the ceiling.

### 4.3 Capture Modes

The capture list contains entries of two kinds:

- **IN capture** — bare name (e.g., `x`). The lambda's hidden field holds a copy of the captured value at construction time. The body sees a local copy at each invocation; writes to the local are not propagated.
- **Reference capture** — `&`-prefixed name (e.g., `&y`). The lambda's hidden field holds a (flattened) reference to the captured slot. The body operates on the slot via per-invocation copy-restore (per §5 below).

IN captures are the lightweight case: the capture is a value, no slot reference, no ceiling implication. Reference captures carry a ceiling and constrain the lambda's lifecycle (per §5).

### 4.4 Why Productive Captures Are Forbidden

The productive marker `'` is not permitted in capture position. A productive capture would mean "the lambda commits to writing the captured slot on every successful invocation." This conflicts with capture semantics in two ways:

- **The contract presupposes uninitialized slots.** Productive parameters are designed for the case where the callee fills an empty slot. A captured slot is bound for the lifetime of the lambda; the first invocation might fill an uninitialized slot, but the second invocation walks into a slot that's already filled. The "must write on success" obligation is satisfied trivially by every invocation after the first.

- **The multi-invocation case admits ambiguity.** Productive parameters were designed precisely to outlaw "sometimes writes, sometimes leaves the prior value" semantics. Captures pull that ambiguity back in: a multi-invocation lambda with a productive capture has no clear answer to "what value does the captured slot hold after the lambda is done."

Reference `&` captures don't have these problems. The reference contract is explicitly "may read, may write, may neither," which is designed for the multi-invocation case. The caller-side initialization requirement (the slot must be initialized at every invocation) is satisfied trivially as long as the slot was initialized at capture time and remains so throughout the lambda's lifetime.

This restriction has a pleasing symmetry with checkpoint 007 §3.4's restriction on partial-application captures: both exclude productive mode for structurally compelling reasons, even though the reasons are different. Captures of writeable positions are allowed in this checkpoint; productive captures are not.

### 4.5 Capture Sources

The captured names in the slash list resolve against the constructor's lexical scope. They may bind to:

- Local variables (introduced via `#` in the constructor's frame).
- The constructor's own parameters and receivers.
- Names from enclosing scopes that are themselves accessible to the constructor.

A captured name that the constructor cannot itself read is a compile-time error. A captured `&y` where `y` is not an existing slot in the constructor's scope is a compile-time error. The slash list is held to the same lexical-resolution rules as ordinary references in the constructor's body.

---

## 5. Ceiling Tracking for Reference Captures

### 5.1 The Reference-Chain Flattening Rule

A reference capture `&y` records a (flattened) pointer to the slot's origin, not to the proximate slot in the constructor's frame. "Origin" is computed by following reference chains:

- If `y` is a local variable in the constructor's frame, the origin is the constructor's frame and the slot is `y`.
- If `y` is itself a reference parameter `&y` of the constructor (or a chain of such), the origin is the slot the constructor was passed at its own call site — recursively flattening through the chain to the bottom.
- If `y` is an IN parameter of the constructor, capturing `&y` is forbidden — there's no slot to reference, only a value-copy local to the constructor's frame, which would be lost when the constructor returned.

The flattening is structural and computed at compile time. The typechecker can determine each captured slot's origin from the constructor's signature and the chain of reference parameters it traverses; no runtime work is needed.

The motivation for flattening is the helper-builds-stateful-lambda pattern:

```
.cmd helper : :<>' lam, &Counter accum =
    'lam <- :<>{ / &accum}{ Counter::increment: &accum}

.cmd outer =
    # state : Counter = (Counter: 0)
    # incrementer
    helper: 'incrementer, &state
    nestedCmd: incrementer
```

Inside `helper`, the lambda captures `&accum`. Without flattening, the captured slot would be `accum`-the-parameter-slot in `helper`'s frame, which ceases to exist when `helper` returns. The lambda would dangle, and the productive return of the lambda via `'lam` would be unsound. With flattening, the captured slot is `state` in `outer`'s frame, the lambda's ceiling is `outer`, the productive return is sound, and the subsequent invocation through `nestedCmd` operates on `outer`'s `state`.

The runtime representation of the captured slot is implementation-defined per checkpoint 001 §2.6 (handles vs. pointers vs. fat pointers). The user-visible semantics commit only to the flattening rule; the implementation chooses a representation that delivers it.

### 5.2 The Ceiling Computation

A lambda's ceiling is computed as:

```
ceiling(lambda) = min over c in capture-list of:
    if c is an IN capture:           ⊤  (no contribution)
    if c is a reference capture &y:  origin-frame(y)  (per §5.1)
```

The minimum is taken over stack frames in the natural ordering (a frame above another frame on the stack is "smaller" in the ceiling sense — smaller frames give tighter ceilings). A lambda with no reference captures has `⊤` as its ceiling, which reduces to the introducing-frame ceiling from checkpoint 007 §3.3 (because the lambda is itself an object and inherits the ordinary object lifecycle).

Once computed, the ceiling governs the lambda's lifecycle:

- The lambda may be passed downward freely (to commands called from any frame at or below the ceiling).
- The lambda may be assigned to slots whose own ceilings are at or below its ceiling.
- The lambda may migrate upward via writeable-parameter copy-restore, up to but not above its ceiling.
- Assignment to a slot whose ceiling is above the lambda's ceiling is rejected at compile time.

This is structurally the fexpr ceiling rule from checkpoint 007 §4.2, applied to lambdas with reference captures. The mechanism that enforces the rule may share machinery with the fexpr ceiling enforcement; this is an implementation matter. The two ceiling concepts are conceptually unified — both fall out of the principle that a value bound to stack-resident state has a ceiling at the frame where that state lives.

### 5.3 Two Lambda Phenotypes

The relaxation produces a lambda mechanism with two phenotypes, both sharing the same surface syntax:

- **Lightweight lambda** — only IN captures (or no captures). Ceiling = introducing frame (the ordinary object ceiling). Can migrate freely upward via copy-restore. No new typechecker machinery beyond ordinary object lifecycle.
- **Ceiling-tracked lambda** — at least one reference capture. Ceiling = min of `&`-captured slots' origin frames. Migration constrained accordingly. Bears the same kind of typechecker cost as fexprs.

The phenotypes are syntactically indistinguishable at construction; the typechecker detects the difference from the capture list. The user's code either compiles or doesn't, and error messages explain which capture is constraining the ceiling.

### 5.4 Per-Invocation Copy-Restore for Reference Captures

Each invocation of a lambda with one or more reference captures performs standard copy-restore against the captured slot, separately for each reference capture:

- **At invocation entry**, the captured slot is read into a per-invocation local (via the flattened reference).
- **The body operates on the local**.
- **On successful return**, the local is written back to the captured slot.
- **On failed return**, no write-back occurs; the captured slot retains its pre-invocation contents.

This is per-invocation, not per-construction or per-lambda-lifetime. Each invocation's mutations are independent; failures during one invocation don't bleed into subsequent invocations.

The choice of copy-restore (rather than direct slot access) preserves the language's failure-atomicity guarantee: a failed invocation leaves the captured slot bit-identical to its pre-invocation state. This is consistent with how all writeable parameters work in the language and avoids creating an exception specifically for lambda captures.

The contrast with fexprs is meaningful: fexprs use direct slot access (the body reads and writes the captured slot directly during invocation), so failures mid-invocation can leave the slot in a partial state. This is sometimes desired (a recovery construct that shows the user the state at the moment of failure can't get that state if copy-restore has rolled it back), and is part of why fexprs remain a distinct mechanism.

### 5.5 Aliasing Prohibition

A captured slot must not also appear as an explicit invoke-method argument in the same invocation. If a lambda captures `&y` and is invoked with `y` (or any slot aliasing the same origin) as an explicit argument, the typechecker rejects the call site.

This is the standard aliasing rule applied to captures. The reasoning: copy-restore semantics depend on the writeable arguments being to disjoint slots; if two arguments name the same slot, the order of write-backs is unspecified and the semantics become ambiguous. Forbidding aliasing keeps the rule simple.

---

## 6. The Bare Form Is Largely Unchanged

### 6.1 Captures Are Positional, Not Nominal

The bare brace-quote form (dispatch capture, partial application) does not need a slash list because its captures are explicit by virtue of being written rather than `_`-elided. A `{cmd: a, _, &y, _}` partial application captures `a` and `y` (in modes inherited from `cmd`'s signature) and leaves the second and fourth positions unbound for invocation-time supply.

The capture modes are inherited from the underlying command's parameter modes. If `cmd`'s third parameter is reference-mode, then writing `&y` at that position captures `y` in reference mode — but that's because the underlying parameter requires reference mode, not because the user has a free choice at the partial application site. A `{cmd: a, _, y, _}` (without `&`) where the third parameter is reference-mode is a compile-time error, because the underlying signature requires reference and the user wrote IN.

Symmetrically, an attempt to capture `&y` for a position whose underlying parameter is IN-mode is a compile-time error, because the partial application is supplying an IN argument and an IN argument is just a value, not a reference.

### 6.2 Reference Captures in Partial Application Carry the Ceiling

The §3.4 rule from checkpoint 007 — that productive `'` and reference `&` positions cannot be partially applied — is **lifted for reference positions** by this checkpoint's machinery. Reference partial application is now permitted, with the same ceiling-tracking rule from §5: a partial application that captures a `&y` argument has its ceiling pinned to the origin frame of `y`.

Productive partial application remains forbidden, for the same reasoning as §4.4: productive contracts presuppose uninitialized slots and don't compose with multi-invocation lambda semantics. The bare form's productive prohibition is structurally identical to the lambda-with-body's productive-capture prohibition.

So the unified rule across both forms is: **reference captures are permitted with ceiling tracking; productive captures are forbidden**.

### 6.3 What the Bare Form Doesn't Need

Because the bare form's captures are positional (already explicit in the syntax) and modes are inherited from the underlying signature (no user choice), the bare form needs neither a slash list nor capture-mode markers as syntactic decorations. The syntax it has is sufficient. The typechecker's ceiling computation for a bare-form value with reference captures uses the same mechanism as for the lambda-with-body form — just with the capture list extracted from the partial-application argument positions rather than from a slash list.

---

## 7. Worked Examples

### 7.1 IN Capture

```
.cmd makeGreeter : :<String> 'g, String name =
    'g <- :<String greeting / name>{
        printLine: greeting, name
    }

.cmd useIt =
    # gr
    makeGreeter: 'gr, "world"
    gr :: invoke: "Hello"           ; prints: Hello world
    gr :: invoke: "Goodbye"         ; prints: Goodbye world
```

The lambda's slash list contains `name`, an IN capture. The captured value (`"world"`) is copied into the lambda's hidden field at construction. The lambda has no reference captures, so its ceiling is the introducing frame (which is `makeGreeter`'s frame). The lambda is returned to `useIt` via the productive `'g` parameter; on success, the lambda's storage migrates to `useIt`'s frame and its ceiling moves with it (per checkpoint 002 §3.3 object-pointer mechanics).

The body uses `greeting` (an invoke-method parameter, IN mode by default) and `name` (an IN capture). Each invocation gets a fresh local copy of `name`; writes within an invocation would be local-only and not propagate to subsequent invocations (though no writes happen in this example).

### 7.2 Reference Capture, Ceiling Tracking

```
.cmd makeIncrementer : :<>' inc, &Int32 counter =
    'inc <- :<> / &counter>{
        'counter <- (add: counter, 1)
    }

.cmd useIt =
    # state : Int32 = (Int32: 0)
    # f
    makeIncrementer: 'f, &state
    f :: invoke                     ; state is now 1
    f :: invoke                     ; state is now 2
```

The lambda's slash list contains `&counter`, a reference capture. The reference-chain flattening rule (§5.1) traces `counter` through `makeIncrementer`'s reference parameter to `state` in `useIt`'s frame. The lambda's ceiling is therefore `useIt`'s frame, not `makeIncrementer`'s frame.

The productive return through `'inc` migrates the lambda's storage to `useIt`'s frame, which is at-or-below the ceiling (it *is* the ceiling). The migration is permitted.

Each invocation performs copy-restore against `state`: read the value, run the body (which computes `state + 1`), write the new value back on success. A failure mid-invocation (e.g., if `add` were `?add`) would leave `state` unchanged.

### 7.3 Reference Capture, Ceiling Violation

```
.cmd makeBadLambda : ^Object 'out =
    # localCounter : Int32 = (Int32: 0)
    'out <- :<> / &localCounter>{           ; ERROR
        'localCounter <- (add: localCounter, 1)
    }
```

The lambda's slash list contains `&localCounter`, which is a local in `makeBadLambda`'s frame. The lambda's ceiling is therefore `makeBadLambda`'s frame. Returning the lambda via the productive `'out` parameter (a `^Object`, with no constraint on the ceiling of the destination) would migrate the lambda above its ceiling — to whatever frame called `makeBadLambda` — which would dangle the captured reference.

The typechecker rejects this at the assignment to `'out`. The error message points at `&localCounter` in the slash list and explains: this lambda's ceiling is `makeBadLambda`'s frame; the productive return would migrate it above its ceiling.

The fix is one of:
- Capture the counter by value (`localCounter` instead of `&localCounter`), making the lambda lightweight and freely returnable.
- Receive the counter as a reference parameter rather than constructing it locally, so the origin frame is the caller's.

### 7.4 Reducer-Style with Writeable Invoke-Method Parameter

```
.cmd reduce : []T items, U init, U 'accum, :<U &state, T item> reducer =
    'accum <- init
    .for: item :| items
        reducer :: invoke: &accum, item

.cmd sum : []Int32 xs, Int32 'total =
    reduce: xs, (Int32: 0), 'total, :<Int32 &state, Int32 item>{
        'state <- (add: state, item)
    }
```

The reducer lambda uses **invoke-method parameters** (per §3) for both its mutable state input (`&state`) and its item input (`item`). It has no captures, so it has no ceiling implications and is freely passable. The state is owned by `reduce`'s caller (and is `'total`-bound through `sum`'s productive parameter all the way to `sum`'s caller).

This pattern is the consumer-style abstraction the previous turn's discussion was reaching for: the lambda's signature describes what it consumes (state and an item) without baking in any particular state. The same lambda can be used by `reduce` (which threads the state through itself) or by any other operation that supplies `&state` arguments. The lambda is ceiling-free because nothing is captured; only invoke-method parameters carry the writeable references.

### 7.5 Stateful Callback with Reference Capture

```
.cmd registerHandler : Subsystem &subsys, :<Event> handler =
    subsys :: addHandler: handler

.cmd setupCounting =
    # subsys : Subsystem = (initialize)
    # eventCount : Int32 = (Int32: 0)
    registerHandler: &subsys, :<Event e / &eventCount>{
        'eventCount <- (add: eventCount, 1)
    }
    runUntilDone: &subsys                       ; events arrive, counter ticks
    log: eventCount                             ; displays final count
```

The handler lambda captures `&eventCount` in reference mode. Its ceiling is `setupCounting`'s frame (where `eventCount` lives). The lambda is passed downward to `registerHandler`, which passes it further into `subsys` — both are at or below the ceiling, so the passing is permitted.

The subsequent invocation pattern (events arriving and triggering the handler from `runUntilDone`'s call frame, which is below `setupCounting`'s) is invocation-from-below-the-ceiling, which is always permitted.

This pattern is exactly the "consumer" abstraction the dialog motivated: a lambda that mediates between a generic operation (`Subsystem` doesn't know about counting) and the state being managed (`eventCount` lives where `setupCounting` does). The slash list makes the state-binding explicit, the ceiling tracking makes the lifecycle sound, and the per-invocation copy-restore preserves failure-atomicity per event.

---

## 8. Implementation Notes

### IN-5: Capture-List Resolution Is Compile-Time

The slash list's contents are resolved at compile time against the constructor's lexical scope. Each entry is bound to a slot (for reference captures) or to a value-source (for IN captures). The typechecker emits, for each lambda construction site:

- For IN captures: a copy operation from the source slot to the lambda's hidden field at construction time.
- For reference captures: a flattened pointer/handle (per checkpoint 001 §2.6) to the captured slot's origin, stored in the lambda's hidden field.

No runtime resolution work is needed; the lambda's hidden fields are populated entirely by compile-time-determined values and references.

### IN-6: Ceiling Tracking Reuses Object-Lifecycle Machinery

A ceiling-tracked lambda's ceiling is recorded as part of its hidden representation (per checkpoint 007 §3.5). The mechanism that enforces the ceiling at assignment sites is the same one that enforces fexpr ceilings (per checkpoint 007 §4.2) and object ceilings (per checkpoint 002 §3.1). All three ceiling concepts are the same concept, applied to different value kinds.

Implementations may share machinery across the three. The conceptual unification — "value bound to stack-resident state has a ceiling at the frame where that state lives" — is the load-bearing rule; the implementation's bookkeeping is a matter of representation choice.

### IN-7: Per-Invocation Copy-Restore Cost

A lambda with many reference captures pays a per-invocation copy-restore cost proportional to the number of captures (each is read at entry, written back at success). This cost is the same as the cost of an ordinary command call with the same number of writeable parameters; the lambda mechanism doesn't impose new overhead beyond what equivalent direct command calls would incur.

The optimizer may elide copy-restore for reference captures whose body doesn't write them, reducing such captures effectively to read-only captures for that invocation. Whether the language commits to this optimization or leaves it to the implementation is a question for the eventual specification; for now, the language commits to the copy-restore semantics and lets the implementation optimize where soundness permits.

### IN-8: Lightweight Lambdas Have No Ceiling Cost

A lambda with only IN captures (or no captures) has no `&`-capture entries in its capture list. Its ceiling is the introducing frame, which is the ordinary object ceiling. The typechecker's ceiling-tracking pass produces no constraints beyond the object-lifecycle ones for such lambdas.

This preserves checkpoint 007 §3.3's lightweight case exactly. The relaxation in this checkpoint is purely additive: lightweight lambdas pay nothing for the new mechanism.

---

## 9. Grammar Enhancements (Consolidated Rollup)

This section consolidates grammar changes implied by checkpoints 001–008. It serves as a running specification of the work needed in the parser and lexer to support the language's intent. Items marked **(this checkpoint)** are introduced in checkpoint 008; others reference their introducing checkpoint.

### 9.1 Identifier-Shape Markers (Checkpoints 004, 005)

- **`'name`** as identifier shape: productive parameter, productive receiver, productive captures-prohibited (lambdas), productive in slash list (lambdas) — **forbidden**. Lexer must accept `'` as part of identifier formation in identifier-bearing positions. *(Checkpoint 005 §1.3 begins this; further confirmation in this checkpoint at §4.4.)*
- **`&name`** as identifier shape: reference parameter, reference receiver, reference capture, prefix-on-name in slash list (lambdas). Lexer must accept `&` as part of identifier formation in identifier-bearing positions. The existing postfix-`&` (`CALL_EXPR_ADDR`, address-of) keeps its current role; disambiguation is by context. *(Checkpoint 005 §1.3.)*

### 9.2 Parameter Mode Markers in Command Definitions (Checkpoint 004)

- **`DEF_CMD_PARM`** must accept productive `'` and reference `&` markers on parameter names. Approximate shape: `all(DEF_CMD_PARM_TYPE, optionally(productive-or-reference-marker), DEF_CMD_PARM_NAME)`, where the marker is part of the name's identifier shape. *(Checkpoint 004 §4.4.)*
- **`DEF_CMD_RECEIVER`** similarly, with the constraint that constructor receivers must be productive `'` and at-stack receivers must be reference `&` (per checkpoint 005 §2.3 and §3 ).

### 9.3 Mode Markers in Command-Type-Expressions (Checkpoint 004; Refined This Checkpoint)

- **`TYPE_CMDEXPR_ARG`** must distinguish productive from reference mode. Current `TYPE_ARG_WRITEABLE` (the apostrophe suffix on type) collapses both. The split: `TYPE_ARG_PRODUCTIVE` (apostrophe-suffix-on-type, e.g. `Int32'`) and `TYPE_ARG_REFERENCE` (per the deferred decision in checkpoint 005 §1.4 — placement TBD, candidates include suffix `Int32&` and prefix `&Int32`). **(this checkpoint flags this as needing resolution; see OQ-19.)**
- For the lambda-with-body form's invoke-method parameter list, where names ARE present (unlike command-type-expressions), markers are identifier-shape on names: `:<Int32 'r, String &x>{...}`. **(this checkpoint, §3.)**

### 9.4 Lambda Slash List (This Checkpoint)

- Lambda-with-body forms `:<...>{...}`, `?<...>{...}`, `!<...>{...}` accept an optional slash delimiter inside the angle brackets, separating invoke-method parameters from captures. The capture list is a comma-separated list of names (with optional `&` markers on names). Syntactic shapes:
  - `:<args>{body}` — invoke-method parameters only, no captures.
  - `:<args / captures>{body}` — both.
  - `:< / captures>{body}` — captures only (thunk with state).
  - `:<>{body}` — pure thunk.
- Capture-list entries are name-only (no types, since captures resolve against the constructor's lexical scope where types are already known).
- Productive `'` markers in capture-list entries are forbidden (per §4.4 of this checkpoint).

### 9.5 Bare Brace-Quote Form (Checkpoint 007; Refined This Checkpoint)

- The bare form `{...}` accepts partial-application syntax with `_` for unbound positions. Captured argument positions may now carry `&` markers when the underlying command's parameter mode is reference (per §6.1 of this checkpoint). The grammar already accepts `&y`-as-identifier in argument position; the typechecker enforces mode compatibility. No new parser productions are needed.

### 9.6 Open Grammar Questions

- **The same-scope rule for `&x` and `x`** (checkpoint 005 §1.2, OQ-14): whether identifiers `&x` and `x` may coexist in the same scope, paralleling the productive case. Open.
- **Marker placement in command-type-expressions** (this checkpoint, OQ-19): with `&` deferred from checkpoint 005, the placement (prefix-on-type vs. suffix-on-type) needs resolution.
- **Slash-list internal grammar details** (this checkpoint, OQ-20): whether the slash list permits a trailing comma, whether captures may be reordered relative to declaration scope, etc. — minor surface details that should be settled when the production is implemented.

---

## 10. Future Intents

This section captures design directions the language *might* eventually take, in the form of desiderata to keep open rather than priorities to pursue. Items here are not commitments; they are notes that the current design should not foreclose these possibilities without deliberate consideration.

### FI-1: Region-Style Cleanup As A First-Class Feature

The language's existing mechanisms are richly compatible with region-style memory management:

- **Frame-as-region.** Every command body is, in effect, a region: objects introduced in the frame have at-stack handlers that fire deterministically at frame exit, and the frame's ceiling prevents objects from outliving the frame.
- **Ceiling-as-region-boundary.** The ceiling rule from checkpoint 002 §3.1 is exactly a region's "no escape" rule: state allocated in a region can be reachable from anywhere within the region but not outside it.
- **At-stack-as-region-cleanup.** The at-stack methods provide region-scoped finalization without an explicit "destroy region" operation.
- **Ceiling-tracked lambdas and fexprs.** These are values that carry region-affinity (their ceilings are regions); the mechanisms that enforce their lifecycles are mechanisms for region-affine values.

What's *not* yet present in the language design is a first-class **named region** construct — something like `region R { ... }` that:

- Establishes a logical region distinct from the enclosing frame.
- Allows allocations explicitly placed in `R` (`# x : T @R = ...`) to outlive the immediate frame but be reclaimed at `R`'s exit.
- Permits passing region-affine values across nominal frame boundaries as long as the region is still active.
- Composes with the existing ceiling machinery so that a region's exit triggers ceiling-respecting cleanup.

This would make region-based programming a *first-class* idiom rather than an emergent property of the frame system. It would also enable patterns the current design makes awkward — a parser that produces an AST in a single region, freeing the entire AST in one operation; a request handler that allocates per-request data in a request-bound region; etc.

The intent here is **not to design this now**, but to maintain awareness that:

1. The current design is structurally compatible with eventually adding such a mechanism.
2. Decisions made in checkpoints 008-onward should be evaluated for whether they accidentally close off this future direction.
3. Specific points of compatibility (the ceiling concept, at-stack methods, the buffer-backed/object-backed split, the no-non-local-state principle) are precisely the pieces that would form the foundation of a region mechanism.

A future checkpoint may revisit this and either commit to a region design or affirmatively decide against one. For now, the design path is preserved.

---

## 11. Open Questions Updated

| OQ | Status | Notes |
| --- | --- | --- |
| OQ-1 | Open | Union discriminator representation. |
| OQ-2 | Open | Implementation latitude for IN parameter passing. |
| OQ-3 | Mostly resolved (per checkpoint 004) | |
| OQ-4 | Resolved (per checkpoint 004) | |
| OQ-5 | Open | Single-class instance coherence. |
| OQ-6 | **Substantially resolved by this checkpoint** | Partial application now permitted on reference `&` positions with ceiling tracking; productive `'` positions remain forbidden. The remaining sub-question — surface syntax for partial application of v-command receiver tuples — is still open but narrower. |
| OQ-7 | Refined (per checkpoint 007) | Fexpr design proper. |
| OQ-8 | Mostly resolved (per checkpoint 002) | |
| OQ-9 | Resolved (per checkpoint 006) | |
| OQ-10 | Open, load-bearing | Composite initializers (`=` vs. `<-`). |
| OQ-11 | Resolved (per checkpoint 005) | |
| OQ-12 | Resolved (per checkpoint 005) | |
| OQ-13 | Open | Implicit context parameters and initialization. |
| OQ-14 | Open | Same-scope rule for `&x` and `x`. |
| OQ-15 | Open | Full design of the downcast intrinsic. |
| OQ-16 | Open | Overloading restriction on dynamically-dispatched commands. |
| OQ-17 | Open | Compound literal syntax and literal-as-input rules. |
| OQ-18 | Open, refined this checkpoint | Lambda visible-signature representation. The signature now must surface invoke-method-parameter modes (`'`, `&`, IN) but should hide capture-list details (those are an implementation matter of the lambda, not part of its visible signature). The user-facing intent is: callers see the invoke-method shape including mode markers; they do not see what's captured. |
| OQ-19 | **New** | Placement of the reference marker `&` in command-type-expressions (`TYPE_CMDEXPR_ARG`). Checkpoint 005 §1.4 deferred this; this checkpoint's introduction of reference-mode invoke-method parameters in the lambda-with-body form makes the deferral pressing. Candidates: suffix `Int32&` (parallel to `Int32'`), prefix `&Int32` (parallel to identifier-shape `&x`). The first preserves the type-side suffix convention; the second creates visual parallel between command-type-expressions and named parameter declarations. |
| OQ-20 | **New** | Slash-list internal grammar details: trailing comma permitted, capture ordering rules, interaction with empty-parameter-list angle brackets, and other minor surface details to settle when the production is implemented. |
| OQ-21 | **New** | Capture-list interaction with implicit context parameters. A lambda's body may reference names that, in an ordinary command, would resolve as implicit context parameters. The capture list's closure rule says all body free names must appear in the parameter list or capture list, but it's unclear whether captures can cover this case (an implicit-context-style capture) or whether the closure rule needs an exemption. The natural reading is that captures cover it (a name in the slash list, captured by name, is the explicit form of what context parameters do implicitly), but this should be confirmed. |

The OQs marked "New" in checkpoint 008 are summarized: OQ-19 (marker placement), OQ-20 (slash-list grammar details), OQ-21 (capture vs. implicit context).

---

## 12. Summary of Changes from Checkpoint 007

| Topic | Checkpoint 007 | This Checkpoint |
| --- | --- | --- |
| Lambda invoke-method parameter modes | Implicit (bare form OK; lambda-with-body form had no syntactic provision) | Explicit and uniform: any mode marker on either form. |
| Captures in lambda-with-body | Implicit (body free names silently bound) | Explicit (slash list; closure rule rejects free names) |
| Capture modes | IN only | IN and reference; productive forbidden |
| Lambda ceiling | Always introducing-frame | Min of `&`-captured slots' origin frames; `⊤` for IN-only or no captures |
| Reference-chain handling | (No reference captures, so n/a) | Flattened to origin frame at compile time |
| Per-invocation semantics for writeable captures | (n/a) | Copy-restore against captured slot |
| Aliasing rule for captured slots | (n/a) | Captured slot must not also appear as explicit invoke-method argument |
| Lambda phenotypes | One (lightweight) | Two (lightweight, ceiling-tracked) |

---

## 13. Provenance

**Authored:** Distilled from continued intent dialog between JimDesu and Claude (Opus 4.7) on 2026-04-28.

**Source materials read for this checkpoint:** intent-checkpoint-001.md through intent-checkpoint-007.md.

**Grammar changes implied:** §9 of this checkpoint enumerates them. The lambda-with-body slash list is the load-bearing new production; the rest are extensions or refinements of grammar changes already noted in earlier checkpoints.

**Recommended next step:** Commit alongside the prior checkpoints (suggested path: `docs/intent-checkpoint-008.md`). The natural next threads remain OQ-10 (composite initializers) for the construction story, the refined OQ-7 for the fexpr design, and the failure-mode-and-typechecker integration thread that has been pending since checkpoint 002. The open questions introduced in this checkpoint (OQ-19, OQ-20, OQ-21) are surface-syntax and edge-case matters that can be settled either inline with implementation work or in a small dedicated checkpoint.

The Future Intents section (§10) introduces FI-1 (region-style cleanup compatibility) as a non-priority desideratum to be revisited in some future checkpoint when other priorities have settled.
