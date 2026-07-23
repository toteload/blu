# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Blu is a hobby programming language with a compiler written in C11 (previously C++17).
The long-term design supports compile-time code execution: AST → IR, interpret the IR for comptime evaluation, and eventually translate IR → C.
The rewrite is in progress; `TODO.md` and `NOTES.md` track current design work.

## Code style

Never add comments to code. Tests are the exception — comments are allowed in test files.

## Build

```bash
python build.py        # regenerates build.ninja, runs ninja, refreshes compile_commands.json
```

Dependencies: `clang`, `ninja`, Python 3.
Outputs land in `out/`:

- `blu` / `blu.exe` — the compiler
- `blu.test` / `blu.test.exe` — unit test binary

Build configuration lives entirely in `build.py`:
compiler flags are hardcoded in the `compile_c_debug` rule, and the source file list is the `inputs` array.
To add a new `.c` file to the build, append it to that array.
Do not add CLI flags to `build.py`; hardcode options in the rules directly.
Note that `-Werror=switch` is enabled — every `switch` over an enum must handle all values.
Address and UB sanitizers are on by default (`USE_SANITIZERS` in `build.py`).

## Testing

```bash
out/blu path/to/file.blu       # run the compiler on a source file
out/blu.test                   # unit tests
```

Unit tests are C files registered in `test/main_test.c` (e.g. `test/tokenize.test.c`, `test/parse.test.c`) and use the assertion macros from `test/test.h`.
The unit test binary's source list is in `add_test_suite` in `build.py`.

## Compiler pipeline

`src/main.c` parses CLI options (`cli_options.c`), creates a `Compiler`, adds a source file, calls `compile()`, and prints all messages.

`compile()` in `src/compiler.c` runs the following, currently for a single source file:

1. `source_read_file` — load file contents
2. `source_tokenize` (`tokenize.c`) — text → `Tokens` (parallel `kinds`/`spans` arrays)
3. `source_parse` (`parse.c`) — `Tokens` → `AstNodes`
4. `source_index_declarations` — collect the file's top-level/module declarations
5. **global declaration map** — every source's declarations are interned into `Compiler.decls`, a `DeclarationInterner` keyed by `(parent, name)`; module nesting is walked with a small stack
6. `source_generate_code` (`codegen.c`) — AST → IR chunk per declaration (`ir.c` / `ir.h`, built via an `IrBuilder`)
7. `source_interpret_declaration` (`interpret.c`) — interpret each IR chunk to produce a "runtime IR" chunk (partial evaluation: comptime work is folded to constants)

The IR chunks are printed (`ir_print.c` / `ir_chunk_print`) after codegen and after interpretation.

`Compiler.common` caches interned builtin types and values (`nil`, `type`, `i32`, `comptime_int`).

Not yet wired into the pipeline:

- `src/check.c` / `check.h` — type checker (`Checker`); **not in the `build.py` inputs list**

Diagnostics flow through `MessageSink` (`src/messages.h`), a function-pointer sink that stages append errors to. Messages are collected on the `Compiler` (and per-`Source`) and printed at the end by `compiler_print_all_messages`.
Note: message formatting (`{tok}`/`{str}` placeholders) and the vararg arg path are still unimplemented / known-buggy — see `TODO.md`.

## Key files

