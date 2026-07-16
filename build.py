#!/usr/bin/env python3

import ninja_syntax as n
from subprocess import run, call
import sys
import os
import platform

join = os.path.join

is_macos = platform.system() == 'Darwin'
is_windows = platform.system() == 'Windows'

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
    ]

    outputs = []
    outputs.append(outd('toteload.c.o'))
    outputs.append(outd('tokenize.c.o'))
    outputs.append(outd('parse.c.o'))

    for f in inputs:
        fout = outd(f'{f}.o')
        outputs.append(fout)
        out.build(
            outputs   = fout,
            rule      = 'compile_c_debug',
            inputs    = join('test', f),
            variables = {
                'cflags': '-Iext -Isrc',
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
                  '-Wno-unused-parameter',
                  '-Wimplicit-function-declaration',
                  '-Werror=switch', # Enforce all enum values are handled
                  #'-O2 -S -mllvm --x86-asm-syntax=intel',
                  '-fansi-escape-codes -fcolor-diagnostics',
                  '-std=c11',
                  '-march=native',
                  '-DTTLD_DEBUG',
                  '-g',
                  '-gcodeview -D_CRT_SECURE_NO_WARNINGS' if is_windows else '',
                  #'-fsanitize=address,undefined',
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
            #'-fsanitize=address,undefined', 
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
        'env.c',
        'eval.c',
        'ir.c',
        'source_file.c',
        'compiler.c',
        'cli_options.c',
        'codegen.c',
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
                'cflags': '-Iext -Isrc',
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
