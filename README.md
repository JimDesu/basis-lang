# The Basis Programming Language.  

This's one of several projects being done at the pace of a parent's spare time, so it will be a while before the code here matches the intent here.  I have a pretty solid understanding of what I want to achieve, and I've written this doc so far mostly as a means of clarifying corners of my intent -- as Leslie Lamport pointed out, "writing is nature's way of telling you how lousy your thinking is" -- so please expect changes. 

## Introduction
There is no greater technical obscurity than creating a new programming language, and this is my contribution to that vast heap of better mousetraps.  It's inspired by aspects of Zig, Julia, Scala, Icon, Haskell, and Kernel.  

### Practical Upshot:
The core differentiating reasons for going through the effort of making this language are:
- Direct semantic match to Hexagonal Architecture... the langauge makes Hexagonal Architecture the natural way to code
- Bounded scope of side effects will simplify review of AI-generated code
- Stylistic support for high and low level coding tasks

### Guiding Principles:
- Strong typing saves lives &#9786;
- No non-local state access
- The fundamental datatype is a buffer
- Mutation either succeeds fully or fails fully
- No hidden control flow
- Polymorphism and statecharts aren't just for object types 
- Computational status is orthogonal to result state
- Prefer small orthogonal concepts to rich overlapping ones
- Special forms should be visually distinct from user-defined forms
- Syntactic sugar is superior to semantic sugar
- Fexprs and macros, not fexprs vs macros
- Syntactic whitespace improves legibility 

### Core Semantics
Given program state as a tuple $\langle V,\Phi,\Sigma \rangle$ where
* V is the current verb to be executed:
    * $\overrightarrow{v}$ represents the continuation from $v$
    * $exec(c)$ executes a user defined command
    * $fail(\phi)$ sets a failure state
    * $recover$ recovers from a failure status
    * $recover(\phi,\sigma,c)$ recovers a particular failure state type, binding the failure to $\sigma$ in $exec(c)$
    * $scope(c)$ executes at the current scope boundary
    * $scopefail(c)$ executes c at the current scope boundary under a failure
    * $rewind(v)$ continues execution at a previous verb in the current scope
* $\Phi$ represents the current failure status:
    * $\epsilon$ represents no current failure
    * $\phi$ represents a particular failure
* $\Sigma$ represents the current variable state
    * $\sigma/c$ is sigma bound within the scope of c  

General excution is described by the following rules:

$$
\begin{align}
\text{normal execution}\quad & \langle v, \epsilon, \Sigma \rangle \Downarrow \langle v', \epsilon, \Sigma' \rangle & \implies & \langle \vec{v}, \epsilon, \Sigma' \rangle \\
\text{generating failure}\quad & v = fail(\phi) \quad \langle v, \epsilon, \Sigma \rangle & \implies & \langle \vec{v}, \phi, \Sigma \rangle \\
\text{command failure}\quad & \langle v, \epsilon, \Sigma \rangle \Downarrow \langle v', \phi, \Sigma' \rangle & \implies & \langle \vec{v}, \phi, \Sigma \rangle \\
\text{skips from failure}\quad & v \in \{exec(c),rewind(w),fail(\gamma)\} \quad \langle v, \phi, \Sigma \rangle & \implies & \langle \vec{v}, \phi, \Sigma \rangle \\
\text{generic recovery}\quad & v=recover \quad \langle v, \phi, \Sigma \rangle & \implies & \langle \vec{v}, \epsilon, \Sigma \rangle \\
\text{specific recovery}\quad & v=recover(\phi,\sigma,c) \quad \langle v, \phi, \Sigma \rangle & \implies & \langle c, \epsilon, \Sigma+\sigma/c \rangle \quad \vec{c} \leftarrow \vec{v} \\
\text{recovery failure}\quad & v=recover(\alpha,\sigma,c) \quad \langle v, \phi, \Sigma \rangle \quad \phi \neq \sigma & \implies & \langle \vec{v}, \phi, \Sigma \rangle \\
\text{scope boundary}\quad & v=scope(c) \quad \kappa\in\Phi \quad \langle v, \kappa, \Sigma \rangle & \implies & \langle c,\epsilon,\Sigma \rangle \quad \vec{c} \leftarrow \langle \vec{v},\kappa,\Sigma' \rangle \\
\text{scope boundary under failure}\quad & v=scopefail(c) \quad \langle v, \epsilon, \Sigma \rangle & \implies & \langle \vec{v},\epsilon,\Sigma \rangle \\
\text{} & v=scopefail(c) \quad \langle v, \phi, \Sigma \rangle & \implies & \langle c,\epsilon,\Sigma \rangle \quad \vec{c} \leftarrow \langle \vec{v},\phi,\Sigma' \rangle \\
\text{looping}\quad & v=rewind(c) \quad \langle v, \epsilon, \Sigma \rangle & \implies & \langle c,\epsilon,\Sigma \rangle
\end{align}
$$

## Overview
 
The rest of this document walks the language from the outside in. We begin with a few small examples to ground the visual texture (§1), then describe the shape of a source file (§2), then commands (§3) — the unit of execution. Failure and recovery (§4) come early because the language's failure semantics are everywhere in normal code. The type system (§5) and the parameter-mode discipline (§6) follow. Construction (§7) covers how values come into being. The four forms of first-class command-typed values — command reference, command literal, lambda, and fexpr — are §8. Classes, instances, and dispatch are §9. 
 
Some of what follows is settled and parsing today; some is design that the compiler does not yet enforce. I have one or two things in mind that I haven't included yet, so please expect further changes.
 
## 1. At a Glance
 
Hello, world:
 
```
.program writeLn: "Hello, world!"
```
 
A regular command, with a parameter and a productive output:
 
```
.cmd double: Int 'result, Int x = 'result <- x + x
```
 
The signature reads "command `double` takes a productive `Int` named `result` and an `Int` named `x`; the body assigns `x + x` into the productive slot." Productive parameters — the slots a command writes to on success — are marked by a `'` prefix on the parameter name; the lexer treats `'name` and `name` as distinct identifier shapes, so the body writes to `'result`, not `result`. There is no `return` statement: production is by writing to a named slot.
 
