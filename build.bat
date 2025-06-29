@echo off

set options=-Wall -Werror -g -gcodeview -c -std=c++17 -D_CRT_SECURE_NO_WARNINGS -DTTLD_DEBUG -Iext

clang++ %options% -o tokenize.o src\tokenize.cc
clang++ %options% -o parse.o src\parse.cc
clang++ %options% -o string_interner.o src\string_interner.cc
clang++ %options% -o type_interner.o src\type_interner.cc
clang++ %options% -o c_generator.o src\c_generator.cc
clang++ %options% -o toteload.o src\toteload.cc
clang++ %options% -o types.o src\types.cc
clang++ %options% -o main.o src\main.cc
clang++ -g -o blu.exe main.o string_interner.o tokenize.o parse.o c_generator.o toteload.o type_interner.o types.o

clang++ -Wall -Werror -g -gcodeview -std=c++17 -Isrc -o hashmap.test.exe test\hashmap.test.cc
