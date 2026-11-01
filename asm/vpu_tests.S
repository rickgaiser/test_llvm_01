# Hand-written R5900 versions of the vpu_tests.c kernels (bench variant asm).
# n32 ABI; the v4sf kernels use VU0 macro instructions (COP2). See
# int_tests.S for what "naive" means, and fpu_tests.S for the VU0 costs: one
# COP2 instruction per cycle (VU0 operations, lqc2, sqc2, qmtc2 and qmfc2
# share the slot), 4 cycles from a VU0 operation to its use (5 to sqc2 or
# qmfc2), 1 cycle per step of a chain through ACC; a GPR store or a PREF
# pairs with a COP2 instruction.

	.set	noreorder
	.set	noat
	.text

#ifdef ASM_VU0

# vpu_vector_divide(a $4, b $5, result $6, count $7):  result[i] = a[i] / b[i]
# Optimized: the four divisions of a vector run on two dividers at once:
# x and y on VU0 (vdiv to Q, 7 cycles each, then vaddq of vf0 to place Q
# in the field), z and w on the FPU (div.s, 7 cycles each); sqc2 stores
# the vector, then swc1 the z and w of the FPU.
	.p2align 3
	.globl	vpu_vector_divide
	.ent	vpu_vector_divide
vpu_vector_divide:
	blez	$7, 2f
	nop
	.p2align 3
1:	lqc2	$vf1, 0($4)
	lqc2	$vf2, 0($5)
	lwc1	$f0, 8($4)
	lwc1	$f1, 8($5)
	vdiv	$Q, $vf1x, $vf2x
	div.s	$f2, $f0, $f1
	lwc1	$f0, 12($4)
	lwc1	$f1, 12($5)
	vwaitq
	vaddq.x	$vf3, $vf0, $Q
	vdiv	$Q, $vf1y, $vf2y
	div.s	$f3, $f0, $f1
	addiu	$4, $4, 16
	addiu	$5, $5, 16
	vwaitq
	vaddq.y	$vf3, $vf0, $Q
	addiu	$7, $7, -1
	sqc2	$vf3, 0($6)
	swc1	$f2, 8($6)
	swc1	$f3, 12($6)
	bgtz	$7, 1b
	addiu	$6, $6, 16
2:	jr	$ra
	nop
	.end	vpu_vector_divide

# vpu_vector_add(a $4, b $5, result $6, count $7):
#   result[i] = a[i] + b[i]
# Optimized: a 64-byte line of each array per iteration (4 vectors): the
# arrays are 4 KB apart, so their lines compete for the 2 ways of a data
# cache set; all the loads of a line come before its stores. The inputs are
# prefetched 2 lines ahead (PREF pairs with a COP2 instruction). count is a
# multiple of 4.
	.p2align 3
	.globl	vpu_vector_add
	.ent	vpu_vector_add
vpu_vector_add:
	blez	$7, 2f
	nop
	.p2align 3
1:
	pref	0, 128($4)
	lqc2	$vf1, 0($4)
	lqc2	$vf2, 16($4)
	lqc2	$vf3, 32($4)
	lqc2	$vf4, 48($4)
	pref	0, 128($5)
	lqc2	$vf5, 0($5)
	lqc2	$vf6, 16($5)
	lqc2	$vf7, 32($5)
	lqc2	$vf8, 48($5)
	vadd.xyzw	$vf1, $vf1, $vf5
	vadd.xyzw	$vf2, $vf2, $vf6
	vadd.xyzw	$vf3, $vf3, $vf7
	vadd.xyzw	$vf4, $vf4, $vf8
	sqc2	$vf1, 0($6)
	sqc2	$vf2, 16($6)
	sqc2	$vf3, 32($6)
	sqc2	$vf4, 48($6)
	addiu	$4, $4, 64
	addiu	$5, $5, 64
	addiu	$7, $7, -4
	bgtz	$7, 1b
	addiu	$6, $6, 64
2:	jr	$ra
	nop
	.end	vpu_vector_add

# vpu_vector_mul(a $4, b $5, result $6, count $7):
#   result[i] = a[i] * b[i]
# Optimized: a 64-byte line of each array per iteration (4 vectors): the
# arrays are 4 KB apart, so their lines compete for the 2 ways of a data
# cache set; all the loads of a line come before its stores. The inputs are
# prefetched 2 lines ahead (PREF pairs with a COP2 instruction). count is a
# multiple of 4.
	.p2align 3
	.globl	vpu_vector_mul
	.ent	vpu_vector_mul
