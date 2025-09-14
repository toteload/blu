#!/usr/bin/env python3

import ninja_syntax as n
from subprocess import run
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
        name    = 'compile_cpp_debug',
        depfile = '$out.d',
        command = ' '.join([
                  'clang++',
                  '-MD -MF $out.d',
                  '-Wall -Wextra',
                  '-Wno-unused-parameter',
                  #'-O2 -S -mllvm --x86-asm-syntax=intel',
                  '-fansi-escape-codes -fcolor-diagnostics',
                  '-march=native -std=c++17',
                  '-DTTLD_DEBUG',
                  '-g' if is_macos else '',
                  '-g -gcodeview -D_CRT_SECURE_NO_WARNINGS' if is_windows else '',
                  '$cflags',
                  '-c',
                  '$in',
                  '-o $out',
                  ])
        )

    out.rule(
        name = 'build_binary',
        command = 'clang++ -g $in -o $out',
        )

    inputs = [
        'main.cc',
        'toteload.cc',
        'tokenize.cc',
        'parse.cc',
        'type_interner.cc',
        'string_interner.cc',
        'types.cc',
        'typecheck.cc',
        'message_manager.cc',
        join('tools', 'tokenviewer.cc'),
        join('utils', 'stdlib.cc'),
    ]

    outputs = []

    for f in inputs:
        fout = outd(f'{f}.o')
        outputs.append(fout)
        out.build(
            outputs   = fout,
            rule      = 'compile_cpp_debug',
            inputs    = join('src', f),
            variables = {
                'cflags': '-Iext -Isrc',
            },
            )

    out.build(
        outputs = outd(exe('blu')),
        rule    = 'build_binary',
        inputs  = [outd(f'{f}.o') for f in [
                'main.cc',
                'toteload.cc',
                'tokenize.cc',
                'parse.cc',
                'type_interner.cc',
                'string_interner.cc',
                'types.cc',
                'typecheck.cc',
                'message_manager.cc',
                join('utils', 'stdlib.cc'),
        ]],
        )

    out.build(
        outputs = outd(exe('tokenviewer')),
        rule    = 'build_binary',
        inputs  = [outd(f'{f}.o') for f in [
                'toteload.cc',
                'tokenize.cc',
                join('tools', 'tokenviewer.cc'),
                join('utils', 'stdlib.cc'),
        ]],
        )

if __name__ == '__main__':
    create_build_ninja()
    run(['ninja'] + sys.argv[1:])
