# Blu

```blu
mod main

main := ||: i32 {
  #debug(0)
}
```

## Syntax

```blu
// Examples for if-else

// Putting braces around the if- and else-value is optional, because `do` is used.
// If you use `do` then `else` must also be used.
msg : []u8 = if is_good_boi do "*pets*" else "No pets for you"

// `do` is not used, so braces are mandatory for the branches.
if big_problem {
  ...
} else {
  ...
}
```

```blu
// Blocks

// Blocks are used to introduce a new scope.
// Variables declared inside a scope are no longer available after the scope ends.
// The last expression of a block is returned.
// This way you can return values from a block.
// `break` can be used to return from a labeled block.
// `break` can also return a value.

// The last expression in the block (`1`) is returned.
one : i32 = { 1 }

// A label starts with `::`
two : i32 = ::b { break ::b 2 }

// Blocks are also the way to force a certain order of operations.
// Most languages use `(...)` instead for this, but Blu reserves parentheses for function calls.
a : i32 = {b + c} * d
```

## Variable scoping

```blu
mod main

main := ||: i32 {
  a : i32 = 0

  // variables may be overridden if they are in the same scope
  a : []u8 = "hello"

  {
    // error: a has already been declared in a parent scope
    a : bool = false
  }

  ...
}
```

## Builtins

`#len` can be used on arrays and slices and returns their size as a `usize`.

`#debug` can be used on any value and will print it to stdout and returns the same value again.
It is used for debugging and getting output.