vpu_vector_mul:
	blez	$7, 2f
	nop
	.p2align 3
1:
	pref	0, 128($4)
	lqc2	$vf1, 0($4)
	lqc2	$vf2, 16($4)
	lqc2	$vf3, 32($4)
	lqc2	$vf4, 48($4)
	pref	0, 128($5)
	lqc2	$vf5, 0($5)
	lqc2	$vf6, 16($5)
	lqc2	$vf7, 32($5)
	lqc2	$vf8, 48($5)
	vmul.xyzw	$vf1, $vf1, $vf5
	vmul.xyzw	$vf2, $vf2, $vf6
	vmul.xyzw	$vf3, $vf3, $vf7
	vmul.xyzw	$vf4, $vf4, $vf8
	sqc2	$vf1, 0($6)
	sqc2	$vf2, 16($6)
	sqc2	$vf3, 32($6)
	sqc2	$vf4, 48($6)
	addiu	$4, $4, 64
	addiu	$5, $5, 64
	addiu	$7, $7, -4
	bgtz	$7, 1b
	addiu	$6, $6, 64
2:	jr	$ra
	nop
	.end	vpu_vector_mul

# vpu_vector_madd(a $4, b $5, c $6, result $7, count $8):
#   result[i] = a[i] * b[i] + c[i] (ACC = a * b, then ACC + c * vf0.w, which is 1)
# Optimized: a 64-byte line of each array per iteration (4 vectors): the
# arrays are 4 KB apart, so their lines compete for the 2 ways of a data
# cache set; all the loads of a line come before its stores. The inputs are
# prefetched 2 lines ahead (PREF pairs with a COP2 instruction). count is a
# multiple of 4.
	.p2align 3
	.globl	vpu_vector_madd
	.ent	vpu_vector_madd
vpu_vector_madd:
	blez	$8, 2f
	nop
	.p2align 3
1:
	pref	0, 128($4)
	lqc2	$vf1, 0($4)
	lqc2	$vf2, 16($4)
	lqc2	$vf3, 32($4)
	lqc2	$vf4, 48($4)
	pref	0, 128($5)
	lqc2	$vf5, 0($5)
	lqc2	$vf6, 16($5)
	lqc2	$vf7, 32($5)
	lqc2	$vf8, 48($5)
	pref	0, 128($6)
	lqc2	$vf9, 0($6)
	lqc2	$vf10, 16($6)
	lqc2	$vf11, 32($6)
	lqc2	$vf12, 48($6)
	vmula.xyzw	$ACC, $vf1, $vf5
	vmaddw.xyzw	$vf1, $vf9, $vf0w
	vmula.xyzw	$ACC, $vf2, $vf6
	vmaddw.xyzw	$vf2, $vf10, $vf0w
	vmula.xyzw	$ACC, $vf3, $vf7
	vmaddw.xyzw	$vf3, $vf11, $vf0w
	vmula.xyzw	$ACC, $vf4, $vf8
	vmaddw.xyzw	$vf4, $vf12, $vf0w
	sqc2	$vf1, 0($7)
	sqc2	$vf2, 16($7)
	sqc2	$vf3, 32($7)
	sqc2	$vf4, 48($7)
	addiu	$4, $4, 64
	addiu	$5, $5, 64
	addiu	$6, $6, 64
	addiu	$8, $8, -4
	bgtz	$8, 1b
	addiu	$7, $7, 64
2:	jr	$ra
	nop
	.end	vpu_vector_madd

# vpu_matrix_vector_multiply(mat $4, in $5, out $6, count $7):
#   out[i] = mat[0] * in[i].x + mat[1] * in[i].y + mat[2] * in[i].z
#          + mat[3] * in[i].w
# Optimized: the loop of vpu_matrix_vector_multiply_asm (vpu_tests.c,
# measured 6 cycles per vector, the bound of the COP2 slot: 4 VU0 operations,
# lqc2 and sqc2): software pipelined, 2 vectors per iteration. count is even
# and at least 2.
	.p2align 3
	.globl	vpu_matrix_vector_multiply
	.ent	vpu_matrix_vector_multiply
vpu_matrix_vector_multiply:
	lqc2	$vf1, 0($4)
	lqc2	$vf2, 16($4)
	lqc2	$vf3, 32($4)
	lqc2	$vf4, 48($4)
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
	.end	vpu_matrix_vector_multiply

