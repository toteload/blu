# Specializer bug hunt

Findings from running `samples/aoc_2025_day_01.blu` and about 30 small test programs.
The test programs are in the scratchpad and are not checked in. Repros are inlined below.

## Status overview

| # | Issue | Kind | Status |
|---|-------|------|--------|
| 1 | Unifying the types of several value-less `break`s → `Todo()` | panic | **fixed** (by you) |
| 2 | `condbr` jumps to the wrong place when the else-arm starts with a constant | wrong jump / crash | **fixed** |
| 3 | `SIR_repeat` data decoded as `SRef`, but stored as a raw offset | assert | **fixed** |
| 4 | Constant `condbr` compares value keys instead of contents | wrong output | **fixed** |
| 5 | `SIR_br` does not skip the instructions after it | wrong output | open |
| 6 | Runtime `comptime_int` has no size | panic | open, needs a design decision |
| 7 | Folding binary arithmetic on two constants | `Todo()` | open |
| 8 | Assigning to a parameter | `Todo()` | open |
| 9 | Slices print as `0x0x...` | cosmetic | open |
| 10 | `main`'s return value is not the exit code | behavior | open, maybe intended |

---

## Fixed

### 1. Unifying the types of several value-less `break`s

`finalize_residual_block` merged the types of all `break`s into the block. A `break` without a value has a nil type, and
`eval_unify(nil, nil)` returns `UnifyResult_type_is_incomplete`, which hit the `Todo()` after the loop. Any block with
two or more value-less branches into it triggered this: a `while` with a `break` has the loop's own exit plus the
`break`.

```blu
while i < 10 {
  if i == 3 { break }
  i += 1
}
```

You fixed it: an incomplete result is now accepted when the merged type is nil.

### 2. `condbr` jumps to the wrong place when the else-arm starts with a constant

The `condbr` target was patched by looking up the first SIR instruction of the else-arm in `inst_map`. When that
instruction folds to a constant (here `as i32 comptime_int 1`), the entry is a **value** ref. `iref_to_instruction`
treated the value's key as an instruction offset.

```blu
sign: i32 = if text[i] == u8"L" do -1 else 1
```

```
condbr cond=%24 otherwise=%90   ; should be %27
br block=%20 i32 -1
br block=%20 i32 1
```

At runtime, the else path jumped into a block that had never been entered. That block's `br` then made
`scopes_pop_to` pop the whole scope stack → assert.

**Fix:** `Scope.residual_otherwise` stores the offset of the next residual instruction when the specializer reaches the
SIR `otherwise` offset. The `condbr` is patched with that offset.

### 3. `SIR_repeat` data decoded as `SRef`, but stored as a raw offset

`codegen.c:791` stores the loop offset as a raw number. The specializer decoded it with `sref_to_instruction`. This worked
by accident, because masking out the flag bit didn't change a raw offset. The new `ASSERT_SAFE` in `ref.h` exposed it.

**Fix:** the specializer reads `sir_chunk_data(...)` directly, like `print.c` does.

### 4. Constant `condbr` compares value keys instead of contents

```c
if (key_eq(v, ctx->common->val._true))
```

`resolve()` returns a *copy* of the value with a new key, so a constant condition never compared equal to the shared
`true` value and always took `otherwise`. As a result, `while true { ... }` dropped its body entirely.

```blu
i: i32 = 0
while true {
  i += 1
  if i > 5 { break }
}
#debug(i) // printed 0, expected 6
```

**Fix:** read the bool from the value's data, the way `SIR_not` already does.

---

## Open

### 5. `SIR_br` does not skip the instructions after it

Both paths of `SIR_br` end with `s->pc += 1`. Whatever follows a `br` in the same scope is still specialized.

`condbr` only hides this when the condition is **false**: it jumps to `otherwise`, so the then-arm is skipped. With
**true** it does `pc += 1`, runs the then-arm, its `br` does `pc += 1`, and the else-arm is specialized too.