A command that may fail, and a caller that handles the failure:
 
```
.cmd ?safeDivide: Int 'result, Int n, Int d =
    ? d = 0
        .fail DivByZero: n
    'result <- n / d
 
.cmd useIt =
    #q <- safeDivide: 10, 0
    | DivByZero e ->
        writeLn: "couldn't divide"
```
 
The `?` prefix on `safeDivide`'s name in `.cmd ?safeDivide` declares "may fail." The body uses `?` as a block marker — "when `d = 0`," do the indented block, which fires `.fail DivByZero: n` to construct a payload-bearing failure with the numerator carried in the payload. Because `safeDivide` has exactly one writeable parameter — productive `'result` — that slot is automatically the value when the command is invoked as an expression, which is what makes `#q <- safeDivide: 10, 0` a legal use site: the call's `'result` slot is supplied by `#q`. If the call fails, the typed-recovery block `| DivByZero e -> ...` catches and recovers, binding the constructed payload to `e`; the bound name `e` is in scope only inside the recovery body.
 
Three points distinguish this from mainstream-language exception handling. First, failure messages in Basis are not exceptions — there is no stack unwinding (§4 covers propagation in detail). Second, the pattern `| DivByZero e ->` is a typed match that selects `DivByZero` and any of its subtypes within the message hierarchy, so a recovery block written for a parent message catches all of its descendants. Third, `e` binds only to the message's payload (the `Int` numerator here), not to the message as a whole — the identity `DivByZero` is matched by the pattern, never bound.
 
A small class with one declared method and one default-implementation method, plus an instance for some type:
 
```
.class Renderable:
    .decl render: String 'output
    .cmd describe: String 'output = render: 'output
 
.instance Widget: Renderable
```
 
`.decl` is signature-only (instances must supply); `.cmd` inside a class body is a default that instances may override. The line `.instance Widget: Renderable` says "the type `Widget` satisfies `Renderable`" — bodies for `Renderable`'s declared methods are supplied either elsewhere (via methods on `Widget` itself, with matching shapes) or by delegation, with delegation named explicitly via `(delegate fieldName)` if needed.
 
Whitespace is significant. Indentation determines what belongs to what, in the spirit of Python or Haskell. A line indented under another line is part of the construct begun by that line.
 
## 2. Source Files
 
A source file is a sequence of three optional sections:
 
- Optionally, a single `.module` declaration naming the file's module.
- Zero or more `.import` declarations.
- Zero or more top-level definitions: `.alias`, `.class`, `.cmd`, `.decl`, `.intrinsic`, `.domain`, `.enum`, `.instance`, `.object`, `.program`, `.record`, `.test`, `.union`, `.variant`.
```
.module App::Models
.import Std:Core
.alias StringList: List[String]
.domain UserId: Int
.enum UserRole: admin = 0, user = 1, guest = 2
.record User: UserId id, String name, String email, UserRole role
.object UserManager: List[User] users
.instance User: Serializable, Comparable
```
 
Module names use `::` to qualify namespaces (`Std::Utils`, `App::Models`). Imports use `Alias:Module` form when binding the imported module to a local alias, or `.import "filename"` for file imports. Type names begin with an uppercase letter; identifier names begin with lowercase. The lexer enforces this: `.instance widget: Interface` is a syntax error (lowercase type name).
 
`.program` defines the program's entry expression:
 
```
.program  runSimulation: (getCommandLine)
```
 
The right of `=` is an expression that runs at program start; here, `runSimulation` is invoked with the result of `getCommandLine` as its argument.
 
`.test` defines a named test:
 
```
.test "round-trip serialization" = roundTripCheck
```
 
Tests and programs are the only frames in which top-level effects compose freely; everywhere else, the no-non-local-state principle (Introduction) governs.
 
## 3. Commands
 
A command is the unit of execution. Commands take parameters, may produce output through writeable parameters, may fail, and compose. Commands are first-class values — they can be referenced, partially applied, stored, and invoked indirectly.
 
### 3.1 Signatures
 
The most common shape:
 
```
.cmd name: Type1 'resultName, Type2 param2, ..., TypeN paramN / Type ctxParam = body
```
 
After `.cmd`, the failure mark prefix is optional:
 
| Prefix | Meaning |
|---|---|
| (none) | Never fails. The body must reach its exit on a non-failing path. |
| `?` | May fail. The body's exits may be in any state. |
| `!` | Must fail. The body must reach its exit on a propagating-failure path. |
 
A failing-mark example:
 
```
.cmd ?parse: ParsedValue 'result, String s = ...
.cmd !explode: String reason = .fail Explosion: reason
```
 
Parameters declare `Type name`. The `'` prefix on a parameter name marks the parameter as **productive** — the body must write to it on every successful path:
 
```
.cmd compute: Int x, Int 'result = 'result <- x * 2
```
 
Without `'`, the parameter is read-only (READ-mode). Reference semantics — a parameter the body may both read and write — are expressed today by passing a pointer (`^Type`) and using the postfix `^` and `&` operators in the body to dereference and take addresses; the parameter-mode design space includes a dedicated reference mode that the references describe.
 
A command's *expression-style result* — the value when the command is invoked inline as an expression like `#q <- compute: 7` — is determined by the writeable parameters in its signature. A writeable parameter is either productive (`'name`, write-once on success) or reference (`&name`, read-and-write with copy-restore semantics):
 
- **Exactly one writeable parameter:** that single slot is automatically the expression-style result. No further annotation is needed.
- **More than one writeable parameter:** an explicit `-> name` clause is required, naming which already-declared writeable parameter is the expression-style result. The `->` does *not* introduce a new parameter — it only designates which existing writeable parameter participates in the expression-style sugar.
- **Zero writeable parameters:** a read parameter may still be designated as the expression-style result via an explicit `-> name` clause. Without such a designation, the command is not expression-callable.
For example, a command with two productive outputs needs the `->` clause to make one of them the expression-style result:
 
```
.cmd quotrem: Int 'q, Int 'r, Int n, Int d -> q =
    'q <- n / d
    'r <- n % d
