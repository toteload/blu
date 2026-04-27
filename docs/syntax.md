# Syntax

## Root

The root contains declarations.

## Declaration

`<name> : <type> = <value>`

## Function expression

The idea is to allow type annotations on function expression parameters and to also specify the return type on it.

```
add : (i32, i32): i32 = |a, b| { a + b }
```
