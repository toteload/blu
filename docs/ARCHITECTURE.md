# Architecture

This document describes how the compiler is build up and the process of compilation done by the compiler.

All of the state, with the exception of the CLI options, is in `Compiler` (`compiler.h`) and all compilation work is done with associated `compiler_*` functions.
First, all source file names are registered in the `Compiler`.
Then for each source file its contents are read, tokenized, parsed and its declarations are indexed.
If any of the source files have errors they are reported and compilation stops.

Data for a source file is stored in a `Source` (`source_file.h`).
A `Source` owns an arena in which the file contents, the tokens, the ast, and any diagnostic messages that may have been produced while processing the source.
While not yet implemented, the initial processing of each individual source file can be parallelized.

Next, a global declaration map is produced by combining the indexed declarations of all the source files.
This stage also makes sure that no declarations are defined more than once.
For each declaration specializer intermediate representation (SIR) code is generated, which will be executed by the specializer.
The specializer is an interpreter that executes SIR resulting either in compile time computed values or residual instructions.
Residual instructions are interpreter intermediate representation (IIR) and is the runtime program.
The order in which SIR code of each declaration is executed is determined by the resolver.
The resolver iterates over all the declarations and executes their SIR code.
If a declaration `A` depends on the value of another declaration `B`, the evaluation of `A` is suspended and `B` is first evaluated.
At this stage circular dependencies are detected.

> [!NOTE]
> At the moment IIR is the last format the program will be in and it is interpreted to execute the program.
> In the future IIR can be translated to binary or to LLVM IR to compile the program with LLVM.