| Case | Result |
|------|--------|
| `if false do "no" else 1` | ok, the dead arm is skipped |
| runtime `if true do 1 else "no"` | type error on the dead `"no"`. Code is generated for the dead arm, but the output is correct |
| comptime `N : i32 = if true do 1 else 2` | **`N == 2`**, because the else `br` overwrites the stored value |
| comptime labeled break (below) | **`N == 20`**, expected 10 |

```blu
N : i32 = ::o {
  if true {
    break ::o 10
  }
  20
}
```

```
 11 | block count=2
 12 |   br block=%6 comptime_int 10   ; break ::o 10
 13 | br block=%7 %11                 ; still runs
 14 | br block=%7 _
 15 | as %1 comptime_int 20           ; still runs, overwrites 10
```

#### Fix options

**A. Unwind in `SIR_br` (comptime).** After a comptime `br`, set `pc = end` on every scope up to and including the
target block. This fixes both wrong values. Runtime `br` can keep `pc += 1`: dead code after a runtime `break` is
harmless.

**B. Arm scopes for `condbr`.** With a constant condition, push one scope for the active arm. With a runtime condition,
push two (else first, so the then-arm is on top). This gives if/else the right structure, keeps the dead arm from being
type-checked at runtime, and is where `residual_otherwise` naturally gets recorded. **It does not fix the labeled-break
case**, because the instructions after the `break` inside the arm still run.

Pitfalls of B:

- `find_scope` matches on `start` and scans from the bottom of the stack. If the then-arm starts with a `block` and the
  arm scope uses `start = condbr + 1`, both scopes share a `start` and the arm is returned. Set the arm's `pc` explicitly
  instead of deriving it from `start`.
- Use a separate `Scope_arm`, or a `Scope_block` with `residual = 0` and no `condbr`, so `finalize_residual_block` and
  the `condbr` patch-up never touch it.

**Recommendation:** A + B. A alone is the smallest change that fixes the wrong values. The runtime dead-arm
type-checking then remains.

### 6. Runtime `comptime_int` has no size

```blu
#debug(1)
#debug({ 2 })
#debug(if c do 1 else 2)
```

→ `Reach unreachable code` in `types_size_info` (`src/types.c:201`). A literal without a target type ends up as a
runtime value of type `comptime_int`. Assigning it to a typed variable works.

**Decision needed:** default such literals to a concrete type (`i32` / `i64`), or report an error when a `comptime_int`
reaches runtime.

### 7. Folding binary arithmetic on two constants

`src/specialize.c` (~1674, the `SIR_add`/`sub`/`mul`/`div`/`mod`/bit-op case): when both operands are values, it hits
`Todo()`.

```blu
N : i32 = 6 * 7
#debug(-7 / 2)
```

### 8. Assigning to a parameter

`gen_code_for_ptr` → `LOOKUP_PARAM: Todo()` (`src/codegen.c:127`).

```blu
gcd := |a: i32, b: i32|: i32 {
  while b != 0 {
    t: i32 = a % b
    a = b
    b = t
  }
  a
}
```

### 9. Slices print as `0x0x...`

`print.c:85` adds `0x` in front of `%p`, which already prints `0x` on macOS.

### 10. `main`'s return value is not the exit code

`main := ||: i32 { 42 }` exits with code 0. `main.c` only returns `ok ? 0 : 1`. This may be intended.

---

## Works

Recursion (`fib`), iterative loops with `i64`, labeled blocks (constant and runtime), `else if` chains, `while` with
`break`, nested loops with `if`/`else` inside, short-circuit `and`/`or` (side effects are skipped correctly), shadowing
in the same scope and the error for shadowing in a nested scope, runtime overflow errors, and the AoC sample (`3` / `6`).

## Note

On the first run, the `and`/`or` program printed wrong values. I couldn't reproduce it afterwards: the output was
identical across 40 runs, also with fix 4 reverted. The most likely cause is that the binary was rebuilt between runs
during the session.
