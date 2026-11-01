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
# of a vector of ones: 1 cycle per step), qmfc2; ordered by asm/gen/sched.py
# (the loads interleaved with the work of the vectors before), the inputs
# prefetched 5 lines ahead and the output 5 vectors ahead, as feature clang
# (model: 35 cycles per 4). count is a multiple of 4.
# Measured: the loads of 4 vectors first, PREF 2 lines ahead: 122 MFLOPS
# (feature-vu0 151).
	.p2align 3
	.globl	vpu_dot_product
	.ent	vpu_dot_product
vpu_dot_product:
	vaddw.xyzw	$vf20, $vf0, $vf0w	# x = 1 (vf0 = 0, 0, 0, 1)
	blez	$7, 2f
	nop
	.p2align 3
1:	lqc2	$vf1, 0($4)
	addiu	$7, $7, -4
	lqc2	$vf2, 0($5)
	lqc2	$vf3, 16($4)
	vmul.xyzw	$vf1, $vf1, $vf2
	pref	0, 320($4)
	lqc2	$vf4, 16($5)
	lqc2	$vf5, 32($4)
	vmul.xyzw	$vf3, $vf3, $vf4
	pref	0, 320($5)
	vadday.x	$ACC, $vf1, $vf1y
	pref	0, 80($6)
	vmaddaz.x	$ACC, $vf20, $vf1z
	vmaddw.x	$vf11, $vf20, $vf1w
	vadday.x	$ACC, $vf3, $vf3y
	lqc2	$vf6, 32($5)
	vmaddaz.x	$ACC, $vf20, $vf3z
	vmaddw.x	$vf12, $vf20, $vf3w
	vmul.xyzw	$vf5, $vf5, $vf6
	lqc2	$vf7, 48($4)
	lqc2	$vf8, 48($5)
	addiu	$4, $4, 64
	qmfc2	$8, $vf11
	addiu	$5, $5, 64
	vadday.x	$ACC, $vf5, $vf5y
	sw	$8, 0($6)
	vmaddaz.x	$ACC, $vf20, $vf5z
	vmaddw.x	$vf13, $vf20, $vf5w
	vmul.xyzw	$vf7, $vf7, $vf8
	qmfc2	$9, $vf12
	sw	$9, 4($6)
	vadday.x	$ACC, $vf7, $vf7y
	vmaddaz.x	$ACC, $vf20, $vf7z
	vmaddw.x	$vf14, $vf20, $vf7w
	qmfc2	$10, $vf13
	sw	$10, 8($6)
	qmfc2	$11, $vf14
	sw	$11, 12($6)
	bgtz	$7, 1b
	addiu	$6, $6, 16
2:	jr	$ra
	nop
	.end	vpu_dot_product

# vpu_hmax_hmin(a $4, b $5, max_out $6, min_out $7, count $8):
#   v = a[i] + b[i]; max_out[i] = max(max(v.x, v.y), max(v.z, v.w)), min_out
#   the same with min
# Optimized: per vector a max chain (vmaxy.x, vmaxz.x, vmaxw.x) and a min
# chain in field x of two registers: 11 COP2 instructions per vector; 4
# vectors per iteration ordered by asm/gen/sched.py, the inputs prefetched 5
# lines ahead (model: 52 cycles per 4). count is a multiple of 4.
# Measured: the chains interleaved by hand after the loads, PREF 2 lines
# ahead: 10.7 Mvec/s; without PREF 10.9 (feature-vu0 12.0).
	.p2align 3
	.globl	vpu_hmax_hmin
	.ent	vpu_hmax_hmin
vpu_hmax_hmin:
	blez	$8, 2f
	nop
	.p2align 3