# vpu_dot_product(a $4, b $5, result $6, count $7):
#   result[i] = ((p.x + p.y) + p.z) + p.w with p = a[i] * b[i]
# Optimized: 4 vectors per iteration, 7 COP2 instructions each: the loads,
# vmul, the sum as a chain through ACC (vadday.x, then vmaddaz.x and vmaddw.x
# of a vector of ones: 1 cycle per step instead of 4 for dependent vadds),
# qmfc2; the vmuls come 4 cycles before their chains, the qmfc2s 5 after.
# The inputs are prefetched 2 lines ahead. count is a multiple of 4.
	.p2align 3
	.globl	vpu_dot_product
	.ent	vpu_dot_product
vpu_dot_product:
	vaddw.xyzw	$vf20, $vf0, $vf0w	# x = 1 (vf0 = 0, 0, 0, 1)
	blez	$7, 2f
	nop
	.p2align 3
1:	pref	0, 128($4)
	pref	0, 128($5)
	lqc2	$vf1, 0($4)
	lqc2	$vf2, 0($5)
	lqc2	$vf3, 16($4)
	lqc2	$vf4, 16($5)
	lqc2	$vf5, 32($4)
	lqc2	$vf6, 32($5)
	lqc2	$vf7, 48($4)
	lqc2	$vf8, 48($5)
	vmul.xyzw	$vf1, $vf1, $vf2
	vmul.xyzw	$vf3, $vf3, $vf4
	vmul.xyzw	$vf5, $vf5, $vf6
	vmul.xyzw	$vf7, $vf7, $vf8
	vadday.x	$ACC, $vf1, $vf1y
	vmaddaz.x	$ACC, $vf20, $vf1z
	vmaddw.x	$vf11, $vf20, $vf1w
	vadday.x	$ACC, $vf3, $vf3y
	vmaddaz.x	$ACC, $vf20, $vf3z
	vmaddw.x	$vf12, $vf20, $vf3w
	vadday.x	$ACC, $vf5, $vf5y
	vmaddaz.x	$ACC, $vf20, $vf5z
	vmaddw.x	$vf13, $vf20, $vf5w
	vadday.x	$ACC, $vf7, $vf7y
	vmaddaz.x	$ACC, $vf20, $vf7z
	vmaddw.x	$vf14, $vf20, $vf7w
	qmfc2	$8, $vf11
	qmfc2	$9, $vf12
	sw	$8, 0($6)
	qmfc2	$10, $vf13
	sw	$9, 4($6)
	qmfc2	$11, $vf14
	qmfc2	$12, $vf15
	qmfc2	$13, $vf16
	qmfc2	$14, $vf17
	qmfc2	$15, $vf18
	sw	$10, 8($6)
	addiu	$4, $4, 64
	sw	$11, 12($6)
	addiu	$5, $5, 64
	addiu	$7, $7, -4
	bgtz	$7, 1b
	addiu	$6, $6, 16
2:	jr	$ra
	nop
	.end	vpu_dot_product

# vpu_hmax_hmin(a $4, b $5, max_out $6, min_out $7, count $8):
#   v = a[i] + b[i]; max_out[i] = max(max(v.x, v.y), max(v.z, v.w)), min_out
#   the same with min
# Optimized: per vector a max chain (vmaxy.x, vmaxz.x, vmaxw.x) and a min
# chain in field x of two registers: 11 COP2 instructions per vector. 4
# vectors per iteration, their chains interleaved so each step comes 4
# cycles after the one it depends on; the inputs are prefetched 2 lines
# ahead (lqc2 waits for a miss). count is a multiple of 4.
	.p2align 3
	.globl	vpu_hmax_hmin
	.ent	vpu_hmax_hmin
vpu_hmax_hmin:
	blez	$8, 2f
	nop
	.p2align 3
