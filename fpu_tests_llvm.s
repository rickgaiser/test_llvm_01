	.section	.mdebug.abiN32,"",@progbits
	.nan	legacy
	.text
	.file	"fpu_tests.c"
	.globl	print_matrix                    # -- Begin function print_matrix
	.p2align	3
	.type	print_matrix,@function
	.set	nomicromips
	.set	nomips16
	.ent	print_matrix
print_matrix:                           # @print_matrix
	.frame	$sp,48,$ra
	.mask 	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	.set	noat
# %bb.0:
	addiu	$sp, $sp, -48
	sd	$ra, 40($sp)                    # 8-byte Folded Spill
	sd	$20, 32($sp)                    # 8-byte Folded Spill
	sd	$19, 24($sp)                    # 8-byte Folded Spill
	sd	$18, 16($sp)                    # 8-byte Folded Spill
	sd	$17, 8($sp)                     # 8-byte Folded Spill
	sd	$16, 0($sp)                     # 8-byte Folded Spill
	move	$16, $5
	move	$5, $4
	jal	printf
	addiu	$4, $gp, %gp_rel(.L.str)
	addiu	$18, $gp, %gp_rel(.L.str.1)
	jal	printf
	move	$4, $18
	sll	$20, $16, 0
	lwc1	$f12, 0($20)
	jal	__extendsfdf2
	nop
	addiu	$16, $gp, %gp_rel(.L.str.2)
	move	$5, $2
	jal	printf
	move	$4, $16
	addiu	$17, $gp, %gp_rel(.L.str.3)
	jal	printf
	move	$4, $17
	lwc1	$f12, 4($20)
	jal	__extendsfdf2
	nop
	move	$5, $2
	jal	printf
	move	$4, $16
	jal	printf
	move	$4, $17
	lwc1	$f12, 8($20)
	jal	__extendsfdf2
	nop
	move	$5, $2
	jal	printf
	move	$4, $16
	jal	printf
	move	$4, $17
	lwc1	$f12, 12($20)
	jal	__extendsfdf2
	nop
	move	$5, $2
	jal	printf
	move	$4, $16
	addiu	$19, $gp, %gp_rel(.Lstr.38)
	jal	puts
	move	$4, $19
	jal	printf
	move	$4, $18
	lwc1	$f12, 16($20)
	jal	__extendsfdf2
	nop
	move	$5, $2
	jal	printf
	move	$4, $16
	jal	printf
	move	$4, $17
	lwc1	$f12, 20($20)
	jal	__extendsfdf2
	nop
	move	$5, $2
	jal	printf
	move	$4, $16
	jal	printf
	move	$4, $17
	lwc1	$f12, 24($20)
	jal	__extendsfdf2
	nop
	move	$5, $2
	jal	printf
	move	$4, $16
	jal	printf
	move	$4, $17
	lwc1	$f12, 28($20)
	jal	__extendsfdf2
	nop
	move	$5, $2
	jal	printf
	move	$4, $16
	jal	puts
	move	$4, $19
	jal	printf
	move	$4, $18
	lwc1	$f12, 32($20)
	jal	__extendsfdf2
	nop
	move	$5, $2
	jal	printf
	move	$4, $16
	jal	printf
	move	$4, $17
	lwc1	$f12, 36($20)
	jal	__extendsfdf2
	nop
	move	$5, $2
	jal	printf
	move	$4, $16
	jal	printf
	move	$4, $17
	lwc1	$f12, 40($20)
	jal	__extendsfdf2
	nop
	move	$5, $2
	jal	printf
	move	$4, $16
	jal	printf
	move	$4, $17
	lwc1	$f12, 44($20)
	jal	__extendsfdf2
	nop
	move	$5, $2
	jal	printf
	move	$4, $16
	jal	puts
	move	$4, $19
	jal	printf
	move	$4, $18
	lwc1	$f12, 48($20)
	jal	__extendsfdf2
	nop
	move	$5, $2
	jal	printf
	move	$4, $16
	jal	printf
	move	$4, $17
	lwc1	$f12, 52($20)
	jal	__extendsfdf2
	nop
	move	$5, $2
	jal	printf
	move	$4, $16
	jal	printf
	move	$4, $17
	lwc1	$f12, 56($20)
	jal	__extendsfdf2
	nop
	move	$5, $2
	jal	printf
	move	$4, $16
	jal	printf
	move	$4, $17
	lwc1	$f12, 60($20)
	jal	__extendsfdf2
	nop
	move	$5, $2
	jal	printf
	move	$4, $16
	jal	puts
	move	$4, $19
	ld	$16, 0($sp)                     # 8-byte Folded Reload
	ld	$17, 8($sp)                     # 8-byte Folded Reload
	ld	$18, 16($sp)                    # 8-byte Folded Reload
	ld	$19, 24($sp)                    # 8-byte Folded Reload
	ld	$20, 32($sp)                    # 8-byte Folded Reload
	ld	$ra, 40($sp)                    # 8-byte Folded Reload
	jr	$ra
	addiu	$sp, $sp, 48
	.set	at
	.set	macro
	.set	reorder
	.end	print_matrix
.Lfunc_end0:
	.size	print_matrix, .Lfunc_end0-print_matrix
                                        # -- End function
	.globl	print_vector                    # -- Begin function print_vector
	.p2align	3
	.type	print_vector,@function
	.set	nomicromips
	.set	nomips16
	.ent	print_vector
print_vector:                           # @print_vector
	.frame	$sp,32,$ra
	.mask 	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	.set	noat
# %bb.0:
	addiu	$sp, $sp, -32
	sd	$ra, 24($sp)                    # 8-byte Folded Spill
	sd	$18, 16($sp)                    # 8-byte Folded Spill
	sd	$17, 8($sp)                     # 8-byte Folded Spill
	sd	$16, 0($sp)                     # 8-byte Folded Spill
	move	$16, $5
	move	$5, $4
	jal	printf
	addiu	$4, $gp, %gp_rel(.L.str.5)
	sll	$18, $16, 0
	lwc1	$f12, 0($18)
	jal	__extendsfdf2
	nop
	addiu	$16, $gp, %gp_rel(.L.str.2)
	move	$5, $2
	jal	printf
	move	$4, $16
	addiu	$17, $gp, %gp_rel(.L.str.3)
	jal	printf
	move	$4, $17
	lwc1	$f12, 4($18)
	jal	__extendsfdf2
	nop
	move	$5, $2
	jal	printf
	move	$4, $16
	jal	printf
	move	$4, $17
	lwc1	$f12, 8($18)
	jal	__extendsfdf2
	nop
	move	$5, $2
	jal	printf
	move	$4, $16
	jal	printf
	move	$4, $17
	lwc1	$f12, 12($18)
	jal	__extendsfdf2
	nop
	move	$5, $2
	jal	printf
	move	$4, $16
	jal	puts
	addiu	$4, $gp, %gp_rel(.Lstr.38)
	ld	$16, 0($sp)                     # 8-byte Folded Reload
	ld	$17, 8($sp)                     # 8-byte Folded Reload
	ld	$18, 16($sp)                    # 8-byte Folded Reload
	ld	$ra, 24($sp)                    # 8-byte Folded Reload
	jr	$ra
	addiu	$sp, $sp, 32
	.set	at
	.set	macro
	.set	reorder
	.end	print_vector