1:	lqc2	$vf1, 0($4)
	addiu	$8, $8, -4
	lqc2	$vf2, 0($5)
	lqc2	$vf3, 16($4)
	vadd.xyzw	$vf1, $vf1, $vf2
	pref	0, 320($4)
	lqc2	$vf4, 16($5)
	lqc2	$vf5, 32($4)
	vadd.xyzw	$vf3, $vf3, $vf4
	pref	0, 320($5)
	lqc2	$vf6, 32($5)
	vmaxy.x	$vf11, $vf1, $vf1y
	vadd.xyzw	$vf5, $vf5, $vf6
	lqc2	$vf7, 48($4)
	lqc2	$vf8, 48($5)
	addiu	$4, $4, 64
	vminiy.x	$vf15, $vf1, $vf1y
	addiu	$5, $5, 64
	vmaxy.x	$vf12, $vf3, $vf3y
	vadd.xyzw	$vf7, $vf7, $vf8
	vminiy.x	$vf16, $vf3, $vf3y
	vmaxz.x	$vf11, $vf11, $vf1z
	vmaxy.x	$vf13, $vf5, $vf5y
	vminiz.x	$vf15, $vf15, $vf1z
	vminiy.x	$vf17, $vf5, $vf5y
	vmaxz.x	$vf12, $vf12, $vf3z
	vmaxy.x	$vf14, $vf7, $vf7y
	vminiz.x	$vf16, $vf16, $vf3z
	vminiy.x	$vf18, $vf7, $vf7y
	vmaxw.x	$vf11, $vf11, $vf1w
	vmaxz.x	$vf13, $vf13, $vf5z
	vminiw.x	$vf15, $vf15, $vf1w
	vminiz.x	$vf17, $vf17, $vf5z
	vmaxw.x	$vf12, $vf12, $vf3w
	vmaxz.x	$vf14, $vf14, $vf7z
	vminiw.x	$vf16, $vf16, $vf3w
	vminiz.x	$vf18, $vf18, $vf7z
	vmaxw.x	$vf13, $vf13, $vf5w
	qmfc2	$9, $vf11
	vminiw.x	$vf17, $vf17, $vf5w
	qmfc2	$13, $vf15
	sw	$9, 0($6)
	vmaxw.x	$vf14, $vf14, $vf7w
	sw	$13, 0($7)
	qmfc2	$10, $vf12
	vminiw.x	$vf18, $vf18, $vf7w
	qmfc2	$14, $vf16
	sw	$10, 4($6)
	sw	$14, 4($7)
	qmfc2	$11, $vf13
	qmfc2	$15, $vf17
	sw	$11, 8($6)
	sw	$15, 8($7)
	qmfc2	$12, $vf14
	qmfc2	$24, $vf18
	sw	$12, 12($6)
	sw	$24, 12($7)
	addiu	$6, $6, 16
	bgtz	$8, 1b
	addiu	$7, $7, 16
2:	jr	$ra
	nop
	.end	vpu_hmax_hmin


#else // ASM_VU0: the FPU versions (asm/gen/fpu_novu0.py)

# vpu_vector_divide(a $4, b $5, result $6, count $7):  result[i] = a[i] / b[i]
# Optimized (FPU): div.s back to back (the divider takes one every 7 cycles:
# 28 cycles per vector); the loads and stores run while it divides; 2
# vectors per iteration (model: 64 cycles). count is even.
	.p2align 3
	.globl	vpu_vector_divide
	.ent	vpu_vector_divide
vpu_vector_divide:
	blez	$7, 2f
	nop
	.p2align 3
