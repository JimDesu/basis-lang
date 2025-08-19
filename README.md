# The Basis Programming Language.  

### Background
This is the programming language that I want to write code in, which doesn't actually exist yet.  It's inspired by aspects of Zig, Julia, Scala, Icon, and Kernel.  I was originally looking at a functional programming language with lower level primitives, but my brief experiences with functional languages has convinced me that they're great for hard-core computing, but lousy for real world software (I might lose friends over this).  The $64K question is: how do we get the benefits that come from a functionally pure language, without immolating ourselves with category-theoretic ways of declaring imperatives and reacting to computational statuses that have nothing to do with pure functions?  The fundamental problem here is that the basis of getting things done is mutation, and that's the realm of the procedural (with or without object-orientation or other abstractons).  So, in this spirit I've torn up my prior designs and have embarked on a pure procedural programming language that eschews the complexities that inevitably result from the collision of pure functional programming and real-world state.  The result is, IMO, a language that is conceptually simple, concise, and, most especially, easy to understand at first read.

This's a part-time side project done at the pace of a parent's spare time, so it will be a while before the code here matches the intent.  I have a pretty solid understanding of what I want to achieve here, but I'm writing the "doc" first to ensure I don't leave a gap I'll have to code my way out of later.  As Leslie Lamport pointed out, "writing is nature's way of telling you how lousy your thinking is".  

By the time I'm done this intro, anyone interested will be able to ask pointed questions and will know if awaiting the implementation's worthwhile.  &#9786;

#### Core Semantics
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
#### Hello World
Once things are in place, hello world will look like this.  Note that all text following semicolons are ignored as comments
```
writeLn: "Hello, world!" ; writes "Hello, World!\n" to standard output
```

As a general rule, things get done via command invocation.  The basic syntax follows the following pattern:
```mermaid
flowchart LR
  id0(( ))
  id1([command name])
  id2([:])
  id3([argument])
  id4([,])
  id5([argument])
  id6(( ))
  id0 --> id1
  id1 --> id2
  id1 --> id6
  id2 --> id3
  id3 --> id6
  id3 --> id4
  id4 --> id5
  id5 --> id4
  id5 --> id6
```

The full syntax is a little more complicated, as we'll see soon.
### Failure Semantics