.Lfunc_end1:
	.size	print_vector, .Lfunc_end1-print_vector
                                        # -- End function
	.globl	matrix_vector_multiply          # -- Begin function matrix_vector_multiply
	.p2align	3
	.type	matrix_vector_multiply,@function
	.set	nomicromips
	.set	nomips16
	.ent	matrix_vector_multiply
matrix_vector_multiply:                 # @matrix_vector_multiply
	.frame	$sp,0,$ra
	.mask 	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	.set	noat
# %bb.0:
	sll	$1, $4, 0
	sll	$2, $5, 0
	mtc1	$zero, $f4
	lwc1	$f2, 0($2)
	lwc1	$f3, 0($1)
	lwc1	$f0, 4($2)
	lwc1	$f1, 4($1)
	lwc1	$f5, 8($1)
	lwc1	$f6, 12($1)
	mul.s	$f3, $f3, $f2
	mul.s	$f1, $f1, $f0
	add.s	$f3, $f3, $f4
	add.s	$f1, $f1, $f3
	lwc1	$f3, 8($2)
	mul.s	$f5, $f5, $f3
	add.s	$f1, $f5, $f1
	lwc1	$f5, 12($2)
	sll	$2, $6, 0
	mul.s	$f6, $f6, $f5
	add.s	$f1, $f6, $f1
	lwc1	$f6, 16($1)
	mul.s	$f6, $f6, $f2
	swc1	$f1, 0($2)
	lwc1	$f1, 20($1)
	mul.s	$f1, $f1, $f0
	add.s	$f6, $f6, $f4
	add.s	$f1, $f1, $f6
	lwc1	$f6, 24($1)
	mul.s	$f6, $f6, $f3
	add.s	$f1, $f6, $f1
	lwc1	$f6, 28($1)
	mul.s	$f6, $f6, $f5
	add.s	$f1, $f6, $f1
	lwc1	$f6, 32($1)
	mul.s	$f6, $f6, $f2
	swc1	$f1, 4($2)
	lwc1	$f1, 36($1)
	mul.s	$f1, $f1, $f0
	add.s	$f6, $f6, $f4
	add.s	$f1, $f1, $f6
	lwc1	$f6, 40($1)
	mul.s	$f6, $f6, $f3
	add.s	$f1, $f6, $f1
	lwc1	$f6, 44($1)
	mul.s	$f6, $f6, $f5
	add.s	$f1, $f6, $f1
	swc1	$f1, 8($2)
	lwc1	$f1, 52($1)
	mul.s	$f0, $f1, $f0
	lwc1	$f1, 48($1)
	mul.s	$f1, $f1, $f2
	add.s	$f1, $f1, $f4
	add.s	$f0, $f0, $f1
	lwc1	$f1, 56($1)
	mul.s	$f1, $f1, $f3
	add.s	$f0, $f1, $f0
	lwc1	$f1, 60($1)
	mul.s	$f1, $f1, $f5
	add.s	$f0, $f1, $f0
	jr	$ra
	swc1	$f0, 12($2)
	.set	at
	.set	macro
	.set	reorder
	.end	matrix_vector_multiply
.Lfunc_end2:
	.size	matrix_vector_multiply, .Lfunc_end2-matrix_vector_multiply
                                        # -- End function
	.globl	matrix_matrix_multiply          # -- Begin function matrix_matrix_multiply
	.p2align	3
	.type	matrix_matrix_multiply,@function
	.set	nomicromips
	.set	nomips16
	.ent	matrix_matrix_multiply
matrix_matrix_multiply:                 # @matrix_matrix_multiply
	.frame	$sp,16,$ra
	.mask 	0x00000000,0
	.fmask	0x00100000,-4
	.set	noreorder
	.set	nomacro
	.set	noat
