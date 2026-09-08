Current problem is that some `as` instructions in a function body try to reference
the return type of a function.
When the body is being specialized the signature is no longer in scope and thus the param types
and return type are also out of scope.
First, I am introducing a separate `function_signature` instruction.
This may also be used for function type definitions.
A `func` will always be followed by a function signature and then the body.
The body will always start with a `return_type` instruction followed by `param` instructions.

## Changing type coercion

Eventually I want to remove most of these `as` instructions and explicit type checks in instructions.
They are there currently because I am pushing down type information, but if you don't do this,
like Rust or Zig, you can remove all those instructions too.
I do think that blocks will need to get an extra `result_type` field that 'pushes down' the expected
type, such that you can write `x : i32 = if y do 1 else 0`.

Where the result type comes from will depend on the kind of scope.

For example, for a function signature each item is a type, either for the return type or a parameter type.
Here the result type comes from what we are evaluating.

When specializing a function, the signature will have been evaluated before.
Somewhere in the specializer the type of the function can be stored and used where necessary.

When specializing a top level declaration first the declared type will be evaluated.
The result can then be stored somewhere in the specializer and used.
This probably means that top level declarations will need a unique scope kind.


## get_decl_value + get_decl_ptr

## Specializer step result codes

Currently the specializer can return `resolve_declaration_value` and `register_function` among other results.
The `resolve_declaration_value` results in a new entry being pushed on the primary stack of the resolver and
the `register_function` pushes an entry on the deferred stack.
Mechanically speaking it makes more sense to have a `resolve_now` and `resolve_deferred` result.
These than carry a payload that can refer to either a declaration or a function.

On the other hand, deferring the resolution of a declaration does not make sense?
I don't know in what context that would happen.

`resolve_now` with a function can happen if you want to inline a function inside another function (at call site).

- Nicer handling of the optional fields on the scope in specialize.h
