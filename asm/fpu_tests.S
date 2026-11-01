# Hand-written R5900 versions of the fpu_tests.c kernels (bench variant asm).
# n32 ABI. See int_tests.S for what "naive" means.
#
# The kernels are 4x4 float matrices: VU0 macro mode does them in far fewer
# instructions than the FPU (a vector multiply-add per instruction), so they
# use it. Measured (bench uarch tests): every COP2 instruction (VU0
# operation, lqc2, sqc2, qmtc2, qmfc2) takes the one COP2 issue slot, a chain
# through ACC (vmula, vmadda, vmadd) takes 1 cycle per step, a VU0 result
# takes 5 cycles to sqc2/qmfc2, lqc2 and qmtc2 2 cycles to their use. VU0
# rounds toward zero as the FPU does; the sums are in the order of the C
# loops.

	.set	noreorder
	.set	noat
	.text

#ifdef ASM_VU0

# vf1..vf4 = the columns of the row-major 4x4 matrix at reg: lq of the rows,
# an MMI transpose (PEXTLW/PEXTUW, then PCPYLD/PCPYUD), qmtc2. Uses $8-$15.
#define LOAD_COLUMNS(reg) \
	lq	$8, 0(reg); lq $9, 16(reg); lq $10, 32(reg); lq $11, 48(reg); \
	pextlw	$12, $9, $8;	/* {r0.0, r1.0, r0.1, r1.1} */ \
	pextuw	$13, $9, $8;	/* {r0.2, r1.2, r0.3, r1.3} */ \
	pextlw	$14, $11, $10; \
	pextuw	$15, $11, $10; \
	pcpyld	$8, $14, $12;	/* column 0 */ \
	pcpyud	$9, $12, $14;	/* column 1 */ \
	pcpyld	$10, $15, $13;	/* column 2 */ \
	pcpyud	$11, $13, $15;	/* column 3 */ \
	qmtc2	$8, $vf1; qmtc2 $9, $vf2; qmtc2 $10, $vf3; qmtc2 $11, $vf4

# matrix_vector_multiply(mat $4, vec $5, result $6):
#   result[i] = sum over j of mat[i][j] * vec[j]
# Optimized: VU0. The matrix is transposed into columns (MMI), then
# result = col0 * v.x + col1 * v.y + col2 * v.z + col3 * v.w in one chain
# through ACC.
	.p2align 3
	.globl	matrix_vector_multiply
	.ent	matrix_vector_multiply
matrix_vector_multiply:
	LOAD_COLUMNS($4)
	lqc2	$vf5, 0($5)
	vmulax.xyzw	$ACC, $vf1, $vf5x
	vmadday.xyzw	$ACC, $vf2, $vf5y
	vmaddaz.xyzw	$ACC, $vf3, $vf5z
	vmaddw.xyzw	$vf6, $vf4, $vf5w
	jr	$ra
	sqc2	$vf6, 0($6)
	.end	matrix_vector_multiply

# matrix_matrix_multiply(a $4, b $5, result $6):
#   result[i][j] = sum over k of a[i][k] * b[k][j]
# Optimized: VU0. Row i of the result is b0 * a[i][0] + b1 * a[i][1] +
# b2 * a[i][2] + b3 * a[i][3] with the rows of b, one ACC chain per row; the
# store of a row waits 5 cycles for its vmaddw, so it comes after the next
# row's chain.
	.p2align 3
	.globl	matrix_matrix_multiply
	.ent	matrix_matrix_multiply
