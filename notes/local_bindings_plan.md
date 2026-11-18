# Plan: immutable local bindings and type inference

## Semantics

- Locals are immutable by default, like globals. `mut` makes a local a storage slot.
- An immutable local is a name for a value: no `alloc`, no `store`, no `load`. Whether it is comptime
  follows from its value, because the specializer already folds known values.
  - `T := i32` is valid in a runtime function. Its uses fold away.
  - `x := 5` stays `comptime_int` and coerces at each use.
  - `s := a + b` names a runtime instruction.
  - A comptime-only value that reaches IIR is reported by `expect_runtime_types`, as today.
- A `mut` local in a runtime frame must have a runtime type. There is no defaulting: `mut x := 5` and
  `mut T := i32` are errors. In a comptime frame (a global's value block), any type is allowed.
- `a: T = v` on an immutable local coerces `v` to `T` with `SIR_as`.
- A declaration's name is in scope only after its value, so `x := x` refers to an outer `x`.
- Parameters and immutable locals are the same kind of entry ("binding").

## Steps

1. **Parser** (`parse.c`, expression `case Tok_identifier` around line 797): add `case Tok_keyword_mut`,
   which parses a declaration. `parse_declaration` already reads `mut`.

2. **Symbol map** (`symbol_map.h`):
   - `ScopeEntry.inst` becomes `SRef ref`. Scope entries (block, loop, function) store
     `sref_from_instruction`.
   - `ScopeEntry_param` becomes `ScopeEntry_binding`. `ScopeEntry_local` is a `mut` slot.

3. **Codegen lookup** (`codegen.c:55-101`):
   - `LOOKUP_PARAM` becomes `LOOKUP_BINDING`. `LookupResult.data` gets `SRef ref`.
   - Reading an identifier: `LOOKUP_BINDING` returns `ref` directly. `LOOKUP_LOCAL` emits `load`.
   - `gen_code_for_ptr`: `LOOKUP_BINDING` reports "Cannot assign to an immutable variable" (for both
     parameters and locals).

4. **Codegen `Ast_declaration`**, in this order:
   1. `type` = the declared type if present.
   2. `value` = `gen_code(value, type)`.
   3. Immutable: if typed, `value = SIR_as(type, value)`. Push a `ScopeEntry_binding` with `value`.
   4. `mut`: if untyped, `type = SIR_typeof(value)`. Emit `alloc type`, `store`, then push a
      `ScopeEntry_local` with the alloc.

5. **Specializer `SIR_alloc`** (`specialize.c:1487`): in a runtime frame, a comptime-only type is an
   error. This replaces the `TODO type may not be comptime` comment. Proposed message:
   "A mutable variable at runtime cannot have comptime-only type %type".

6. **Existing snippets**:
   - Add `mut` to reassigned locals in `local`, `while`, `nested_control_flow`, `usize_arithmetic` and
     `comptime_local`. Their snapshots should not change.
   - The message in `error/assign_to_param.blu.snap` changes.

7. **New snippets**:
   - `basic/infer_local`:
     - `x := 5` used as `a: i32 = x + 1`
     - `T := i32` then `b: T = 3`
     - `s := p + q` with `i32` parameters
     - `mut m := s` with `m += 1`
   - `basic/comptime_infer_local`: a global block that uses `mut a := 1`, `a = a + 2`, `T := i64`
     and `b: T = a`.
   - `error/assign_immutable_local`
   - `error/mut_comptime_int_at_runtime`: `mut x := 5` in a function.
   - `error/mut_type_at_runtime`: `mut T := i32` in a function.
   - `error/binding_self_reference`: `x := x` gives "Undeclared name".

8. **Notes**: remove the `TODO.md` item, and the "Known bugs" line and the immutable-locals question
   from `session_2026-11-17.md`.

## Out of scope

- The `comptime` keyword (`comptime mut T := i32`, and `comptime x := f()` as a requirement).
- `&x` on an immutable binding (it will need storage created on demand).
- Defaulting `comptime_int`.