| File | Role |
|------|------|
| `src/toteload.h` / `toteload.c` | Base layer: fixed-width typedefs (`u32`, `b32`, …), `Arena`, `Allocator`, `String`, `Stack`, common macros (`Cast`, `internal`, `Null`/`True`/`False`) |
| `src/blu.h` | Shared index typedefs (`StringIndex`, `TypeIndex`, `AstIndex`, `ValueIndex`, `DeclarationIndex`, …) and forward declarations |
| `src/compiler.h` / `compiler.c` | `Compiler` — owns the `Source` list, arenas, string/type/value/declaration interners, `Common`, and message sink; drives `compile()` |
| `src/cli_options.h` / `cli_options.c` | `CLIOptions` and `parse_cli_options` |
| `src/source_file.h` / `source_file.c` | `Source` — one per file; owns its own arena holding filename, text, tokens, AST, declarations, IR chunks, and messages |
| `src/tokens.h` / `tokenize.c` | `TokenKind` enum, `Tokens` parallel-array store |
| `src/ast.h` / `parse.c` | `AstKind` enum, `AstNodes` store |
| `src/codegen.h` / `codegen.c` | AST → IR generation (`source_generate_code`) |
| `src/ir.h` / `ir.c` | IR representation and `IrBuilder`; design rationale in the header comment |
| `src/ir_print.c` | Human-readable IR dump (`ir_chunk_print`) |
| `src/interpret.h` / `interpret.c` | IR interpreter producing runtime IR (`source_interpret_declaration`) |
| `src/types.h` / `types.c` | `TypeInterner` — interns types by value; `TypeIndex` is an opaque `u32` |
| `src/value.h` / `value.c` | `ValueStore` — runtime/comptime values |
| `src/eval.c` | Value reading/casting helpers used during evaluation |
| `src/env.h` / `env.c` | `Env` / `EnvAllocator` — lexical scoping |
| `src/messages.h` / `messages.c` | `Message`, `MessageSink`, severity and location kinds |
| `src/string_interner.h` / `string_interner.c` | `StringInterner` — string deduplication (an `interner.h` instantiation) |
| `src/interner.h` | Macro-templated interner (see below) |
| `src/segment_list.h` | Macro-templated segmented list (see below) |
| `src/hashmap.h` | Macro-templated hash map (see below) |
| `src/x_ast_kinds.h`, `src/x_ir.h` | X-macro tables (kind enum ↔ payload type ↔ name string) `#include`d with an `X(...)` macro defined by the includer |
| `ext/` | Vendored single-file libraries (`xxhash.h`, `khash.h`, …) |

`src/builder.cc`, `formatcode.sh`/`formatcode.bat`, and the `.bat` build scripts are leftovers from the pre-rewrite C++ codebase and are not part of the build.

## Generic container pattern

`interner.h`, `segment_list.h`, and `hashmap.h` are C "templates" instantiated with macros.
Define the configuration macros, then `#include` the header:

```c
#define SEGMENTLIST_NAME          SourceList
#define SEGMENTLIST_TYPE          Source
#define SEGMENTLIST_MIN_SIZE_LOG2 4
#define SEGMENTLIST_SEGMENT_COUNT 20
#define SEGMENTLIST_OUTPUT_TYPES        // emit typedefs + declarations (in headers)
#include "segment_list.h"
```

Repeat the same include with `SEGMENTLIST_OUTPUT_DEFINITIONS` (and optionally `SEGMENTLIST_LINKAGE internal`, `SEGMENTLIST_FUNCTION_PREFIX`) in exactly one `.c` file to emit the function bodies.
`hashmap.h` follows the same `HASHMAP_NAME` / `HASHMAP_KEY_TYPE` / `HASHMAP_VALUE_TYPE` pattern.

`interner.h` interns values to opaque `u32` indices via `INTERNER_NAME` / `INTERNER_TYPE` / `INTERNER_INDEX_TYPE`, plus `INTERNER_HASH_FN` / `INTERNER_COMPARE_FN` in the definition include.
It can optionally hold an `INTERNER_EXTRA_TYPE` per entry (e.g. `DeclarationInterner` maps a `(parent, name)` key to a `Declaration`), and copy items into its arena on first insert via `INTERNER_COPY_FN`.
`StringInterner`, `TypeInterner`, and `DeclarationInterner` are all instantiations.

## Memory model

Everything is arena-based (`Arena` in `toteload.h` reserves virtual memory upfront and commits pages on demand).
The `Compiler` owns an `arena` and a `scratch` arena.
Each `Source` additionally owns its own `arena`/`scratch`, which back its filename, text, tokens, IR, and messages.
`Allocator` is a function-pointer allocator interface; `compiler.c` wraps `realloc`/`free` as `cstd_allocator` for growable collections.

## Language syntax (Blu)

```
x : i32 = 42                              // declaration
add : (i32, i32) i32 = |a, b| { a + b }  // function
```
