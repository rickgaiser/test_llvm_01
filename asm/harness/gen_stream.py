#!/usr/bin/env python3
"""Generates streamv.S and streamv.h for stream.c: variants of the STREAM
kernels (asm/gen/stream.py) and feature clang's, to measure on the PS2.
Build stream.c with clang -mabi=n32 like the bench and link with streamv.o;
run with the ps2-hw-test runner."""
import sys, re
import os
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'gen'))
# usage: gen_stream.py <feature build dir of the bench> <output dir>
FEATURE, OUT = sys.argv[1], sys.argv[2]
import stream
specs = []
for U in (1, 2):
    for Dd in (0, 3, 4, 5, 6, 8):
        specs.append(('sum', f'sum_mmi_u{U}_d{Dd}', lambda n, Dd=Dd, U=U: stream.sum_(n, Dd, U, True)))
for Dd in (4, 5):
    specs.append(('sum', f'sum_gpr_d{Dd}', lambda n, Dd=Dd: stream.sum_(n, Dd, 1, False)))
for U in (1, 2):
    for Dd in (0, 4, 5, 6, 8):
        for w in (0, 1):
            if Dd == 0 and w: continue
            specs.append(('scale', f'scale_u{U}_d{Dd}_w{w}', lambda n, Dd=Dd, U=U, w=w: stream.scale(n, Dd, U, w)))
for Dd in (0, 4, 5, 6):
    specs.append(('saxpy', f'saxpy_fpu_d{Dd}', lambda n, Dd=Dd: stream.saxpy_fpu(n, Dd, 1)))
for U in (1, 2):
    for Dd in (0, 4, 5, 6):
        specs.append(('saxpy', f'saxpy_vu0_u{U}_d{Dd}', lambda n, Dd=Dd, U=U: stream.saxpy_vu0(n, Dd, U)))
out = ['\t.set noreorder', '\t.set noat', '\t.text']
for kind, n, f in specs:
    out.append(f('v_' + n))
# feature clang's kernels, renamed
s = open(os.path.join(FEATURE, 'stream_tests.s')).read()
for fn in ('stream_sum', 'stream_scale', 'stream_saxpy', 'stream_sum_pref', 'stream_scale_pref', 'stream_saxpy_pref'):
    a = s.index(f'\n{fn}:'); b = s.index(f'.end\t{fn}', a)
    body = [l for l in s[a:b].split('\n')[2:] if l.strip() and not re.match(r'^\s*\.(cfi|loc|file|frame|mask|fmask|set|size|type)', l)]
    body = [re.sub(r'\.(LBB|LCPI)(\d+_\d+)', lambda m: f'.{m.group(1)}{m.group(2)}_{fn}', l.split('#')[0].rstrip()) for l in body]
    out += ['\t.p2align 6', f'\t.globl c_{fn}', f'c_{fn}:'] + [l for l in body if l.strip()]
    kind = fn.split('_')[1]
    specs.append((kind, 'feature_' + fn[7:], None))
open(os.path.join(OUT, 'streamv.S'), 'w').write('\n'.join(out) + '\n')
h = []
for kind, n, f in specs:
    sym = ('v_' + n) if f else ('c_stream_' + n[8:])
    proto = {'sum': 'int %s(const int *, int);', 'scale': 'void %s(const int *, int *, int);',
             'saxpy': 'void %s(const float *, float *, int, float);'}[kind]
    h.append(proto % sym)
h.append('static const struct { int kind; const char *name; void *f; } variants[] = {')
for kind, n, f in specs:
    sym = ('v_' + n) if f else ('c_stream_' + n[8:])
    h.append(f'    {{{ {"sum":0,"scale":1,"saxpy":2}[kind] }, "{n}", (void *){sym}}},')
h.append('};')
open(os.path.join(OUT, 'streamv.h'), 'w').write('\n'.join(h) + '\n')
print(len(specs), 'variants', stream.EST)
