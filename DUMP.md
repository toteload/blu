I just ran into a problem; types may be dependent on const code.
This means that you cannot first typecheck everything and then run const code.
You have to interleave the two where necessary.

Typecheck a bit of code, then evaluate it, which results in a type, which you can use to typecheck something else, etc...
I knew this, but I did not adapt my compiler to this. Why not?!?!
I feel stupid about this :( but not defeated. I can fix it.

1. tokenize
1. parse
1. typecheck + const evaluation
1. evaluate program

Wants:

- No implicit uninitialized variables.
- Static checking for overflows and out of bounds access with possible override using `assume` keyword.

## Comparison to other languages

### Comparison to C

- No surprising implicit conversion to 32-bit signed integers.
- Language provided defer. You don't have to rely on compiler-dependent extensions.
- A different model to code organization. No header files or includes.
- Primitive slice type.
- Checking for overflows and out of bounds access.

Eventually:

- Pointers must point to a value. No `NULL`.

### Comparison to Zig



## AST

Modelled as a SoA with the following fields per node:

1. A small 'tag' to denote what kind of node it is.
1. A span of token index, to denote which tokens this node was built from. This is non-owning and is just two integer indices.
1. A node kind specific payload.

### Compiler inserted casts for coercions

Blu allows certain implicit casts of values (type coercion).
However, internally these implicit casts are first made explicit by inserting the appropriate explicit casts into the AST.
This presents an issue: the AST is modelled as having a token span for each node, but these inserted nodes were not written explicitly and thus have no backing tokens.
A solution to this is to use a 0-length span to denote compiler inserted nodes.
The span can still point to a location in the token stream and may be used to point to the token location before which this code was inserted.

### Values inserted into AST by `eval`/const evaluation

During compilation certain expressions must be evaluated.
These expressions produce values that replace their expression in the AST.
The AST uses a special index/reference type to refer to other AST nodes, but this type may also point to values which enables this combination.

## Type checking, type inference and type coercion

Full type checking may not be possible without running code in Blu.
This means that type checking (and other semantic checks) are interleaved with evaluating code to produce the final program.

### Coercion of `const function`

For the coercion of a `const function` to a fully typed function one of the following must be true:

1. The place you are coercing the `const function` to has a known type.
1. The `const function` is fully annotated.

> Question: Are you allowed to bind a `const function` to a `const` variable?
>
> Answer: Yes, its type will be `const_function` or `function_literal` or something like that.
> It may not be used in this state, but it can be cast/coerced to a fully typed function at a later point. 

### Coercion of `const list`

...

## `eval`, compile time evaluation

At first, I had the idea of using the `const` keyword for both denoting const 'bindings' (identifiers whose value must be known at compile time and references to these values are inlined) and forcing the compile time evaluation of arbitrary expressions. 
However, this is not possible if you want to have `const` as a qualifier on types.
Example: `const a`, is this a type `a` with a `const` qualifier (meaning no values of this type are allowed at runtime) or is this an expression `a` that we want to evaluate at compile time?
This would make the meaning of `const` context-dependent, which I don't want.
A solution is to introduce another keyword: `eval`. 
`const` is always used as a qualifier on types or bindings.
`eval` is used to evaluate an expression at compile time.

## Types

_Concept_: A type may also be `const`.
This means that values of this type may only exist at compile time.
Builtin examples of this are `const_int` and `type`.
If a binding has a `const` type, then the binding is implicitly `const` itself, since it cannot exist at runtime due to its `const` type.

## Environments

Modelled as stacked hashmaps.
This mirrors how identifiers are defined in the language; identifiers in a narrower scope may shadow identifiers in a parent scope.
However, when we leave the scope the identifiers from the parent scope that were shadowed are now reachable again.

### Patterns in environments

The root environment will probably be the largest one since it contains all the builtin identifiers, like primitive types and values.
I want it to be impossible to shadow builtin identifiers.
To facilitate this you can use the same environment as the "builtin identifiers environment" and make replacement illegal.
This is convenient, because it automatically gives you detection for identical user-defined top-level identifiers: which are ambiguous and thus illegal.
As a consequence, the root environment will contain many items relative to block scope or function scope environments.
This presents an optimization opportunity.