1:	lwc1	$f0, 0($4)
	addiu	$7, $7, -2
	lwc1	$f4, 0($5)
	lwc1	$f1, 4($4)
	lwc1	$f5, 4($5)
	lwc1	$f2, 8($4)
	lwc1	$f6, 8($5)
	div.s	$f16, $f0, $f4
	lwc1	$f3, 12($4)
	lwc1	$f7, 12($5)
	lwc1	$f8, 16($4)
	lwc1	$f12, 16($5)
	lwc1	$f9, 20($4)
	lwc1	$f13, 20($5)
	div.s	$f17, $f1, $f5
	lwc1	$f10, 24($4)
	lwc1	$f14, 24($5)
	lwc1	$f11, 28($4)
	lwc1	$f15, 28($5)
	addiu	$4, $4, 32
	swc1	$f16, 0($6)
	addiu	$5, $5, 32
	div.s	$f18, $f2, $f6
	swc1	$f17, 4($6)
	div.s	$f19, $f3, $f7
	swc1	$f18, 8($6)
	div.s	$f21, $f8, $f12
	swc1	$f19, 12($6)
	div.s	$f23, $f9, $f13
	swc1	$f21, 16($6)
	div.s	$f25, $f10, $f14
	swc1	$f23, 20($6)
	div.s	$f27, $f11, $f15
	swc1	$f25, 24($6)
	swc1	$f27, 28($6)
	bgtz	$7, 1b
	addiu	$6, $6, 32
2:	jr	$ra
	nop
	.end	vpu_vector_divide

# vpu_vector_add(a $4, b $5, result $6, count $7):
#   result[i] = a[i] + b[i]
# Optimized (FPU): a 64-byte line of each array per iteration (4 vectors),
# the inputs and the output prefetched 4 lines ahead (as feature clang), ordered by asm/gen/sched.py
# (model: 65 cycles without misses; the loop waits for the misses of
# arrays 4 KB apart). count is a multiple of 4.
	.p2align 3
	.globl	vpu_vector_add
	.ent	vpu_vector_add
vpu_vector_add:
	blez	$7, 2f
	nop
	.p2align 3
1:	lwc1	$f0, 0($4)
	addiu	$7, $7, -4
	lwc1	$f1, 0($5)
	lwc1	$f2, 4($4)
	lwc1	$f3, 4($5)
	add.s	$f0, $f0, $f1
	lwc1	$f4, 8($4)
	lwc1	$f5, 8($5)
	add.s	$f2, $f2, $f3
	pref	0, 256($4)
	lwc1	$f6, 12($4)
	lwc1	$f7, 12($5)
	add.s	$f4, $f4, $f5
	pref	0, 256($5)
	lwc1	$f8, 16($4)
	lwc1	$f9, 16($5)
	add.s	$f6, $f6, $f7
	pref	0, 256($6)
	lwc1	$f10, 20($4)
	lwc1	$f11, 20($5)
	add.s	$f8, $f8, $f9
	lwc1	$f12, 24($4)
	lwc1	$f13, 24($5)
	swc1	$f0, 0($6)
	add.s	$f10, $f10, $f11
	lwc1	$f14, 28($4)
	lwc1	$f15, 28($5)
	swc1	$f2, 4($6)
	add.s	$f12, $f12, $f13
	lwc1	$f16, 32($4)
	lwc1	$f17, 32($5)
	swc1	$f4, 8($6)
	add.s	$f14, $f14, $f15
	lwc1	$f18, 36($4)
	lwc1	$f19, 36($5)
	swc1	$f6, 12($6)
	add.s	$f16, $f16, $f17
	lwc1	$f21, 40($4)
	lwc1	$f23, 40($5)
	swc1	$f8, 16($6)
	add.s	$f18, $f18, $f19
	lwc1	$f25, 44($4)
	lwc1	$f27, 44($5)
	swc1	$f10, 20($6)
	add.s	$f21, $f21, $f23
	lwc1	$f29, 48($4)
	lwc1	$f31, 48($5)
	swc1	$f12, 24($6)
	add.s	$f25, $f25, $f27
	lwc1	$f0, 52($4)
	lwc1	$f1, 52($5)
	swc1	$f14, 28($6)
	add.s	$f29, $f29, $f31
	lwc1	$f2, 56($4)
	lwc1	$f3, 56($5)
	swc1	$f16, 32($6)
	add.s	$f0, $f0, $f1
	lwc1	$f4, 60($4)
	lwc1	$f5, 60($5)
	addiu	$4, $4, 64
	swc1	$f18, 36($6)
	addiu	$5, $5, 64
	add.s	$f2, $f2, $f3
	swc1	$f21, 40($6)
	add.s	$f4, $f4, $f5
	swc1	$f25, 44($6)
	swc1	$f29, 48($6)
	swc1	$f0, 52($6)
	swc1	$f2, 56($6)
	swc1	$f4, 60($6)
	bgtz	$7, 1b
	addiu	$6, $6, 64