# %bb.0:
	addiu	$sp, $sp, -16
	swc1	$f20, 12($sp)                   # 4-byte Folded Spill
	sll	$3, $4, 0
	sll	$1, $5, 0
	mtc1	$zero, $f0
	sll	$2, $6, 0
	lwc1	$f2, 0($1)
	lwc1	$f14, 0($3)
	lwc1	$f1, 16($1)
	lwc1	$f13, 4($3)
	lwc1	$f17, 8($3)
	lwc1	$f18, 12($3)
	lwc1	$f8, 4($1)
	lwc1	$f7, 20($1)
	lwc1	$f12, 8($1)
	lwc1	$f19, 16($3)
	lwc1	$f29, 28($3)
	mul.s	$f4, $f14, $f2
	mul.s	$f3, $f13, $f1
	mul.s	$f11, $f14, $f12
	mul.s	$f21, $f19, $f8
	mul.s	$f23, $f19, $f12
	add.s	$f4, $f4, $f0
	add.s	$f11, $f11, $f0
	add.s	$f21, $f21, $f0
	add.s	$f23, $f23, $f0
	add.s	$f4, $f3, $f4
	lwc1	$f3, 32($1)
	mul.s	$f5, $f17, $f3
	add.s	$f5, $f5, $f4
	lwc1	$f4, 48($1)
	mul.s	$f6, $f18, $f4
	add.s	$f5, $f6, $f5
	mul.s	$f6, $f14, $f8
	swc1	$f5, 0($2)
	mul.s	$f5, $f13, $f7
	add.s	$f6, $f6, $f0
	add.s	$f5, $f5, $f6
	lwc1	$f6, 36($1)
	mul.s	$f9, $f17, $f6
	add.s	$f9, $f9, $f5
	lwc1	$f5, 52($1)
	mul.s	$f10, $f18, $f5
	mul.s	$f20, $f29, $f5
	add.s	$f9, $f10, $f9
	lwc1	$f10, 24($1)
	swc1	$f9, 4($2)
	mul.s	$f9, $f13, $f10
	add.s	$f9, $f9, $f11
	lwc1	$f11, 40($1)
	mul.s	$f15, $f17, $f11
	add.s	$f15, $f15, $f9
	lwc1	$f9, 56($1)
	mul.s	$f16, $f18, $f9
	mul.s	$f31, $f29, $f9
	add.s	$f15, $f16, $f15
	lwc1	$f16, 12($1)
	mul.s	$f14, $f14, $f16
	swc1	$f15, 8($2)
	lwc1	$f15, 28($1)
	mul.s	$f13, $f13, $f15
	add.s	$f14, $f14, $f0
	add.s	$f14, $f13, $f14
	lwc1	$f13, 44($1)
	mul.s	$f17, $f17, $f13
	add.s	$f17, $f17, $f14
	lwc1	$f14, 60($1)
	mul.s	$f18, $f18, $f14
	add.s	$f17, $f18, $f17
	swc1	$f17, 12($2)
	lwc1	$f17, 20($3)
	mul.s	$f18, $f17, $f7
	add.s	$f18, $f18, $f21
	mul.s	$f21, $f17, $f10
	add.s	$f21, $f21, $f23
	lwc1	$f23, 24($3)
	mul.s	$f27, $f23, $f11
	mul.s	$f25, $f23, $f6
	add.s	$f21, $f27, $f21
	mul.s	$f27, $f19, $f16
	add.s	$f18, $f25, $f18
	mul.s	$f25, $f17, $f15
	mul.s	$f17, $f17, $f1
	add.s	$f27, $f27, $f0
	add.s	$f18, $f20, $f18
	add.s	$f21, $f31, $f21
	lwc1	$f20, 12($sp)                   # 4-byte Folded Reload
	add.s	$f25, $f25, $f27
	mul.s	$f27, $f23, $f13
	add.s	$f25, $f27, $f25
	mul.s	$f27, $f29, $f14
	add.s	$f25, $f27, $f25
	swc1	$f25, 28($2)
	swc1	$f21, 24($2)
	swc1	$f18, 20($2)
	mul.s	$f18, $f19, $f2
	add.s	$f18, $f18, $f0
	add.s	$f17, $f17, $f18
	mul.s	$f18, $f23, $f3
	add.s	$f17, $f18, $f17
	mul.s	$f18, $f29, $f4
	add.s	$f17, $f18, $f17
	lwc1	$f18, 32($3)
	mul.s	$f8, $f18, $f8
	mul.s	$f2, $f18, $f2
	swc1	$f17, 16($2)
	lwc1	$f17, 36($3)
	mul.s	$f7, $f17, $f7
	add.s	$f8, $f8, $f0
	mul.s	$f1, $f17, $f1
	add.s	$f2, $f2, $f0
	add.s	$f7, $f7, $f8
	mul.s	$f8, $f17, $f10
	mul.s	$f10, $f18, $f12
	lwc1	$f12, 44($3)
	add.s	$f1, $f1, $f2
	mul.s	$f9, $f12, $f9
	mul.s	$f5, $f12, $f5
	add.s	$f10, $f10, $f0
	add.s	$f8, $f8, $f10
	lwc1	$f10, 40($3)
	mul.s	$f11, $f10, $f11
	mul.s	$f6, $f10, $f6
	mul.s	$f2, $f10, $f3
	lwc1	$f3, 0($1)
	add.s	$f8, $f11, $f8
	mul.s	$f11, $f18, $f16
	add.s	$f6, $f6, $f7
	mul.s	$f7, $f17, $f15
	add.s	$f1, $f2, $f1
	mul.s	$f2, $f12, $f4
	lwc1	$f4, 48($3)
	mul.s	$f3, $f4, $f3
	add.s	$f11, $f11, $f0
	add.s	$f5, $f5, $f6
	add.s	$f6, $f9, $f8
	mul.s	$f8, $f10, $f13
	add.s	$f1, $f2, $f1
	lwc1	$f2, 52($3)
	add.s	$f3, $f3, $f0
	add.s	$f7, $f7, $f11
	add.s	$f7, $f8, $f7
	mul.s	$f8, $f12, $f14
	add.s	$f7, $f8, $f7
	swc1	$f7, 44($2)
	swc1	$f6, 40($2)
	swc1	$f5, 36($2)
	swc1	$f1, 32($2)
	lwc1	$f1, 16($1)
	lwc1	$f5, 56($3)
	lwc1	$f6, 60($3)
	mul.s	$f1, $f2, $f1
	add.s	$f1, $f1, $f3
	lwc1	$f3, 32($1)
	mul.s	$f3, $f5, $f3
	add.s	$f1, $f3, $f1
	lwc1	$f3, 48($1)
	mul.s	$f3, $f6, $f3
	add.s	$f1, $f3, $f1
	lwc1	$f3, 4($1)
	mul.s	$f3, $f4, $f3
	swc1	$f1, 48($2)
	lwc1	$f1, 20($1)
	mul.s	$f1, $f2, $f1
	add.s	$f3, $f3, $f0
	add.s	$f1, $f1, $f3
	lwc1	$f3, 36($1)
	mul.s	$f3, $f5, $f3
	add.s	$f1, $f3, $f1
	lwc1	$f3, 52($1)
	mul.s	$f3, $f6, $f3
	add.s	$f1, $f3, $f1
	lwc1	$f3, 8($1)
	mul.s	$f3, $f4, $f3
	swc1	$f1, 52($2)
	lwc1	$f1, 24($1)
	mul.s	$f1, $f2, $f1
	add.s	$f3, $f3, $f0
	add.s	$f1, $f1, $f3
	lwc1	$f3, 40($1)
	mul.s	$f3, $f5, $f3
	add.s	$f1, $f3, $f1
	lwc1	$f3, 56($1)
	mul.s	$f3, $f6, $f3
	add.s	$f1, $f3, $f1
	swc1	$f1, 56($2)
	lwc1	$f1, 28($1)
	mul.s	$f1, $f2, $f1
	lwc1	$f2, 12($1)
	mul.s	$f2, $f4, $f2
	add.s	$f0, $f2, $f0
	add.s	$f0, $f1, $f0
	lwc1	$f1, 44($1)
	mul.s	$f1, $f5, $f1
	add.s	$f0, $f1, $f0
	lwc1	$f1, 60($1)
	mul.s	$f1, $f6, $f1
	add.s	$f0, $f1, $f0
	swc1	$f0, 60($2)
	jr	$ra
	addiu	$sp, $sp, 16
	.set	at
	.set	macro
	.set	reorder
	.end	matrix_matrix_multiply
.Lfunc_end3:
	.size	matrix_matrix_multiply, .Lfunc_end3-matrix_matrix_multiply
                                        # -- End function
	.globl	transform_vector_list           # -- Begin function transform_vector_list
	.p2align	3
	.type	transform_vector_list,@function
	.set	nomicromips
	.set	nomips16
	.ent	transform_vector_list
transform_vector_list:                  # @transform_vector_list
	.frame	$sp,16,$ra
	.mask 	0x00000000,0
	.fmask	0x00100000,-4
	.set	noreorder
	.set	nomacro
	.set	noat
# %bb.0:
	blez	$7, .LBB4_4
	nop
# %bb.1:
	addiu	$sp, $sp, -16
	swc1	$f20, 12($sp)                   # 4-byte Folded Spill
	sll	$2, $5, 0
	sll	$5, $4, 0
	sll	$1, $6, 0
	sll	$4, $7, 4
	mtc1	$zero, $f16
	addiu	$2, $2, 8
	lwc1	$f0, 60($5)
	lwc1	$f1, 56($5)
	lwc1	$f2, 52($5)
	lwc1	$f3, 48($5)
	lwc1	$f4, 44($5)
	lwc1	$f5, 40($5)
	lwc1	$f6, 36($5)
	lwc1	$f7, 32($5)
	lwc1	$f8, 28($5)
	lwc1	$f9, 24($5)
	lwc1	$f10, 20($5)
	lwc1	$f11, 16($5)
	lwc1	$f12, 12($5)
	lwc1	$f13, 8($5)
	lwc1	$f14, 4($5)
	lwc1	$f15, 0($5)
	addiu	$3, $1, 8
	addu	$4, $2, $4
