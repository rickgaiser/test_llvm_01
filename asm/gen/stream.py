#!/usr/bin/env python3
"""Generates the STREAM kernels (stream_tests.c) with parameters, their
loops ordered by sched.py. Used by the PS2 harness of build/ps2-hw-test/src/
hand/stream.c to measure variants, and for asm/stream_tests.S.

  sum(name, D, U, mmi)            sum of the ints, U lines per iteration,
                                  PREF D lines ahead (0: none)
  scale(name, D, U, wpref)        out = in * 3 + 1 with MMI; wpref: PREF
                                  of the output too
  saxpy_fpu(name, D, U)           y = a * x + y on the FPU, a in $f15
  saxpy_vu0(name, D, U)           the same with VU0
Every kernel takes the C arguments; count is a multiple of 16 * U."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sched
from fpu_novu0 import loop

EST = {}


def emit(name, comment, pre, body, branch, post):
    lp = loop(body, branch)
    from fpu_novu0 import EST as E
    EST[name] = E[-1]
    return (f'\n{comment}\n\t.p2align 3\n\t.globl\t{name}\n\t.ent\t{name}\n{name}:\n' +
            ''.join(f'\t{l}\n' for l in pre) + lp +
            ''.join(f'{l}\n' for l in post) + f'\t.end\t{name}\n')


def sum_(name, D, U, mmi):
    pre, body = [], []
    if D:
        body += [f'pref\t0, {64 * (D + u)}($4)' for u in range(U)]
    if mmi:
        # four word sums in $8 (MMI), added at the end
        pre = ['blez\t$5, 2f', 'pcpyld\t$8, $0, $0']
        regs = ['$9', '$10', '$11', '$12', '$13', '$14', '$15', '$24']
        for k in range(4 * U):
            body.append(f'lq\t{regs[k % 8]}, {16 * k}($4)')
            body.append(f'paddw\t$8, $8, {regs[k % 8]}')
        # the four words, each sign-extended as addu requires
        post = ['2:\tpcpyud\t$9, $8, $8', '\tsll\t$10, $8, 0', '\tdsra32\t$11, $8, 0',
                '\tsll\t$12, $9, 0', '\tdsra32\t$13, $9, 0', '\taddu\t$2, $10, $11',
                '\taddu\t$3, $12, $13', '\tjr\t$ra', '\taddu\t$2, $2, $3']
        how = 'lq and MMI paddw into four word sums'
    else:
        pre = ['move\t$2, $0', 'blez\t$5, 2f', 'move\t$3, $0']
        regs = ['$8', '$9', '$10', '$11', '$12', '$13', '$14', '$15']
        for k in range(16 * U):
            body.append(f'lw\t{regs[k % 8]}, {4 * k}($4)')
            body.append(f'addu\t{"$2" if k % 2 == 0 else "$3"}, {"$2" if k % 2 == 0 else "$3"}, {regs[k % 8]}')
        post = ['2:\tjr\t$ra', '\taddu\t$2, $2, $3']
        how = 'lw and addu into two sums'
    body += [f'addiu\t$4, $4, {64 * U}', f'addiu\t$5, $5, -{16 * U}']
    return emit(name, f'# {name}: {how}, {U} line(s) per iteration, PREF {D} lines ahead',
                pre, body, 'bgtz\t$5, 1b', post)


def scale(name, D, U, wpref):
    pre = ['blez\t$6, 2f', 'addiu\t$3, $0, 1', 'dsll32\t$3, $3, 0', 'ori\t$3, $3, 1',
           'pcpyld\t$3, $3, $3']        # 1 in each word
    body = []
    if D:
        body += [f'pref\t0, {64 * (D + u)}($4)' for u in range(U)]
        if wpref:
            body += [f'pref\t0, {64 * (D + u)}($5)' for u in range(U)]
    regs = ['$8', '$9', '$10', '$11', '$12', '$13', '$14', '$15']
    for k in range(4 * U):
        r, t = regs[(2 * k) % 8], regs[(2 * k + 1) % 8]
        body += [f'lq\t{r}, {16 * k}($4)', f'psllw\t{t}, {r}, 1', f'paddw\t{t}, {t}, {r}',
                 f'paddw\t{t}, {t}, $3', f'sq\t{t}, {16 * k}($5)']
    body += [f'addiu\t$4, $4, {64 * U}', f'addiu\t$6, $6, -{16 * U}', f'addiu\t$5, $5, {64 * U}']
    return emit(name, f'# {name}: lq, psllw, paddw x2, sq; {U} line(s) per iteration, PREF {D} lines ahead{" (also the output)" if wpref else ""}',
                pre, body, 'bgtz\t$6, 1b', ['2:\tjr\t$ra', '\tnop'])


def saxpy_fpu(name, D, U):
    pre = ['blez\t$6, 2f', 'nop']
    body = []
    if D:
        body += [f'pref\t0, {64 * (D + u)}($4)' for u in range(U)]
        body += [f'pref\t0, {64 * (D + u)}($5)' for u in range(U)]
    F = [f'$f{i}' for i in range(15)] + ['$f16', '$f17', '$f18', '$f19']
    for k in range(16 * U):
        x, y = F[(2 * k) % 18], F[(2 * k + 1) % 18]
        body += [f'lwc1\t{x}, {4 * k}($4)', f'lwc1\t{y}, {4 * k}($5)', f'mul.s\t{x}, {x}, $f15',
                 f'add.s\t{y}, {y}, {x}', f'swc1\t{y}, {4 * k}($5)']
    body += [f'addiu\t$4, $4, {64 * U}', f'addiu\t$6, $6, -{16 * U}', f'addiu\t$5, $5, {64 * U}']
    return emit(name, f'# {name}: FPU mul.s and add.s per element (5 COP1 slots), {U} line(s) per iteration, PREF {D} lines ahead',
                pre, body, 'bgtz\t$6, 1b', ['2:\tjr\t$ra', '\tnop'])


def saxpy_vu0(name, D, U):
    pre = ['blez\t$6, 2f', 'mfc1\t$8, $f15', 'qmtc2\t$8, $vf20']   # a in vf20.x
    body = []
    if D:
        body += [f'pref\t0, {64 * (D + u)}($4)' for u in range(U)]
        body += [f'pref\t0, {64 * (D + u)}($5)' for u in range(U)]
    for k in range(4 * U):
        x, y = f'$vf{1 + 2 * (k % 8)}', f'$vf{2 + 2 * (k % 8)}'
        body += [f'lqc2\t{x}, {16 * k}($4)', f'lqc2\t{y}, {16 * k}($5)',
                 f'vmulx.xyzw\t{x}, {x}, $vf20x', f'vadd.xyzw\t{y}, {y}, {x}',
                 f'sqc2\t{y}, {16 * k}($5)']
    body += [f'addiu\t$4, $4, {64 * U}', f'addiu\t$6, $6, -{16 * U}', f'addiu\t$5, $5, {64 * U}']
    return emit(name, f'# {name}: VU0 vmulx (a broadcast) and vadd, {U} line(s) per iteration, PREF {D} lines ahead',
                pre, body, 'bgtz\t$6, 1b', ['2:\tjr\t$ra', '\tnop'])
