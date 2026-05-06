# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## General instructions

**ALWAYS** put sentences on their own line when writing Markdown.

## What this is

Blu is a compiled programming language written in C++17.
The current goal is correctness over performance — the interpreter evaluates the AST directly rather than compiling to native code.

## Build

```bash
python build.py        # regenerates build.ninja and runs ninja
```

Outputs land in `out/`:
- `blu` / `blu.exe` — main interpreter binary
- `tokenviewer` / `tokenviewer.exe` — HTML token-stream visualizer

Dependencies: `clang++`, `ninja`, Python 3.

## Running

```bash
out/blu <file.blu>          # run a .blu source file
```

Test files live in `test/basic/`.
There is no automated test runner; run each file manually.

## Compiler pipeline

`src/main.cc` wires together the full pipeline:

1. **Tokenize** (`tokenize.cc`) — source text → `Tokens`
   (parallel `kinds`/`spans` vectors)
2. **Parse** (`parse.cc`) — `Tokens` → `AstNodes`
   (parallel `kinds`/`spans`/`datas` vectors)
3. **Type check** (`typecheck2.cc`) — fills a `Vector<TypeIndex>`
   (`type_annotations`) with one entry per AST node
4. **Interpret** (`interpreter.cc`) — walks the AST; reads
   `type_annotations` to know the resolved type of each node

On any error, `messages.print_messages()` is called and the process
exits non-zero.

## Key files

| File | Role |
|------|------|
| `src/blu.hh` | Central aggregation header; defines `Source`, `TypeCheckContext`, `Messages`, `Declaration` |
| `src/ast.hh` | `AstKind` enum, all `Ast*` data structs, `AstNodes` parallel-vector store |
| `src/tokens.hh` | `TokenKind` enum, `Tokens` parallel-vector store |
| `src/types.hh` / `types.cc` | `Type` struct, `TypeInterner` — interns types by value |
| `src/value.hh` / `value.cc` | `Value` (type + data pointer), `ValueStore` |
| `src/interpreter.hh` / `interpreter.cc` | Tree-walk `Interpreter`; uses `Env<ValueIndex>` |
| `src/env.hh` | Generic `Env<T>` / `EnvManager<T>` — lexical scope via parent-chain hash maps |
| `src/toteload.hh` / `toteload.cc` | Core utility lib: `Arena`, `Allocator`, `Str`, `Slice`, `ArenaSnapshot`, common macros |
| `src/index.hh` | Type-safe index wrappers: `Index<T,Tag>`, `OptionalIndex<T,Tag>` |
| `src/hashmap.hh` | Open-addressing hash map with fingerprinting |
| `src/vector.hh` | Dynamic array `Vector<T>` |
| `src/segment_list.hh` | Arena-backed segmented list for variable-length AST children |
| `src/arena_item_pool.hh` | Fixed-size object pool with 32-bit indices |
| `src/string_interner.hh` / `string_interner.cc` | `StringInterner` — deduplicates strings; returns `StrKey` |
| `src/messages.cc` | Error/warning formatting with source locations |
| `src/utils/stdlib.cc` | `stdlib_alloc` — `malloc`/`realloc`/`free` wrapper |

## Memory model

Two arenas are used throughout:
- `arena` — persistent; lives for the entire compilation
- `work_arena` — scratch space; callers call `work_arena.take_snapshot()`
  before a sub-task and `work_arena.restore(snapshot)` after

`stdlib_alloc` (from `src/utils/stdlib.cc`) wraps `malloc`/`free` and is used for growable `Vector` collections and hash maps.

The `Arena` type is defined in `toteload.hh`.
It reserves virtual memory upfront and commits pages on demand.
`ArenaSnapshot` captures the current allocation pointer so scratch allocations can be discarded cheaply.

## AST layout

`AstNodes` stores the tree as three parallel vectors:
- `kinds` — `AstKind` per node
- `spans` — `Span<TokenIndex>` per node (byte range in source)
- `datas` — `AstNodeData` union per node

Variable-length children (block items, call arguments, etc.) are stored as `SegmentList<NodeIndex>` inside `AstNodeData`, allocated from `nodes.segment_allocator` (backed by `arena`).

## Type system

All types are interned by value in `TypeInterner`.
`TypeIndex` is a 32-bit opaque index; use `types.get(idx)` to retrieve `Type *`.

Integer types: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`.
Other types: `bool`, `nil`, `never`, `literal_int`, `literal_function`, `function`, `slice`, `array`, `distinct`, `sequence`, `type`.

The type checker writes one `TypeIndex` per AST node into a flat `type_annotations` slice.
The interpreter then reads that slice instead of re-inferring types.

Environments differ between phases:
- Type-check: `Env<Declaration>` maps names → `TypeIndex` or `NodeIndex`
- Interpret: `Env<ValueIndex>` maps names → runtime value indices

## Language syntax (Blu)

```
x : i32 = 42                              // declaration
add : (i32, i32): i32 = |a, b| { a + b }  // function
arr : [4]i32 = .{ 1, 2, 3, 4 }            // array literal
```

Supported types: integer types listed above, `bool`, `nil`, function `(T, T): R`, slice `[]T`, array `[N]T`.

Keywords: `if`, `else`, `while`, `break`, `continue`, `return`, `and`, `or`.

Operators: `+`, `-`, `*`, `/`, `%`, `&`, `|`, `^`, `<<`, `>>`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `and`, `or`, `!`, unary `-`.

Builtins (prefixed with `#`): only `#print(...)` is currently implemented.