1:
	pref	0, 128($4)
	pref	0, 128($5)
	lqc2	$vf1, 0($4)
	lqc2	$vf2, 0($5)
	lqc2	$vf3, 16($4)
	lqc2	$vf4, 16($5)
	lqc2	$vf5, 32($4)
	lqc2	$vf6, 32($5)
	lqc2	$vf7, 48($4)
	lqc2	$vf8, 48($5)
	vadd.xyzw	$vf1, $vf1, $vf2
	vadd.xyzw	$vf3, $vf3, $vf4
	vadd.xyzw	$vf5, $vf5, $vf6
	vadd.xyzw	$vf7, $vf7, $vf8
	vmaxy.x	$vf11, $vf1, $vf1y
	vmaxy.x	$vf12, $vf3, $vf3y
	vmaxy.x	$vf13, $vf5, $vf5y
	vmaxy.x	$vf14, $vf7, $vf7y
	vminiy.x	$vf15, $vf1, $vf1y
	vminiy.x	$vf16, $vf3, $vf3y
	vminiy.x	$vf17, $vf5, $vf5y
	vminiy.x	$vf18, $vf7, $vf7y
	vmaxz.x	$vf11, $vf11, $vf1z
	vmaxz.x	$vf12, $vf12, $vf3z
	vmaxz.x	$vf13, $vf13, $vf5z
	vmaxz.x	$vf14, $vf14, $vf7z
	vminiz.x	$vf15, $vf15, $vf1z
	vminiz.x	$vf16, $vf16, $vf3z
	vminiz.x	$vf17, $vf17, $vf5z
	vminiz.x	$vf18, $vf18, $vf7z
	vmaxw.x	$vf11, $vf11, $vf1w
	vmaxw.x	$vf12, $vf12, $vf3w
	vmaxw.x	$vf13, $vf13, $vf5w
	vmaxw.x	$vf14, $vf14, $vf7w
	vminiw.x	$vf15, $vf15, $vf1w
	vminiw.x	$vf16, $vf16, $vf3w
	vminiw.x	$vf17, $vf17, $vf5w
	vminiw.x	$vf18, $vf18, $vf7w
	qmfc2	$9, $vf11
	qmfc2	$10, $vf12
	qmfc2	$11, $vf13
	qmfc2	$12, $vf14
	qmfc2	$13, $vf15
	qmfc2	$14, $vf16
	qmfc2	$15, $vf17
	qmfc2	$24, $vf18
	sw	$9, 0($6)
	sw	$13, 0($7)
	sw	$10, 4($6)
	sw	$14, 4($7)
	sw	$11, 8($6)
	sw	$15, 8($7)
	sw	$12, 12($6)
	sw	$24, 12($7)
	addiu	$4, $4, 64
	addiu	$5, $5, 64
	addiu	$6, $6, 16
	addiu	$8, $8, -4
	bgtz	$8, 1b
	addiu	$7, $7, 16
2:	jr	$ra
	nop
	.end	vpu_hmax_hmin

#else // ASM_VU0: the FPU versions (asm/gen/fpu_novu0.py)

# vpu_vector_divide(a $4, b $5, result $6, count $7):  result[i] = a[i] / b[i]
# Optimized (FPU): four div.s back to back (the divider takes one every 7
# cycles); the loads and stores run while it divides.
	.p2align 3
	.globl	vpu_vector_divide
	.ent	vpu_vector_divide
vpu_vector_divide:
	blez	$7, 2f
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
	.end	vpu_vector_divide

# vpu_vector_add(a $4, b $5, result $6, count $7):
#   result[i] = a[i] + b[i]
# Optimized (FPU): a 64-byte line of each array per iteration (4 vectors),
# with the inputs prefetched 2 lines ahead; per vector the loads, the 4
# operations and the 4 stores. count is a multiple of 4.
	.p2align 3
	.globl	vpu_vector_add
	.ent	vpu_vector_add
vpu_vector_add:
	blez	$7, 2f
	nop
	.p2align 3
