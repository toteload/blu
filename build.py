#!/usr/bin/env python3

import ninja_syntax as n
from subprocess import run
import sys
import os
import platform

join = os.path.join

def exe(name):
    if platform.system() == 'Windows':
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
        name    = 'compile_c_debug',
        depfile = '$out.d',
        command = ' '.join((
                  'clang',
                  '-MD -MF $out.d',
                  '-Wall -Wextra',
                  #'-O2 -S -mllvm --x86-asm-syntax=intel',
                  '-g -gcodeview',
                  '-fansi-escape-codes -fcolor-diagnostics',
                  '-march=native',
                  '-Wno-unused-parameter',
                  '$cflags',
                  '-c',
                  '$in',
                  '-o $out',
                  ))
        )

    out.rule(
        name = 'build_binary',
        command = 'clang -g $in -o $out',
        )

    out.build(
        outputs   = outd('main.o'),
        rule      = 'compile_c_debug',
        inputs    = join('src', 'main.c'),
        variables = {
            'cflags': '',
        },
        )

    out.build(
        outputs   = outd('toteload.o'),
        rule      = 'compile_c_debug',
        inputs    = join('src', 'toteload.c'),
        variables = {
            'cflags': '',
        },
        )

    out.build(
        outputs   = outd('tokenize.o'),
        rule      = 'compile_c_debug',
        inputs    = join('src', 'tokenize.c'),
        variables = {
            'cflags': '',
        },
        )

    out.build(
        outputs   = outd('parse.o'),
        rule      = 'compile_c_debug',
        inputs    = join('src', 'parse.c'),
        variables = {
            'cflags': '',
        },
        )

    out.build(
        outputs   = outd('keke_helpers.o'),
        rule      = 'compile_c_debug',
        inputs    = join('src', 'keke_helpers.c'),
        variables = {
            'cflags': '',
        },
        )

    out.build(
        outputs   = outd('value_store.o'),
        rule      = 'compile_c_debug',
        inputs    = join('src', 'value_store.c'),
        variables = {
            'cflags': '-Iext',
        },
        )

    out.build(
        outputs   = outd('string_interner.o'),
        rule      = 'compile_c_debug',
        inputs    = join('src', 'string_interner.c'),
        variables = {
            'cflags': '-Iext',
        },
        )

    out.build(
        outputs   = outd('ir.o'),
        rule      = 'compile_c_debug',
        inputs    = join('src', 'ir.c'),
        variables = {
            'cflags': '',
        },
        )

    out.build(
        outputs   = outd('typecheck.o'),
        rule      = 'compile_c_debug',
        inputs    = join('src', 'typecheck.c'),
        variables = {
            'cflags': '',
        },
        )

    out.build(
        outputs   = outd('value_check.o'),
        rule      = 'compile_c_debug',
        inputs    = join('src', 'value_check.c'),
        variables = {
            'cflags': '',
        },
        )

    out.build(
        outputs   = outd('typeresolve.o'),
        rule      = 'compile_c_debug',
        inputs    = join('src', 'typeresolve.c'),
        variables = {
            'cflags': '',
        },
        )

    out.build(
        outputs = outd(exe('keke')),
        rule = 'build_binary',
        inputs = [
            outd('main.o'),
            outd('tokenize.o'),
            outd('parse.o'),
            outd('string_interner.o'),
            outd('value_store.o'),
            outd('keke_helpers.o'),
            outd('ir.o'),
            outd('typecheck.o'),
            outd('typeresolve.o'),
            outd('value_check.o'),
            outd('toteload.o'),
        ],
        )

    toteload_test(out)
    hashmap_test(out)
    string_interner_test(out)
    hashmap_fuzz_test(out)

def toteload_test(out):
    out.build(
        outputs   = outd('toteload.test.o'),
        rule      = 'compile_c_debug',
        inputs    = join('test', 'toteload.test.c'),
        variables = {
            'cflags': '-Isrc',
        },
        )

    out.build(
        outputs = outd(exe('toteload.test')),
        rule = 'build_binary',
        inputs = [
            outd('toteload.test.o'),
            outd('toteload.o'),
        ],
        )

def string_interner_test(out):
    out.build(
        outputs = outd('string_interner.test.o'),
        rule    = 'compile_c_debug',
        inputs  = join('test', 'string_interner.test.c'),
        variables = {
            'cflags': '-Isrc',
        },
        )

    out.build(
        outputs = outd(exe('string_interner.test')),
        rule = 'build_binary',
        inputs = [
            outd('string_interner.test.o'),
            outd('string_interner.o'),
            outd('toteload.o'),
        ],
        )

def hashmap_test(out):
    out.build(
        outputs = outd('hashmap.test.o'),
        rule    = 'compile_c_debug',
        inputs  = join('test', 'hashmap.test.c'),
        variables = {
            'cflags': '-Isrc',
        },
        )

    out.build(
        outputs = outd(exe('hashmap.test')),
        rule = 'build_binary',
        inputs = [
            outd('hashmap.test.o'),
            outd('toteload.o'),
        ],
        )

def hashmap_fuzz_test(out):
    out.build(
        outputs   = outd('hashmap.fuzz.o'),
        rule      = 'compile_c_debug',
        inputs    = join('test', 'hashmap.fuzz.c'),
        variables = {
            'cflags': '-Isrc -Iext',
        },
        )
    
    out.build(
        outputs = outd(exe('hashmap.fuzz')),
        rule = 'build_binary',
        inputs = [
            outd('hashmap.fuzz.o'),
        ],
        )

if __name__ == '__main__':
    create_build_ninja()
    run(['ninja'] + sys.argv[1:])