```
 
A caller may now write `#x <- quotrem: 'remainder, 10, 3` to bind the quotient to `x` while supplying `'remainder` as the slot for the remainder. The same rule applies to commands that mix productive and reference parameters: any combination of two or more writeable slots requires `-> name` to designate which is the expression-style result.
 
Implicit context parameters appear after a `/` separator, taking precedence-by-uniqueness-of-type from the caller's lexical scope:
 
```
.cmd writeAll: List[String] lines / Logger logger = ...
```
 
The caller of `writeAll` need not pass `logger` explicitly if their scope contains exactly one in-scope value of type `Logger`. The mechanism resolves at compile time; ambiguity (two `Logger` values in scope) and absence (none) are both call-site errors that the user resolves by passing the value explicitly.
 
### 3.2 Constructors and methods
 
A constructor's signature names a type as the productive receiver:
 
```
.cmd Widget 'w: Int x, Int y =
    w <- ${x <- x, y <- y}
```
 
The receiver `Widget 'w` is the constructed-value slot; all other parameters are inputs. The body's job is to fill `w`. The body uses an aggregate literal `${...}` to construct in place (§7).
 
A method takes one or more receivers, then `::`, then the command name and any further parameters. With a single receiver — the common case — the receiver appears bare; with two or more, the receivers are listed in parentheses:
 
```
.cmd Logger logger :: log: String message = ...
.cmd (Logger logger, Severity severity) :: format: String 'result, String message = ...
```
 
The receivers may carry mode markers (`'r`, no marker for READ). Calling uses the same `::` syntax, parenthesized only when there is more than one receiver:
 
```
myLogger :: log: "ready"
(myLogger, warning) :: format: "couldn't open file"
```
 
Methods dispatch on the runtime types of all receivers in concert. The implementation composes single-class dispatches per receiver — there is no joint-instance dictionary — so methods work cleanly across module boundaries (see §9.4).
 
### 3.3 Frame-exit hooks: `@` and `@!`
 
A class or module may declare methods that run at frame exit:
 
```
.class Resource:
    .decl Resource r: String name
    .decl @ Resource r           ; runs at frame exit (success or failure)
    .decl @! Resource r          ; runs at frame exit only on failure
```
 
`@` is read "at exit"; `@!` is "at exit on failure." These execute when the frame holding the value retires, in reverse order of registration (most-recently-introduced first), composing cleanly with the failure system.
 
This is an `RAII`-equivalent mechanism, but the hooks are *not* destructors: they are tied to **stack-frame lifetime**, not to object lifetime. An `@` handler fires only when a frame slot directly holding the value retires; a value that exists solely as a field within a record or object does *not* fire its `@` handler when that container goes away. The value must occupy a slot in the stack frame — not be buried inside another structure — for the handler to fire.
 
### 3.4 Calling commands
 
A regular call:
 
```
process: arg1, arg2
```
 
A constructor call:
 
```
Widget: x, y
```
 
A method call with a single receiver (the common case):
 
```
myLogger :: log: "ready"
```
 
A method call with multiple receivers — receivers parenthesized:
 
```
(receiver1, receiver2) :: methodName: arg1, arg2
```
 
Arguments may span lines under indentation; inter-argument commas are required:
 
```
process: arg1,
  arg2,
  arg3
```
 
The `_` token is a placeholder; like `::`, it serves multiple purposes. In a productive parameter position at a call site, `_` says "I don't care about this result" — for example, a caller of `quotrem` (§3.1) who needs only the quotient may write:
 
```
#x <- quotrem: _, 10, 3
```
 
The remainder is computed and discarded. The discard form of `_` is valid only in productive parameter positions. Its other uses are partial application (§9.5) and variant construction (§5.6, §7).
 
## 4. Failure and Recovery
 
Failures in Basis are neither exceptions nor error codes. A failure is a *message* — an identity together with an optional payload — carried in a slot in the active frame's record; it propagates by skipping subsequent commands until it reaches a structurally-marked recovery context. The static type system tracks which commands may fail, must fail, or never fail.
 
### 4.1 The `.fail` directive
 
`.fail` fires a failure:
 
```
.fail                              ; no message
.fail DivByZero                    ; message, no payload
.fail BoundError: (Widget: x, y)   ; message with constructed payload
```
 
`.fail` takes a message — either a message identifier alone (no payload), or a message identifier followed by `:` and a payload expression — or may be used alone with no message at all. The message identifier is always required when a payload is supplied: a raw value cannot be passed to `.fail` without a message identifier preceding it. When a payload expression's evaluation itself fails, *that* failure propagates rather than the intended one.
 
### 4.2 Failure marks on commands
 
Every command's signature declares its failure profile via the prefix on its name:
 
```
.cmd safeWrite: String text = ...                       ; never fails
.cmd ?openFile: Handle 'handle, String path = ...       ; may fail
.cmd !abort: String reason = .fail Abort: reason        ; must fail
```
 
The mark is enforced. A never-fails body whose internal `?`-call could fail without recovery is a static error. A must-fail body whose path could succeed-and-return is a static error. Subsumption applies on the mark: never-fails values may stand in for may-fail expectations; must-fail values may also stand in for may-fail expectations; never-fails and must-fail are mutually incomparable.
 
### 4.3 Block markers for control flow
 
Indentation establishes block scopes. The first character of a block-bearing line names the block's role:
 
