# Blu codebase audit

Scan of `src/` (plus the container templates and tests) for bugs and discrepancies,
sorted by severity. No code was modified as part of this audit.

Notes:

- The tree currently compiles cleanly.
- `IR_unify` and `IR_function_return_type` do exist in the `IrOpcode` enum.
- Findings in the code-gen path that is under active development are marked *(codegen WIP path)*.

---

## High

### H1 — `compile()` declaration-tree walk is off-by-one (declarations skipped / `decl_idxs` misaligned)

`src/compiler.c:259-334`

The traversal seeds the stack with the root's child count and then reads `decls[offset]`
starting at `offset = 0`, which is the **root itself** (`decls[0]`):

```c
u32 offset = 0;
...
SourceDeclaration const *decl = &source->decls[offset];  // first iter reads decls[0] = root
offset += 1;
...
source->decl_idxs[offset] = idx;                          // writes at post-increment offset
```

Traced on `mod main\nx : i32 = 42` (`decls = [root(cc=1), mod(cc=1), decl]`): iteration 1 reads
the root, decrements the root frame's `n` from 1→0, and matches neither the `_mod` nor
`_declaration` branch; iteration 2 sees `n == 0` and pops. **The module and its declaration are
never registered.** With two modules, the *last* module + its decls are dropped for the same
reason.

Separately, `decl_idxs[offset]` is written *after* `offset += 1`, so a node read at `decls[k]`
records its global index at `decl_idxs[k+1]`. That contradicts the convention established by
`decl_idxs[0] = 0` for `decls[0]`, and `codegen_init` (`src/codegen.c:82-87`) reads `decl_idxs[i]`
indexed by the decls-array position — so it reads mismatched/unset indices.

Strong candidate for the current "segfaults on test files" behavior. Core name-resolution logic;
worth fixing before trusting any codegen output.

---

## Medium

### M1 — `source_index_declarations` reads the AST root at index 0, but the root is now at index 1

`src/source_file.c:144-146`

```c
Assert(ast->kinds[0] == Ast_source);
AstSource *s = ast_data(ast, 0);
```

`parse()` now reserves AST index 0 as the nil node and puts the real `Ast_source` at index 1. This
currently "works" only by accident: `datas[0]` is zero-initialized and the root's payload is
flattened first at offset 0, so `ast_data(ast, 0)` aliases the real root. The `Assert` also passes
deceptively because `Ast_source == 0` equals the zeroed reserved node's kind. Should be index 1.
Fragile — any change to flatten ordering or arena zeroing breaks it silently.

### M2 — `hash_type` hashes the wrong memory when the scratch arena isn't empty

`src/types.c:110-121`

```c
void *data = scratch->base;             // start of arena
u32 size = push_type_data(scratch, x);  // writes at scratch->at
u32 hash = XXH32(data, size, 0);
```

`push_type_data` pushes at `scratch->at`, but the hash is taken from `scratch->base`. These
coincide only when the scratch arena is empty. Currently masked because types are interned only
during `compiler_init` (scratch empty), but if any type is interned later (during checking/codegen
while scratch holds data) you'll hash stale memory → wrong hashes → the interner stops
deduplicating (or mis-dedups) types. Should hash from `snapshot.at`.

### M3 — Message formatting is unimplemented and the vararg path is UB

`src/messages.c:69-95`, `src/messages.h:29-61`, `src/source_file.c:6-30`, `src/compiler.c:61-83`

`print_message` prints the raw format string (`{tok}`, `{str}` placeholders appear literally) and
never consumes the collected `args`. Meanwhile the sinks read each argument with
`va_arg(vl, MessageArg)`, where `MessageArg` is a 4-byte union — but callers pass a 16-byte
`String` for `{str}` (e.g. `source_read_file`). Reading a `String` as a 4-byte union is undefined
and would desync any message with multiple args. The `{tok}`/`{type}` cases happen to work only
because a promoted `int` and a 4-byte union pass identically on the ABI. The whole arg-collection
path is effectively dead until formatting is implemented — but it's live UB in the meantime.

### M4 — `read_file` leaks the `FILE*`

`src/source_file.c:58-102`

`fopen` succeeds but there is no `fclose` on **any** return path (ok, ftell error, short read). One
descriptor leaked per source file read.

### M5 — `add_declaration` desyncs `decls` from the interner on duplicate keys

`src/compiler.c:170-173`

```c
DeclarationIndex add_declaration(...) {
  decls_push(&compiler->decls, &compiler->arena);        // unconditional
  return decl_keys_add_checked(&compiler->decl_keys, key, already_exists);
}
```

