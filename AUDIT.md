# Codebase Audit

Performed on 2025-05-27 against the `const-eval` branch.
Files excluded: `interpreter.cc`, `typecheck.cc`.

---

## Critical — Crashes or silent data corruption

### 1. `Env::lookup` dereferences uninitialized pointer on failed lookup

`src/blu.hh:861-866`

```cpp
bool lookup(StrKey identifier, T *out) {
  T   *p;
  auto res = lookup_ptr(identifier, &p);
  *out     = *p;   // p is uninitialized when res == false
  return res;
}
```

If the identifier is not found, `lookup_ptr` returns `false` and never writes to `p`.
The next line dereferences whatever garbage `p` contains.

### 2. `str_eq` reads past the shorter buffer

`src/toteload.hh:195-199`

```cpp
b32 is_same_len     = a.len() == b.len();
b32 is_same_content = memcmp(a.str, b.str, a.len()) == 0;
return is_same_len && is_same_content;
```

Both variables are evaluated eagerly.
When `b` is shorter than `a`, `memcmp` reads past `b`'s valid memory.
Fix: short-circuit (`if (!is_same_len) return false;`) or use `min(a.len(), b.len())`.

### 3. `Arena::commit_size()` returns remaining space but `Arena::commit()` treats it as total

`src/toteload.hh:307` / `src/toteload.cc:135-147`

```cpp
// Returns remaining committed-but-unallocated bytes
usize commit_size() const { return ptr_diff(commit_end, at); }
```

```cpp
void Arena::commit(usize commit_size_in_bytes) {
  ...
  usize current_commit_size = commit_size();       // remaining, not total
  if (rounded_commit_size <= current_commit_size) { // compared against desired total
    return;
  }
  ttld::os::mem_commit(base, rounded_commit_size);
  commit_end = ptr_offset(base, rounded_commit_size); // can move commit_end BACKWARDS
}
```

After the arena has allocated some bytes, `commit_size()` returns a small number.
A caller passing a modest total (e.g. `ArenaItemPool::reserve_index`) can cause `commit_end` to be set to an address before `at`, corrupting the arena's internal state.

### 4. `HashMap::first_valid_entry()` skips bucket at index 0

`src/hashmap.hh:58-69`

```cpp
Bucket<K, V> const *next(Bucket<K, V> const *at) const {
  u32 i_at = ptr_diff(at, buckets) / sizeof(Bucket<K, V>);
  for (u32 i = i_at + 1; i < cap(); i++) { // starts at i_at + 1
```

```cpp
Bucket<K, V> const *first_valid_entry() const { return next(buckets); }
```

`next(buckets)` computes `i_at = 0`, then starts from index 1.
Any entry hashed to bucket 0 is silently skipped during iteration.
Currently affects the verbose debug output loop in `main.cc:142`.

### 5. `check_and_resolve_coercion` dereferences garbage type when called without a hint

`src/builder.cc:705-715`

When `check_expression` is called without a type hint (e.g. for the value inside `Ast_cast`), `hint.type` is `TypeIndex{0}`.
`check_and_resolve_coercion` compares the expression's type against `TypeIndex{0}`, finds them unequal, and calls `types->is_coercible_to(type_src, TypeIndex{0})`.
This calls `types->get(TypeIndex{0})` which returns `list[0]` — a slot reserved by `push_empty()` during init that contains an uninitialized pointer.
Dereferencing it is undefined behavior.

### 6. `Builder::eval_expression` ignores `lookup` failure for identifiers

`src/builder.cc:106-109`

```cpp
case Ast_identifier: {
  auto        key = intern_identifier(data.identifier.token_index);
  Declaration decl;
  env->lookup(key, &decl);  // return value ignored
  *result = decl.node_index.as_value_idx();
```

