# Citu Compiler — Phase 3 (Lexer, Parser, Semantic Analyzer)

Citu is a Roman Urdu-inspired, statically typed language with structured control flow and object-oriented features. This repository contains the Phase 3 implementation of the front end, written entirely in C++ with rule and condition based code (no libraries, no generated code).

The grammar implemented here is the one specified in `Citu_Phase1_BSCS23122.docx` (91 productions), extended with one additional statement so programs can show output:

```
print EX
```

## Files

- `lexer.h` — token definition and the scanner (keywords, identifiers, integer and character literals, operators, punctuation, `//` comments, line/column tracking). Rejects `==`, `++`, `--`, malformed numeric tokens such as `12abc`, and invalid character escapes.
- `lexer.cpp` — standalone lexical analyzer. Prints the full token table.
- `parser.h` — AST definition, the complete LL(1) grammar (93 numbered productions), the predictive lookup table, the table-driven stack parser, and parse tree to AST conversion.
- `parser.cpp` — standalone syntax analyzer. Prints the AST on success.
- `semantic_analyzer.cpp` — full Phase 3 semantic analysis: class hierarchy construction (inheritance, field layout, virtual slots, override checks), scoped symbol table, type checking with internal exact-integer semantics, context checks, definite assignment analysis and return coverage.
- `program1.citu`, `program2.citu`, `program3.citu` — the three example programs (loops and conditions, classes and objects, inheritance and polymorphism).

## How to build

Each tool compiles standalone:

```cmd
g++ lexer.cpp -o lexer.exe
g++ parser.cpp -o parser.exe
g++ semantic_analyzer.cpp -o semantic_analyzer.exe
```

## How to run

```cmd
lexer.exe program1.citu
parser.exe program2.citu
semantic_analyzer.exe program3.citu
```

Each tool stops at the first lexical or syntax error. The semantic analyzer prints the symbol table, the class table (flattened fields with offsets and vtables with virtual slots) and all diagnostics, then reports whether analysis passed.

## What the semantic analyzer checks

- Class names unique; parent must exist; inheritance cycles rejected; parents processed before children.
- Flattened field slots (inherited offsets unchanged); duplicate members and field hiding rejected; overrides must match the inherited signature and reuse the virtual slot.
- Duplicate locals, duplicate parameters, duplicate declarations in the same scope; shadowing allowed.
- Type checking with internal exact integer values: arithmetic requires integers, ordering accepts two integers or two chars, equality accepts integers, chars, bools or compatible references; constant range checks for `int16`, `int32`, `uint16`, `uint32`; no implicit bool-to-int or int-to-char conversion.
- Method calls: static receiver type resolves the member; argument count and types checked; upcasts allowed, downcasts rejected.
- `break` / `continue` only inside loops; `return` only inside methods; `this` only inside methods and never assignable.
- Definite assignment: reads of possibly unassigned locals rejected; branch joins intersect facts; loops conservatively assume zero iterations; parameters start assigned; field writes do not initialize locals.
- Return coverage: every method body must return on every reachable normal exit.

## Language quick reference

| Construct | Citu notation |
|---|---|
| While loop | `jabtak (EX) shuru ... khtm` |
| Count-controlled loop | `for (AS ; EX ; AS) shuru ... khtm` |
| Conditional chain | `agr (EX) ... wrna (EX) ... wrna_phir ...` |
| Assignment | `medalo` |
| Equality (comparison) | `=` (single equals) |
| Blocks | `shuru` / `khtm` |
| Class / inheritance | `class Name extends Parent shuru ... khtm` |
| Object type / allocation | `object Name ref` / `ref medalo new Name()` |
| Method / receiver / result | `method TYPE ID(...)`, `this`, `return` |
| Output | `print EX` |