When the key already exists, the interner returns the existing index (no growth) but `decls` still
grew — so `decls.len` runs ahead of the interner, and subsequent `set_declaration_value(idx)`
writes land on the wrong slot. Only bites on duplicate declarations (already error cases), but it
corrupts state right when trying to report the error cleanly.

### M6 — `IR_unify` is emitted with an extra payload but isn't in the flatten classifier *(codegen WIP path)*

`src/codegen.c:235-241` vs `281-331`

`gen_code`'s `Ast_function` case does `inst_push_data(gen, inst_unify, IrUnify)`, but
`opcode_references_extra` / `extra_payload_size` / `extra_payload_align` don't list `IR_unify`. So
`source_generate_code` flattens it as *inline* data: it stores the truncated `.ptr` as a `u32` and
never copies the `IrUnify` payload into `extra`. The interpreter would then read garbage. First
instance of the hand-maintained parallel-table hazard (see N7) actually firing.

### M7 — `gen_code`'s `type_destination` has inconsistent meaning across call sites *(codegen WIP path)*

`src/codegen.c:171, 188, 216, 266, 347`; `inst_add_check_coerce` at `158`

The parameter is now typed `IrRef`, but it's fed three different things: a raw `TypeIndex`
(`common.type.type` at :188, `common.type.nil` at :347), a raw **unwrapped** `ValueIndex`
(`common.val.type` at :216 — missing `ir_ref_from_value_index`), and a properly value-encoded ref
(:266). `inst_add_check_coerce` then treats it as a `TypeIndex` and re-wraps it via
`inst_add_const_type`. So depending on the path, a value-encoded ref (MSB set) can be interpreted
as a `TypeIndex`, producing a bogus type. This is the "in-between" state around the `IR_const` →
`IrRef` migration.

---

## Low / nits

- **N1 — `main()` always returns 0** (`src/main.c:88-93`): exit code ignores compile success.
  Prints `error` but exits 0, so scripts/CI can't detect failure.
- **N2 — `arena_push` can commit past the reservation** (`src/toteload.c:108-117`): near the end of
  the reserved range it commits `Max(512KiB, needed)` rounded up, without clamping to `reserve_end`,
  so `vmem_commit` could run past the mapping and trip the `Assert`. Only reachable with near-full
  arenas.
- **N3 — hashmap rehash can theoretically drop a bucket** (`src/hashmap.h:321-349`): if the chained
  relocation `for j` loop runs `cap` iterations without placing the displaced bucket, it falls
  through to `next` and the in-hand bucket is lost. Very low probability, but silent data loss
  rather than a `return False` retry.
- **N4 — `segment_list` `_copy_to_array` does `memcpy(dst, NULL, 0)`** (`src/segment_list.h:142`)
  when `len` lands exactly on a segment boundary (the last segment pointer is still `NULL`).
  Harmless in practice but technically UB.
- **N5 — little-endian assumption** in `read_unsigned_integer_extend` / `eval_cast_int`
  (`src/eval.c:24-29`) and the 8-byte-pointer assumptions in `types_size_info`. Fine for current
  targets; worth a comment.
- **N6 — `read_file` uses `i32` for `ftell`** (2 GB cap) and `arena_init` doesn't check for `mmap`'s
  `MAP_FAILED` (`src/toteload.c:56`, `src/source_file.c:79`).
- **N7 — parallel-table maintenance hazard** (`src/codegen.c:281-331`): `opcode_references_extra` /
  `extra_payload_size` / `extra_payload_align` must be updated by hand every time `gen_code` emits a
  new opcode. There's no compile-time link between the emit site and these tables — M6 is the first
  instance of it breaking. An X-macro (like `x_ast_kinds.h`) over the opcodes would make this
  self-consistent.
- **N8 — dead / unwired code**: `env.c` + `env.h` and `check.c` + `check.h` have no callers
  (`check.c` isn't even in `build.py`), the IR interpreter (`ir_run`, `generate_ir_function`) and its
  `IR_const` / `IrComptimeFunc` references are `#if 0`'d and stale, and `write_tokens` /
  `write_nodes` + the commented-out arena setup in `main.c` are leftovers. Not bugs, but they
  obscure what's live.

---

## Suggested triage order

1. **H1** and **M2** first — most likely behind the current segfaults; verify with a debugger.
2. **M1**, **M4**, **M5** — small, contained correctness/leak fixes in the stable pipeline.
3. **M3** — decide whether to implement `{...}` substitution or strip the arg machinery for now.
4. **M6** / **M7** / **N7** — part of the code-gen work in progress.