If the lookup fails, `decl` is filled with garbage (see issue #1), and `as_value_idx()` asserts or returns nonsense.

### 7. `round_up_to_nearest_power_of_two` has undefined behavior for large inputs

`src/toteload.hh:113-119`

```cpp
template<typename T> constexpr T round_up_to_nearest_power_of_two(T x) {
  if (x <= 1) { return 1; }
  return 1 << bitwidth(x);
}
```

`1` is an `int` (32-bit).
When `x` is large enough that `bitwidth(x) >= 32`, `1 << 32` is undefined behavior.
For `u64` values the shift can be up to 64.
Should be `T(1) << bitwidth(x)`.

### 8. `Arena::raw_alloc` returns NULL on OOM — no caller checks

`src/toteload.cc:158-161`

```cpp
if (at_after_alloc >= reserve_end) {
  // TODO OOM
  return NULL;
}
```

Every caller blindly writes through the returned pointer.
An OOM in any arena silently corrupts memory at address 0 or crashes.

---

## High — Incorrect behavior or build failures

### 9. `parse_if_else` fails when if-without-else is at end of tokens

`src/parse.cc:524-525`

```cpp
TokenKind tok;
Try(peek(&tok));           // returns false at end-of-tokens
if (tok != Tok_keyword_else) {
```

If the if-expression is the last thing in a block and there is no `else`, `peek` returns false (no more tokens) and `Try` propagates the failure as a parse error.
An `if` without `else` at the end of a file or block is valid and should succeed.

### 10. `parse_type` falls through silently for unexpected tokens after `[`

`src/parse.cc:233-266`

After consuming `[`, only `]` (slice) and `Tok_literal_int` (array) are handled.
Any other token falls through the entire `case Tok_bracket_open` block without setting the AST node, leaving the output node in an indeterminate state.

### 11. `Builder::eval_binary_op` is not defined for `Builder`

`src/builder.cc:180`

```cpp
Try(eval_binary_op(binop.kind, lhs, rhs, node_index, result));
```

`eval_binary_op` is only declared on `Interpreter`, not `Builder`.
This is a compile error in the current state of `builder.cc` if the `Ast_binary_op` code path is reached during compilation.

### 12. `Arena::raw_alloc` writes debug pattern `0xaa` in all builds

`src/toteload.cc:171`

```cpp
memset(commit_end, 0xaa, commit_size);
```

This poisons freshly committed pages in every build, not just debug builds.
In release builds it wastes time and can mask uninitialized-memory bugs that sanitizers would otherwise catch.

### 13. `type_to_string` missing commas between function parameters

`src/types.cc:76-83`

```cpp
if (type->function.param_count > 0) {
  Update(type_to_string(type->function.param_types[0], buf, buf_size));
}
for (u32 i = 1; i < type->function.param_count; i += 1) {
  Update(type_to_string(type->function.param_types[i], buf, buf_size));
  // no comma printed between parameters
}
```

A function type like `(i32, i32): bool` prints as `(i32i32): bool`.

### 14. `parse_i64` has no overflow detection

`src/toteload.cc:223-237`

```cpp
i64 acc = 0;
for (u32 i = 0; i < s.len(); i++) {
  acc *= 10;
  acc += s[i] - '0';
}
```

Very large integer literals in source files silently wrap around.

### 15. `tokenviewer.cc` is completely broken

`src/tools/tokenviewer.cc:218-233`

References `Source` (struct removed), `messages.init` with wrong signature, `messages.source` (field removed), and `tokenize(&messages, text, &tokens)` (wrong signature).
Also line 213: `arena.init(MiB(1))` re-initializes `arena` instead of `work_arena`.

### 16. `SourceUnit::run_const_code()` defined but not declared

`src/source_unit.cc:114`

The method `bool SourceUnit::run_const_code()` is defined in the `.cc` file but never declared in the `SourceUnit` struct in `blu.hh`.
This is a compile error if `source_unit.cc` is included in the build.

### 17. `SourceUnit::deinit` doesn't clean up partially initialized state

`src/source_unit.cc:13-38`

If `tokenize()` fails, `tokens` was already `init()`-ed but `stage` stays at `Stage_tokenize`, so `deinit()` won't call `tokens.deinit()` — memory leak.
Same pattern applies to all subsequent stages: if a stage fails after initializing its resources but before advancing the stage counter, those resources are leaked.

---

## Medium — Semantic bugs or portability issues

### 18. `HashMap::deinit` and `grow_and_rehash` pass wrong size to `alloc.free`

`src/hashmap.hh:107-109, 238-239`

```cpp
u32 byte_size = cap * (sizeof(Bucket<K, V>) + sizeof(u32));
alloc.free(buckets, byte_size);
```

`alloc.free<Bucket<K,V>>(buckets, byte_size)` internally does `raw_free(p, byte_size * sizeof(Bucket<K,V>))`.
The intent is to pass the total byte size, but the template multiplies again by `sizeof(T)`.
Not harmful with the current `stdlib_alloc` (which ignores size in `free`), but would corrupt state with a size-aware allocator.

### 19. `ObjectPool::grow(KiB(2))` allocates 2048 items, not 2 KiB of items

`src/toteload.hh:367`

`KiB(2)` is 2048.
If `Item` (a union of `T` and `Item*`) is large, this allocates `sizeof(Block) + 2048 * sizeof(Item)` bytes in a single malloc — potentially many megabytes.

### 20. No Linux platform support

`src/toteload.cc:10-64`

Only `_WIN32` and `__APPLE__` are handled.
On Linux, `page_size()`, `mem_reserve()`, `mem_commit()`, and `mem_release()` are undefined — linker errors.

### 21. Windows `VirtualFree` called with wrong flags

`src/toteload.cc:34`

```cpp
VirtualFree(p, size, MEM_RELEASE);
```

When using `MEM_RELEASE`, the size parameter must be 0.
Passing non-zero silently fails the call, leaking the virtual memory.

### 22. Windows `is_sys_info_initialized` never set to true

`src/toteload.cc:14-19`

`GetSystemInfo` is called every time `page_size()` is invoked because the flag is never set.

### 23. `TypeInterner::deinit()` is empty

`src/types.cc:290`

All interned types allocated via `storage.raw_alloc` are leaked.
The `map` and `list` are also not cleaned up.

### 24. `StringInterner::deinit()` leaks storage memory

`src/string_interner.cc:16-21`

Has a TODO acknowledging the leak.
All strings copied into `storage` are never freed.

### 25. `Arena::push_format_string` truncates length to `u32`

`src/toteload.cc:193`

```cpp
return { s, cast<u32>(len), };
```

`Str._len` is `usize` (64-bit on most platforms).
The cast silently truncates for strings exceeding 4 GB (unlikely, but semantically wrong).

### 26. Zero-length arrays are non-standard C++

`src/blu.hh:185, 189` / `src/toteload.hh:352`

`TypeIndex param_types[0]`, `TypeIndex item_types[0]`, `Item items[0]` — flexible array members are a C99 feature, not valid C++.
Works on GCC/Clang as an extension but is not portable.

### 27. `ObjectPool::allocation_count` is never initialized or used

`src/toteload.hh:358`

Dead field.

---

## Low — Resource management, style, minor issues

### 28. `read_file` leaks file handle and memory on error paths

`src/utils/stdlib.cc:16-48`

- If `malloc(size + 1)` returns null, `fclose(f)` is never called.
- If `fread` reads fewer bytes than expected, `data` is leaked (only `buf` is freed).

### 29. `read_file` uses `ftell` stored in `u32`

`src/utils/stdlib.cc:29`

`ftell` returns `long` (signed), cast to `u32`.
Returns -1 on error, which becomes `UINT32_MAX`.
Files larger than 2 GB are also silently mishandled.

### 30. `main.cc` doesn't clean up most resources on exit

`src/main.cc`

Arenas, tokens, nodes, strings, types, envs, values, and messages are allocated but never `deinit`-ed.
The OS reclaims everything on process exit, but this makes leak-detection tools noisy.

### 31. `Queue::pop_front` is O(n)

`src/vector.hh:29-37`

`shift_left()` does a `memmove` of the entire backing array.
Fine for small queues, but degrades badly at scale.

### 32. `VMemBlock::ensure_commited` — typo

`src/toteload.cc:82`

"commited" should be "committed".

### 33. `Str::is_ok()` returns false for valid empty strings

`src/toteload.hh:161`

`return str && _len;` — an empty string with a valid pointer returns false.
This is inconsistent with `is_empty()` which only checks length.

### 34. `tokenviewer` leaks `StringBuilder` buffers

`src/tools/tokenviewer.cc`

Multiple `StringBuilder` objects are created, their internal `Vector<char>` is heap-allocated, but `deinit` is never called.

### 35. `tokenviewer` XSS

`src/tools/tokenviewer.cc:138-141`

Source text is inserted into HTML unescaped.
Source code containing `<script>` tags would execute.
Not user-facing, but worth noting.

---

## Summary

| Severity | Count |
|----------|-------|
| Critical | 8     |
| High     | 9     |
| Medium   | 10    |
| Low      | 8     |
| **Total**| **35**|