matrix_matrix_multiply:
	lqc2	$vf1, 0($5)
	lqc2	$vf2, 16($5)
	lqc2	$vf3, 32($5)
	lqc2	$vf4, 48($5)
	lqc2	$vf5, 0($4)
	lqc2	$vf6, 16($4)
	lqc2	$vf7, 32($4)
	lqc2	$vf8, 48($4)
	vmulax.xyzw	$ACC, $vf1, $vf5x
	vmadday.xyzw	$ACC, $vf2, $vf5y
	vmaddaz.xyzw	$ACC, $vf3, $vf5z
	vmaddw.xyzw	$vf9, $vf4, $vf5w
	vmulax.xyzw	$ACC, $vf1, $vf6x
	vmadday.xyzw	$ACC, $vf2, $vf6y
	vmaddaz.xyzw	$ACC, $vf3, $vf6z
	vmaddw.xyzw	$vf10, $vf4, $vf6w
	sqc2	$vf9, 0($6)
	vmulax.xyzw	$ACC, $vf1, $vf7x
	vmadday.xyzw	$ACC, $vf2, $vf7y
	vmaddaz.xyzw	$ACC, $vf3, $vf7z
	vmaddw.xyzw	$vf11, $vf4, $vf7w
	sqc2	$vf10, 16($6)
	vmulax.xyzw	$ACC, $vf1, $vf8x
	vmadday.xyzw	$ACC, $vf2, $vf8y
	vmaddaz.xyzw	$ACC, $vf3, $vf8z
	vmaddw.xyzw	$vf12, $vf4, $vf8w
	sqc2	$vf11, 32($6)
	jr	$ra
	sqc2	$vf12, 48($6)
	.end	matrix_matrix_multiply

# transform_vector_list(mat $4, in $5, out $6, count $7):
#   out[v][i] = sum over j of mat[i][j] * in[v][j]
# Optimized: VU0. The matrix is transposed into columns (MMI) once, then the
# loop of vpu_matrix_vector_multiply_asm (vpu_tests.c, measured 6 cycles per
# vector, the bound of the COP2 slot): software pipelined, 2 vectors per
# iteration. count is even and at least 2.
	.p2align 3
	.globl	transform_vector_list
	.ent	transform_vector_list
transform_vector_list:
	LOAD_COLUMNS($4)
	sll	$7, $7, 4
	addu	$7, $7, $5		# end of the input
	lqc2	$vf5, 0($5)
	lqc2	$vf6, 16($5)
	addiu	$5, $5, 32
	vmulax.xyzw	$ACC, $vf1, $vf5x
	vmadday.xyzw	$ACC, $vf2, $vf5y
	vmaddaz.xyzw	$ACC, $vf3, $vf5z
	vmaddw.xyzw	$vf7, $vf4, $vf5w
	vmulax.xyzw	$ACC, $vf1, $vf6x
	vmadday.xyzw	$ACC, $vf2, $vf6y
	.p2align 3
1:	vmaddaz.xyzw	$ACC, $vf3, $vf6z
	sqc2	$vf7, 0($6)
	beq	$5, $7, 2f
	vmaddw.xyzw	$vf8, $vf4, $vf6w
	lqc2	$vf5, 0($5)
	lqc2	$vf6, 16($5)
	addiu	$5, $5, 32
	vmulax.xyzw	$ACC, $vf1, $vf5x
	vmadday.xyzw	$ACC, $vf2, $vf5y
	vmaddaz.xyzw	$ACC, $vf3, $vf5z
	vmaddw.xyzw	$vf7, $vf4, $vf5w
	sqc2	$vf8, 16($6)
	vmulax.xyzw	$ACC, $vf1, $vf6x
	addiu	$6, $6, 32
	b	1b
	vmadday.xyzw	$ACC, $vf2, $vf6y
2:	jr	$ra
	sqc2	$vf8, 16($6)
	.end	transform_vector_list

#else // ASM_VU0: the FPU versions (asm/gen/fpu_novu0.py)

# matrix_vector_multiply(mat $4, vec $5, result $6):
#   result[i] = sum over j of mat[i][j] * vec[j]
# Optimized (FPU): each row a chain through ACC (mula.s, madda.s, madd.s:
# 1 cycle per step), in the order of the C sum; the loads of a row come
# before its chain (lwc1 does not pair with an FPU operation).
	.p2align 3
	.globl	matrix_vector_multiply
	.ent	matrix_vector_multiply
