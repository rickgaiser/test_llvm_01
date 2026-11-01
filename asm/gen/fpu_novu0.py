#!/usr/bin/env python3
"""Prints the FPU (non-VU0) versions of the fpu_tests.c and vpu_tests.c
kernels, for the #else branches of asm/fpu_tests.S and asm/vpu_tests.S.
Usage: fpu_novu0.py fpu|vpu"""
import sys

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
    """dst = ((m0 * v0 + m1 * v1) + m2 * v2) + m3 * v3 through ACC."""
    return (f'\tmula.s\t{m[0]}, {v[0]}\n\tmadda.s\t{m[1]}, {v[1]}\n'
            f'\tmadda.s\t{m[2]}, {v[2]}\n\tmadd.s\t{dst}, {m[3]}, {v[3]}\n')

F = [f'$f{i}' for i in range(32)]
RES = ['$f21', '$f23', '$f25', '$f27']      # caller-saved (odd) result regs

if sys.argv[1] == 'fpu':
    # matrix_vector_multiply
    b = ''.join(f'\tlwc1\t{F[12+j]}, {4*j}($5)\n' for j in range(4))
    for r in range(4):
        b += ''.join(f'\tlwc1\t{F[4*(r%2)+j]}, {16*r+4*j}($4)\n' for j in range(4))
        b += dot4(RES[r], F[4*(r%2):4*(r%2)+4], F[12:16])
    b += ''.join(f'\tswc1\t{RES[r]}, {4*r}($6)\n' for r in range(4))
    b += '\tjr\t$ra\n\tnop\n'
    fn('matrix_vector_multiply', '''# matrix_vector_multiply(mat $4, vec $5, result $6):
#   result[i] = sum over j of mat[i][j] * vec[j]
# Optimized (FPU): each row a chain through ACC (mula.s, madda.s, madd.s:
# 1 cycle per step), in the order of the C sum; the loads of a row come
# before its chain (lwc1 does not pair with an FPU operation).''', b)
    # matrix_matrix_multiply: b in $f0-$f15, a row in $f16-$f19
    b = ''.join(f'\tlwc1\t{F[i]}, {4*i}($5)\n' for i in range(16))
    for i in range(4):
        b += ''.join(f'\tlwc1\t{F[16+k]}, {16*i+4*k}($4)\n' for k in range(4))
        for j in range(4):
            b += dot4(RES[j], F[16:20], [F[4*k+j] for k in range(4)])
        b += ''.join(f'\tswc1\t{RES[j]}, {16*i+4*j}($6)\n' for j in range(4))
    b += '\tjr\t$ra\n\tnop\n'
    fn('matrix_matrix_multiply', '''# matrix_matrix_multiply(a $4, b $5, result $6):
#   result[i][j] = sum over k of a[i][k] * b[k][j]
# Optimized (FPU): b in $f0-$f15, a row of a in $f16-$f19; each result a
# chain through ACC in the order of the C sum.''', b)
    # transform_vector_list: matrix in $f0-$f15, vector $f16-$f19
    b = ''.join(f'\tlwc1\t{F[i]}, {4*i}($4)\n' for i in range(16))
    b += '\tblez\t$7, 2f\n\tnop\n\t.p2align 3\n1:'
    b += ''.join(f'\tlwc1\t{F[16+j]}, {4*j}($5)\n' for j in range(4))
    for r in range(4):
        b += dot4(RES[r], F[4*r:4*r+4], F[16:20])
    b += '\taddiu\t$5, $5, 16\n\taddiu\t$7, $7, -1\n'
    b += ''.join(f'\tswc1\t{RES[r]}, {4*r}($6)\n' for r in range(4))
    b += '\tbgtz\t$7, 1b\n\taddiu\t$6, $6, 16\n2:\tjr\t$ra\n\tnop\n'
    fn('transform_vector_list', '''# transform_vector_list(mat $4, in $5, out $6, count $7):
#   out[v][i] = sum over j of mat[i][j] * in[v][j]
# Optimized (FPU): the matrix in $f0-$f15; per vector 4 lwc1, 4 chains
# through ACC and 4 swc1.''', b)
else:
    # vpu_vector_divide
    b = '''	blez	$7, 2f
	nop
	.p2align 3
1:	lwc1	$f0, 0($4)
	lwc1	$f1, 0($5)
	lwc1	$f2, 4($4)
	lwc1	$f3, 4($5)
	div.s	$f8, $f0, $f1
	lwc1	$f4, 8($4)
	lwc1	$f5, 8($5)
	lwc1	$f6, 12($4)
	lwc1	$f7, 12($5)
	div.s	$f9, $f2, $f3
	div.s	$f10, $f4, $f5
	div.s	$f11, $f6, $f7
	addiu	$4, $4, 16
	addiu	$5, $5, 16
	swc1	$f8, 0($6)
	swc1	$f9, 4($6)
	swc1	$f10, 8($6)
	swc1	$f11, 12($6)
	addiu	$7, $7, -1
	bgtz	$7, 1b
	addiu	$6, $6, 16
2:	jr	$ra
	nop
'''
    fn('vpu_vector_divide', '''# vpu_vector_divide(a $4, b $5, result $6, count $7):  result[i] = a[i] / b[i]
# Optimized (FPU): four div.s back to back (the divider takes one every 7
# cycles); the loads and stores run while it divides.''', b)
    # add / mul / madd: a line (4 vectors) per iteration, PREF 2 lines ahead
    def line(name, sig, doc, nin, op):
        ins = ['$4', '$5', '$6'][:nin]; out = ['$5', '$6', '$7'][nin - 1]
        cnt = ['$6', '$7', '$8'][nin - 1]
        b = f'\tblez\t{cnt}, 2f\n\tnop\n\t.p2align 3\n1:'
        b += ''.join(f'\tpref\t0, 128({p})\n' for p in ins)
        for j in range(4):           # one vector: lanes 0..3
            for l in range(4):
                for k, p in enumerate(ins):
                    b += f'\tlwc1\t{F[4*k+l]}, {16*j+4*l}({p})\n'
            for l in range(4):
                b += op([F[4*k+l] for k in range(nin)], F[16+l])
            for l in range(4):
                b += f'\tswc1\t{F[16+l]}, {16*j+4*l}({out})\n'
        b += ''.join(f'\taddiu\t{p}, {p}, 64\n' for p in ins)
        b += f'\taddiu\t{cnt}, {cnt}, -4\n\tbgtz\t{cnt}, 1b\n\taddiu\t{out}, {out}, 64\n2:\tjr\t$ra\n\tnop\n'
        fn(name, f'''# {name}({sig}):
#   {doc}
# Optimized (FPU): a 64-byte line of each array per iteration (4 vectors),
# with the inputs prefetched 2 lines ahead; per vector the loads, the 4
# operations and the 4 stores. count is a multiple of 4.''', b)
    line('vpu_vector_add', 'a $4, b $5, result $6, count $7', 'result[i] = a[i] + b[i]', 2,
         lambda s, d: f'\tadd.s\t{d}, {s[0]}, {s[1]}\n')
    line('vpu_vector_mul', 'a $4, b $5, result $6, count $7', 'result[i] = a[i] * b[i]', 2,
         lambda s, d: f'\tmul.s\t{d}, {s[0]}, {s[1]}\n')
    line('vpu_vector_madd', 'a $4, b $5, c $6, result $7, count $8', 'result[i] = a[i] * b[i] + c[i]', 3,
         lambda s, d: f'\tmul.s\t{d}, {s[0]}, {s[1]}\n\tadd.s\t{d}, {d}, {s[2]}\n')
    # matrix x vector, columns in $f0-$f15
    b = ''.join(f'\tlwc1\t{F[i]}, {4*i}($4)\n' for i in range(16))
    b += '\tblez\t$7, 2f\n\tnop\n\t.p2align 3\n1:'
    b += ''.join(f'\tlwc1\t{F[16+j]}, {4*j}($5)\n' for j in range(4))
    for i in range(4):
        b += dot4(RES[i], [F[4*c+i] for c in range(4)], F[16:20])
    b += '\taddiu\t$5, $5, 16\n\taddiu\t$7, $7, -1\n'
    b += ''.join(f'\tswc1\t{RES[i]}, {4*i}($6)\n' for i in range(4))
    b += '\tbgtz\t$7, 1b\n\taddiu\t$6, $6, 16\n2:\tjr\t$ra\n\tnop\n'
    fn('vpu_matrix_vector_multiply', '''# vpu_matrix_vector_multiply(mat $4, in $5, out $6, count $7):
#   out[i] = mat[0] * in[i].x + mat[1] * in[i].y + mat[2] * in[i].z
#          + mat[3] * in[i].w
# Optimized (FPU): the columns in $f0-$f15; per vector 4 lwc1, a chain
# through ACC per lane and 4 swc1.''', b)
    # dot product: 2 vectors per iteration
    b = '\tblez\t$7, 2f\n\tnop\n\t.p2align 3\n1:\tpref\t0, 128($4)\n\tpref\t0, 128($5)\n'
    for j in range(4):
        b += ''.join(f'\tlwc1\t{F[l]}, {16*j+4*l}($4)\n\tlwc1\t{F[4+l]}, {16*j+4*l}($5)\n' for l in range(4))
        b += dot4(RES[j], F[0:4], F[4:8])
    b += '\taddiu\t$4, $4, 64\n\taddiu\t$5, $5, 64\n'
    b += ''.join(f'\tswc1\t{RES[j]}, {4*j}($6)\n' for j in range(4))
    b += '\taddiu\t$7, $7, -4\n\tbgtz\t$7, 1b\n\taddiu\t$6, $6, 16\n2:\tjr\t$ra\n\tnop\n'
    fn('vpu_dot_product', '''# vpu_dot_product(a $4, b $5, result $6, count $7):
#   result[i] = ((p.x + p.y) + p.z) + p.w with p = a[i] * b[i]
# Optimized (FPU): the sum of products is one chain through ACC (mula.s,
# madda.s, madda.s, madd.s), in the order of the C sum; 4 vectors per
# iteration, prefetched 2 lines ahead. count is a multiple of 4.''', b)
    # hmax/hmin: max(max(v0, v1), max(v2, v3))
    b = '\tblez\t$8, 2f\n\tnop\n\t.p2align 3\n1:\tpref\t0, 128($4)\n\tpref\t0, 128($5)\n'
    for j in range(4):
        b += ''.join(f'\tlwc1\t{F[l]}, {16*j+4*l}($4)\n\tlwc1\t{F[4+l]}, {16*j+4*l}($5)\n' for l in range(4))
        b += ''.join(f'\tadd.s\t{F[l]}, {F[l]}, {F[4+l]}\n' for l in range(4))
        b += (f'\tmax.s\t$f8, $f0, $f1\n\tmax.s\t$f9, $f2, $f3\n\tmin.s\t$f10, $f0, $f1\n'
              f'\tmin.s\t$f11, $f2, $f3\n\tmax.s\t$f8, $f8, $f9\n\tmin.s\t$f10, $f10, $f11\n'
              f'\tswc1\t$f8, {4*j}($6)\n\tswc1\t$f10, {4*j}($7)\n')
    b += '\taddiu\t$4, $4, 64\n\taddiu\t$5, $5, 64\n\taddiu\t$6, $6, 16\n\taddiu\t$8, $8, -4\n\tbgtz\t$8, 1b\n\taddiu\t$7, $7, 16\n2:\tjr\t$ra\n\tnop\n'
    fn('vpu_hmax_hmin', '''# vpu_hmax_hmin(a $4, b $5, max_out $6, min_out $7, count $8):
#   v = a[i] + b[i]; max_out[i] = max(max(v.x, v.y), max(v.z, v.w)), min_out
#   the same with min
# Optimized (FPU): add.s per lane, then max.s and min.s trees; 4 vectors per
# iteration, prefetched 2 lines ahead. count is a multiple of 4.''', b)