.LBB4_2:                                # =>This Inner Loop Header: Depth=1
	lwc1	$f18, -8($2)
	lwc1	$f17, -4($2)
	lwc1	$f29, 0($2)
	addiu	$1, $3, 16
	mul.s	$f23, $f3, $f18
	mul.s	$f19, $f7, $f18
	mul.s	$f21, $f2, $f17
	mul.s	$f27, $f11, $f18
	mul.s	$f18, $f15, $f18
	mul.s	$f25, $f10, $f17
	mul.s	$f31, $f1, $f29
	mul.s	$f20, $f5, $f29
	add.s	$f23, $f23, $f16
	add.s	$f19, $f19, $f16
	add.s	$f27, $f27, $f16
	add.s	$f18, $f18, $f16
	add.s	$f21, $f21, $f23
	mul.s	$f23, $f6, $f17
	mul.s	$f17, $f14, $f17
	add.s	$f25, $f25, $f27
	add.s	$f19, $f23, $f19
	lwc1	$f23, 4($2)
	add.s	$f21, $f31, $f21
	mul.s	$f31, $f9, $f29
	add.s	$f17, $f17, $f18
	mul.s	$f18, $f13, $f29
	addiu	$2, $2, 16
	mul.s	$f27, $f0, $f23
	add.s	$f25, $f31, $f25
	mul.s	$f31, $f8, $f23
	add.s	$f17, $f18, $f17
	mul.s	$f18, $f12, $f23
	add.s	$f19, $f20, $f19
	add.s	$f21, $f27, $f21
	mul.s	$f27, $f4, $f23
	add.s	$f25, $f31, $f25
	add.s	$f17, $f18, $f17
	add.s	$f19, $f27, $f19
	swc1	$f21, 4($3)
	swc1	$f19, 0($3)
	swc1	$f25, -4($3)
	swc1	$f17, -8($3)
	bne	$2, $4, .LBB4_2
	move	$3, $1
# %bb.3:
	lwc1	$f20, 12($sp)                   # 4-byte Folded Reload
	addiu	$sp, $sp, 16
.LBB4_4:
	jr	$ra
	nop
	.set	at
	.set	macro
	.set	reorder
	.end	transform_vector_list
.Lfunc_end4:
	.size	transform_vector_list, .Lfunc_end4-transform_vector_list
                                        # -- End function
	.globl	fpu_compare_test                # -- Begin function fpu_compare_test
	.p2align	3
	.type	fpu_compare_test,@function
	.set	nomicromips
	.set	nomips16
	.ent	fpu_compare_test
fpu_compare_test:                       # @fpu_compare_test
	.frame	$sp,0,$ra
	.mask 	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	.set	noat
# %bb.0:
	c.olt.s	$f13, $f12
	addiu	$2, $zero, 0
	bc1t	.LBB5_8
	addiu	$4, $zero, 0
# %bb.1:
	c.ole.s	$f13, $f12
	nop
	bc1t	.LBB5_9
	addiu	$5, $zero, 0
.LBB5_2:
	c.eq.s	$f12, $f13
	nop
	bc1t	.LBB5_4
	addiu	$7, $zero, 0
.LBB5_3:
	addiu	$7, $zero, 1
.LBB5_4:
	sll	$3, $6, 0
	c.ole.s	$f12, $f13
	sw	$7, 20($3)
	sw	$5, 16($3)
	sw	$4, 12($3)
	bc1t	.LBB5_11
	addiu	$4, $zero, 0
# %bb.5:
	c.olt.s	$f12, $f13
	sw	$4, 8($3)
	bc1t	.LBB5_12
	addiu	$4, $zero, 0
.LBB5_6:
	c.eq.s	$f12, $f13
	nop
	bc1t	.LBB5_13
	sw	$4, 4($3)
.LBB5_7:
	jr	$ra
	sw	$2, 0($3)
.LBB5_8:
	addiu	$4, $zero, 1
	c.ole.s	$f13, $f12
	nop
	bc1f	.LBB5_2
	addiu	$5, $zero, 0
.LBB5_9:
	addiu	$5, $zero, 1
	c.eq.s	$f12, $f13
	nop
	bc1t	.LBB5_4
	addiu	$7, $zero, 0
# %bb.10:
	j	.LBB5_3
	nop
.LBB5_11:
	addiu	$4, $zero, 1
	c.olt.s	$f12, $f13
	sw	$4, 8($3)
	bc1f	.LBB5_6
	addiu	$4, $zero, 0
.LBB5_12:
	addiu	$4, $zero, 1
	c.eq.s	$f12, $f13
	nop
	bc1f	.LBB5_7
	sw	$4, 4($3)
.LBB5_13:
	addiu	$2, $zero, 1
	jr	$ra
	sw	$2, 0($3)
	.set	at
	.set	macro
	.set	reorder
	.end	fpu_compare_test
.Lfunc_end5:
	.size	fpu_compare_test, .Lfunc_end5-fpu_compare_test
                                        # -- End function
	.section	.sdata,"aw",@progbits
	.p2align	2, 0x0                          # -- Begin function fpu_compare_validate_all
.LCPI6_0:
	.4byte	0x7f800000                      # float +Inf
.LCPI6_1:
	.4byte	0x3f800000                      # float 1
.LCPI6_2:
	.4byte	0xff800000                      # float -Inf
.LCPI6_3:
	.4byte	0x7fc00000                      # float NaN
	.text
	.globl	fpu_compare_validate_all
	.p2align	3
	.type	fpu_compare_validate_all,@function
	.set	nomicromips
	.set	nomips16
	.ent	fpu_compare_validate_all
fpu_compare_validate_all:               # @fpu_compare_validate_all
	.frame	$sp,176,$ra
	.mask 	0x00000000,0
	.fmask	0x01500000,-4
	.set	noreorder
	.set	nomacro
	.set	noat
# %bb.0:
	addiu	$sp, $sp, -176
	swc1	$f24, 172($sp)                  # 4-byte Folded Spill
	swc1	$f22, 168($sp)                  # 4-byte Folded Spill
	swc1	$f20, 164($sp)                  # 4-byte Folded Spill
	sd	$ra, 152($sp)                   # 8-byte Folded Spill
	sd	$fp, 144($sp)                   # 8-byte Folded Spill
	sd	$23, 136($sp)                   # 8-byte Folded Spill
	sd	$22, 128($sp)                   # 8-byte Folded Spill
	sd	$21, 120($sp)                   # 8-byte Folded Spill
	sd	$20, 112($sp)                   # 8-byte Folded Spill
	sd	$19, 104($sp)                   # 8-byte Folded Spill
	sd	$18, 96($sp)                    # 8-byte Folded Spill
	sd	$17, 88($sp)                    # 8-byte Folded Spill
	sd	$16, 80($sp)                    # 8-byte Folded Spill
	lui	$1, %hi(.Lstr.39)
	jal	puts
	addiu	$4, $1, %lo(.Lstr.39)
	lui	$1, %hi(.L__const.fpu_compare_validate_all.tests)
	addiu	$22, $zero, 0
	addiu	$21, $zero, 1
	addiu	$16, $sp, 56
	addiu	$17, $sp, 32
	addiu	$1, $1, %lo(.L__const.fpu_compare_validate_all.tests)
	addiu	$18, $1, 16
	j	.LBB6_2
	addiu	$20, $1, 556
.LBB6_1:                                #   in Loop: Header=BB6_2 Depth=1
	addu	$22, $2, $22
	addiu	$18, $18, 36
	beq	$18, $20, .LBB6_17
	move	$21, $19
