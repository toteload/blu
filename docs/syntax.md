# Syntax

## Root

The root contains declarations.

## Declaration

`<name> : <type> = <value>`

## Function expression

A function expression is similarly typed to a function type, but must start with a `.` and has no type annotations.
For example, a function that adds two `i32`s is declared like this.

The idea is to allow type annotations on function expression parameters and to also specify the return type on it.

```
add : (i32, i32): i32 = .(a, b) { a + b }
```