matrix_vector_multiply:
	lwc1	$f12, 0($5)
	lwc1	$f13, 4($5)
	lwc1	$f14, 8($5)
	lwc1	$f15, 12($5)
	lwc1	$f0, 0($4)
	lwc1	$f1, 4($4)
	lwc1	$f2, 8($4)
	lwc1	$f3, 12($4)
	mula.s	$f0, $f12
	madda.s	$f1, $f13
	madda.s	$f2, $f14
	madd.s	$f21, $f3, $f15
	lwc1	$f4, 16($4)
	lwc1	$f5, 20($4)
	lwc1	$f6, 24($4)
	lwc1	$f7, 28($4)
	mula.s	$f4, $f12
	madda.s	$f5, $f13
	madda.s	$f6, $f14
	madd.s	$f23, $f7, $f15
	lwc1	$f0, 32($4)
	lwc1	$f1, 36($4)
	lwc1	$f2, 40($4)
	lwc1	$f3, 44($4)
	mula.s	$f0, $f12
	madda.s	$f1, $f13
	madda.s	$f2, $f14
	madd.s	$f25, $f3, $f15
	lwc1	$f4, 48($4)
	lwc1	$f5, 52($4)
	lwc1	$f6, 56($4)
	lwc1	$f7, 60($4)
	mula.s	$f4, $f12
	madda.s	$f5, $f13
	madda.s	$f6, $f14
	madd.s	$f27, $f7, $f15
	swc1	$f21, 0($6)
	swc1	$f23, 4($6)
	swc1	$f25, 8($6)
	swc1	$f27, 12($6)
	jr	$ra
	nop
	.end	matrix_vector_multiply

# matrix_matrix_multiply(a $4, b $5, result $6):
#   result[i][j] = sum over k of a[i][k] * b[k][j]
# Optimized (FPU): b in $f0-$f15, a row of a in $f16-$f19; each result a
# chain through ACC in the order of the C sum.
	.p2align 3
	.globl	matrix_matrix_multiply
	.ent	matrix_matrix_multiply
matrix_matrix_multiply:
	lwc1	$f0, 0($5)
	lwc1	$f1, 4($5)
	lwc1	$f2, 8($5)
	lwc1	$f3, 12($5)
	lwc1	$f4, 16($5)
	lwc1	$f5, 20($5)
	lwc1	$f6, 24($5)
	lwc1	$f7, 28($5)
	lwc1	$f8, 32($5)
	lwc1	$f9, 36($5)
	lwc1	$f10, 40($5)
	lwc1	$f11, 44($5)
	lwc1	$f12, 48($5)
	lwc1	$f13, 52($5)
	lwc1	$f14, 56($5)
	lwc1	$f15, 60($5)
	lwc1	$f16, 0($4)
	lwc1	$f17, 4($4)
	lwc1	$f18, 8($4)
	lwc1	$f19, 12($4)
	mula.s	$f16, $f0
	madda.s	$f17, $f4
	madda.s	$f18, $f8
	madd.s	$f21, $f19, $f12
	mula.s	$f16, $f1
	madda.s	$f17, $f5
	madda.s	$f18, $f9
	madd.s	$f23, $f19, $f13
	mula.s	$f16, $f2
	madda.s	$f17, $f6
	madda.s	$f18, $f10
	madd.s	$f25, $f19, $f14
	mula.s	$f16, $f3
	madda.s	$f17, $f7
	madda.s	$f18, $f11
	madd.s	$f27, $f19, $f15
	swc1	$f21, 0($6)
	swc1	$f23, 4($6)
	swc1	$f25, 8($6)
	swc1	$f27, 12($6)
	lwc1	$f16, 16($4)
	lwc1	$f17, 20($4)
	lwc1	$f18, 24($4)
	lwc1	$f19, 28($4)
	mula.s	$f16, $f0
	madda.s	$f17, $f4
	madda.s	$f18, $f8
	madd.s	$f21, $f19, $f12
	mula.s	$f16, $f1
	madda.s	$f17, $f5
	madda.s	$f18, $f9
	madd.s	$f23, $f19, $f13
	mula.s	$f16, $f2
	madda.s	$f17, $f6
	madda.s	$f18, $f10
	madd.s	$f25, $f19, $f14
	mula.s	$f16, $f3
	madda.s	$f17, $f7
	madda.s	$f18, $f11
	madd.s	$f27, $f19, $f15
	swc1	$f21, 16($6)
	swc1	$f23, 20($6)
	swc1	$f25, 24($6)
	swc1	$f27, 28($6)
	lwc1	$f16, 32($4)
	lwc1	$f17, 36($4)
	lwc1	$f18, 40($4)
	lwc1	$f19, 44($4)
	mula.s	$f16, $f0
	madda.s	$f17, $f4
	madda.s	$f18, $f8
	madd.s	$f21, $f19, $f12
	mula.s	$f16, $f1
	madda.s	$f17, $f5
	madda.s	$f18, $f9
	madd.s	$f23, $f19, $f13
	mula.s	$f16, $f2
	madda.s	$f17, $f6
	madda.s	$f18, $f10
	madd.s	$f25, $f19, $f14
	mula.s	$f16, $f3
	madda.s	$f17, $f7
	madda.s	$f18, $f11
	madd.s	$f27, $f19, $f15
	swc1	$f21, 32($6)
	swc1	$f23, 36($6)
	swc1	$f25, 40($6)
	swc1	$f27, 44($6)
	lwc1	$f16, 48($4)
	lwc1	$f17, 52($4)
	lwc1	$f18, 56($4)
	lwc1	$f19, 60($4)
	mula.s	$f16, $f0
	madda.s	$f17, $f4
	madda.s	$f18, $f8
	madd.s	$f21, $f19, $f12
	mula.s	$f16, $f1
	madda.s	$f17, $f5
	madda.s	$f18, $f9
	madd.s	$f23, $f19, $f13
	mula.s	$f16, $f2
	madda.s	$f17, $f6
	madda.s	$f18, $f10
	madd.s	$f25, $f19, $f14
	mula.s	$f16, $f3
	madda.s	$f17, $f7
	madda.s	$f18, $f11
	madd.s	$f27, $f19, $f15
	swc1	$f21, 48($6)
	swc1	$f23, 52($6)
	swc1	$f25, 56($6)
	swc1	$f27, 60($6)
	jr	$ra
	nop
	.end	matrix_matrix_multiply

