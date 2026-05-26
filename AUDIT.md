# Code Audit: `src/` — Bugs and Security Issues

## Summary

Blu is an early-stage, single-user interpreter for `.blu` source files.
It has no networking, no privilege boundary, and no untrusted code execution surface beyond the source file passed on the command line, so most findings are correctness/robustness issues rather than security vulnerabilities in the classical sense.
That said, several findings would let a crafted input file crash, hang, or corrupt the process, and a few are real C/C++ memory-safety footguns sitting in the foundational utilities that everything else builds on.

Findings are organized by category and ranked by impact (Critical / High / Medium / Low).
"Critical" means definitely-broken-now or trivially-exploitable; "High" means a correctness bug that almost certainly fires on real input; "Medium/Low" are latent or quality-of-implementation issues.

Note: `interpreter.cc`, `source_unit.cc`, and `typecheck.cc` are commented out of `build.py` and don't even compile against the current `blu.hh`, so they were excluded from severity ranking (they're dead code).

---

## 1. Memory safety / undefined behavior

### High

- **`src/types.cc:21-31` — `Update` macro in `type_to_string` advances `buf` by 0 on overflow.**
  ```cpp
  if (_offset >= buf_size) {
    buf_size  = 0;
    buf      += buf_size;   // adds 0 — should be _offset
  } else { ... }
  ```
  After a truncating write, subsequent `snprintf` calls write *over the previous chars* (technically harmless because `buf_size == 0` from then on and `snprintf` with size 0 writes nothing, but the intent is clearly broken and the pattern is bug-prone).

- **`src/types.cc:96-124` — `size_info` returns wrong sizes for arrays and sequences.**
  `Type_array` returns `sizeof(void*)` (8 bytes) — arrays are stored inline so this should be `stride(base) * size`.
  `Type_sequence` falls through to a `Todo()` that returns `{}` (size 0), but coercion paths actually use sequences.

- **`src/parse.cc:222-256` — `parse_type` silently succeeds with an uninitialized node.**
  After `Tok_bracket_open`, if the next token is neither `Tok_bracket_close` nor `Tok_literal_int`, the switch case falls through without calling `nodes->set(...)`.
  The function returns `true` and `*out` references a node whose `kind/span/data` are whatever `push_empty()` left there.
  Later passes will misinterpret it.

- **`src/messages.cc:5-18` — `count_args` is fragile.**
  Counts every literal `}` in the format string, including the second `}` of a `{{` (which `next_arg_offset` actually treats as an escape) and any stray `}`.
  `_error` then reads exactly that many `va_arg`s — wrong count → reads uninitialized stack memory.

- **`src/builder.cc:94-99` — `eval_expression` for `Ast_identifier` ignores `env->lookup` result.**
  `env->lookup(key, &decl)` return value is discarded; if lookup fails `decl` is uninitialized and `decl.node_index.as_value_idx()` returns garbage.
  `Env::lookup` itself dereferences a NULL `p` (`*out = *p`) when `map.get_ptr` returns NULL — see `blu.hh:859-864`.

- **`src/blu.hh:859-864` — `Env::lookup` dereferences NULL on miss.**
  ```cpp
  bool lookup(StrKey identifier, T *out) {
    T *p;
    auto res = lookup_ptr(identifier, &p);
    *out = *p;     // crashes when lookup_ptr returned false → p never assigned
    return res;
  }
  ```

- **`src/hashmap.hh:53` — `HashMap::get(k)` dereferences NULL on miss.**
  `V get(K key) { return *get_ptr(key); }` with `get_ptr` returning `nullptr` on miss.

### Medium

- **`src/messages.cc:49-65` — `find_source_location` asserts `offset < source.len()`** but is called for end-of-file tokens (whose span may equal `source.len()`).

- **`src/utils.cc:19-40` — `string_literal_byte_size` / `decode_string_literal` underflow on empty literals.**
  `for (usize i = 1; i < literal.len() - 1; ...)`: if `literal.len() < 2` then `len() - 1` wraps to `SIZE_MAX`.
  Practically the tokenizer only emits length-≥2 string tokens, but no preconditions are checked.

- **`src/tokenize.cc:101-154` — multi-char operators read past EOF via the source's null terminator.**
  `if (c == '=') { if (*at == '=') ... }` only works because `read_file` happens to null-terminate.
  Any tokenizer entry point that bypasses `read_file` would read OOB.

- **`src/tokenize.cc:156-174` — string literal with EOF after `\\` calls `Todo()` (abort).**
  Malformed source crashes the process instead of producing an error message.

- **`src/toteload.cc:170-184` — `parse_i64` has no overflow check, no validation, returns 0 for empty string.**
  Allows literal `"99999999999999999999"` (more than `i64`) to produce undefined wrapping.