2:	jr	$ra
	nop
	.end	vpu_vector_add

# vpu_vector_mul(a $4, b $5, result $6, count $7):
#   result[i] = a[i] * b[i]
# Optimized (FPU): a 64-byte line of each array per iteration (4 vectors),
# the inputs and the output prefetched 4 lines ahead (as feature clang), ordered by asm/gen/sched.py
# (model: 65 cycles without misses; the loop waits for the misses of
# arrays 4 KB apart). count is a multiple of 4.
	.p2align 3
	.globl	vpu_vector_mul
	.ent	vpu_vector_mul
vpu_vector_mul:
	blez	$7, 2f
	nop
	.p2align 3
1:	lwc1	$f0, 0($4)
	addiu	$7, $7, -4
	lwc1	$f1, 0($5)
	lwc1	$f2, 4($4)
	lwc1	$f3, 4($5)
	mul.s	$f0, $f0, $f1
	lwc1	$f4, 8($4)
	lwc1	$f5, 8($5)
	mul.s	$f2, $f2, $f3
	pref	0, 256($4)
	lwc1	$f6, 12($4)
	lwc1	$f7, 12($5)
	mul.s	$f4, $f4, $f5
	pref	0, 256($5)
	lwc1	$f8, 16($4)
	lwc1	$f9, 16($5)
	mul.s	$f6, $f6, $f7
	pref	0, 256($6)
	lwc1	$f10, 20($4)
	lwc1	$f11, 20($5)
	mul.s	$f8, $f8, $f9
	lwc1	$f12, 24($4)
	lwc1	$f13, 24($5)
	swc1	$f0, 0($6)
	mul.s	$f10, $f10, $f11
	lwc1	$f14, 28($4)
	lwc1	$f15, 28($5)
	swc1	$f2, 4($6)
	mul.s	$f12, $f12, $f13
	lwc1	$f16, 32($4)
	lwc1	$f17, 32($5)
	swc1	$f4, 8($6)
	mul.s	$f14, $f14, $f15
	lwc1	$f18, 36($4)
	lwc1	$f19, 36($5)
	swc1	$f6, 12($6)
	mul.s	$f16, $f16, $f17
	lwc1	$f21, 40($4)
	lwc1	$f23, 40($5)
	swc1	$f8, 16($6)
	mul.s	$f18, $f18, $f19
	lwc1	$f25, 44($4)
	lwc1	$f27, 44($5)
	swc1	$f10, 20($6)
	mul.s	$f21, $f21, $f23
	lwc1	$f29, 48($4)
	lwc1	$f31, 48($5)
	swc1	$f12, 24($6)
	mul.s	$f25, $f25, $f27
	lwc1	$f0, 52($4)
	lwc1	$f1, 52($5)
	swc1	$f14, 28($6)
	mul.s	$f29, $f29, $f31
	lwc1	$f2, 56($4)
	lwc1	$f3, 56($5)
	swc1	$f16, 32($6)
	mul.s	$f0, $f0, $f1
	lwc1	$f4, 60($4)
	lwc1	$f5, 60($5)
	addiu	$4, $4, 64
	swc1	$f18, 36($6)
	addiu	$5, $5, 64
	mul.s	$f2, $f2, $f3
	swc1	$f21, 40($6)
	mul.s	$f4, $f4, $f5
	swc1	$f25, 44($6)
	swc1	$f29, 48($6)
	swc1	$f0, 52($6)
	swc1	$f2, 56($6)
	swc1	$f4, 60($6)
	bgtz	$7, 1b
	addiu	$6, $6, 64
