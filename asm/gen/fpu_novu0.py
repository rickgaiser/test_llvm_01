#!/usr/bin/env python3
"""Prints the FPU (non-VU0) versions of the fpu_tests.c and vpu_tests.c
kernels, for the #else branches of asm/fpu_tests.S and asm/vpu_tests.S.
Usage: fpu_novu0.py fpu|vpu

The loop bodies are unrolled with renamed registers and ordered by sched.py
(the R5900 model); the bound of these kernels is the COP1 slot, which lwc1
and swc1 take too (they do not pair with an FPU operation)."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sched

F = [f'$f{i}' for i in range(32)]
# FPRs free to use: $f0-$f19 and the odd ones above (n32: the even $f20-$f30
# are callee-saved)
FREE = F[0:20] + [F[i] for i in range(21, 32, 2)]
ODD = [F[21], F[23], F[25], F[27], F[29], F[31]]
EST = []


def fn(name, comment, body):
    print(f'''
{comment.rstrip()}
	.p2align 3
	.globl	{name}
	.ent	{name}
{name}:''')
    print(body.rstrip('\n'))
    print(f'\t.end\t{name}')


def dot4(dst, m, v):
    """dst = ((m0 v0 + m1 v1) + m2 v2) + m3 v3 through ACC (1 cycle a step)."""
    return [f'mula.s\t{m[0]}, {v[0]}', f'madda.s\t{m[1]}, {v[1]}',
            f'madda.s\t{m[2]}, {v[2]}', f'madd.s\t{dst}, {m[3]}, {v[3]}']


def loop(body, branch, label='1'):
    """The loop: body ordered by sched.py, the branch, and in its delay slot
    the last instruction that neither the branch nor a later one reads."""
    order = sched.schedule(body)
    bsrcs = set(sched.parse(branch)[2])
    slot = 'nop'
    for k in range(len(order) - 1, -1, -1):
        op, dests, srcs = sched.parse(order[k])
        later = order[k + 1:]
        if not set(dests) & bsrcs and all(
                not set(dests) & set(sched.parse(l)[2]) and
                not set(sched.parse(l)[1]) & (set(srcs) | set(dests))
                for l in later) and \
           not (op in sched.STORES and any(sched.parse(l)[0] in sched.STORES for l in later)):
            slot = order.pop(k)
            break
    lines = order + [branch, slot]
    EST.append(sched.cycles(lines))
    return f'\t.p2align 3\n{label}:\t' + '\n\t'.join(lines) + '\n'


def asm(lines):
    return ''.join(f'\t{l}\n' for l in lines)


MAIN = __name__ == '__main__'
if MAIN and sys.argv[1] == 'fpu':
    # matrix_vector_multiply: straight-line
    v = F[16:20]
    res = ODD[:4]
    body = [f'lwc1\t{v[j]}, {4*j}($5)' for j in range(4)]
    for r in range(4):
        m = F[4*r:4*r+4]
        body += [f'lwc1\t{m[j]}, {16*r+4*j}($4)' for j in range(4)]
        body += dot4(res[r], m, v)
    body += [f'swc1\t{res[r]}, {4*r}($6)' for r in range(4)]
    order = sched.schedule(body)
    fn('matrix_vector_multiply', f'''# matrix_vector_multiply(mat $4, vec $5, result $6):
#   result[i] = sum over j of mat[i][j] * vec[j]
# Optimized (FPU): each row a chain through ACC (mula.s, madda.s, madd.s:
# 1 cycle per step) in the order of the C sum, ordered by asm/gen/sched.py.
# The bound is the COP1 slot: 20 lwc1, 16 FPU operations, 4 swc1 (model:
# {sched.cycles(order)} cycles).''', asm(order) + '\tjr\t$ra\n\tnop\n')

    # matrix_matrix_multiply: b in $f0-$f15, a row of a in $f16-$f19
    body = [f'lwc1\t{F[i]}, {4*i}($5)' for i in range(16)]
    for i in range(4):
        a = F[16:20]
        body += [f'lwc1\t{a[k]}, {16*i+4*k}($4)' for k in range(4)]
        for j in range(4):
            body += dot4(ODD[j], a, [F[4*k+j] for k in range(4)])
            body.append(f'swc1\t{ODD[j]}, {16*i+4*j}($6)')
    order = sched.schedule(body)
    fn('matrix_matrix_multiply', f'''# matrix_matrix_multiply(a $4, b $5, result $6):
#   result[i][j] = sum over k of a[i][k] * b[k][j]
# Optimized (FPU): b in $f0-$f15; each result a chain through ACC in the
# order of the C sum, ordered by asm/gen/sched.py. The bound is the COP1
# slot: 32 lwc1, 64 FPU operations, 16 swc1 (model: {sched.cycles(order)} cycles).''',
       asm(order) + '\tjr\t$ra\n\tnop\n')

    # transform_vector_list: the matrix in $f0-$f15, 2 vectors per iteration
    pro = [f'lwc1\t{F[i]}, {4*i}($4)' for i in range(16)]
    vregs = [F[16:20], ODD[:4], F[16:20], ODD[:4]]
    body = []
    for k in range(4):
        body += [f'lwc1\t{vregs[k][j]}, {16*k+4*j}($5)' for j in range(4)]
        for r in range(4):
            d = ODD[4 + r % 2]
            body += dot4(d, F[4*r:4*r+4], vregs[k])
            body.append(f'swc1\t{d}, {16*k+4*r}($6)')
    body += ['addiu\t$5, $5, 64', 'addiu\t$7, $7, -4', 'addiu\t$6, $6, 64']
    lp = loop(body, 'bgtz\t$7, 1b')
    fn('transform_vector_list', f'''# transform_vector_list(mat $4, in $5, out $6, count $7):
#   out[v][i] = sum over j of mat[i][j] * in[v][j]
# Optimized (FPU): the matrix in $f0-$f15; 4 vectors per iteration, ordered
# by asm/gen/sched.py. The bound is the COP1 slot: per vector 4 lwc1, 16 FPU
# operations, 4 swc1 (model: {EST[-1]} cycles per 4 vectors). count is a
# multiple of 4. Measured with 2 vectors per iteration: 349 MFLOPS.''',
       asm(pro) + '\tblez\t$7, 2f\n\tnop\n' + lp + '2:\tjr\t$ra\n\tnop\n')
elif MAIN:
    # vpu_vector_divide: 2 vectors per iteration
    body = []
    for k in range(2):
        a = F[8*k:8*k+4]; b = F[8*k+4:8*k+8]
        body += [f'lwc1\t{a[l]}, {16*k+4*l}($4)' for l in range(4)]
        body += [f'lwc1\t{b[l]}, {16*k+4*l}($5)' for l in range(4)]
    res = [F[16], F[17], F[18], F[19]] + ODD[:4]
    for k in range(2):
        for l in range(4):
            body.append(f'div.s\t{res[4*k+l]}, {F[8*k+l]}, {F[8*k+4+l]}')
    body += [f'swc1\t{res[i]}, {4*i}($6)' for i in range(8)]
    body += ['addiu\t$4, $4, 32', 'addiu\t$5, $5, 32', 'addiu\t$7, $7, -2',
             'addiu\t$6, $6, 32']
    lp = loop(body, 'bgtz\t$7, 1b')
    fn('vpu_vector_divide', f'''# vpu_vector_divide(a $4, b $5, result $6, count $7):  result[i] = a[i] / b[i]
# Optimized (FPU): div.s back to back (the divider takes one every 7 cycles:
# 28 cycles per vector); the loads and stores run while it divides; 2
# vectors per iteration (model: {EST[-1]} cycles). count is even.''',
       '\tblez\t$7, 2f\n\tnop\n' + lp + '2:\tjr\t$ra\n\tnop\n')

    def line(name, sig, doc, nin, op, prefetch):
        ins = ['$4', '$5', '$6'][:nin]; out = ['$5', '$6', '$7'][nin - 1]
        cnt = ['$6', '$7', '$8'][nin - 1]
        body = [f'pref\t0, 256({p})' for p in ins + [out]] if prefetch else []
        k = 0
        for j in range(4):
            for l in range(4):
                src = [FREE[(k + i) % len(FREE)] for i in range(nin)]
                k += nin
                body += [f'lwc1\t{src[i]}, {16*j+4*l}({ins[i]})' for i in range(nin)]
                body += op(src, src[0])
                body.append(f'swc1\t{src[0]}, {16*j+4*l}({out})')
        body += [f'addiu\t{p}, {p}, 64' for p in ins]
        body += [f'addiu\t{cnt}, {cnt}, -4', f'addiu\t{out}, {out}, 64']
        lp = loop(body, f'bgtz\t{cnt}, 1b')
        fn(name, f'''# {name}({sig}):
#   {doc}
# Optimized (FPU): a 64-byte line of each array per iteration (4 vectors),
# {"the inputs and the output prefetched 4 lines ahead (as feature clang), " if prefetch else ""}ordered by asm/gen/sched.py
# (model: {EST[-1]} cycles without misses; the loop waits for the misses of
# arrays 4 KB apart). count is a multiple of 4.''',
           f'\tblez\t{cnt}, 2f\n\tnop\n' + lp + '2:\tjr\t$ra\n\tnop\n')
    line('vpu_vector_add', 'a $4, b $5, result $6, count $7', 'result[i] = a[i] + b[i]', 2,
         lambda s, d: [f'add.s\t{d}, {s[0]}, {s[1]}'], True)
    line('vpu_vector_mul', 'a $4, b $5, result $6, count $7', 'result[i] = a[i] * b[i]', 2,
         lambda s, d: [f'mul.s\t{d}, {s[0]}, {s[1]}'], True)
    line('vpu_vector_madd', 'a $4, b $5, c $6, result $7, count $8', 'result[i] = a[i] * b[i] + c[i]', 3,
         lambda s, d: [f'mul.s\t{d}, {s[0]}, {s[1]}', f'add.s\t{d}, {d}, {s[2]}'], True)

    # matrix x vector, the columns in $f0-$f15, 2 vectors per iteration
    pro = [f'lwc1\t{F[i]}, {4*i}($4)' for i in range(16)]
    vregs = [F[16:20], ODD[:4], F[16:20], ODD[:4]]
    body = []
    for k in range(4):
        body += [f'lwc1\t{vregs[k][j]}, {16*k+4*j}($5)' for j in range(4)]
        for i in range(4):
            d = ODD[4 + i % 2]
            body += dot4(d, [F[4*c+i] for c in range(4)], vregs[k])
            body.append(f'swc1\t{d}, {16*k+4*i}($6)')
    body += ['addiu\t$5, $5, 64', 'addiu\t$7, $7, -4', 'addiu\t$6, $6, 64']
    lp = loop(body, 'bgtz\t$7, 1b')
    fn('vpu_matrix_vector_multiply', f'''# vpu_matrix_vector_multiply(mat $4, in $5, out $6, count $7):
#   out[i] = mat[0] * in[i].x + mat[1] * in[i].y + mat[2] * in[i].z
#          + mat[3] * in[i].w
# Optimized (FPU): the columns in $f0-$f15; 4 vectors per iteration, a chain
# through ACC per lane, ordered by asm/gen/sched.py (model: {EST[-1]} cycles
# per 4 vectors). count is a multiple of 4. Measured with 2 vectors per
# iteration: 321 MFLOPS.''',
       asm(pro) + '\tblez\t$7, 2f\n\tnop\n' + lp + '2:\tjr\t$ra\n\tnop\n')

    # dot product: 4 vectors per iteration
    body = ['pref\t0, 256($4)', 'pref\t0, 256($5)', 'pref\t0, 64($6)']
    for j in range(4):
        a = FREE[8*(j % 2):8*(j % 2)+4]; b = FREE[8*(j % 2)+4:8*(j % 2)+8]
        body += [f'lwc1\t{a[l]}, {16*j+4*l}($4)' for l in range(4)]
        body += [f'lwc1\t{b[l]}, {16*j+4*l}($5)' for l in range(4)]
        d = [F[16], F[17], F[18], F[19]][j]
        body += dot4(d, a, b)
        body.append(f'swc1\t{d}, {4*j}($6)')
    body += ['addiu\t$4, $4, 64', 'addiu\t$5, $5, 64', 'addiu\t$7, $7, -4',
             'addiu\t$6, $6, 16']
    lp = loop(body, 'bgtz\t$7, 1b')
    fn('vpu_dot_product', f'''# vpu_dot_product(a $4, b $5, result $6, count $7):
#   result[i] = ((p.x + p.y) + p.z) + p.w with p = a[i] * b[i]
# Optimized (FPU): the sum of products is one chain through ACC (mula.s,
# madda.s, madda.s, madd.s) in the order of the C sum; 4 vectors per
# iteration, prefetched 4 lines ahead (as feature clang), ordered by
# asm/gen/sched.py. The bound is the COP1 slot: 13 per vector (model:
# {EST[-1]} cycles per 4). count is a multiple of 4.''',
       '\tblez\t$7, 2f\n\tnop\n' + lp + '2:\tjr\t$ra\n\tnop\n')

    # hmax/hmin: 2 vectors per iteration (a line every second iteration)
    body = ['pref\t0, 256($4)', 'pref\t0, 256($5)']
    for j in range(2):
        a = FREE[8*j:8*j+4]; b = FREE[8*j+4:8*j+8]
        body += [f'lwc1\t{a[l]}, {16*j+4*l}($4)' for l in range(4)]
        body += [f'lwc1\t{b[l]}, {16*j+4*l}($5)' for l in range(4)]
        body += [f'add.s\t{a[l]}, {a[l]}, {b[l]}' for l in range(4)]
        body += [f'max.s\t{b[0]}, {a[0]}, {a[1]}', f'max.s\t{b[1]}, {a[2]}, {a[3]}',
                 f'min.s\t{b[2]}, {a[0]}, {a[1]}', f'min.s\t{b[3]}, {a[2]}, {a[3]}',
                 f'max.s\t{b[0]}, {b[0]}, {b[1]}', f'min.s\t{b[2]}, {b[2]}, {b[3]}',
                 f'swc1\t{b[0]}, {4*j}($6)', f'swc1\t{b[2]}, {4*j}($7)']
    body += ['addiu\t$4, $4, 32', 'addiu\t$5, $5, 32', 'addiu\t$6, $6, 8',
             'addiu\t$8, $8, -2', 'addiu\t$7, $7, 8']
    lp = loop(body, 'bgtz\t$8, 1b')
    fn('vpu_hmax_hmin', f'''# vpu_hmax_hmin(a $4, b $5, max_out $6, min_out $7, count $8):
#   v = a[i] + b[i]; max_out[i] = max(max(v.x, v.y), max(v.z, v.w)), min_out
#   the same with min
# Optimized (FPU): add.s per lane, then max.s and min.s trees; 2 vectors per
# iteration, the inputs prefetched 4 lines ahead, ordered by
# asm/gen/sched.py. The bound is the COP1 slot: 20 per vector (model:
# {EST[-1]} cycles per 2). count is even.''',
       '\tblez\t$8, 2f\n\tnop\n' + lp + '2:\tjr\t$ra\n\tnop\n')
if MAIN:
    print(f'# model estimates (cycles per loop iteration): {EST}', file=sys.stderr)