| Marker | Read as | Role |
|---|---|---|
| `?` | "when" | The first statement is a *guard*. If it succeeds, subsequent statements run; if it fails, the construct's failure is consumed and execution proceeds after the construct. |
| `?-` | "unless" | Inverse: the block runs only if the first statement fails. |
| `?:` | "select" | First statement is a guard; if it succeeds, the rest of the block runs and execution exits the surrounding indentation level. Useful for chained alternatives. |
| `??` | "elevated when" | Meta-marker that wraps a `?` or `?-` as its first executable inner block; on inner-guard failure, execution resumes past the `??`-block's own siblings (one structural level of elevation) rather than just past the inner construct. Used with `^` for structured loops. |
| `-` | "else" | The else-companion of a preceding `?` or `?-`; runs if that block's guard failed. |
| `%` | "block" | Plain grouping; the body is a single logical unit but does not consume any failure of its own. |
| `^` | "rewind" | Sibling block that rewinds control to its *preceding sibling* at the same indentation level. Body is optional: a bodiless `^` rewinds unconditionally; a `^` with a body rewinds on body success and consumes any body failure (terminating the loop). Requires a preceding sibling — a bare `^` with none is a static error. |
| `\|` | "recover" | Catch-all recovery: runs only when an earlier sibling at the same indentation has produced a propagating failure. |
| `\| TypeName name ->` | "recover, when of type" | Typed recovery: runs only on failures whose message is `TypeName` (or a subtype within the message hierarchy); binds the payload to `name` for the body's duration. |
| `@` | "at exit" | At frame exit, fire the body. |
| `@!` | "at exit, on failure" | At frame exit, fire the body only on the failure path. |
 
The if-then-else idiom uses `?` and `-`:
 
```
? x > 5
    largeBranch: x
- smallBranch: x
```
 
A simple loop uses the canonical `?? ?` + `^` pattern:
 
```
?? ? hasMore: queue
        process: pop: queue
    ^
```
 
While `hasMore: queue` succeeds, the `?` body runs; control then reaches `^` (the sibling of `?` inside `??`'s body), which rewinds back to `?`. When `hasMore: queue` eventually fails, `??` elevates the failure past its own block — skipping `^` — and the loop terminates.
 
A loop framed by setup and teardown:
 
```
% acquire: lock
    ?? ? hasMore: queue
            process: pop: queue
        ^
    release: lock
```
 
`acquire: lock` runs once; the `?? ?` + `^` loop iterates inside `??`'s body; `release: lock` runs after the loop terminates.
 
This example contains a latent bug: if `process: pop: queue` can fail, the failure propagates out through `??` and out of `%`'s body, skipping `release: lock` and leaking the lock. The proper mechanism for cleanup that must run on every exit path is the `@` frame-exit hook (§3.3).
 
### 4.4 Typed recovery and class-bound payloads
 
A failure message may be **bound to a class** — meaning that the payload's runtime type is required to satisfy that class, and the recovery handler operates on the payload through the class's operations:
 
```
; declaration of a payload-bearing message (forward syntax)
; ... Net::Disconnected payloads must satisfy Diagnosable
 
.cmd ?fetchAll: Url u =
    ?- ping: u
        .fail Net::Disconnected: (lastError: u)
 
.cmd useIt =
    fetchAll: someUrl
    | Net::Disconnected e ->
        ; e satisfies Diagnosable; we can call its diagnostic ops on it
        renderDiagnostic: e
```
 
The class binding makes failures *contractual*: a recovery handler does not need to know the concrete type of the payload, only the operations the bound class promises. Different `.fail` sites for the same message may pass values of different concrete types, all satisfying the same class, and consumers continue to work without change. This is Haskell-style typeclass dispatch sliced through the failure machinery.
 
### 4.5 At-stack handlers and lifecycle
 
`@` and `@!` blocks register cleanup that runs at frame exit:
 
```
.cmd processFile: String path =
    #handle <- openFile: path
    @ closeHandle: handle           ; runs whether we succeed or fail
    @! logIncomplete: path          ; runs only on failure path
    process: handle
```
 
If `process: handle` fails, both `closeHandle` and `logIncomplete` fire as the frame retires (in reverse order of registration). On success, only `closeHandle` fires. The handlers cannot themselves create new in-flight failures during exit-cleanup processing; they may, however, invoke commands that fail internally and recover internally.
 
### 4.6 What the static analysis tracks
 
For every reachable point in a command body, the typechecker maintains a *failure-state lattice*: each path is in one of `clear`, `failing(?)`, `failing(!)`, or `mixed`. The body's exit-edge state must conform to the signature's mark:
 
- A MUST-PASS body's exits must all be `clear`.
- A MAY-FAIL body's exits may be any combination, with the propagating failures' messages constrained to the declared class of failure messages.
- A MUST-FAIL body's exits must all be `failing(!)`.
Block markers and recovery contexts manipulate the lattice precisely; the typechecker's job is to confirm the body's structure conforms.
 
 
## 5. Types
 
The type system has two layers. **Buffer-backed** types are pure-bytes values: their layout is fully described by their fields' bytes, and they may be embedded inline in records, in arrays, in unions. **Non-buffer** types — pointers, objects, command-typed values, variants — have their own lifecycle machinery and are referenced indirectly. The two-layer split is what gives Basis predictable layouts at low level (records stay packable, domains stay byte-faithful) while preserving rich object-and-variant semantics where they are useful.
 
### 5.1 Buffers and ranges
 
The fundamental type is the buffer — a fixed-size sequence of bytes. The grammar shape `[size]` gives a buffer of `size` bytes:
 
```
[16]                    ; sixteen bytes
```
 
A typed range is `[size] T` for some buffer-backed `T`:
 
```
[64] Byte               ; 64 bytes, statically typed as bytes
[16] Float              ; 16 floats laid out contiguously
```
 
An unsized range `[]T` describes a buffer whose size is determined at the call site:
 
```
.cmd run: []String args = ...
```
 
Indexing uses the postfix `[i]` syntax. Indexing is failable (out-of-bounds is a first-class failure). C-style pointer arithmetic does not exist; stepping through buffer contents requires indexing.
 
### 5.2 Domains
 
A domain is a refinement of an existing buffer-backed type, declared with `.domain`:
 
```
.domain UserId: Int
.domain Inches: Int
.domain Centimeters: Int
```
 
`UserId`, `Inches`, and `Centimeters` are nominally distinct from each other and from `Int`. A child domain's value implicitly upcasts to its parent (so an `Inches` may be supplied where `Int` is expected). For buffer-backed domains, this upcast is structural and lossy with respect to dispatch: a value passed through a parent buffer-backed parameter (e.g., `Inches` flowing through an `Int`-typed parameter) loses its child-domain identity at that boundary, since buffer-backed parameter slots carry only bytes — no witness, no runtime tag. Subsequent class dispatch on the value sees only the parent type. Domain-specific dispatch is preserved when the value flows directly into a class-typed slot (where its identity is captured in the slot's witness) or through a chain of class-typed slots; intermediate upcast through a buffer-backed parent-type parameter discards the dispatch identity. (See §9.7 for the full mechanism.)
 