.LBB6_2:                                # =>This Inner Loop Header: Depth=1
	lwc1	$f1, -12($18)
	lwc1	$f0, -16($18)
	c.eq.s	$f0, $f1
	nop
	bc1t	.LBB6_11
	addiu	$2, $zero, 0
# %bb.3:                                #   in Loop: Header=BB6_2 Depth=1
	c.olt.s	$f0, $f1
	nop
	bc1t	.LBB6_12
	addiu	$3, $zero, 0
.LBB6_4:                                #   in Loop: Header=BB6_2 Depth=1
	c.ole.s	$f0, $f1
	nop
	bc1t	.LBB6_13
	addiu	$4, $zero, 0
.LBB6_5:                                #   in Loop: Header=BB6_2 Depth=1
	c.olt.s	$f1, $f0
	nop
	bc1t	.LBB6_14
	addiu	$5, $zero, 0
.LBB6_6:                                #   in Loop: Header=BB6_2 Depth=1
	c.ole.s	$f1, $f0
	nop
	bc1t	.LBB6_15
	addiu	$6, $zero, 0
.LBB6_7:                                #   in Loop: Header=BB6_2 Depth=1
	c.eq.s	$f0, $f1
	nop
	bc1t	.LBB6_9
	addiu	$7, $zero, 0
.LBB6_8:                                #   in Loop: Header=BB6_2 Depth=1
	addiu	$7, $zero, 1
.LBB6_9:                                #   in Loop: Header=BB6_2 Depth=1
	ldl	$1, 15($18)
	sw	$7, 76($sp)
	sw	$6, 72($sp)
	sw	$5, 68($sp)
	sw	$4, 64($sp)
	lw	$4, 16($18)
	move	$5, $16
	move	$6, $17
	sw	$3, 60($sp)
	sw	$2, 56($sp)
	ldr	$1, 8($18)
	sd	$1, 48($sp)
	ldl	$1, 7($18)
	ldr	$1, 0($18)
	sd	$1, 40($sp)
	ldl	$1, -1($18)
	ldr	$1, -8($18)
	jal	validate_cmp
	sd	$1, 32($sp)
	sltiu	$1, $2, 1
	bnez	$1, .LBB6_1
	addiu	$19, $zero, 0
# %bb.10:                               #   in Loop: Header=BB6_2 Depth=1
	j	.LBB6_1
	move	$19, $21
.LBB6_11:                               #   in Loop: Header=BB6_2 Depth=1
	addiu	$2, $zero, 1
	c.olt.s	$f0, $f1
	nop
	bc1f	.LBB6_4
	addiu	$3, $zero, 0
.LBB6_12:                               #   in Loop: Header=BB6_2 Depth=1
	addiu	$3, $zero, 1
	c.ole.s	$f0, $f1
	nop
	bc1f	.LBB6_5
	addiu	$4, $zero, 0
.LBB6_13:                               #   in Loop: Header=BB6_2 Depth=1
	addiu	$4, $zero, 1
	c.olt.s	$f1, $f0
	nop
	bc1f	.LBB6_6
	addiu	$5, $zero, 0
.LBB6_14:                               #   in Loop: Header=BB6_2 Depth=1
	addiu	$5, $zero, 1
	c.ole.s	$f1, $f0
	nop
	bc1f	.LBB6_7
	addiu	$6, $zero, 0
.LBB6_15:                               #   in Loop: Header=BB6_2 Depth=1
	addiu	$6, $zero, 1
	c.eq.s	$f0, $f1
	nop
	bc1t	.LBB6_9
	addiu	$7, $zero, 0
# %bb.16:                               #   in Loop: Header=BB6_2 Depth=1
	j	.LBB6_8
	nop
.LBB6_17:
	lui	$1, %hi(.Lstr.40)
	jal	puts
	addiu	$4, $1, %lo(.Lstr.40)
	lwc1	$f22, %gp_rel(.LCPI6_0)($gp)
	lui	$1, %hi(.L.str.23)
	addiu	$17, $sp, 56
	addiu	$18, $sp, 32
	daddiu	$16, $zero, 1
	addiu	$4, $1, %lo(.L.str.23)
	move	$5, $17
	move	$6, $18
	sdl	$16, 79($sp)
	sdl	$16, 71($sp)
	sdl	$16, 63($sp)
	sd	$16, 48($sp)
	sd	$16, 40($sp)
	sdr	$16, 72($sp)
	sdr	$16, 64($sp)
	sd	$16, 32($sp)
	sdr	$16, 56($sp)
	mov.s	$f0, $f22
	jal	validate_cmp
	mov.s	$f1, $f22
	lwc1	$f20, %gp_rel(.LCPI6_1)($gp)
	mov.s	$f0, $f22
	dsll	$20, $16, 32
	lui	$1, %hi(.L.str.24)
	move	$5, $17
	move	$6, $18
	move	$23, $2
	sw	$2, 12($sp)                     # 4-byte Folded Spill
	daddiu	$21, $20, 1
	addiu	$4, $1, %lo(.L.str.24)
	mov.s	$f1, $f20
	sdl	$21, 79($sp)
	sdl	$20, 71($sp)
	sdl	$zero, 63($sp)
	sd	$21, 48($sp)
	sd	$20, 40($sp)
	sdr	$21, 72($sp)
	sdr	$20, 64($sp)
	sd	$zero, 32($sp)
	jal	validate_cmp
	sdr	$zero, 56($sp)
	lwc1	$f24, %gp_rel(.LCPI6_2)($gp)
	mov.s	$f1, $f20
	lui	$1, %hi(.L.str.25)
	move	$5, $17
	move	$6, $18
	sdl	$20, 79($sp)
	sdl	$16, 71($sp)
	sdl	$20, 63($sp)
	sd	$20, 48($sp)
	sd	$16, 40($sp)
	sdr	$20, 72($sp)
	sdr	$16, 64($sp)
	sw	$2, 8($sp)                      # 4-byte Folded Spill
	sd	$20, 32($sp)
	sdr	$20, 56($sp)
	addiu	$4, $1, %lo(.L.str.25)
	jal	validate_cmp
	mov.s	$f0, $f24
	mov.s	$f0, $f22
	mov.s	$f1, $f24
	lui	$1, %hi(.L.str.26)
	move	$5, $17
	move	$6, $18
	sdl	$21, 79($sp)
	sdl	$20, 71($sp)
	sdl	$zero, 63($sp)
	sd	$21, 48($sp)
	sd	$20, 40($sp)
	sdr	$21, 72($sp)
	sdr	$20, 64($sp)
	move	$16, $2
	sd	$zero, 32($sp)
	sdr	$zero, 56($sp)
	jal	validate_cmp
	addiu	$4, $1, %lo(.L.str.26)
	lui	$1, %hi(.Lstr.41)
	move	$21, $2
	jal	puts
	addiu	$4, $1, %lo(.Lstr.41)
	lui	$1, %hi(.L.str.31)
	lwc1	$f22, %gp_rel(.LCPI6_3)($gp)
	mov.s	$f1, $f20
	lui	$3, %hi(.L.str.28)
	lui	$2, %hi(.L.str.29)
	move	$5, $17
	move	$6, $18
	sdl	$zero, 63($sp)
	sdl	$zero, 71($sp)
	sdl	$20, 79($sp)
	sd	$zero, 32($sp)
	sd	$zero, 40($sp)
	sd	$20, 48($sp)
	sdr	$zero, 56($sp)
	sdr	$zero, 64($sp)
	sdr	$20, 72($sp)
	sw	$16, 28($sp)                    # 4-byte Folded Spill
	addiu	$1, $1, %lo(.L.str.31)
	addiu	$4, $3, %lo(.L.str.28)
	addiu	$fp, $2, %lo(.L.str.29)
	sd	$1, 16($sp)                     # 8-byte Folded Spill
	lui	$1, %hi(.L.str.30)
	mov.s	$f0, $f22
	addiu	$1, $1, %lo(.L.str.30)
	sw	$1, 4($sp)                      # 4-byte Folded Spill
	addu	$1, $23, $22
	lw	$22, 8($sp)                     # 4-byte Folded Reload
	addu	$1, $1, $22
	jal	validate_cmp
	addu	$23, $1, $16
	mov.s	$f0, $f20
	mov.s	$f1, $f22
	move	$4, $fp
	move	$5, $17
	move	$6, $18
	sdl	$zero, 63($sp)
	sdl	$zero, 71($sp)
	sdl	$20, 79($sp)
	move	$16, $2
	sd	$zero, 32($sp)
	sd	$zero, 40($sp)
	sd	$20, 48($sp)
	sdr	$zero, 56($sp)
	sdr	$zero, 64($sp)
	sdr	$20, 72($sp)
	jal	validate_cmp
	addu	$23, $23, $21
	mov.s	$f0, $f22
	mov.s	$f1, $f22
	lw	$4, 4($sp)                      # 4-byte Folded Reload
	addu	$1, $23, $16
	move	$5, $17
	move	$6, $18
	sdl	$zero, 63($sp)
	sdl	$zero, 71($sp)
	sdl	$20, 79($sp)
	move	$fp, $2
	sd	$zero, 32($sp)
	sd	$zero, 40($sp)
	sd	$20, 48($sp)
	sdr	$20, 72($sp)
	sdr	$zero, 56($sp)
	sdr	$zero, 64($sp)
	jal	validate_cmp
	addu	$20, $1, $2
	ld	$4, 16($sp)                     # 8-byte Folded Reload
	addu	$5, $20, $2
	daddiu	$6, $zero, 22
	jal	printf
	move	$17, $2
	lw	$1, 12($sp)                     # 4-byte Folded Reload
	addiu	$2, $zero, 0
	sltiu	$3, $22, 1
	sltiu	$1, $1, 1
	beqz	$1, .LBB6_26
	addiu	$5, $zero, 0
