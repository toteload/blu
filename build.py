#!/usr/bin/env python3

import ninja_syntax as n
from subprocess import run, call
import sys
import os
import platform

join = os.path.join

is_macos = platform.system() == 'Darwin'
is_windows = platform.system() == 'Windows'

USE_SANITIZERS = False

def exe(name):
    if is_windows:
        return f'{name}.exe'
    else:
        return name

def outd(p):
    return join('$outdir', p.replace('\\', '__'))

def add_test_suite(out):
    inputs = [
        'main_test.c',
        'test.c',
        'tokenize.test.c',
        'parse.test.c',
        'messages.test.c',
    ]

    outputs = []
    outputs.append(outd('toteload.c.o'))
    outputs.append(outd('tokenize.c.o'))
    outputs.append(outd('parse.c.o'))
    outputs.append(outd('messages.c.o'))
    outputs.append(outd('types.c.o'))

    for f in inputs:
        fout = outd(f'{f}.o')
        outputs.append(fout)
        out.build(
            outputs   = fout,
            rule      = 'compile_c_debug',
            inputs    = join('test', f),
            variables = {
                'cflags': '-Ivendor -Isrc',
            },
            )

    out.build(
        outputs = outd(exe('blu.test')),
        rule    = 'build_binary',
        inputs  = outputs,
        )

def create_build_ninja():
    fout = open('build.ninja', 'w')
    out = n.Writer(fout)

    out._line('builddir = .ninja')

    out.variable(
        key   = 'outdir',
        value = 'out',
        )

    out.rule(
        name    = 'compile_c_debug',
        depfile = '$out.d',
        command = ' '.join([
                  'clang',
                  '-MD -MF $out.d',
                  '-Wall -Wextra -Wpedantic',
                  '-Wsign-conversion',
                  '-Wimplicit-function-declaration',
                  '-Wno-unused-function',
                  '-Werror=switch', # Enforce all enum values are handled
                  '-Werror=incompatible-pointer-types',
                  #'-O2 -S -mllvm --x86-asm-syntax=intel',
                  '-fansi-escape-codes -fcolor-diagnostics',
                  '-std=c11',
                  '-march=native',
                  '-DTTLD_DEBUG',
                  '-g',
                  '-gcodeview -D_CRT_SECURE_NO_WARNINGS' if is_windows else '',
                  '-fsanitize=address,undefined' if USE_SANITIZERS else '',
                  '$cflags',
                  '-c',
                  '$in',
                  '-o $out',
                  ])
        )

    out.rule(
        name = 'build_binary',
        command = ' '.join([
            'clang',
            '-g',
            '-fsanitize=address,undefined' if USE_SANITIZERS else '',
            '$in',
            '-o $out']),
        )

    inputs = [
        'main.c',
        'toteload.c',
        'tokenize.c',
        'string_interner.c',
        'parse.c',
        'types.c',
        'value.c',
        'messages.c',
        'eval.c',
        'source_file.c',
        'compiler.c',
        'cli_options.c',
        'codegen.c',
        'specialize.c',
        'interpret.c',
        'ir.c',
        'print.c',
        'util.c',
    ]

    outputs = []

    for f in inputs:
        fout = outd(f'{f}.o')
        outputs.append(fout)
        out.build(
            outputs   = fout,
            rule      = 'compile_c_debug',
            inputs    = join('src', f),
            variables = {
                'cflags': '-Ivendor -Isrc',
            },
            )

    out.build(
        outputs = outd(exe('blu')),
        rule    = 'build_binary',
        inputs  = [outd(f'{f}.o') for f in inputs],
        )

    add_test_suite(out)

if __name__ == '__main__':
    create_build_ninja()
    run(['ninja'] + sys.argv[1:])
    comp_commands = open('compile_commands.json', 'w')
    call(['ninja', '-t', 'compdb', 'compile_c_debug'], stdout=comp_commands)
