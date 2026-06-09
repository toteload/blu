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
                  '$cflags',
                  '-c',
                  '$in',
                  '-o $out',
                  ])
        )

    out.rule(
        name = 'build_binary',
        command = 'clang -g $in -o $out',
        )

    inputs = [
        'main.c',
        'toteload.c',
        'tokenize.c',
        'string_interner.c',
        'parse.c',
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

if __name__ == '__main__':
    create_build_ninja()
    run(['ninja'] + sys.argv[1:])
    comp_commands = open('compile_commands.json', 'w')
    call(['ninja', '-t', 'compdb', 'compile_c_debug'], stdout=comp_commands)