1:	pref	0, 128($4)
	pref	0, 128($5)
	lwc1	$f0, 0($4)
	lwc1	$f4, 0($5)
	lwc1	$f1, 4($4)
	lwc1	$f5, 4($5)
	lwc1	$f2, 8($4)
	lwc1	$f6, 8($5)
	lwc1	$f3, 12($4)
	lwc1	$f7, 12($5)
	add.s	$f16, $f0, $f4
	add.s	$f17, $f1, $f5
	add.s	$f18, $f2, $f6
	add.s	$f19, $f3, $f7
	swc1	$f16, 0($6)
	swc1	$f17, 4($6)
	swc1	$f18, 8($6)
	swc1	$f19, 12($6)
	lwc1	$f0, 16($4)
	lwc1	$f4, 16($5)
	lwc1	$f1, 20($4)
	lwc1	$f5, 20($5)
	lwc1	$f2, 24($4)
	lwc1	$f6, 24($5)
	lwc1	$f3, 28($4)
	lwc1	$f7, 28($5)
	add.s	$f16, $f0, $f4
	add.s	$f17, $f1, $f5
	add.s	$f18, $f2, $f6
	add.s	$f19, $f3, $f7
	swc1	$f16, 16($6)
	swc1	$f17, 20($6)
	swc1	$f18, 24($6)
	swc1	$f19, 28($6)
	lwc1	$f0, 32($4)
	lwc1	$f4, 32($5)
	lwc1	$f1, 36($4)
	lwc1	$f5, 36($5)
	lwc1	$f2, 40($4)
	lwc1	$f6, 40($5)
	lwc1	$f3, 44($4)
	lwc1	$f7, 44($5)
	add.s	$f16, $f0, $f4
	add.s	$f17, $f1, $f5
	add.s	$f18, $f2, $f6
	add.s	$f19, $f3, $f7
	swc1	$f16, 32($6)
	swc1	$f17, 36($6)
	swc1	$f18, 40($6)
	swc1	$f19, 44($6)
	lwc1	$f0, 48($4)
	lwc1	$f4, 48($5)
	lwc1	$f1, 52($4)
	lwc1	$f5, 52($5)
	lwc1	$f2, 56($4)
	lwc1	$f6, 56($5)
	lwc1	$f3, 60($4)
	lwc1	$f7, 60($5)
	add.s	$f16, $f0, $f4
	add.s	$f17, $f1, $f5
	add.s	$f18, $f2, $f6
	add.s	$f19, $f3, $f7
	swc1	$f16, 48($6)
	swc1	$f17, 52($6)
	swc1	$f18, 56($6)
	swc1	$f19, 60($6)
	addiu	$4, $4, 64
	addiu	$5, $5, 64
	addiu	$7, $7, -4
	bgtz	$7, 1b
	addiu	$6, $6, 64
2:	jr	$ra
	nop
	.end	vpu_vector_add

# vpu_vector_mul(a $4, b $5, result $6, count $7):
#   result[i] = a[i] * b[i]
# Optimized (FPU): a 64-byte line of each array per iteration (4 vectors),
# with the inputs prefetched 2 lines ahead; per vector the loads, the 4
# operations and the 4 stores. count is a multiple of 4.
	.p2align 3
	.globl	vpu_vector_mul
	.ent	vpu_vector_mul
vpu_vector_mul:
	blez	$7, 2f
	nop
	.p2align 3
1:	pref	0, 128($4)
	pref	0, 128($5)
	lwc1	$f0, 0($4)
	lwc1	$f4, 0($5)
	lwc1	$f1, 4($4)
	lwc1	$f5, 4($5)
	lwc1	$f2, 8($4)
	lwc1	$f6, 8($5)
	lwc1	$f3, 12($4)
	lwc1	$f7, 12($5)
	mul.s	$f16, $f0, $f4
	mul.s	$f17, $f1, $f5
	mul.s	$f18, $f2, $f6
	mul.s	$f19, $f3, $f7
	swc1	$f16, 0($6)
	swc1	$f17, 4($6)
	swc1	$f18, 8($6)
	swc1	$f19, 12($6)
	lwc1	$f0, 16($4)
	lwc1	$f4, 16($5)
	lwc1	$f1, 20($4)
	lwc1	$f5, 20($5)
	lwc1	$f2, 24($4)
	lwc1	$f6, 24($5)
	lwc1	$f3, 28($4)
	lwc1	$f7, 28($5)
	mul.s	$f16, $f0, $f4
	mul.s	$f17, $f1, $f5
	mul.s	$f18, $f2, $f6
	mul.s	$f19, $f3, $f7
	swc1	$f16, 16($6)
	swc1	$f17, 20($6)
	swc1	$f18, 24($6)
	swc1	$f19, 28($6)
	lwc1	$f0, 32($4)
	lwc1	$f4, 32($5)
	lwc1	$f1, 36($4)
	lwc1	$f5, 36($5)
	lwc1	$f2, 40($4)
	lwc1	$f6, 40($5)
	lwc1	$f3, 44($4)
	lwc1	$f7, 44($5)
	mul.s	$f16, $f0, $f4
	mul.s	$f17, $f1, $f5
	mul.s	$f18, $f2, $f6
	mul.s	$f19, $f3, $f7
	swc1	$f16, 32($6)
	swc1	$f17, 36($6)
	swc1	$f18, 40($6)
	swc1	$f19, 44($6)
	lwc1	$f0, 48($4)
	lwc1	$f4, 48($5)
	lwc1	$f1, 52($4)
	lwc1	$f5, 52($5)
	lwc1	$f2, 56($4)
	lwc1	$f6, 56($5)
	lwc1	$f3, 60($4)
	lwc1	$f7, 60($5)
	mul.s	$f16, $f0, $f4
	mul.s	$f17, $f1, $f5
	mul.s	$f18, $f2, $f6
	mul.s	$f19, $f3, $f7
	swc1	$f16, 48($6)
	swc1	$f17, 52($6)
	swc1	$f18, 56($6)
	swc1	$f19, 60($6)
	addiu	$4, $4, 64
	addiu	$5, $5, 64
	addiu	$7, $7, -4
	bgtz	$7, 1b
	addiu	$6, $6, 64