Sibling domains do not implicitly convert. An explicit constructor invocation is required to move between siblings.
 
### 5.3 Records
 
A record is a buffer-backed product type with named fields:
 
```
.record Point: Int x, Int y
.record User: UserId id, String name, String email, UserRole role
```
 
Records are nominal — two records with structurally identical fields are distinct types. Record fields may be other buffer-backed types, including other records. Records may be parameterized:
 
```
.record Pair[T, U]: T first, U second
```
 
### 5.4 Objects
 
An object is a non-buffer type — a heap-residing structure with an identity that survives copying its slot. Objects are declared with `.object`:
 
```
.object UserManager: List[User] users
```
 
Object fields may be any type, buffer-backed or non-buffer. Objects participate in the class system as receivers; they have lifecycle methods (`@`, `@!`), and pointers to them carry runtime type information for safe downcasting through the `-<` operator (§7.5).
 
### 5.5 Unions
 
A union is a buffer-backed sum type — a single block of bytes that may be interpreted as any of several declared candidates. Unions carry no runtime tag; the user is responsible for tracking which candidate is active:
 
```
.union RGBA8: [4] AsBytes, Int32 AsPacked, Pixel AsRgba
```
 
A union value implicitly subsumes (by zero-cost byte reinterpretation) to any type that appears on at least one declared candidate's chain — so a `RGBA8` may be supplied where an `Int32` is expected. The reverse direction (reading a union slot at a different candidate's interpretation) requires the user to track which candidate is active. This is the C-style overlay semantics, kept structurally available for byte-aliasing efficiency.
 
### 5.6 Variants
 
A variant is a non-buffer sum type — a tagged union where the language tracks which candidate is active:
 
```
.variant Shape: Circle circle, Rectangle rectangle, Polygon polygon
.variant Tree[T]: T leaf, ^Tree[T] branch
```
 
Variants have an **absent state** by default — a variant slot may be empty, holding none of its candidates. The absent state is part of the variant's type, not a separate "Optional" wrapper. A variant with a single candidate is usefully an "optional" of that candidate.
 
A variant type provides a built-in constructor that accepts an aggregate literal designating the active candidate:
 
```
#shape <- Shape: ${Circle <- (Circle: 1.0)}        ; Circle state
#shape <- Shape: ${}                               ; absent state
```
 
The aggregate literal names which candidate is active and supplies its value; the type prefix `Shape:` provides the explicit type context the literal requires. The empty form `${}` constructs the variant in its absent state. (Aggregate-literal forms are detailed in §7.3.)
 
A variant may be examined and narrowed via the `-<` dynamic-narrowing operator (§7.5):
 
```
?- _ -< shape
    handleAbsent
?: Circle 'c -< shape
    handleCircle: c
?: Rectangle 'r -< shape
    handleRectangle: r
handleOtherShape
```
 
The `?-` engages first as the absent-state test; each `?:` then attempts a candidate narrowing in turn; the final unindented line runs as the default arm if none of the chain's guards engaged.
 
A variant may be reset to its absent state via `v -< _`. The form `_ -< v` tests whether `v` is non-absent.
 
A variant may be an instance of a class: every candidate must satisfy that class, and the class's methods may be dispatched on the variant's active candidate without explicit narrowing.
 
### 5.7 Pointers
 
Pointers are the indirection mechanism for non-buffer types. A pointer to `T` is `^T`:
 
```
.cmd traverse: ^Node n = ...
```
 
In expressions, postfix `^` dereferences a pointer (`p^` is the value the pointer points to); postfix `&` takes the address of a value (`x&` is a pointer to `x`). Pointer chains — pointers to pointers — are written `^^T`. There is no pointer arithmetic.
 
For object types, pointers carry runtime type information at the implementation level. The `-<` operator (§7.5) uses this to perform safe downcasting through a class hierarchy.
 
### 5.8 Command-typed values
 
Commands are first-class. A command-typed value is described by a command-type expression that names the failure mark and the parameter types in declaration order, with a postfix `'` on each writeable type:
 
```
:<Int, Int'>                ; never-fails command taking an Int and a writeable Int
?<String'>                  ; may-fail command taking a writeable String slot
!<>                         ; pure must-fail command, no parameters
```
 
The mark prefix (`:`, `?`, `!`) inside the angle brackets matches the failure-mark discipline on command names. Command-type expressions list types, not parameter names — the `'` writeable marker is postfix on the type itself, marking a slot that must be written by the command. Command-typed values may be stored in fields, passed as arguments, captured in lambdas, and bound from method dispatch — they are values like any other, subject to the language's standard mode and ceiling rules. The four constructional forms that produce them are §8.
 
### 5.9 Aliases
 
A `.alias` declaration names an existing type expression for convenience:
 
```
.alias StringList: List[String]
.alias Bytes: []Byte
```
 
Aliases erase: the alias name and its right-hand side are interchangeable in all contexts.
 
### 5.10 Enums
 
Enumerations are nominal sets of named values:
 
```
.enum Status: ok = 0, warning = 1, error = 2
.enum HttpStatus Method: get = 100, post = 200, ...
```
 
`.enum` accepts either one or two type names. With one name (e.g., `Status`), that name is the enumeration's type name. With two names (e.g., `HttpStatus Method`), the first is a *type constraint* — the type the literal values must represent — and the second is the enumeration's type name. An enumeration may represent any literal type; integer values are common but not required. If the constraint type is not itself a literal type, a constructor for the constraint type must be available that is callable on each enumerated value, so each literal can be converted to the constrained type.
 
Enum values are referenced via `EnumName[itemName]` in expressions.
 