2:	jr	$ra
	nop
	.end	vpu_vector_mul

# vpu_vector_madd(a $4, b $5, c $6, result $7, count $8):
#   result[i] = a[i] * b[i] + c[i]
# Optimized (FPU): a 64-byte line of each array per iteration (4 vectors),
# the inputs and the output prefetched 4 lines ahead (as feature clang), ordered by asm/gen/sched.py
# (model: 97 cycles without misses; the loop waits for the misses of
# arrays 4 KB apart). count is a multiple of 4.
	.p2align 3
	.globl	vpu_vector_madd
	.ent	vpu_vector_madd
vpu_vector_madd:
	blez	$8, 2f
	nop
	.p2align 3
1:	lwc1	$f0, 0($4)
	addiu	$8, $8, -4
	lwc1	$f1, 0($5)
	lwc1	$f3, 4($4)
	lwc1	$f4, 4($5)
	mul.s	$f0, $f0, $f1
	lwc1	$f6, 8($4)
	lwc1	$f7, 8($5)
	mul.s	$f3, $f3, $f4
	pref	0, 256($4)
	lwc1	$f9, 12($4)
	lwc1	$f10, 12($5)
	lwc1	$f2, 0($6)
	mul.s	$f6, $f6, $f7
	lwc1	$f12, 16($4)
	lwc1	$f13, 16($5)
	lwc1	$f5, 4($6)
	mul.s	$f9, $f9, $f10
	lwc1	$f15, 20($4)
	lwc1	$f16, 20($5)
	add.s	$f0, $f0, $f2
	pref	0, 256($5)
	lwc1	$f8, 8($6)
	mul.s	$f12, $f12, $f13
	lwc1	$f18, 24($4)
	lwc1	$f19, 24($5)
	add.s	$f3, $f3, $f5
	pref	0, 256($6)
	lwc1	$f11, 12($6)
	mul.s	$f15, $f15, $f16
	add.s	$f6, $f6, $f8
	pref	0, 256($7)
	lwc1	$f14, 16($6)
	mul.s	$f18, $f18, $f19
	lwc1	$f23, 28($4)
	lwc1	$f25, 28($5)
	add.s	$f9, $f9, $f11
	lwc1	$f17, 20($6)
	lwc1	$f29, 32($4)
	lwc1	$f31, 32($5)
	add.s	$f12, $f12, $f14
	lwc1	$f21, 24($6)
	mul.s	$f23, $f23, $f25
	lwc1	$f1, 36($4)
	lwc1	$f2, 36($5)
	add.s	$f15, $f15, $f17
	mul.s	$f29, $f29, $f31
	lwc1	$f4, 40($4)
	lwc1	$f5, 40($5)
	swc1	$f0, 0($7)
	add.s	$f18, $f18, $f21
	lwc1	$f27, 28($6)
	mul.s	$f1, $f1, $f2
	lwc1	$f7, 44($4)
	lwc1	$f8, 44($5)
	swc1	$f3, 4($7)
	lwc1	$f0, 32($6)
	mul.s	$f4, $f4, $f5
	lwc1	$f10, 48($4)
	lwc1	$f11, 48($5)
	swc1	$f6, 8($7)
	add.s	$f23, $f23, $f27
	lwc1	$f3, 36($6)
	mul.s	$f7, $f7, $f8
	lwc1	$f13, 52($4)
	lwc1	$f14, 52($5)
	swc1	$f9, 12($7)
	add.s	$f29, $f29, $f0
	lwc1	$f6, 40($6)
	mul.s	$f10, $f10, $f11
	lwc1	$f16, 56($4)
	lwc1	$f17, 56($5)
	swc1	$f12, 16($7)
	add.s	$f1, $f1, $f3
	lwc1	$f9, 44($6)
	mul.s	$f13, $f13, $f14
	lwc1	$f19, 60($4)
	addiu	$4, $4, 64
	lwc1	$f21, 60($5)
	addiu	$5, $5, 64
	swc1	$f15, 20($7)
	add.s	$f4, $f4, $f6
	lwc1	$f12, 48($6)
	mul.s	$f16, $f16, $f17
	swc1	$f18, 24($7)
	add.s	$f7, $f7, $f9
	lwc1	$f15, 52($6)
	mul.s	$f19, $f19, $f21
	swc1	$f23, 28($7)
	add.s	$f10, $f10, $f12
	lwc1	$f18, 56($6)
	swc1	$f29, 32($7)
	add.s	$f13, $f13, $f15
	lwc1	$f23, 60($6)
	swc1	$f1, 36($7)
	addiu	$6, $6, 64
	add.s	$f16, $f16, $f18
	swc1	$f4, 40($7)
	add.s	$f19, $f19, $f23
	swc1	$f7, 44($7)
	swc1	$f10, 48($7)
	swc1	$f13, 52($7)
	swc1	$f16, 56($7)
	swc1	$f19, 60($7)
	bgtz	$8, 1b
	addiu	$7, $7, 64
