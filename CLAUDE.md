# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Blu is a hobby programming language with a compiler written in C11 (previously C++17; the `c-rewrite` branch is a ground-up rewrite in C).
The long-term design supports compile-time code execution: AST → IR, interpret the IR for comptime evaluation, and eventually translate IR → C.
The rationale is documented in the comment at the top of `src/ir.h`.
The rewrite is in progress; `TODO.md` and `NOTES.md` track current design work.

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

## Testing

```bash
out/blu test/basic/foo.blu     # run a single test file
out/blu.test                   # unit tests
```

Unit tests are C files registered in `test/main_test.c` (e.g. `test/tokenize.test.c` via `register_tokenizer_tests`) and use the assertion macros from `test/test.h`.
The unit test binary's source list is in `add_test_suite` in `build.py`.

## Compiler pipeline

`src/main.c` parses CLI options, creates a `Compiler`, adds the source file, calls `compile()`, and prints all messages.

`compile()` in `src/compiler.c` loops over sources and runs, per `Source`:

1. `source_read_file` — load file contents
2. `source_tokenize` (`tokenize.c`) — text → `Tokens` (parallel `kinds`/`spans` arrays)
3. `source_parse` (`parse.c`) — `Tokens` → `AstNodes`

That is where the wired-up pipeline currently ends.
Later stages exist but are not yet integrated:

- `src/check.c` / `check.h` — type checker (`Checker`); **not in the `build.py` inputs list**
- `src/ir.c` / `ir.h` — IR generation (`generate_ir`) and the IR interpreter (`IrMachine`, `ir_run`)
- `src/eval.c` — value reading/casting helpers used by evaluation

Diagnostics flow through `MessageSink` (`src/messages.h`), a function-pointer sink that stages append errors to; each `Source` collects its `Message`s and `compiler_print_all_messages` prints them at the end.

## Key files

| File | Role |
|------|------|
| `src/toteload.h` / `toteload.c` | Base layer: fixed-width typedefs (`u32`, `b32`, …), `Arena`, `Allocator`, `String`, common macros (`Cast`, `internal`, `Null`/`True`/`False`) |
| `src/blu.h` | Shared index typedefs (`StringIndex`, `TypeIndex`, `AstIndex`, `ValueIndex`, …) and forward declarations |
| `src/compiler.h` / `compiler.c` | `Compiler` — owns the `Source` list, arenas, and message sink; comment sketches the planned multi-file compilation strategy |
| `src/source_file.h` / `source_file.c` | `Source` — one per file; owns its own arena holding filename, text, tokens, AST, and messages |
| `src/tokens.h` / `tokenize.c` | `TokenKind` enum, `Tokens` parallel-array store |
| `src/ast.h` / `parse.c` | `AstKind` enum, `AstNodes` store |
| `src/types.h` / `types.c` | `TypeInterner` — interns types by value; `TypeIndex` is an opaque `u32` |
| `src/value.h` / `value.c` | `ValueStore` — runtime/comptime values |
| `src/env.h` / `env.c` | `Env` / `EnvAllocator` — lexical scoping |
| `src/messages.h` / `messages.c` | `Message`, `MessageSink`, severity and location kinds |
| `src/string_interner.h` / `string_interner.c` | String deduplication |
| `src/segment_list.h` | Macro-templated segmented list (see below) |
| `src/hashmap.h` | Macro-templated hash map (see below) |
| `ext/` | Vendored single-file libraries (`xxhash.h`, `khash.h`, …) |

`src/builder.cc`, `formatcode.sh`/`formatcode.bat`, and the `.bat` build scripts are leftovers from the pre-rewrite C++ codebase and are not part of the build.

## Generic container pattern

`segment_list.h` and `hashmap.h` are C "templates" instantiated with macros.
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

## Memory model

Everything is arena-based (`Arena` in `toteload.h` reserves virtual memory upfront and commits pages on demand).
The `Compiler` owns an `arena` and a `scratch` arena.
Each `Source` additionally owns its own `arena`/`scratch`, which back its filename, text, tokens, and messages.
`Allocator` is a function-pointer allocator interface; `main.c` wraps `realloc`/`free` as `cstd_allocator` for growable collections.

## Language syntax (Blu)

```
x : i32 = 42                              // declaration
add : (i32, i32) i32 = |a, b| { a + b }  // function
```