Enumerations are compile-time constants — read-only values fixed at module compile time. As such, they are the language's one principled exception to the no-non-local-state principle. Whether the constructed values are built up-front at module load or on-demand at first reference is an implementation-dependent concern.
 
## 6. Parameters and Mode Markers
 
Every parameter has a *mode* describing what the body and caller can do with it. Today's grammar admits two mode shapes — READ (the default, read-only) and productive `'` (write-once on success). The full design includes a third — reference `&` (read-and-write with copy-restore semantics) — currently realized via pointer-typed parameters until first-class reference mode is implemented.
 
### 6.1 The productive marker `'`
 
A parameter name prefixed with `'` is productive: the body must write to it on every successful exit path, and the caller must supply an uninitialized slot to receive the write:
 
```
.cmd compute: Int x, Int 'result =
    'result <- x * x
```
 
A command with exactly one writeable parameter — productive (`'name`) or reference (`&name`) — is automatically expression-callable: the single writeable slot becomes the expression-style result, with no further annotation required:
 
```
.cmd square: Int x, Int 'result =
    'result <- x * x
 
; caller:
#nine <- square: 3
```
 
When a command has two or more writeable parameters, an explicit `-> name` clause is required to designate which one becomes the expression-style result; the named parameter must already be declared in the list with the appropriate writeable marker. The `->` does not introduce a new parameter.
 
A productive parameter that the body fails to write on some path is a static error. A productive parameter the body writes more than once on the same path is a static error. The discipline composes with the failure-state lattice: writes on a path that ends in a propagating failure are exempt from the obligation, since the productive write-once obligation is *write-once-on-success*.
 
### 6.2 The implicit-READ default
 
A parameter without `'` is read-only. The body may inspect it but cannot modify it. The caller need not provide an uninitialized slot; any value of the right type may be supplied.
 
### 6.3 The same-scope rule
 
A name introduced as a writeable parameter (`'r` or, in the future, `&r`) cannot be shadowed by another binding of the same base name (`r`) in the same scope. The rule prevents the user from accidentally referring to "the same name" while operating at different mode contracts. The lexer treats `'r`, `&r`, and `r` as identifier-shape variants of the same name.
 
### 6.4 The transitive READ contract
 
A read-only parameter is read-only *transitively*. The body may not pass it to any sub-call that would write to it through any reachable access path. This is what makes the READ contract trustworthy: a value passed in READ-mode cannot be smuggled into a writeable position somewhere downstream.
 
### 6.5 Implicit context parameters
 
Parameters listed after a `/` separator are *implicit context parameters* — Scala-style implicits. At the call site, they are filled automatically from the caller's lexical scope by uniqueness-of-type:
 
```
.cmd writeAll: List[String] lines / Logger logger = ...
 
.cmd reportAll: List[String] lines =
    #log <- (Logger: "console")
    writeAll: lines           ; logger=log filled by uniqueness-of-type
```
 
Ambiguity (two `Logger` values in scope) is a compile error; absence (no `Logger` value in scope) is also a compile error. Both are resolvable by passing the value explicitly with the full positional form.
 
## 7. Construction and Initialization
 
Construction is the act of bringing values into being. The `<-` polymorphic-RHS operator is the one mechanism for committing values into slots.
 
### 7.1 `<-` and `#`
 
Variable introduction uses the `#` prefix on the lvalue:
 
```
#count <- 0
#name <- "Hello"
#user <- (User: id, name, email, role)
```
 
`#count <- 0` declares a new local `count` and binds the value `0`. The `<-` operator's right-hand side may be:
 
- A literal (number, string, hex, binary).
- A bare identifier, evaluating to a value-copy.
- A constructor call: `Type: arg1, arg2`.
- A command call: `name: arg1, arg2`.
- An aggregate or sequence literal (§7.2, §7.3).
- An expression with operators.
- A choice expression: `lhs <- a | b | c` evaluates each alternative left-to-right, committing the first that succeeds.
The choice form gives concise fallback behavior:
 
```
#config <- readFile: "user.cfg" | readFile: "default.cfg" | (Config: emptyDefaults)
```
 
`#` is not restricted to the LHS of `<-`; it may also appear in argument position at a call site, introducing a fresh local that the called command may write into. See §6 for the parameter-mode discipline that governs such locals.
 
### 7.2 Sequence literals
 
A sequence literal builds a buffer-backed range:
 
```
#nums <- $[ 1, 2, 3, 4, 5 ]
#bytes <- $[ 0xAB, 0xCD, 0xEF ]
```
 
