#!/usr/bin/env python3
"""Prints the VU0 versions of vpu_dot_product and vpu_hmax_hmin, their loop
bodies ordered by sched.py, for the ASM_VU0 branch of asm/vpu_tests.S."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sched
from fpu_novu0 import loop, EST

def fn(name, comment, body):
    print(f'\n{comment.rstrip()}\n\t.p2align 3\n\t.globl\t{name}\n\t.ent\t{name}\n{name}:')
    print(body.rstrip('\n'))
    print(f'\t.end\t{name}')

# dot product: 4 vectors per iteration
body = ['pref\t0, 320($4)', 'pref\t0, 320($5)', 'pref\t0, 80($6)']
for j in range(4):
    a, b, r = f'$vf{1+2*j}', f'$vf{2+2*j}', f'$vf{11+j}'
    body += [f'lqc2\t{a}, {16*j}($4)', f'lqc2\t{b}, {16*j}($5)',
             f'vmul.xyzw\t{a}, {a}, {b}',
             f'vadday.x\t$ACC, {a}, {a}y', f'vmaddaz.x\t$ACC, $vf20, {a}z',
             f'vmaddw.x\t{r}, $vf20, {a}w', f'qmfc2\t${8+j}, {r}', f'sw\t${8+j}, {4*j}($6)']
body += ['addiu\t$4, $4, 64', 'addiu\t$5, $5, 64', 'addiu\t$7, $7, -4', 'addiu\t$6, $6, 16']
lp = loop(body, 'bgtz\t$7, 1b')
fn('vpu_dot_product', f'''# vpu_dot_product(a $4, b $5, result $6, count $7):
#   result[i] = ((p.x + p.y) + p.z) + p.w with p = a[i] * b[i]
# Optimized: 4 vectors per iteration, 7 COP2 instructions each: the loads,
# vmul, the sum as a chain through ACC (vadday.x, then vmaddaz.x and vmaddw.x
# of a vector of ones: 1 cycle per step), qmfc2; ordered by asm/gen/sched.py
# (the loads interleaved with the work of the vectors before), the inputs
# prefetched 5 lines ahead and the output 5 vectors ahead, as feature clang
# (model: {EST[-1]} cycles per 4). count is a multiple of 4.
# Measured: the loads of 4 vectors first, PREF 2 lines ahead: 122 MFLOPS
# (feature-vu0 151).''',
   '\tvaddw.xyzw\t$vf20, $vf0, $vf0w\t# x = 1 (vf0 = 0, 0, 0, 1)\n\tblez\t$7, 2f\n\tnop\n' + lp + '2:\tjr\t$ra\n\tnop\n')

# hmax/hmin: 4 vectors per iteration
body = ['pref\t0, 320($4)', 'pref\t0, 320($5)']
gmax = ['$9', '$10', '$11', '$12']; gmin = ['$13', '$14', '$15', '$24']
for j in range(4):
    v, b, R, N = f'$vf{1+2*j}', f'$vf{2+2*j}', f'$vf{11+j}', f'$vf{15+j}'
    body += [f'lqc2\t{v}, {16*j}($4)', f'lqc2\t{b}, {16*j}($5)', f'vadd.xyzw\t{v}, {v}, {b}',
             f'vmaxy.x\t{R}, {v}, {v}y', f'vminiy.x\t{N}, {v}, {v}y',
             f'vmaxz.x\t{R}, {R}, {v}z', f'vminiz.x\t{N}, {N}, {v}z',
             f'vmaxw.x\t{R}, {R}, {v}w', f'vminiw.x\t{N}, {N}, {v}w',
             f'qmfc2\t{gmax[j]}, {R}', f'qmfc2\t{gmin[j]}, {N}',
             f'sw\t{gmax[j]}, {4*j}($6)', f'sw\t{gmin[j]}, {4*j}($7)']
body += ['addiu\t$4, $4, 64', 'addiu\t$5, $5, 64', 'addiu\t$6, $6, 16', 'addiu\t$8, $8, -4',
         'addiu\t$7, $7, 16']
lp = loop(body, 'bgtz\t$8, 1b')
fn('vpu_hmax_hmin', f'''# vpu_hmax_hmin(a $4, b $5, max_out $6, min_out $7, count $8):
#   v = a[i] + b[i]; max_out[i] = max(max(v.x, v.y), max(v.z, v.w)), min_out
#   the same with min
# Optimized: per vector a max chain (vmaxy.x, vmaxz.x, vmaxw.x) and a min
# chain in field x of two registers: 11 COP2 instructions per vector; 4
# vectors per iteration ordered by asm/gen/sched.py, the inputs prefetched 5
# lines ahead (model: {EST[-1]} cycles per 4). count is a multiple of 4.
# Measured: the chains interleaved by hand after the loads, PREF 2 lines
# ahead: 10.7 Mvec/s; without PREF 10.9 (feature-vu0 12.0).''',
   '\tblez\t$8, 2f\n\tnop\n' + lp + '2:\tjr\t$ra\n\tnop\n')
print(f'# model estimates: {EST}', file=sys.stderr)