# %bb.18:
	beqz	$3, .LBB6_27
	addiu	$4, $zero, 0
.LBB6_19:
	lw	$1, 28($sp)                     # 4-byte Folded Reload
	sltiu	$1, $1, 1
	beqz	$1, .LBB6_28
	addiu	$3, $zero, 0
.LBB6_20:
	sltiu	$1, $21, 1
	beqz	$1, .LBB6_29
	addiu	$4, $zero, 0
.LBB6_21:
	sltiu	$1, $16, 1
	beqz	$1, .LBB6_30
	addiu	$3, $zero, 0
.LBB6_22:
	sltiu	$1, $fp, 1
	beqz	$1, .LBB6_31
	addiu	$4, $zero, 0
.LBB6_23:
	sltiu	$1, $17, 1
	bnez	$1, .LBB6_25
	nop
.LBB6_24:
	move	$2, $4
.LBB6_25:
	ld	$16, 80($sp)                    # 8-byte Folded Reload
	ld	$17, 88($sp)                    # 8-byte Folded Reload
	ld	$18, 96($sp)                    # 8-byte Folded Reload
	ld	$19, 104($sp)                   # 8-byte Folded Reload
	ld	$20, 112($sp)                   # 8-byte Folded Reload
	ld	$21, 120($sp)                   # 8-byte Folded Reload
	ld	$22, 128($sp)                   # 8-byte Folded Reload
	ld	$23, 136($sp)                   # 8-byte Folded Reload
	ld	$fp, 144($sp)                   # 8-byte Folded Reload
	ld	$ra, 152($sp)                   # 8-byte Folded Reload
	lwc1	$f20, 164($sp)                  # 4-byte Folded Reload
	lwc1	$f22, 168($sp)                  # 4-byte Folded Reload
	lwc1	$f24, 172($sp)                  # 4-byte Folded Reload
	sll	$2, $2, 0
	jr	$ra
	addiu	$sp, $sp, 176
.LBB6_26:
	move	$5, $19
	bnez	$3, .LBB6_19
	addiu	$4, $zero, 0
.LBB6_27:
	move	$4, $5
	lw	$1, 28($sp)                     # 4-byte Folded Reload
	sltiu	$1, $1, 1
	bnez	$1, .LBB6_20
	addiu	$3, $zero, 0
.LBB6_28:
	move	$3, $4
	sltiu	$1, $21, 1
	bnez	$1, .LBB6_21
	addiu	$4, $zero, 0
.LBB6_29:
	move	$4, $3
	sltiu	$1, $16, 1
	bnez	$1, .LBB6_22
	addiu	$3, $zero, 0
.LBB6_30:
	move	$3, $4
	sltiu	$1, $fp, 1
	bnez	$1, .LBB6_23
	addiu	$4, $zero, 0
.LBB6_31:
	sltiu	$1, $17, 1
	bnez	$1, .LBB6_25
	move	$4, $3
# %bb.32:
	j	.LBB6_24
	nop
	.set	at
	.set	macro
	.set	reorder
	.end	fpu_compare_validate_all
.Lfunc_end6:
	.size	fpu_compare_validate_all, .Lfunc_end6-fpu_compare_validate_all
                                        # -- End function
	.p2align	3                               # -- Begin function validate_cmp
	.type	validate_cmp,@function
	.set	nomicromips
	.set	nomips16
	.ent	validate_cmp
validate_cmp:                           # @validate_cmp
	.frame	$sp,64,$ra
	.mask 	0x00000000,0
	.fmask	0x00500000,-4
	.set	noreorder
	.set	nomacro
	.set	noat
# %bb.0:
	addiu	$sp, $sp, -64
	swc1	$f22, 60($sp)                   # 4-byte Folded Spill
	swc1	$f20, 56($sp)                   # 4-byte Folded Spill
	sd	$ra, 48($sp)                    # 8-byte Folded Spill
	sd	$21, 40($sp)                    # 8-byte Folded Spill
	sd	$20, 32($sp)                    # 8-byte Folded Spill
	sd	$19, 24($sp)                    # 8-byte Folded Spill
	sd	$18, 16($sp)                    # 8-byte Folded Spill
	sd	$17, 8($sp)                     # 8-byte Folded Spill
	sd	$16, 0($sp)                     # 8-byte Folded Spill
	mov.s	$f20, $f1
	mov.s	$f22, $f0
	lw	$20, 0($6)
	lw	$21, 0($5)
	move	$18, $6
	move	$17, $5
	move	$16, $4
	bne	$21, $20, .LBB7_8
	addiu	$2, $zero, 1