The fence is `$[ ... ]`. The element type is inferred from the contents (or from the lhs slot's declared type, if known). The element count is fixed at construction.
 
### 7.3 Aggregate literals
 
An aggregate literal builds a record, object, or variant value. The fence is `${...}`. The named form binds each field by name with `<-`:
 
```
#p <- ${x <- 3, y <- 4}
```
 
A positional form is admitted where the lhs type is contextually explicit — for example, at typed parameter positions or on the rhs of a typed productive slot. Values are listed in the lhs type's declaration order, without field names:
 
```
'origin <- ${3, 4}                                  ; positional; 'origin's declared type provides the field order
```
 
Where the lhs type is not contextually explicit, the positional form is rejected and the named form is required. In positional form, every field — including variant-typed fields — must be supplied; `_` stands in for variants in the absent state. The named form is always well-formed.
 
### 7.4 Atomic compound construction
 
Compound construction is *atomic*: a constructor whose body partially succeeds and then fails leaves the productive output uninitialized — never partially constructed. The compute-then-commit pattern means a constructor body computes all the work first, then commits via a single productive `<-` write near the end. This guarantees the slot's pre-call state survives any failure.
 
### 7.5 The `-<` dynamic-narrowing operator
 
`-<` is the runtime-checked sibling of `<-`. It narrows a wider value to a more-specific type at runtime, producing a propagating failure on type mismatch:
 
```
?: Circle 'c -< shape         ; if shape's active candidate is Circle, bind to c
    handleCircle: c
fallback                      ; runs only if the guard failed
```
 
Forms:
 
| Form | Meaning |
|---|---|
| `'narrow -< v` | Narrow `v` (variant or class-typed) to `narrow`'s declared type; failure on mismatch. |
| `v -< _` | Reset variant `v` to its absent state. |
| `_ -< v` | Test whether `v` is non-absent; succeeds iff some candidate is active. |
 
`-<` integrates with the failure system — its mismatches produce propagating failures that ordinary recovery handlers catch, rather than special-cased control-flow constructs.
 
### 7.6 `.implicit` constructors
 
A `.implicit` declaration registers a constructor that the typechecker can insert automatically when a literal of one type appears in a context expecting another. The form parallels `.cmd`'s constructor form:
 
```
.implicit Float32 'r: Decimal d = ...
```
 
Wherever a decimal literal like `3.14` appears at a slot expecting `Float32`, the typechecker rewrites the bare literal to invoke this constructor — the user writes `3.14` and the compiler reads `(Float32: 3.14)`. `.implicit` is purely additive: the explicit form `(Float32: 3.14)` continues to work regardless of whether the constructor is declared `.implicit` or `.cmd`.
 
### 7.7 Defaults via `=`
 
A `=` declaration binds a default value or default constructor for a slot:
 
```
.record Config:
    Int port = 8080,
    String host = "localhost",
    LogLevel level = LogLevel[info]
```
 
Defaults are evaluated lazily — at the moment the slot would otherwise be uninitialized.
 
## 8. Lambda and Fexpr
 
Four constructional forms produce command-typed values:
 
| Form | Surface | Captures? | Body? | Use |
|---|---|---|---|---|
| Command reference | `{name}` or `{cmd: x, _, y}` | No | No (refers to existing command) | Function-pointer-style dispatch capture; partial application |
| Command literal | `:<args>{body}` (also `?<...>`, `!<...>`) | No | Yes | Eagerly-evaluated thunks; pure callbacks |
| Lambda | `:<args / caps>{body}` | Yes (explicit slash list) | Yes | Closures over defining-frame state |
| Fexpr | `:{body}` (also `?{body}`, `!{body}`) | Yes (implicit by free name) | Yes | User-defined control-flow combinators |
 
### 8.1 Command reference
 
`{name}` is a value-typed reference to an existing command — a function pointer. Partial application binds some of the underlying command's parameters:
 
```
{add: 5, _}                  ; partial: 5 is bound, second arg deferred
{logger :: log}              ; method dispatch resolved; receiver baked in
{logger :: log: _, "warn"}   ; method-style with one deferred and one bound
```
 
The `_` token marks deferred parameters. Receivers, when present, are always applied at the partial-application site — the dispatch is resolved and captured immediately. This is a user-level optimization for calling methods in tight loops: resolving the method dispatch once at capture time and reusing the resulting reference avoids the per-call dispatch overhead that would otherwise apply to each invocation.
 
### 8.2 Command literal
 
A command literal has an explicit signature and body, no captures:
 
```
:<Int x, Int 'r>{ 'r <- x * 2 }
```
 
This is an eagerly-evaluated, pure callback — no defining-frame state is captured; the body's only inputs are its declared parameters. Command literals have no ceiling beyond the ordinary object lifecycle.
 
### 8.3 Lambda
 
A lambda is a command literal extended with an explicit capture list, separated from the parameter list by a slash:
 
```
:<Int x / Int counter>{ counter <- counter + x }
```
 
Captures are explicit: any defining-frame name the body uses must appear in the capture list. The body's free names are otherwise restricted to parameters and top-level names. Captures may be READ (by-copy) or, in the full design, reference (live, with per-invocation copy-restore). A lambda's *ceiling* — the highest frame to which the lambda value may travel — is computed from its captures: a lambda with only READ captures can travel anywhere; a lambda with reference captures cannot escape the frames where its captured slots live.
 
### 8.4 Fexpr
 
A fexpr captures defining-frame state implicitly, by free-name reference:
 
```
:{ counter <- counter + 1 }
```
 
The body references `counter` directly; the typechecker resolves the name against the defining frame's lexical scope and captures it implicitly. Fexprs are the user-defined-control-flow-combinator form: a fexpr body acts as if inlined at its invocation site, with access to the surrounding scope. The purpose of fexprs is to allow down-stack code — a callee receiving the fexpr as a parameter — to read and modify the contents of the originating stack frame, the mechanism by which user-defined control-flow constructs can access caller-frame state.
 
Fexpr-typed parameter slots are denoted `:<*>`, with `?<*>` and `!<*>` for the may-fail and must-fail marks. The `*` inside the angle brackets distinguishes fexpr-typed values from ordinary command-typed values (`:<>`, `?<>`, `!<>`) — a fexpr is not assignable to an ordinary command-typed slot, even when the failure mark matches.
 
A fexpr's ceiling is *uniformly its defining frame*. It cannot escape upward, cannot be stored in long-lived structures, cannot be passed to anything that could outlive its defining frame. The discipline is enforced through the language's parameter-mode and storage rules — a parameter typed as a fexpr can only be READ, fexpr-typed slots cannot appear in long-lived storage, and so on.
 
### 8.5 Failure marks across the four forms
 
All four forms participate in the standard failure-mark discipline:
 
- A command reference inherits its mark from the underlying command.
- Command literals, lambdas, and fexprs declare their mark via the prefix (`:`, `?`, `!`) on the angle-bracket or brace-quote.
- Mark subsumption (`:` and `!` may stand in for `?`) applies symmetrically across all four forms.
## 9. Classes, Instances, and Dispatch
 
A class is a single-parameter type contract — structurally a Haskell typeclass, a Java interface, or a Scala trait. Classes describe operations that types must provide; instances supply those operations for specific types. Dispatch is via Haskell-style dictionary passing.
 
### 9.1 Classes
 
A class declaration enumerates the methods that any instance must provide:
 
```
.class Showable:
    .decl render: String 'output
 
.class Mergeable:
    .decl merge: Mergeable 'result, Mergeable other
    .cmd describe: String 'output =
        render: 'output                  ; default body, uses Showable.render
```
 
`.decl` is signature-only: the instance must supply the body. `.cmd` inside a class body is a *default* implementation that any instance may override; an instance that does not override the default uses the class's body.
 
A class may carry additional type parameters beyond the implementing type. Type variables and class-bound constraints attach to the class's typename, not to individual method signatures:
 
```
.class Container[T:Itemable]:
    .decl insert: T item
```
 
Methods reference `T` directly; the constraint `T:Itemable` applies across the class.
 
### 9.2 Instances
 
An instance declaration says "this implementing type satisfies these classes":
 
```
.instance Widget: Showable
.instance User: Serializable, Comparable
.instance Map[K, V]: Container (hashImpl), Iterable
```
 
Multiple classes on the right of `:` are independent instances; there is no joint-instance dictionary. The optional `(name)` clause names a *delegate* — a field of the implementing type whose existing instance for the class supplies the methods. Delegation is always explicit; the language does not auto-pick a delegate by type-uniqueness.
 
When delegation is used, the *delegate itself* is the receiver in calls to the delegated class's methods — not the outer object that named it. This receiver-substitution is unusual; most languages' delegation patterns keep the wrapper as the conceptual receiver and treat the delegate as a private implementation provider. In Basis, calls dispatched through a delegate operate directly on the delegate's value. Among other uses, this lets a context object collect any number of services into a single context-variable binding, and supports rebinding of the delegate field during a call — useful for implementing statecharts.
 
For every `(class, type)` pair, an instance produces a *dictionary* — a record-like value whose fields hold command-typed values for each of the class's methods. Dispatch is an indirect call through the appropriate dictionary slot.
 
### 9.3 Dispatch via `::`
 
The expression `{receiver :: methodName}` resolves the method `methodName` on `receiver` and produces a command-typed value with the receiver baked in. The double-colon `::` makes dispatch syntactically visible — the reader sees `::` and knows that what runs depends on the receiver's runtime type.
 
The resolved value may be bound and reused — useful for hoisting dispatch out of a tight loop:
 
```
#renderFn <- {myWidget :: render}
?? ? hasMore: queue
        renderFn: 'localBuffer
    ^
```
 
Dispatch happens once at the binding; thereafter `renderFn` is invoked directly through its captured command-value.
 
### 9.4 Methods and multiple dispatch
 
A method invocation over multiple receivers takes a tuple of receivers that must be in parentheses, dispatching based on all their types in concert:
 
```
.cmd (Logger logger, Severity severity) :: format: String 'result, String message =
    ; body authored against Logger and Severity classes;
    ; (logger :: emit) and (severity :: prefix) dispatch independently
    ...
```
 
Calling:
 
```
(consoleLogger, warning) :: format: "couldn't open file"
```
 
The implementation composes per-receiver single-class dispatches — there is no tuple-keyed joint-instance table. The combined behavior is the product of the receivers' types, but each receiver's dispatch resolves through its own class's dictionary independently. Modules defining `Logger` and `Severity` need not coordinate; methods using both classes work cleanly across module boundaries.
 
Method receivers carry mode markers:
 
| Receiver mode | Surface | Idiomatic use |
|---|---|---|
| Productive `'` | `'r` | "Re-initialize the receiver" — factory-like operations |
| Reference `&` (designed; current via `^`) | `&r` | In-place modification |
| READ (no marker) | `r` | Externalized-effect operations (logging, sending, emitting) — the receiver mediates an effect on the world |
 
Constructors take productive receivers only: `.cmd Widget 'w: Int x, Int y = ...`. At-stack methods (`@`, `@!`) take READ or reference receivers, not productive.
 
### 9.5 Partial application
 
Partial application generalizes the receiver-baked-in form to bind any subset of the underlying command's parameters:
 
```
{add: 5, _, _}                    ; first arg bound, second and third deferred
{logger :: log: _, "warn", _}     ; receiver applied; second arg "warn" bound; first and third deferred
```
 
Receivers are always applied at the partial-application site — never deferred. This keeps dispatch resolved at compile time. Non-receiver parameters may be applied or deferred (`_`) freely. The resulting value's type covers only the deferred parameters in declaration order.
 
### 9.6 Class-typed parameters: Cases A and B
 
A parameter typed with a class has two structurally distinct forms:
 
**Case A — class as constraint on a type variable.** A type variable is introduced, constrained to a class, and parameters are typed as the variable:
 
```
.cmd doIt: (T:Showable) 'r, T x = ...
```
 
Multiple `T`-typed slots in the signature share a single concrete type at the call site. The dictionary witness travels once as a hidden parameter; slots are in their natural representation.
 
**Case B — class as existential at parameter position.** A parameter is typed *as a class*, accepting any value satisfying the class:
 
```
.cmd render: String 'output, Showable s = ...
```
 
Each slot independently carries its dictionary witness in the slot itself (a 3-word slot pattern). Different `Showable`-typed parameters in the same signature may carry values of different concrete types.
 
### 9.7 Instance coherence
 
Multiple instances for the same `(class, type)` pair may exist across modules. Resolution at a use site follows Julia-style "more specialized module wins" pragmatics: the most-specialized in-scope instance wins; genuine ties are static errors. Intra-module duplicates (two `instance C: T` declarations in the same module) are always errors. Orphan instances — instances declared in a module that defined neither the class nor the type — are permitted; the language warns at import time when a new import competes with an already-visible instance for an already-used `(class, type)` pair, restoring local predictability without restricting what may be declared.
 
Class dispatch resolves through a witness captured at the value's first class-typed-slot boundary. For non-buffer types — objects, variants — runtime identity is carried by the value's representation directly (object headers, variant 3-word slots), and dispatch through any class-typed parameter consults that runtime identity. For buffer-backed values, dispatch identity is captured when the value enters a class-typed slot and is preserved through subsequent class-typed slots — but is **lost** if the value passes through an intermediate *non-class-typed* parent buffer-backed parameter, which carries only bytes. The next class-typed slot the value enters then captures the parent's instance, not the child's. To preserve domain-specific dispatch on a buffer-backed value, the dispatch must reach the class-typed slot directly, or through a chain of class-typed slots — without intermediate upcast through a parent buffer-backed parameter.