2:	jr	$ra
	nop
	.end	vpu_vector_mul

# vpu_vector_madd(a $4, b $5, c $6, result $7, count $8):
#   result[i] = a[i] * b[i] + c[i]
# Optimized (FPU): a 64-byte line of each array per iteration (4 vectors),
# with the inputs prefetched 2 lines ahead; per vector the loads, the 4
# operations and the 4 stores. count is a multiple of 4.
	.p2align 3
	.globl	vpu_vector_madd
	.ent	vpu_vector_madd
vpu_vector_madd:
	blez	$8, 2f
	nop
	.p2align 3
1:	pref	0, 128($4)
	pref	0, 128($5)
	pref	0, 128($6)
	lwc1	$f0, 0($4)
	lwc1	$f4, 0($5)
	lwc1	$f8, 0($6)
	lwc1	$f1, 4($4)
	lwc1	$f5, 4($5)
	lwc1	$f9, 4($6)
	lwc1	$f2, 8($4)
	lwc1	$f6, 8($5)
	lwc1	$f10, 8($6)
	lwc1	$f3, 12($4)
	lwc1	$f7, 12($5)
	lwc1	$f11, 12($6)
	mul.s	$f16, $f0, $f4
	add.s	$f16, $f16, $f8
	mul.s	$f17, $f1, $f5
	add.s	$f17, $f17, $f9
	mul.s	$f18, $f2, $f6
	add.s	$f18, $f18, $f10
	mul.s	$f19, $f3, $f7
	add.s	$f19, $f19, $f11
	swc1	$f16, 0($7)
	swc1	$f17, 4($7)
	swc1	$f18, 8($7)
	swc1	$f19, 12($7)
	lwc1	$f0, 16($4)
	lwc1	$f4, 16($5)
	lwc1	$f8, 16($6)
	lwc1	$f1, 20($4)
	lwc1	$f5, 20($5)
	lwc1	$f9, 20($6)
	lwc1	$f2, 24($4)
	lwc1	$f6, 24($5)
	lwc1	$f10, 24($6)
	lwc1	$f3, 28($4)
	lwc1	$f7, 28($5)
	lwc1	$f11, 28($6)
	mul.s	$f16, $f0, $f4
	add.s	$f16, $f16, $f8
	mul.s	$f17, $f1, $f5
	add.s	$f17, $f17, $f9
	mul.s	$f18, $f2, $f6
	add.s	$f18, $f18, $f10
	mul.s	$f19, $f3, $f7
	add.s	$f19, $f19, $f11
	swc1	$f16, 16($7)
	swc1	$f17, 20($7)
	swc1	$f18, 24($7)
	swc1	$f19, 28($7)
	lwc1	$f0, 32($4)
	lwc1	$f4, 32($5)
	lwc1	$f8, 32($6)
	lwc1	$f1, 36($4)
	lwc1	$f5, 36($5)
	lwc1	$f9, 36($6)
	lwc1	$f2, 40($4)
	lwc1	$f6, 40($5)
	lwc1	$f10, 40($6)
	lwc1	$f3, 44($4)
	lwc1	$f7, 44($5)
	lwc1	$f11, 44($6)
	mul.s	$f16, $f0, $f4
	add.s	$f16, $f16, $f8
	mul.s	$f17, $f1, $f5
	add.s	$f17, $f17, $f9
	mul.s	$f18, $f2, $f6
	add.s	$f18, $f18, $f10
	mul.s	$f19, $f3, $f7
	add.s	$f19, $f19, $f11
	swc1	$f16, 32($7)
	swc1	$f17, 36($7)
	swc1	$f18, 40($7)
	swc1	$f19, 44($7)
	lwc1	$f0, 48($4)
	lwc1	$f4, 48($5)
	lwc1	$f8, 48($6)
	lwc1	$f1, 52($4)
	lwc1	$f5, 52($5)
	lwc1	$f9, 52($6)
	lwc1	$f2, 56($4)
	lwc1	$f6, 56($5)
	lwc1	$f10, 56($6)
	lwc1	$f3, 60($4)
	lwc1	$f7, 60($5)
	lwc1	$f11, 60($6)
	mul.s	$f16, $f0, $f4
	add.s	$f16, $f16, $f8
	mul.s	$f17, $f1, $f5
	add.s	$f17, $f17, $f9
	mul.s	$f18, $f2, $f6
	add.s	$f18, $f18, $f10
	mul.s	$f19, $f3, $f7
	add.s	$f19, $f19, $f11
	swc1	$f16, 48($7)
	swc1	$f17, 52($7)
	swc1	$f18, 56($7)
	swc1	$f19, 60($7)
	addiu	$4, $4, 64
	addiu	$5, $5, 64
	addiu	$6, $6, 64
	addiu	$8, $8, -4
	bgtz	$8, 1b
	addiu	$7, $7, 64