2:	jr	$ra
	nop
	.end	vpu_vector_madd

# vpu_matrix_vector_multiply(mat $4, in $5, out $6, count $7):
#   out[i] = mat[0] * in[i].x + mat[1] * in[i].y + mat[2] * in[i].z
#          + mat[3] * in[i].w
# Optimized (FPU): the columns in $f0-$f15; 4 vectors per iteration, a chain
# through ACC per lane, ordered by asm/gen/sched.py (model: 100 cycles
# per 4 vectors). count is a multiple of 4. Measured with 2 vectors per
# iteration: 321 MFLOPS.
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
	addiu	$7, $7, -4
	lwc1	$f17, 4($5)
	lwc1	$f18, 8($5)
	mula.s	$f0, $f16
	lwc1	$f19, 12($5)
	madda.s	$f4, $f17
	madda.s	$f8, $f18
	madd.s	$f29, $f12, $f19
	mula.s	$f1, $f16
	madda.s	$f5, $f17
	madda.s	$f9, $f18
	madd.s	$f31, $f13, $f19
	mula.s	$f2, $f16
	madda.s	$f6, $f17
	swc1	$f29, 0($6)
	madda.s	$f10, $f18
	madd.s	$f29, $f14, $f19
	mula.s	$f3, $f16
	madda.s	$f7, $f17
	swc1	$f31, 4($6)
	madda.s	$f11, $f18
	madd.s	$f31, $f15, $f19
	lwc1	$f21, 16($5)
	lwc1	$f23, 20($5)
	lwc1	$f25, 24($5)
	mula.s	$f0, $f21
	lwc1	$f27, 28($5)
	madda.s	$f4, $f23
	swc1	$f29, 8($6)
	madda.s	$f8, $f25
	madd.s	$f29, $f12, $f27
	mula.s	$f1, $f21
	madda.s	$f5, $f23
	swc1	$f31, 12($6)
	madda.s	$f9, $f25
	madd.s	$f31, $f13, $f27
	mula.s	$f2, $f21
	madda.s	$f6, $f23
	swc1	$f29, 16($6)
	madda.s	$f10, $f25
	madd.s	$f29, $f14, $f27
	mula.s	$f3, $f21
	madda.s	$f7, $f23
	swc1	$f31, 20($6)
	madda.s	$f11, $f25
	madd.s	$f31, $f15, $f27
	lwc1	$f16, 32($5)
	lwc1	$f17, 36($5)
	lwc1	$f18, 40($5)
	mula.s	$f0, $f16
	lwc1	$f19, 44($5)
	madda.s	$f4, $f17
	swc1	$f29, 24($6)
	madda.s	$f8, $f18
	madd.s	$f29, $f12, $f19
	mula.s	$f1, $f16
	madda.s	$f5, $f17
	swc1	$f31, 28($6)
	madda.s	$f9, $f18
	madd.s	$f31, $f13, $f19
	mula.s	$f2, $f16
	madda.s	$f6, $f17
	swc1	$f29, 32($6)
	madda.s	$f10, $f18
	madd.s	$f29, $f14, $f19
	mula.s	$f3, $f16
	madda.s	$f7, $f17
	swc1	$f31, 36($6)
	madda.s	$f11, $f18
	madd.s	$f31, $f15, $f19
	lwc1	$f21, 48($5)
	lwc1	$f23, 52($5)
	lwc1	$f25, 56($5)
	mula.s	$f0, $f21
	lwc1	$f27, 60($5)
	madda.s	$f4, $f23
	addiu	$5, $5, 64
	swc1	$f29, 40($6)
	madda.s	$f8, $f25
	madd.s	$f29, $f12, $f27
	mula.s	$f1, $f21
	madda.s	$f5, $f23
	swc1	$f31, 44($6)
	madda.s	$f9, $f25
	madd.s	$f31, $f13, $f27
	mula.s	$f2, $f21
	madda.s	$f6, $f23
	swc1	$f29, 48($6)
	madda.s	$f10, $f25
	madd.s	$f29, $f14, $f27
	mula.s	$f3, $f21
	madda.s	$f7, $f23
	swc1	$f31, 52($6)
	madda.s	$f11, $f25
	madd.s	$f31, $f15, $f27
	swc1	$f29, 56($6)
	swc1	$f31, 60($6)
	bgtz	$7, 1b
	addiu	$6, $6, 64
