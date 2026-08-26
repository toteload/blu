# Blu

> [!IMPORTANT]
> I am still making big changes on the language so information here may be out of date.

Blu is a programming language that I am designing and building a compiler for.
The goal for the language is to make something that is in the same spirit as C; allow low level control and be a small language.
For a big part Blu is inspired by Zig: allowing code to be run at compile time, including code that produces types.
Through this single mechanism you can precompute stuff at compile time, implement things like generics and have reflection.

At the moment no machine code is generated - instead an IR is interpreted to run a program.
Eventually, I do want to produce machine code but for now my focus is on fleshing out the language.
I would like to implement code generation myself instead of relying on LLVM.

This is very much a personal, educational project, but with big goals to keep it interesting.

## How to run

Run `python build.py` to build the compiler.
You need `clang`, `ninja` and `python` installed to build.
It has only been tested on MacOS and Windows.

## The language

The language syntax is built around expressions: everything returns a value.
For example, the last expression in a block `{ ... }` is the value of the block (like in Rust).

Every declaration lives in a module.
Modules are the language's mechanism to organize code.
You declare what module you are in by writing `mod <name>` where `<name>` is the name of the module.
You can put multiple mod declarations in a single file.
You can write declarations in any order (no forward declarations necessary).

```
mod foo

T: type = i32
x: T = 12

mod main

main := ||: i32 { foo.x }
```

All modules are visible to all other modules in your program, so you don't have to explicitly import another module.
When compiling you have to tell the compiler which files are part of your program.

### Type inference

There is some type inference, but nothing fancy like Hindley-Milner.

## LLM usage

I use Claude Code to assist in debugging and analyzing the code.
Claude Code has generated some of the tests, after first writing an example test, generated the first version of printing IR and generated some of the more boilerplatey code, like the eval functions for integer operations.
I am experimenting with how to use LLMs in a way that works for me, so it is possible that in the future more code will be written with Claude.
