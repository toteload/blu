@echo off

clang-format -i src\toteload.hh
clang-format -i src\vector.hh
clang-format -i src\hashmap.hh
clang-format -i src\blu.hh
clang-format -i src\queue.hh
clang-format -i src\ast.hh
clang-format -i src\tokens.hh
clang-format -i src\types.hh

clang-format -i src\compiler.cc
clang-format -i src\tokenize.cc
clang-format -i src\parse.cc
clang-format -i src\main.cc
clang-format -i src\string_interner.cc
clang-format -i src\type_interner.cc
clang-format -i src\typecheck.cc
clang-format -i src\c_generator.cc
