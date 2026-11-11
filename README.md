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
