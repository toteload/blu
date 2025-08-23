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
    return join('$outdir', p)

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

    out.build(
        outputs   = outd('main.o'),
        rule      = 'compile_cpp_debug',
        inputs    = join('src', 'main.cc'),
        variables = {
            'cflags': '',
        },
        )

    out.build(
        outputs   = outd('toteload.o'),
        rule      = 'compile_cpp_debug',
        inputs    = join('src', 'toteload.cc'),
        variables = {
            'cflags': '',
        },
        )

    out.build(
        outputs   = outd('tokenize.o'),
        rule      = 'compile_cpp_debug',
        inputs    = join('src', 'tokenize.cc'),
        variables = {
            'cflags': '',
        },
        )

    out.build(
        outputs   = outd('parse.o'),
        rule      = 'compile_cpp_debug',
        inputs    = join('src', 'parse.cc'),
        variables = {
            'cflags': '',
        },
        )

    out.build(
        outputs   = outd('type_interner.o'),
        rule      = 'compile_cpp_debug',
        inputs    = join('src', 'type_interner.cc'),
        variables = {
            'cflags': '-Iext',
        },
        )

    #out.build(
    #    outputs   = outd('c_generator.o'),
    #    rule      = 'compile_cpp_debug',
    #    inputs    = join('src', 'c_generator.cc'),
    #    variables = {
    #        'cflags': '',
    #    },
    #    )

    out.build(
        outputs   = outd('string_interner.o'),
        rule      = 'compile_cpp_debug',
        inputs    = join('src', 'string_interner.cc'),
        variables = {
            'cflags': '-Iext',
        },
        )

    out.build(
        outputs   = outd('typecheck.o'),
        rule      = 'compile_cpp_debug',
        inputs    = join('src', 'typecheck.cc'),
        variables = {
            'cflags': '',
        },
        )

    out.build(
        outputs   = outd('types.o'),
        rule      = 'compile_cpp_debug',
        inputs    = join('src', 'types.cc'),
        variables = {
            'cflags': '',
        },
        )

    out.build(
        outputs   = outd('message_manager.o'),
        rule      = 'compile_cpp_debug',
        inputs    = join('src', 'message_manager.cc'),
        variables = {
            'cflags': '',
        },
        )

    out.build(
        outputs   = outd('compiler.o'),
        rule      = 'compile_cpp_debug',
        inputs    = join('src', 'compiler.cc'),
        variables = {
            'cflags': '',
        },
        )

    out.build(
        outputs = outd(exe('blu')),
        rule = 'build_binary',
        inputs = [
            outd('main.o'),
            outd('tokenize.o'),
            outd('parse.o'),
            outd('typecheck.o'),
            outd('string_interner.o'),
            outd('type_interner.o'),
            outd('types.o'),
            outd('toteload.o'),
            outd('message_manager.o'),
            #outd('c_generator.o'),
            outd('compiler.o'),
        ],
        )

if __name__ == '__main__':
    create_build_ninja()
    run(['ninja'] + sys.argv[1:])
