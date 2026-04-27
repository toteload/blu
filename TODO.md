## The plan

Build an AST interpreter first.
Performance is not that important.
It is not compiled ahead of time.
All checks are performed at runtime.
You will still have to write the logic for what the language does, it will "just" be done at a different time compared to an AOT compiler.

### Basic

- [x] `test\basic\main.blu`
- [x] `test\basic\if_else.blu`
  - [ ] Properly cast the literal int to i32 which is the return type of main.
- [ ] `test\basic\arithmetic.blu`
- [ ] `test\basic\slice.blu`
  - [ ] Also includes array type and string literal.
- [ ] `test\basic\print.blu`
- [ ] `test\basic\defer.blu`

### Errors

- [ ] Add error type and add syntax for functions to return errors.
- [ ] Add `try` and `catch`.
- [ ] Add conditional `defer` based on whether the function returned with an error or not.

### Metaprogramming

---

- [ ] `for` loop

---

- [ ] Floating point type
- [ ] Pointer type. Is not allowed to be nil
- [ ] struct type

- [ ] Bounds checking
- [ ] Integer overflow checks

- [ ] Compile time code execution.

- [ ] Pattern matching (?)
