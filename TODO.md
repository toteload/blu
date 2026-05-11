## The plan

Build an AST interpreter first.
Performance is not that important.
It is not compiled ahead of time.
All checks are performed at runtime.
You will still have to write the logic for what the language does, it will "just" be done at a different time compared to an AOT compiler.

### Basic

- [x] `test\basic\main.blu`
- [x] `test\basic\if_else.blu`
- [x] `test\basic\arithmetic.blu` 27-04-2026
- [x] `test\basic\slice.blu` 30-04-2026
  - Includes array type and string literal.
  - Includes coercion of sequence literals to typed slices.
  - Includes indexing of slices.
  - Includes coercion of array to slice.
- [x] `test\basic\print.blu`
  - Add `#print` as a builtin function. This builtin is not meant to stay, but can be used during development for debugging and getting some output.
- [x] `test\basic\defer.blu`
- [x] User functions
- [x] Add assign of variables.
- [x] `for` loop
- [ ] Add `const` qualifier.
- [ ] Add `return` expression.

### Metaprogramming

- [ ] Add `#run` builtin.

### Misc

- [ ] Properly free values that are no longer used in the interpreter.
- [ ] Currently `coerce_value` in `interpreter.cc` can fail if the value of the integer constant is too big for the destination type.
      This should be checked at an earlier stage. The coercion in the interpreter is expected to never fail.

### Errors

Maybe not implement this in the language itself.

- [ ] Add error type and add syntax for functions to return errors.
- [ ] Add `try` and `catch`.
- [ ] Add conditional `defer` based on whether the function returned with an error or not.

---

- [ ] Floating point type
- [ ] Pointer type. Is not allowed to be nil
- [ ] struct type

- [ ] Bounds checking
- [ ] Integer overflow checks

- [ ] Compile time code execution.

- [ ] Pattern matching (?)