# %bb.1:
	lw	$20, 4($18)
	lw	$21, 4($17)
	bne	$21, $20, .LBB7_9
	nop
.LBB7_2:
	lw	$20, 8($18)
	lw	$21, 8($17)
	bne	$21, $20, .LBB7_10
	nop
.LBB7_3:
	lw	$20, 12($18)
	lw	$21, 12($17)
	bne	$21, $20, .LBB7_11
	nop
.LBB7_4:
	lw	$20, 16($18)
	lw	$21, 16($17)
	bne	$21, $20, .LBB7_12
	nop
.LBB7_5:
	lw	$18, 20($18)
	lw	$19, 20($17)
	beq	$19, $18, .LBB7_7
	nop
.LBB7_6:
	jal	__extendsfdf2
	mov.s	$f12, $f22
	mov.s	$f12, $f20
	jal	__extendsfdf2
	move	$17, $2
	lui	$1, %hi(.L.str.37)
	move	$7, $2
	sll	$8, $19, 0
	sll	$9, $18, 0
	move	$5, $16
	move	$6, $17
	jal	printf
	addiu	$4, $1, %lo(.L.str.37)
	addiu	$2, $zero, 0
.LBB7_7:
	ld	$16, 0($sp)                     # 8-byte Folded Reload
	ld	$17, 8($sp)                     # 8-byte Folded Reload
	ld	$18, 16($sp)                    # 8-byte Folded Reload
	ld	$19, 24($sp)                    # 8-byte Folded Reload
	ld	$20, 32($sp)                    # 8-byte Folded Reload
	ld	$21, 40($sp)                    # 8-byte Folded Reload
	ld	$ra, 48($sp)                    # 8-byte Folded Reload
	lwc1	$f20, 56($sp)                   # 4-byte Folded Reload
	lwc1	$f22, 60($sp)                   # 4-byte Folded Reload
	sll	$2, $2, 0
	jr	$ra
	addiu	$sp, $sp, 64
.LBB7_8:
	jal	__extendsfdf2
	mov.s	$f12, $f22
	mov.s	$f12, $f20
	jal	__extendsfdf2
	move	$19, $2
	lui	$1, %hi(.L.str.32)
	move	$7, $2
	sll	$8, $21, 0
	sll	$9, $20, 0
	move	$5, $16
	move	$6, $19
	jal	printf
	addiu	$4, $1, %lo(.L.str.32)
	lw	$20, 4($18)
	lw	$21, 4($17)
	beq	$21, $20, .LBB7_2
	addiu	$2, $zero, 0
.LBB7_9:
	jal	__extendsfdf2
	mov.s	$f12, $f22
	mov.s	$f12, $f20
	jal	__extendsfdf2
	move	$19, $2
	lui	$1, %hi(.L.str.33)
	move	$7, $2
	sll	$8, $21, 0
	sll	$9, $20, 0
	move	$5, $16
	move	$6, $19
	jal	printf
	addiu	$4, $1, %lo(.L.str.33)
	lw	$20, 8($18)
	lw	$21, 8($17)
	beq	$21, $20, .LBB7_3
	addiu	$2, $zero, 0
.LBB7_10:
	jal	__extendsfdf2
	mov.s	$f12, $f22
	mov.s	$f12, $f20
	jal	__extendsfdf2
	move	$19, $2
	lui	$1, %hi(.L.str.34)
	move	$7, $2
	sll	$8, $21, 0
	sll	$9, $20, 0
	move	$5, $16
	move	$6, $19
	jal	printf
	addiu	$4, $1, %lo(.L.str.34)
	lw	$20, 12($18)
	lw	$21, 12($17)
	beq	$21, $20, .LBB7_4
	addiu	$2, $zero, 0
.LBB7_11:
	jal	__extendsfdf2
	mov.s	$f12, $f22
	mov.s	$f12, $f20
	jal	__extendsfdf2
	move	$19, $2
	lui	$1, %hi(.L.str.35)
	move	$7, $2
	sll	$8, $21, 0
	sll	$9, $20, 0
	move	$5, $16
	move	$6, $19
	jal	printf
	addiu	$4, $1, %lo(.L.str.35)
	lw	$20, 16($18)
	lw	$21, 16($17)
	beq	$21, $20, .LBB7_5
	addiu	$2, $zero, 0
.LBB7_12:
	jal	__extendsfdf2
	mov.s	$f12, $f22
	mov.s	$f12, $f20
	jal	__extendsfdf2
	move	$19, $2
	lui	$1, %hi(.L.str.36)
	move	$7, $2
	sll	$8, $21, 0
	sll	$9, $20, 0
	move	$5, $16
	move	$6, $19
	jal	printf
	addiu	$4, $1, %lo(.L.str.36)
	lw	$18, 20($18)
	lw	$19, 20($17)
	beq	$19, $18, .LBB7_7
	addiu	$2, $zero, 0
# %bb.13:
	j	.LBB7_6
	nop
	.set	at
	.set	macro
	.set	reorder
	.end	validate_cmp
.Lfunc_end7:
	.size	validate_cmp, .Lfunc_end7-validate_cmp
                                        # -- End function
	.type	.L.str,@object                  # @.str
	.section	.sdata,"aw",@progbits
.L.str:
	.asciz	"%s:\n"
	.size	.L.str, 5

	.type	.L.str.1,@object                # @.str.1
.L.str.1:
	.asciz	"  ["
	.size	.L.str.1, 4

	.type	.L.str.2,@object                # @.str.2
.L.str.2:
	.asciz	"%7.2f"
	.size	.L.str.2, 6

	.type	.L.str.3,@object                # @.str.3
.L.str.3:
	.asciz	", "
	.size	.L.str.3, 3

	.type	.L.str.5,@object                # @.str.5
.L.str.5:
	.asciz	"%s: ["
	.size	.L.str.5, 6

	.type	.L.str.6,@object                # @.str.6
.L.str.6:
	.asciz	"1 vs 2"
	.size	.L.str.6, 7

	.type	.L.str.7,@object                # @.str.7
.L.str.7:
	.asciz	"2 vs 1"
	.size	.L.str.7, 7

	.type	.L.str.8,@object                # @.str.8
.L.str.8:
	.asciz	"1 vs 1"
	.size	.L.str.8, 7

	.type	.L.str.9,@object                # @.str.9
.L.str.9:
	.asciz	"0 vs 0"
	.size	.L.str.9, 7

	.type	.L.str.10,@object               # @.str.10
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.10:
	.asciz	"-0 vs +0"
	.size	.L.str.10, 9

	.type	.L.str.11,@object               # @.str.11
.L.str.11:
	.asciz	"+0 vs -0"
	.size	.L.str.11, 9

	.type	.L.str.12,@object               # @.str.12
.L.str.12:
	.asciz	"-1 vs -2"
	.size	.L.str.12, 9

	.type	.L.str.13,@object               # @.str.13
.L.str.13:
	.asciz	"-2 vs -1"
	.size	.L.str.13, 9

	.type	.L.str.14,@object               # @.str.14
	.section	.sdata,"aw",@progbits
.L.str.14:
	.asciz	"-1 vs 1"
	.size	.L.str.14, 8

	.type	.L.str.15,@object               # @.str.15
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.15:
	.asciz	"1 vs 1+eps"
	.size	.L.str.15, 11

	.type	.L.str.16,@object               # @.str.16
