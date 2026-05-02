# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## General instructions

**ALWAYS** put sentences on their own line when writing Markdown.

**ALWAYS** try to limit the amount of characters per line to 80.

## What this is

Blu is a compiled programming language (AST interpreter) written in C++17. The current goal is correctness over performance — the interpreter evaluates the AST directly rather than compiling to native code.

## Build

```bash
python build.py        # regenerates build.ninja and runs ninja
```

Outputs land in `out/`:
- `blu` / `blu.exe` — main compiler binary
- `tokenviewer` / `tokenviewer.exe` — token-stream debugger (can be ignored)

Dependencies: `clang++`, `ninja`, Python 3.

## Running

```bash
out/blu <file.blu>          # run a .blu file
```

Test files live in `test/basic/`. There is no automated test runner; run each file manually.

## Compiler pipeline

`main.cc` wires together the full pipeline in order:

1. **Tokenize** (`tokenize.cc`) — source text → token stream (`Tokens`)
2. **Parse** (`parse.cc`) — token stream → AST (`AstNodes`), stored as parallel arrays of kind/span/data
3. **Type check** (`typecheck2.cc`) — semantic validation and type inference; `typecheck.cc` is the older superseded pass
4. **Interpret** (`interpreter.cc`) — AST evaluation

## Key files

| File | Role |
|------|------|
| `src/ast.hh` | AST node kinds and data layouts |
| `src/types.hh` / `types.cc` | Type system — `TypeInterner` interns all types by value |
| `src/value.hh` / `value.cc` | Runtime value representation (`ValueStore`) |
| `src/env.hh` | Lexical scoping (`EnvManager`) |
| `src/toteload.hh` | Arena allocator (`Arena`), typed index types, helper macros — read this first |
| `src/messages.cc` | Error/warning reporting with source locations |
| `src/string_interner.hh` | Deduplicated string storage (`StringInterner`) |

## Memory model

Arena allocation (`Arena` from `toteload.hh`) is used a lot. 
Two arenas are used: a persistent `arena` for long-lived data and a `work_arena` for scratch work that can be snapshotted and restored. 
`stdlib_alloc` is a thin wrapper around `malloc`/`free` used for growable collections.

## Language syntax (Blu)

```
name : type = value              // declaration
add : (i32, i32): i32 = |a, b| { a + b }  // typed function
arr : [4]i32 = .{ 1, 2, 3, 4 }  // array with sequence literal
```

Supported types: `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `bool`, `nil`, function types `(T, T): R`, slice `[]T`, array `[N]T`.