# transform_vector_list(mat $4, in $5, out $6, count $7):
#   out[v][i] = sum over j of mat[i][j] * in[v][j]
# Optimized (FPU): the matrix in $f0-$f15; per vector 4 lwc1, 4 chains
# through ACC and 4 swc1.
	.p2align 3
	.globl	transform_vector_list
	.ent	transform_vector_list
transform_vector_list:
	lwc1	$f0, 0($4)
	lwc1	$f1, 4($4)
	lwc1	$f2, 8($4)
	lwc1	$f3, 12($4)
	lwc1	$f4, 16($4)
	lwc1	$f5, 20($4)
	lwc1	$f6, 24($4)
	lwc1	$f7, 28($4)
	lwc1	$f8, 32($4)
	lwc1	$f9, 36($4)
	lwc1	$f10, 40($4)
	lwc1	$f11, 44($4)
	lwc1	$f12, 48($4)
	lwc1	$f13, 52($4)
	lwc1	$f14, 56($4)
	lwc1	$f15, 60($4)
	blez	$7, 2f
	nop
	.p2align 3
1:	lwc1	$f16, 0($5)
	lwc1	$f17, 4($5)
	lwc1	$f18, 8($5)
	lwc1	$f19, 12($5)
	mula.s	$f0, $f16
	madda.s	$f1, $f17
	madda.s	$f2, $f18
	madd.s	$f21, $f3, $f19
	mula.s	$f4, $f16
	madda.s	$f5, $f17
	madda.s	$f6, $f18
	madd.s	$f23, $f7, $f19
	mula.s	$f8, $f16
	madda.s	$f9, $f17
	madda.s	$f10, $f18
	madd.s	$f25, $f11, $f19
	mula.s	$f12, $f16
	madda.s	$f13, $f17
	madda.s	$f14, $f18
	madd.s	$f27, $f15, $f19
	addiu	$5, $5, 16
	addiu	$7, $7, -1
	swc1	$f21, 0($6)
	swc1	$f23, 4($6)
	swc1	$f25, 8($6)
	swc1	$f27, 12($6)
	bgtz	$7, 1b
	addiu	$6, $6, 16
2:	jr	$ra
	nop
	.end	transform_vector_list

#endif // ASM_VU0