.L.str.16:
	.asciz	"1+eps vs 1"
	.size	.L.str.16, 11

	.type	.L.str.17,@object               # @.str.17
.L.str.17:
	.asciz	"1e30 vs 1e30"
	.size	.L.str.17, 13

	.type	.L.str.18,@object               # @.str.18
.L.str.18:
	.asciz	"1e30 vs -1e30"
	.size	.L.str.18, 14

	.type	.L.str.19,@object               # @.str.19
.L.str.19:
	.asciz	"1e-30 vs 1e-30"
	.size	.L.str.19, 15

	.type	.L.str.20,@object               # @.str.20
.L.str.20:
	.asciz	"1e-30 vs 2e-30"
	.size	.L.str.20, 15

	.type	.L__const.fpu_compare_validate_all.tests,@object # @__const.fpu_compare_validate_all.tests
	.section	.rodata,"a",@progbits
	.p2align	2, 0x0
.L__const.fpu_compare_validate_all.tests:
	.4byte	0x3f800000                      # float 1
	.4byte	0x40000000                      # float 2
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	.L.str.6
	.4byte	0x40000000                      # float 2
	.4byte	0x3f800000                      # float 1
	.4byte	0                               # 0x0
	.4byte	0                               # 0x0
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	1                               # 0x1
	.4byte	1                               # 0x1
	.4byte	.L.str.7
	.4byte	0x3f800000                      # float 1
	.4byte	0x3f800000                      # float 1
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	.L.str.8
	.4byte	0x00000000                      # float 0
	.4byte	0x00000000                      # float 0
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	.L.str.9
	.4byte	0x80000000                      # float -0
	.4byte	0x00000000                      # float 0
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	.L.str.10
	.4byte	0x00000000                      # float 0
	.4byte	0x80000000                      # float -0
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	.L.str.11
	.4byte	0xbf800000                      # float -1
	.4byte	0xc0000000                      # float -2
	.4byte	0                               # 0x0
	.4byte	0                               # 0x0
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	1                               # 0x1
	.4byte	1                               # 0x1
	.4byte	.L.str.12
	.4byte	0xc0000000                      # float -2
	.4byte	0xbf800000                      # float -1
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	.L.str.13
	.4byte	0xbf800000                      # float -1
	.4byte	0x3f800000                      # float 1
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	.L.str.14
	.4byte	0x3f800000                      # float 1
	.4byte	0x3f800001                      # float 1.00000012
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	.L.str.15
	.4byte	0x3f800001                      # float 1.00000012
	.4byte	0x3f800000                      # float 1
	.4byte	0                               # 0x0
	.4byte	0                               # 0x0
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	1                               # 0x1
	.4byte	1                               # 0x1
	.4byte	.L.str.16
	.4byte	0x7149f2ca                      # float 1.00000002E+30
	.4byte	0x7149f2ca                      # float 1.00000002E+30
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	.L.str.17
	.4byte	0x7149f2ca                      # float 1.00000002E+30
	.4byte	0xf149f2ca                      # float -1.00000002E+30
	.4byte	0                               # 0x0
	.4byte	0                               # 0x0
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	1                               # 0x1
	.4byte	1                               # 0x1
	.4byte	.L.str.18
	.4byte	0x0da24260                      # float 1.0E-30
	.4byte	0x0da24260                      # float 1.0E-30
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	.L.str.19
	.4byte	0x0da24260                      # float 1.0E-30
	.4byte	0x0e224260                      # float 2.00000001E-30
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	1                               # 0x1
	.4byte	0                               # 0x0
	.4byte	0                               # 0x0
	.4byte	1                               # 0x1
	.4byte	.L.str.20
	.size	.L__const.fpu_compare_validate_all.tests, 540

	.type	.L.str.23,@object               # @.str.23
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str.23:
	.asciz	"+inf vs +inf"
	.size	.L.str.23, 13

	.type	.L.str.24,@object               # @.str.24
.L.str.24:
	.asciz	"+inf vs 1"
	.size	.L.str.24, 10

	.type	.L.str.25,@object               # @.str.25
.L.str.25:
	.asciz	"-inf vs 1"
	.size	.L.str.25, 10

	.type	.L.str.26,@object               # @.str.26
.L.str.26:
	.asciz	"+inf vs -inf"
	.size	.L.str.26, 13

	.type	.L.str.28,@object               # @.str.28
.L.str.28:
	.asciz	"NaN vs 1"
	.size	.L.str.28, 9

	.type	.L.str.29,@object               # @.str.29
.L.str.29:
	.asciz	"1 vs NaN"
	.size	.L.str.29, 9

	.type	.L.str.30,@object               # @.str.30
.L.str.30:
	.asciz	"NaN vs NaN"
	.size	.L.str.30, 11

	.type	.L.str.31,@object               # @.str.31
.L.str.31:
	.asciz	"FPU Compare: %d/%d tests passed\n"
	.size	.L.str.31, 33

	.type	.L.str.32,@object               # @.str.32
.L.str.32:
	.asciz	"  FAIL %s: %.2f == %.2f: got %d, expected %d\n"
	.size	.L.str.32, 46

	.type	.L.str.33,@object               # @.str.33
.L.str.33:
	.asciz	"  FAIL %s: %.2f < %.2f: got %d, expected %d\n"
	.size	.L.str.33, 45

	.type	.L.str.34,@object               # @.str.34
.L.str.34:
	.asciz	"  FAIL %s: %.2f <= %.2f: got %d, expected %d\n"
	.size	.L.str.34, 46

	.type	.L.str.35,@object               # @.str.35
.L.str.35:
	.asciz	"  FAIL %s: %.2f > %.2f: got %d, expected %d\n"
	.size	.L.str.35, 45

	.type	.L.str.36,@object               # @.str.36
.L.str.36:
	.asciz	"  FAIL %s: %.2f >= %.2f: got %d, expected %d\n"
	.size	.L.str.36, 46

	.type	.L.str.37,@object               # @.str.37
.L.str.37:
	.asciz	"  FAIL %s: %.2f != %.2f: got %d, expected %d\n"
	.size	.L.str.37, 46

	.type	.Lstr.38,@object                # @str.38
	.section	.sdata,"aw",@progbits
	.p2align	2, 0x0
.Lstr.38:
	.asciz	"]"
	.size	.Lstr.38, 2

	.type	.Lstr.39,@object                # @str.39
	.section	.rodata.str1.4,"aMS",@progbits,1
	.p2align	2, 0x0
.Lstr.39:
	.asciz	"--- FPU Compare Tests ---"
	.size	.Lstr.39, 26

	.type	.Lstr.40,@object                # @str.40
	.p2align	2, 0x0
.Lstr.40:
	.asciz	"--- Infinity Tests ---"
	.size	.Lstr.40, 23

	.type	.Lstr.41,@object                # @str.41
	.p2align	2, 0x0
.Lstr.41:
	.asciz	"--- NaN Tests ---"
	.size	.Lstr.41, 18

	.ident	"clang version 23.0.0git (https://github.com/rickgaiser/llvm-project 197832d0537e093c3115dcd38632f873ee548134)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.text