2:	jr	$ra
	nop
	.end	vpu_matrix_vector_multiply

# vpu_dot_product(a $4, b $5, result $6, count $7):
#   result[i] = ((p.x + p.y) + p.z) + p.w with p = a[i] * b[i]
# Optimized (FPU): the sum of products is one chain through ACC (mula.s,
# madda.s, madda.s, madd.s) in the order of the C sum; 4 vectors per
# iteration, prefetched 4 lines ahead (as feature clang), ordered by
# asm/gen/sched.py. The bound is the COP1 slot: 13 per vector (model:
# 54 cycles per 4). count is a multiple of 4.
	.p2align 3
	.globl	vpu_dot_product
	.ent	vpu_dot_product
vpu_dot_product:
	blez	$7, 2f
	nop
	.p2align 3
1:	lwc1	$f0, 0($4)
	addiu	$7, $7, -4
	lwc1	$f4, 0($5)
	lwc1	$f1, 4($4)
	lwc1	$f5, 4($5)
	lwc1	$f2, 8($4)
	lwc1	$f6, 8($5)
	mula.s	$f0, $f4
	lwc1	$f3, 12($4)
	lwc1	$f7, 12($5)
	madda.s	$f1, $f5
	pref	0, 256($4)
	madda.s	$f2, $f6
	pref	0, 256($5)
	madd.s	$f16, $f3, $f7
	pref	0, 64($6)
	lwc1	$f8, 16($4)
	lwc1	$f12, 16($5)
	lwc1	$f9, 20($4)
	lwc1	$f13, 20($5)
	lwc1	$f10, 24($4)
	lwc1	$f14, 24($5)
	mula.s	$f8, $f12
	lwc1	$f11, 28($4)
	lwc1	$f15, 28($5)
	madda.s	$f9, $f13
	madda.s	$f10, $f14
	madd.s	$f17, $f11, $f15
	lwc1	$f0, 32($4)
	lwc1	$f4, 32($5)
	lwc1	$f1, 36($4)
	lwc1	$f5, 36($5)
	lwc1	$f2, 40($4)
	lwc1	$f6, 40($5)
	mula.s	$f0, $f4
	lwc1	$f3, 44($4)
	lwc1	$f7, 44($5)
	madda.s	$f1, $f5
	madda.s	$f2, $f6
	madd.s	$f18, $f3, $f7
	lwc1	$f8, 48($4)
	lwc1	$f12, 48($5)
	lwc1	$f9, 52($4)
	lwc1	$f13, 52($5)
	lwc1	$f10, 56($4)
	lwc1	$f14, 56($5)
	mula.s	$f8, $f12
	lwc1	$f11, 60($4)
	lwc1	$f15, 60($5)
	addiu	$4, $4, 64
	madda.s	$f9, $f13
	addiu	$5, $5, 64
	madda.s	$f10, $f14
	madd.s	$f19, $f11, $f15
	swc1	$f16, 0($6)
	swc1	$f17, 4($6)
	swc1	$f18, 8($6)
	swc1	$f19, 12($6)
	bgtz	$7, 1b
	addiu	$6, $6, 16