- **`src/blu.hh:34-35` — `NodeIndex::is_some()` always reads `idx.value.is_some()`** regardless of `kind`.
  Works because both union members have `idx` at the same offset, but is type-unsafe and a refactoring hazard.

### Low

- **`src/ast_pretty_print.cc:85-86` — ANSI escape codes (`esc_code`) emitted unconditionally**, garbling output when stdout isn't a TTY.

- **`src/ast_pretty_print.cc:396-446` — `%lu` used for `usize len()`**; `usize` is `u64` on macOS/Linux but `unsigned long` is 32-bit on Windows.

- **`src/index.hh:19-25` — index `0` is the sentinel for `none()`**, so the zeroth element of every backing store is unusable.
  Consistently respected today (stores skip slot 0), but a foot-gun.

---

## 2. Error handling / robustness

### High

- **`src/builder.cc:539-718` (`eval_cast`) — every unimplemented cast path is a `Todo()` (abort)** rather than producing a typed error.
  Crafted-but-type-correct source can hit them.
- **`src/builder.cc:43, 55, 143, 152, 396` — many normal user errors are `Todo()`** (e.g. duplicate top-level declaration, missing identifier, circular declaration).
  User input that isn't pristine aborts the process.
- **`src/typecheck.cc` (entire file) — references fields that don't exist on the current `Declaration`/`NodeIndex` types** (`Declaration::kind`, `Declaration::data`, `NodeIndex_ast_node`, `is_const(node)`).
  It's commented out of the build, but it lives in `src/` and will be confusing/dangerous to anyone trying to revive it.
- **`src/messages.cc:67-138` — non-`Error` severities call `Todo()`.**
  Once `Warning`/`Info` are issued they'll abort.

### Medium

- **`src/parse.cc:299, 624-630, 679` — many parse errors return `false` without emitting any message.**
  Caller in `main.cc` just prints `"error: parsing"` with no location.
- **`src/tokenize.cc:209, 236` — `TokResult_unrecognized_token`** is detected but never reported (`// TODO add message of unrecognized token if necessary`).
  Unknown chars silently truncate the token stream.

---

## 3. Resource / lifecycle

### High

- **`src/utils/stdlib.cc:21-48` — `read_file` file-handle and buffer leaks** (covered above).

### Medium

- **`src/string_interner.cc:16-21` — `deinit` explicitly leaks** (`// TODO Currently we are leaking the storage memory`).
  Not a security issue in a short-lived process but is technical debt.
- **`src/builder.cc:134-182` — `resolve_declaration` sets `ResolveStatus_type_resolving` but does not reset it on error.**
  Re-entry after error returns `false` via the `Todo()` in the `resolving` branch.
  Tolerable today because we exit on first error.

---

## 4. Concurrency / signals

None — code is single-threaded throughout.
Globals (`stdlib_alloc`, `macos_page_size`) use no synchronization, which is fine as long as that holds.

---

## 5. Security-relevant observations

This is not a sandbox or a network service.
The only attacker-controlled input is the source file path and contents.
Concrete exposure:

- **Process crash via malformed source**: most `Todo()`/`Unreachable()`/assertion sites in the tokenizer/parser/typechecker are reachable with crafted input (`Tokenize::Todo()` on dangling backslash, `parse_type` returning uninit, every `Todo()` cast).
- **DoS via hashmap rehash loop**: the `hashmap.hh:190-209` infinite loop is reachable via a long enough sequence of declarations that hash-collide on the low bits.
- **No format-string vulnerabilities** — `printf`/`snprintf` always take constant format strings or use the safe `%.*s` pattern.
- **No `system`/`exec`/`popen` calls.**
- **No untrusted code execution** — the interpreter walks an AST it just parsed; the only side effect is `#print`.

The biggest realistic security risk is `read_file`: reading `ftell` into a `u32` and `malloc`ing `size + 1` would integer-overflow on a 4GB+ file (returning a small allocation and then `fread`-ing into it), enabling a heap buffer overflow if an attacker can hand the binary such a file.

---

## Recommendation priorities

If triaging just a handful of fixes, in order:

1. ~Fix `ttld::os::mem_commit` on macOS (return value inverted) — `src/toteload.cc:57-59`.~
2. ~Fix `arena_alloc_fn` free/realloc unreachable branch — `src/toteload.cc:147-161`.~
3. Fix the infinite-loop in `HashMap::grow_and_rehash` and the NULL-deref in `HashMap::get` / `Env::lookup`.
4. ~Fix `Vector::ensure_capacity` to record the actual allocated capacity.~
5. Fix `read_file`'s leaks and `u32` size truncation; null-check `fopen` in `write_file`.
6. ~Fix `Builder::eval_cast`'s array-to-slice `items = v->data` (should be `val->data`).~
7. Fix `parse_type`'s silent-success-with-uninitialized-node fall-through.
8. ~Replace user-facing `Todo()` calls in builder/parser with real diagnostics.~
