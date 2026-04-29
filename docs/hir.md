# HIR (High level Intermediate Representation)

Programs get converted from an AST to HIR.
HIR then gets converted to C.

Use of variables in HIR works the same as in SSA; assignments are only used once.
Control flow is different however does not use basic blocks.

Below is an example where a block returns a value.
This may have come from code like `x := if true { 123 } else { 456 }`.

```
 0 | block
 1 | condbr 1, %2, %3
 2 | break (%0, 123)
 3 | break (%0, 456)
```

## A small example

Take as a small example the program below.
It contains a single function that returns 0.

```
main: (): i32 = () { 0 }
```

```
 0 | declaration (param_count=0, return_type=%1, instruction_count=?)
 1 | type_int (32)
 2 | literal_int (0)
 3 | return %2
```

## Compiler notes

A `Ref` in HIR can refer to either a HIR instruction or a stored value.
