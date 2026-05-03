Here is a project context document you can paste into the project instructions box:

---

**Project: Basis Programming Language**
**GitHub:** https://github.com/JimDesu/basis-lang
**Language:** C++ (CLion on Windows 11)
**Developer:** Working at a parent's pace; project is long-term and ongoing.

---

**What Basis Is**

Basis is a new general-purpose programming language whose design naturally facilitates hexagonal architecture — i.e., the language's semantics make clean separation of concerns and no non-local state the path of least resistance, rather than an imposed discipline. It can be thought of as an imperative analog of pure functional programming.

Core design principles (from README):
- Strong typing
- No non-local state access
- The fundamental datatype is a buffer
- Mutation either succeeds fully or fails fully
- No hidden control flow
- Failure semantics are first-class (not exceptions, not error codes) — failures propagate and skip commands until explicitly recovered
- Syntactically significant whitespace (like Python/Haskell)
- Command-based execution model (not function/statement dichotomy)
- Formal operational semantics defined as state tuple ⟨V, Φ, Σ⟩
- Target: native compilation; bootstrapping alternatives acceptable in the interim

Inspirations: Zig, Julia, Scala, Icon, Haskell, Kernel.

---

**Current Implementation State**

The compiler frontend is in progress in C++:
- **Lexer** — implemented
- **Grammar/Parser** — Grammar2.cpp/h, Parsing2.cpp/h — implemented and accepting the grammar correctly
- **AST** — Ast.h, AstBuilder.cpp/h — in progress
- **Supporting:** Token, ParseObject, CompilerContext, CompileOptions, compiler.cpp/h
- **Tests:** basis_tests/ directory, test_grammar2.cpp is the active test file

The grammar is documented in `grammar2.md` in the repo (1490 lines, ~39KB). Claude has read this file and is familiar with the production structure.

The type system intent exists but has not yet been formally specified.

---

**Current Unit of Work**

The parser accepts the grammar correctly, but the parse tree representation has been identified as inefficient in several ways. The developer is currently manually auditing `test_grammar2.cpp` to assess and address these issues before proceeding to improve AST construction.

**Identified parse tree issues (in priority order):**

1. **`CALL_EXPRESSION` misclassification** — `CALL_EXPRESSION` requires at least one operator per the grammar, but the parse tree shows bare command invocations wrapped in `CALL_EXPRESSION` rather than `CALL_INVOKE`. This may be a semantic misclassification, not just an efficiency issue.

2. **Deep type expression chains** — Simple named types produce 3 levels of wrapper nodes before reaching the type name, e.g. `DEF_RECORD_FIELD_DOMAIN(TYPE_EXPR_DOMAIN(TYPE_NAME_Q(TYPENAME)))`. These should collapse significantly at the AST stage.

3. **Universal `IDENTIFIER(IDENTIFIER)` double-wrapping** — Every identifier is a parse node wrapping a same-named token. Zero semantic value; should collapse to a single node with the token's string value during AST construction.

4. **Deep parameter call chains** — Single identifiers passed as parameters produce 5 levels of nesting: `CALL_PARM_EXPR(SUBCALL_EXPRESSION(CALL_EXPRESSION(CALL_COMMAND(CALL_CMD_TARGET(IDENTIFIER(IDENTIFIER))))))`. Partly a consequence of issue #1.

5. **`DEF_CMD_PARM_NAME` missing child** — Appears without a child node in the tree, unlike analogous field name nodes which show their `IDENTIFIER` children. May be a capture omission.

6. **Some nesting is due to optionality in the grammar** — Some redundant wrapping is inherent to how optional productions are represented; some is a flaw. Developer is auditing to distinguish these.

---

**Claude's Role**

Primarily design and architecture discussions, grammar/AST advice, code review of pushed content. Claude can fetch files directly from the public GitHub repo. Local unpushed changes must be pasted or uploaded manually.

Claude Code (not yet installed) will be used for hands-on implementation once available. Installation path: native Windows PowerShell (`npm install -g @anthropic-ai/claude-code`), requires Node.js 18+ and Git for Windows.

---

Paste that into the project instructions box and you'll have solid context for every future session. Would you like to adjust or add anything before you do?