2:	jr	$ra
	nop
	.end	vpu_dot_product

# vpu_hmax_hmin(a $4, b $5, max_out $6, min_out $7, count $8):
#   v = a[i] + b[i]; max_out[i] = max(max(v.x, v.y), max(v.z, v.w)), min_out
#   the same with min
# Optimized (FPU): add.s per lane, then max.s and min.s trees; 2 vectors per
# iteration, the inputs prefetched 4 lines ahead, ordered by
# asm/gen/sched.py. The bound is the COP1 slot: 20 per vector (model:
# 42 cycles per 2). count is even.
	.p2align 3
	.globl	vpu_hmax_hmin
	.ent	vpu_hmax_hmin
vpu_hmax_hmin:
	blez	$8, 2f
	nop
	.p2align 3
1:	lwc1	$f0, 0($4)
	addiu	$8, $8, -2
	lwc1	$f1, 4($4)
	lwc1	$f2, 8($4)
	lwc1	$f3, 12($4)
	lwc1	$f4, 0($5)
	lwc1	$f5, 4($5)
	lwc1	$f6, 8($5)
	lwc1	$f7, 12($5)
	add.s	$f0, $f0, $f4
	add.s	$f1, $f1, $f5
	pref	0, 256($4)
	add.s	$f2, $f2, $f6
	pref	0, 256($5)
	add.s	$f3, $f3, $f7
	lwc1	$f8, 16($4)
	lwc1	$f9, 20($4)
	lwc1	$f10, 24($4)
	lwc1	$f11, 28($4)
	addiu	$4, $4, 32
	lwc1	$f12, 16($5)
	lwc1	$f13, 20($5)
	lwc1	$f14, 24($5)
	lwc1	$f15, 28($5)
	add.s	$f8, $f8, $f12
	addiu	$5, $5, 32
	add.s	$f9, $f9, $f13
	add.s	$f10, $f10, $f14
	add.s	$f11, $f11, $f15
	max.s	$f4, $f0, $f1
	max.s	$f5, $f2, $f3
	min.s	$f6, $f0, $f1
	min.s	$f7, $f2, $f3
	max.s	$f12, $f8, $f9
	max.s	$f13, $f10, $f11
	min.s	$f14, $f8, $f9
	min.s	$f15, $f10, $f11
	max.s	$f4, $f4, $f5
	min.s	$f6, $f6, $f7
	max.s	$f12, $f12, $f13
	min.s	$f14, $f14, $f15
	swc1	$f4, 0($6)
	swc1	$f6, 0($7)
	swc1	$f12, 4($6)
	swc1	$f14, 4($7)
	addiu	$6, $6, 8
	bgtz	$8, 1b
	addiu	$7, $7, 8
2:	jr	$ra
	nop
	.end	vpu_hmax_hmin

#endif // ASM_VU0
