# Hand-written R5900 versions of the stream_tests.c kernels (bench variant
# asm). n32 ABI (a float argument is in $f12 + its position, so a of
# stream_saxpy is in $f15). See int_tests.S for what "naive" means. The _pref
# kernels prefetch 4 lines (256 bytes) ahead, once per 64-byte line, as the C
# kernels do; PREF does not fault past the end of the arrays.

	.set	noreorder
	.set	noat
	.text

# stream_sum(in $4, count $5):
#   returns the sum of in[i] (mod 2^32)
# Optimized: no (naive: one element and one sum).
	.p2align 3
	.globl	stream_sum
	.ent	stream_sum
stream_sum:
	move	$2, $0
	blez	$5, 2f
	nop
1:	lw	$8, 0($4)
	addu	$2, $2, $8
	addiu	$4, $4, 4
	addiu	$5, $5, -1
	bnez	$5, 1b
	nop
2:	jr	$ra
	nop
	.end	stream_sum

# stream_scale(in $4, out $5, count $6):
#   out[i] = in[i] * 3 + 1
# Optimized: no (naive: mult and addiu per element).
	.p2align 3
	.globl	stream_scale
	.ent	stream_scale
stream_scale:
	blez	$6, 2f
	addiu	$9, $0, 3
1:	lw	$8, 0($4)
	mult	$8, $8, $9
	addiu	$8, $8, 1
	sw	$8, 0($5)
	addiu	$4, $4, 4
	addiu	$5, $5, 4
	addiu	$6, $6, -1
	bnez	$6, 1b
	nop
2:	jr	$ra
	nop
	.end	stream_scale

# stream_saxpy(x $4, y $5, count $6, a $f15):
#   y[i] = a * x[i] + y[i]
# Optimized: no (naive: mul.s and add.s per element).
	.p2align 3
	.globl	stream_saxpy
	.ent	stream_saxpy
stream_saxpy:
	blez	$6, 2f
	nop
1:	lwc1	$f0, 0($4)
	lwc1	$f1, 0($5)
	mul.s	$f0, $f0, $f15
	add.s	$f0, $f0, $f1
	swc1	$f0, 0($5)
	addiu	$4, $4, 4
	addiu	$5, $5, 4
	addiu	$6, $6, -1
	bnez	$6, 1b
	nop
2:	jr	$ra
	nop
	.end	stream_saxpy

# stream_sum_pref(in $4, count $5):
#   stream_sum with a PREF per line
# Optimized: no (naive: as stream_sum, a PREF per line).
	.p2align 3
	.globl	stream_sum_pref
	.ent	stream_sum_pref
stream_sum_pref:
	move	$2, $0
	blez	$5, 3f
	nop
1:	pref	0, 256($4)
	addiu	$9, $4, 64		# end of the line
2:	lw	$8, 0($4)
	addu	$2, $2, $8
	addiu	$4, $4, 4
	bne	$4, $9, 2b
	nop
	addiu	$5, $5, -16
	bgtz	$5, 1b
	nop
3:	jr	$ra
	nop
	.end	stream_sum_pref

# stream_scale_pref(in $4, out $5, count $6):
#   stream_scale with a PREF of in and of out per line
# Optimized: no (naive: as stream_scale, a PREF per line).
	.p2align 3
	.globl	stream_scale_pref
	.ent	stream_scale_pref
stream_scale_pref:
	blez	$6, 3f
	addiu	$9, $0, 3
1:	pref	0, 256($4)
	pref	0, 256($5)
	addiu	$10, $4, 64
2:	lw	$8, 0($4)
	mult	$8, $8, $9
	addiu	$8, $8, 1
	sw	$8, 0($5)
	addiu	$4, $4, 4
	bne	$4, $10, 2b
	addiu	$5, $5, 4
	addiu	$6, $6, -16
	bgtz	$6, 1b
	nop
3:	jr	$ra
	nop
	.end	stream_scale_pref

# stream_saxpy_pref(x $4, y $5, count $6, a $f15):
#   stream_saxpy with a PREF of x and of y per line
# Optimized: no (naive: as stream_saxpy, a PREF per line).
	.p2align 3
	.globl	stream_saxpy_pref
	.ent	stream_saxpy_pref
stream_saxpy_pref:
	blez	$6, 3f
	nop
1:	pref	0, 256($4)
	pref	0, 256($5)
	addiu	$10, $4, 64
2:	lwc1	$f0, 0($4)
	lwc1	$f1, 0($5)
	mul.s	$f0, $f0, $f15
	add.s	$f0, $f0, $f1
	swc1	$f0, 0($5)
	addiu	$4, $4, 4
	bne	$4, $10, 2b
	addiu	$5, $5, 4
	addiu	$6, $6, -16
	bgtz	$6, 1b
	nop
3:	jr	$ra
	nop
	.end	stream_saxpy_pref