2:	jr	$ra
	nop
	.end	vpu_vector_madd

# vpu_matrix_vector_multiply(mat $4, in $5, out $6, count $7):
#   out[i] = mat[0] * in[i].x + mat[1] * in[i].y + mat[2] * in[i].z
#          + mat[3] * in[i].w
# Optimized (FPU): the columns in $f0-$f15; per vector 4 lwc1, a chain
# through ACC per lane and 4 swc1.
	.p2align 3
	.globl	vpu_matrix_vector_multiply
	.ent	vpu_matrix_vector_multiply
vpu_matrix_vector_multiply:
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
	madda.s	$f4, $f17
	madda.s	$f8, $f18
	madd.s	$f21, $f12, $f19
	mula.s	$f1, $f16
	madda.s	$f5, $f17
	madda.s	$f9, $f18
	madd.s	$f23, $f13, $f19
	mula.s	$f2, $f16
	madda.s	$f6, $f17
	madda.s	$f10, $f18
	madd.s	$f25, $f14, $f19
	mula.s	$f3, $f16
	madda.s	$f7, $f17
	madda.s	$f11, $f18
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
	.end	vpu_matrix_vector_multiply

# vpu_dot_product(a $4, b $5, result $6, count $7):
#   result[i] = ((p.x + p.y) + p.z) + p.w with p = a[i] * b[i]
# Optimized (FPU): the sum of products is one chain through ACC (mula.s,
# madda.s, madda.s, madd.s), in the order of the C sum; 4 vectors per
# iteration, prefetched 2 lines ahead. count is a multiple of 4.
	.p2align 3
	.globl	vpu_dot_product
	.ent	vpu_dot_product
vpu_dot_product:
	blez	$7, 2f
	nop
	.p2align 3
1:	pref	0, 128($4)
	pref	0, 128($5)
	lwc1	$f0, 0($4)
	lwc1	$f4, 0($5)
	lwc1	$f1, 4($4)
	lwc1	$f5, 4($5)
	lwc1	$f2, 8($4)
	lwc1	$f6, 8($5)
	lwc1	$f3, 12($4)
	lwc1	$f7, 12($5)
	mula.s	$f0, $f4
	madda.s	$f1, $f5
	madda.s	$f2, $f6
	madd.s	$f21, $f3, $f7
	lwc1	$f0, 16($4)
	lwc1	$f4, 16($5)
	lwc1	$f1, 20($4)
	lwc1	$f5, 20($5)
	lwc1	$f2, 24($4)
	lwc1	$f6, 24($5)
	lwc1	$f3, 28($4)
	lwc1	$f7, 28($5)
	mula.s	$f0, $f4
	madda.s	$f1, $f5
	madda.s	$f2, $f6
	madd.s	$f23, $f3, $f7
	lwc1	$f0, 32($4)
	lwc1	$f4, 32($5)
	lwc1	$f1, 36($4)
	lwc1	$f5, 36($5)
	lwc1	$f2, 40($4)
	lwc1	$f6, 40($5)
	lwc1	$f3, 44($4)
	lwc1	$f7, 44($5)
	mula.s	$f0, $f4
	madda.s	$f1, $f5
	madda.s	$f2, $f6
	madd.s	$f25, $f3, $f7
	lwc1	$f0, 48($4)
	lwc1	$f4, 48($5)
	lwc1	$f1, 52($4)
	lwc1	$f5, 52($5)
	lwc1	$f2, 56($4)
	lwc1	$f6, 56($5)
	lwc1	$f3, 60($4)
	lwc1	$f7, 60($5)
	mula.s	$f0, $f4
	madda.s	$f1, $f5
	madda.s	$f2, $f6
	madd.s	$f27, $f3, $f7
	addiu	$4, $4, 64
	addiu	$5, $5, 64
	swc1	$f21, 0($6)
	swc1	$f23, 4($6)
	swc1	$f25, 8($6)
	swc1	$f27, 12($6)
	addiu	$7, $7, -4
	bgtz	$7, 1b
	addiu	$6, $6, 16
