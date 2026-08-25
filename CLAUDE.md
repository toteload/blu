# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Blu is a hobby programming language with a compiler written in C11 (previously C++17).
The long-term design supports compile-time code execution: AST → IR, interpret the IR for comptime evaluation, and eventually translate IR → C.
The rewrite is in progress; `TODO.md` tracks current design work.

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

`src/main.c` parses CLI options (`cli_options.c`), creates a `Compiler`, adds a source file, calls `compile()`, then (if it succeeded) `run_main()`, printing all messages if either fails.

There are **two IRs**, defined together in `src/ir.h` (opcodes also tabulated as X-macros in `x_sir.h` / `x_iir.h` for `ir.c` and `print.c`):

- **SIR (Specializer IR)** — emitted by codegen from the AST. One `SIrChunk` per declaration, built with `SIrBuilder`. References are `SRef` (`ref.h`): either a comptime `ValueIndex` or an instruction result.
- **IIR (Interpreter IR)** — the *residual* code left over after comptime evaluation (the specializer folds away everything it can and lowers the rest to IIR). Built with `IIrBuilder`; references are `IRef`. This is what actually runs at "execution time".

`compile()` in `src/compiler.c` runs the following, currently for a single source file:

1. `source_read_file` — load file contents
2. `source_tokenize` (`tokenize.c`) — text → `Tokens` (parallel `kinds`/`spans` arrays)
3. `source_parse` (`parse.c`) — `Tokens` → `AstNodes`
4. `source_index_declarations` — collect the file's top-level/module declarations
5. **global declaration map** — every source's declarations are interned into `Compiler.decls`, a `DeclarationInterner` keyed by `(parent, name)`; module nesting is walked with a small stack
6. `generate_code` (`codegen.c`, per user declaration) — AST → **SIR**, stored as `Declaration.data.decl.chunk`. Each declaration's chunk holds two evaluable blocks: one that computes its *type* (`block_type`) and one that computes its *value* (`block_val`).
7. **Resolver / Specializer** (`compiler.c`'s `resolve_declarations` driving `run_toplevel_block` in `specialize.c`) — interprets each declaration's SIR type-block then value-block. This is the comptime partial evaluator ("specialization"): anything comptime-known is folded to a `Value`; anything that depends on runtime state is lowered to a residual **IIR** chunk (built via a stack of `IIrBuilder`s, one per nested function — see `Specializer.builders`). Declarations are resolved on demand: hitting `SIR_lookup_decl_type`/`SIR_lookup_decl_value` for an unresolved declaration suspends the current entry and pushes a new one onto `Resolver.resolve_stack` (circular declarations are detected via `resolve_status`).
8. `run_main` (`compiler.c`) — looks up `main.main`, then `interpreter_call` (`interpret.c`) tree-walks its **IIR** chunk directly to actually execute the program. This is a separate, simpler interpreter from the specializer: it has no comptime/residual split, just executes.

SIR and IIR chunks are printed via `print.c` (`print_sir_chunk` after codegen with `--print-decl-ir`, and the final specialized value — including any residual IIR function bodies — after specialization with `--print-residual`; `--print-tokens` / `--print-ast` dump the earlier stages).

`Compiler.common` caches interned builtin types and values (`nil`, `type`, `i32`, `comptime_int`).

Diagnostics flow through `MessageSink` (`src/messages.h`), a function-pointer sink that stages append errors to. Messages are collected on the `Compiler` (and per-`Source`) and printed at the end by `compiler_print_all_messages`.
Note: message formatting (`{tok}`/`{str}` placeholders) and the vararg arg path are still unimplemented / known-buggy — see `TODO.md` and `unimplemented.md`.

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
| `src/codegen.h` / `codegen.c` | AST → SIR generation (`generate_code`, one call per top-level declaration) |
| `src/ir.h` / `ir.c` | SIR and IIR representations, `SIrBuilder` / `IIrBuilder`; design rationale in the header comment |
| `src/x_sir.h`, `src/x_iir.h` | X-macro opcode tables for SIR / IIR (name string + "has extra payload" flag + payload type), consumed by `ir.c` and `print.c`. `src/x_ir.h` is the pre-split precursor and is now unused/dead |
| `src/specialize.h` / `specialize.c` | `Specializer` / `Resolver` — the comptime partial evaluator. Walks SIR (`run_toplevel_block` → `step`), folding comptime-known code to `Value`s and emitting residual IIR for the rest |
| `src/interpret.h` / `interpret.c` | `Interpreter` — tree-walks a finished IIR chunk directly (`interpreter_call`); used by `run_main` to execute `main.main` after compilation |
| `src/ref.h` | Macro-templated tagged reference (instantiated as `SRef`/`sref_*` and `IRef`/`iref_*`): a `u32` that's either a comptime `ValueIndex` or an `InstructionIndex`, distinguished by the top bit |
| `src/print.h` / `src/print.c` | Human-readable dumps: `print_sir_chunk`, `print_iir_chunk`, `print_value_raw`, `print_type`, `print_tokens`, `print_ast_nodes` (formerly `ir_print.c`) |
| `src/types.h` / `types.c` | `TypeInterner` — interns types by value; `TypeIndex` is an opaque `u32` |
| `src/value.h` / `value.c` | `ValueStore` — runtime/comptime values |
| `src/eval.c` | Value reading/casting/coercion/unification helpers (`eval_cast_int`, `eval_coerce`, `eval_unify`, `eval_int_add/sub/mul/div`) shared by the specializer and interpreter |
| `src/util.c` | Misc helpers; currently just the (unfinished) `decode_string_literal` |
| `src/messages.h` / `messages.c` | `Message`, `MessageSink`, severity and location kinds |
| `src/string_interner.h` / `string_interner.c` | `StringInterner` — string deduplication (an `interner.h` instantiation) |
| `src/interner.h` | Macro-templated interner (see below) |
| `src/segment_list.h` | Macro-templated segmented list (see below) |
| `src/hashmap.h` | Macro-templated hash map (see below) |
| `src/x_ast_kinds.h` | X-macro table for `AstKind` (used by `parse.c` for e.g. `ast_kind_string`), `#include`d with an `X(...)` macro defined by the includer |
| `vendor/` | Vendored code: `xxhash.h`, `khash.h`, and the `splitmix64`/`xoshiro256plusplus` RNGs used by the fuzz tests |

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
Each `Source` additionally owns its own `arena`, which backs its filename, text, tokens, AST, and messages. SIR chunks live on the `Declaration` in `Compiler.decls` and are allocated from `Compiler.arena`/`scratch`, not the `Source`'s arena.
`Allocator` is a function-pointer allocator interface; `compiler.c` wraps `realloc`/`free` as `cstd_allocator` for growable collections.

## Language syntax (Blu)

```
x : i32 = 42                              // declaration
add : (i32, i32) i32 = |a, b| { a + b }  // function
```