Commands in Basis do not have return types like functions in other programming languages do (although there's nearly equivalent syntactic sugaring for this).  Commands do have an explicit result status however, which is partitioned into Success and Failure.  By default, a command's result state is Success, but a command may invoke the `.fail` directive to signal a failure.  Once a failure is signalled, normal commands are skipped until recovery code is reached.

#### Recovery
Recovery comes in two flavors: general and constrained.  General recovery code follows a `|` marker.  A command block covered by this marker is only invoked if one of the preceeding commands has a failure state; if the preceeding commands succeed, the failure code is skipped over.

The `!` marker generates a failure based either on the contents of a variable or else on a command used as an expression.  This is useful when we want to process different failures in different ways (for example, to potentially retry the operation)
```
...
! MyFailureTypeInstance         ; the failure is marked with an instance of MyFailureType
nextCommand: ...                ; this gets skipped because there's a failure
| OtherFailureType f ] ...      ; this gets skipped because the failure has the wrong type
| MyFailureType f ] ...         ; this gets executed because there's a failure with the correct type
| ...                           ; this would be executed if the preceding, above, wasn't here, since it handles all types,
                                ; but in this case it's skipped over because the preceeding code clears the failure status
```
 
##### Command Predication
When commands are defined using the `.cmd` directive (see below), commands that can fail are marked with a `?` preceeding their name.  This `?` character is not part of the command name, but it signals the developer's intent both the other developers and to the compiler

```
.cmd NoFailCommand: ...             ; doesn't fail
.cmd ?MaybeFailComand: ...          ; can fail
.cmd !AlwaysFailCommand: ...        ; cannot succeed
```

If a command that is not marked with a `?` calls a command that is marked with '?' or '!', it MUST recover from the failure.  Failure to do so is a compile-time error.  

### Two Dimensional Layout

Like Python and Haskell, in Basis whitespace is syntactically significant.

#### General Layout Rule

The general rule for understanding whitespace layout is that for syntactic concerns that close over multiple syntacic elements, indentation determines the boundaries over which elements in the text will be considered for inclusion.  
```
.directive        ; some grouping syntax
  stuff           ; indented, so included

  things          ; indented, so included irrespective of preceeding blank line
  more stuff      ; indented, so included
other stuff       ; not indented, so not considered for inclusion in the directive
```
The syntaxes for different directives (special forms) differs somewhat: in syntax diagrams indentation will be denoted by "&rdsh;".  

#### Failure Processing
There are two layout rules for failure processing.  In normal code, the "|" marker only recovers from failures resulting from commands that were more indented.  Until a failure is handled, command execution never drills in to more indented code.
```
   ... doing something
   .fail
   ... doing something else     ; this line is skipped and doesn't run
      | ...                     ; this line is part of the line above it, which is skipped, so this is skipped too
   | ...                        ; this recovery executes
  | ...                         ; if and only if the prior recovery fails, then this runs
```

The other exception is syntactic sugar around variable assignment (see the operators section below).  Variables are assigned via the "<-" operator, with the variable on the left hand side, and the expression within parentheses on the right hand side.  In this case, if the first command executed fails, a fallback can be designated
```
.var v <- (divide: 7, 0 | 1)    ; if the first command fails, then v will be assigned the value 1

; multiple commands may be grouped in this way, which are attempted in turn.
.var m <- (add: 1, 2 | multiply: 2, 3 | 0)

; note that there's no need for a fixed value at the end
.var q <- (add: 1,2 | subtract: 7, 4)

; compile-time constants are a kind of command that never fails, so while the following is correct,
; the second command will never be attempted
.var a <- (7 | add: 1,2)
```

#### Parameterization Layout
Parameters for command invocation follow the same general layout rule, with the extra aspect that comma separators between command arguments are optional where indentation across lines is used.
```
command1: arg1, arg2, ..., argn    ; commas needed because everything's on one line
command2: arg1                     ; no commas needed for this command
  arg2
  arg3
command3: arg1,                    ; comma unnecessary but OK
  arg2,                            ; ditto
  arg3,                            ; last comma is a syntax error
command4: ...
```

This becomes especially useful where a block of commands is passed.  Blocks are designated using a "~" marker, and can be passed as parameters without special syntax.
```
command: arg1                      ; no need for comma
  ~ subcommand1: ...               ; first part of arg2
    subcommand2: ...               ; second part of arg2.  A trailing comma here would be a syntax error
  arg3                             ; arg3
```
### Control Flow
Control flow in Basis is determined by some other markers.
| Marker | Usage |
|--------|-------|
|   ?    | Attempts the next command, but if the command fails, execution proceeds normally |
|   ?:   | Attempts the next command.  If the command fails, execution proceeds normally.  If the command succeeds, this exits the indentation level. |
|   -    | If the previous item at the same indentation level is "?" and that command fails, then the command(s) indented from here are executed.|
|   ~    | Groups a serious of commands into a logical block. |
|   ^    | Rewinds execution to the previous governing item at the same level. |
|   |    | Recover (potentially conditionally) from a failure |
|   !    | Generate a failure signal |

Basic conditional:
```
  ...
  ? ( a < 5 )       ; fails if a >= 5
    ...             ; only runs if the value of a is less than 5
  ...               ; runs irrespective of a's value
```
Block conditional 
```
...
? ~ doSomething:...
    doSomethingElse:...   ; only runs if doSomething succeeds
  doALastThing:...        ; only runs if both the preceesing commands succeed
doOneMoreThing:...        ; runs if doSomething and doSomethingElse succeed or fail
                          ; but not if doALastThing fails
```
If-Else style conditional:
```
  ...
  ? (x > 5 )     ; fails if x <= 5
    ....         ; runs if x > 5
  - ...          ; runs if x <= 5
    ...          ; same
  ...            ; runs irrespective of x's value
```

Selection conditional:
```
  ...
  ?: (x > 5)    ; fails if x <= 5
     a <- 17    ; runs only if x > 5
  ...           ; SKIPPED
...             ; execution resumes here
```

Simple looping:
```
  ...
  doSomething: ...
  ^ ; forever
```

Conditional looping.  So long as the optional block closed over by the "^" marker succeeds, the the first command block is repeated
```
   ~ doSomething: ...
     andThis: ...
   ^ doSomethingElse: ...
     andSomethingElse: ...
   ...
```

If-Else looping.  Note that execution rewinds back up to the "?" marker, not the "-" marker.
```
? doSomething: ...
- orSomethingElse: ...
^ ; one or the other forever
...  ; doesn't run unless there's a failure
```

Another example.
```
? doSomething: ...
  ^ ; forever
- ~ orSomethingElse: ...
    andThisToo: ...
  ^ ; both forever
... ; never runs unless there's a failure above
```

Retry looping
```
doSomething: ...
| RetryableFailure f] ^         ; Note the indent level for ^ is taken from the start of the line it's on.  Usually
                                ; this is the same thing as the position of the ^, but not necessarily so.
| write: "something went wrong"

```
### Data Types
##### Compile time constants
Explicit constants in the code may be one of the following types.
* Numeric -- 1 or more digits optionally followed by a decimal point and one or more trailing digits
* Hexadecimal -- binary data in pairs of 0-F characters (upper or lower case) following a "0x" prefix
* String -- character data delimited by " characters, all on the same line

Compile-time string constants are not zero-terminated like in the language C.  Note also that Basis has no character data type (at all -- see below).  Like in the language Icon, Basis has no boolean data type, and hence no predefined boolean constants. 

Note that compile-time constants are not technically data types: they're inline ad-hoc commands that write a value to an output parameter.  Thus, both of the following are syntactically correct.  
```
.var a <- ("data")
"data": .var a
.var b <- (27.3)
27.3: .var b
```

#### Fundamental Data Types
Basis has the following fundamental data types:
* Buffers
* Pointers
* Objects
* Commands

##### Buffers

##### Pointers

##### Objects

##### Commands

### Directives

### Expressions