2:	jr	$ra
	nop
	.end	vpu_dot_product

# vpu_hmax_hmin(a $4, b $5, max_out $6, min_out $7, count $8):
#   v = a[i] + b[i]; max_out[i] = max(max(v.x, v.y), max(v.z, v.w)), min_out
#   the same with min
# Optimized (FPU): add.s per lane, then max.s and min.s trees; 4 vectors per
# iteration, prefetched 2 lines ahead. count is a multiple of 4.
	.p2align 3
	.globl	vpu_hmax_hmin
	.ent	vpu_hmax_hmin
vpu_hmax_hmin:
	blez	$8, 2f
	nop
	.p2align 3
1:	pref	0, 128($4)
	pref	0, 128($5)
	lwc1	$f0, 0($4)
	lwc1	$f4, 0($5)
	lwc1	$f1, 4($4)
	lwc1	$f5, 4($5)
	lwc1	$f2, 8($4)
	lwc1	$f6, 8($5)
	lwc1	$f3, 12($4)
	lwc1	$f7, 12($5)
	add.s	$f0, $f0, $f4
	add.s	$f1, $f1, $f5
	add.s	$f2, $f2, $f6
	add.s	$f3, $f3, $f7
	max.s	$f8, $f0, $f1
	max.s	$f9, $f2, $f3
	min.s	$f10, $f0, $f1
	min.s	$f11, $f2, $f3
	max.s	$f8, $f8, $f9
	min.s	$f10, $f10, $f11
	swc1	$f8, 0($6)
	swc1	$f10, 0($7)
	lwc1	$f0, 16($4)
	lwc1	$f4, 16($5)
	lwc1	$f1, 20($4)
	lwc1	$f5, 20($5)
	lwc1	$f2, 24($4)
	lwc1	$f6, 24($5)
	lwc1	$f3, 28($4)
	lwc1	$f7, 28($5)
	add.s	$f0, $f0, $f4
	add.s	$f1, $f1, $f5
	add.s	$f2, $f2, $f6
	add.s	$f3, $f3, $f7
	max.s	$f8, $f0, $f1
	max.s	$f9, $f2, $f3
	min.s	$f10, $f0, $f1
	min.s	$f11, $f2, $f3
	max.s	$f8, $f8, $f9
	min.s	$f10, $f10, $f11
	swc1	$f8, 4($6)
	swc1	$f10, 4($7)
	lwc1	$f0, 32($4)
	lwc1	$f4, 32($5)
	lwc1	$f1, 36($4)
	lwc1	$f5, 36($5)
	lwc1	$f2, 40($4)
	lwc1	$f6, 40($5)
	lwc1	$f3, 44($4)
	lwc1	$f7, 44($5)
	add.s	$f0, $f0, $f4
	add.s	$f1, $f1, $f5
	add.s	$f2, $f2, $f6
	add.s	$f3, $f3, $f7
	max.s	$f8, $f0, $f1
	max.s	$f9, $f2, $f3
	min.s	$f10, $f0, $f1
	min.s	$f11, $f2, $f3
	max.s	$f8, $f8, $f9
	min.s	$f10, $f10, $f11
	swc1	$f8, 8($6)
	swc1	$f10, 8($7)
	lwc1	$f0, 48($4)
	lwc1	$f4, 48($5)
	lwc1	$f1, 52($4)
	lwc1	$f5, 52($5)
	lwc1	$f2, 56($4)
	lwc1	$f6, 56($5)
	lwc1	$f3, 60($4)
	lwc1	$f7, 60($5)
	add.s	$f0, $f0, $f4
	add.s	$f1, $f1, $f5
	add.s	$f2, $f2, $f6
	add.s	$f3, $f3, $f7
	max.s	$f8, $f0, $f1
	max.s	$f9, $f2, $f3
	min.s	$f10, $f0, $f1
	min.s	$f11, $f2, $f3
	max.s	$f8, $f8, $f9
	min.s	$f10, $f10, $f11
	swc1	$f8, 12($6)
	swc1	$f10, 12($7)
	addiu	$4, $4, 64
	addiu	$5, $5, 64
	addiu	$6, $6, 16
	addiu	$8, $8, -4
	bgtz	$8, 1b
	addiu	$7, $7, 16
2:	jr	$ra
	nop
	.end	vpu_hmax_hmin

#endif // ASM_VU0
