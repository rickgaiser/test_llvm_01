	.section	.mdebug.abiN32,"",@progbits
	.nan	legacy
	.text
	.file	"vpu_tests.c"
	.globl	vpu_vector_add                  # -- Begin function vpu_vector_add
	.p2align	3
	.type	vpu_vector_add,@function
	.set	nomicromips
	.set	nomips16
	.ent	vpu_vector_add
vpu_vector_add:                         # @vpu_vector_add
	.frame	$sp,0,$ra
	.mask 	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	.set	noat
# %bb.0:
	blez	$7, .LBB0_3
	nop
# %bb.1:
	sll	$2, $6, 0
	sll	$1, $7, 4
	sll	$3, $5, 0
	sll	$4, $4, 0
	addu	$5, $2, $1
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	lwc1	$f0, 8($3)
	lwc1	$f1, 8($4)
	lwc1	$f2, 4($3)
	lwc1	$f3, 4($4)
	lwc1	$f4, 0($3)
	lwc1	$f5, 0($4)
	add.s	$f2, $f3, $f2
	add.s	$f0, $f1, $f0
	lwc1	$f1, 12($3)
	lwc1	$f3, 12($4)
	add.s	$f4, $f5, $f4
	addiu	$4, $4, 16
	add.s	$f1, $f3, $f1
	swc1	$f0, 8($2)
	swc1	$f2, 4($2)
	swc1	$f4, 0($2)
	swc1	$f1, 12($2)
	addiu	$2, $2, 16
	bne	$2, $5, .LBB0_2
	addiu	$3, $3, 16
.LBB0_3:
	jr	$ra
	nop
	.set	at
	.set	macro
	.set	reorder
	.end	vpu_vector_add
.Lfunc_end0:
	.size	vpu_vector_add, .Lfunc_end0-vpu_vector_add
                                        # -- End function
	.globl	vpu_vector_mul                  # -- Begin function vpu_vector_mul
	.p2align	3
	.type	vpu_vector_mul,@function
	.set	nomicromips
	.set	nomips16
	.ent	vpu_vector_mul
vpu_vector_mul:                         # @vpu_vector_mul
	.frame	$sp,0,$ra
	.mask 	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	.set	noat
# %bb.0:
	blez	$7, .LBB1_3
	nop
# %bb.1:
	sll	$2, $6, 0
	sll	$1, $7, 4
	sll	$3, $5, 0
	sll	$4, $4, 0
	addu	$5, $2, $1
.LBB1_2:                                # =>This Inner Loop Header: Depth=1
	lwc1	$f0, 8($3)
	lwc1	$f1, 8($4)
	lwc1	$f2, 4($3)
	lwc1	$f3, 4($4)
	lwc1	$f4, 0($3)
	lwc1	$f5, 0($4)
	mul.s	$f2, $f3, $f2
	mul.s	$f0, $f1, $f0
	lwc1	$f1, 12($3)
	lwc1	$f3, 12($4)
	mul.s	$f4, $f5, $f4
	addiu	$4, $4, 16
	mul.s	$f1, $f3, $f1
	swc1	$f0, 8($2)
	swc1	$f2, 4($2)
	swc1	$f4, 0($2)
	swc1	$f1, 12($2)
	addiu	$2, $2, 16
	bne	$2, $5, .LBB1_2
	addiu	$3, $3, 16
.LBB1_3:
	jr	$ra
	nop
	.set	at
	.set	macro
	.set	reorder
	.end	vpu_vector_mul
.Lfunc_end1:
	.size	vpu_vector_mul, .Lfunc_end1-vpu_vector_mul
                                        # -- End function
	.globl	vpu_vector_madd                 # -- Begin function vpu_vector_madd
	.p2align	3
	.type	vpu_vector_madd,@function
	.set	nomicromips
	.set	nomips16
	.ent	vpu_vector_madd
vpu_vector_madd:                        # @vpu_vector_madd
	.frame	$sp,0,$ra
	.mask 	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	.set	noat
# %bb.0:
	blez	$8, .LBB2_3
	nop
# %bb.1:
	sll	$2, $7, 0
	sll	$1, $8, 4
	sll	$3, $6, 0
	sll	$5, $5, 0
	sll	$4, $4, 0
	addu	$6, $2, $1
.LBB2_2:                                # =>This Inner Loop Header: Depth=1
	lwc1	$f0, 0($5)
	lwc1	$f1, 0($4)
	lwc1	$f2, 4($5)
	lwc1	$f3, 4($4)
	lwc1	$f4, 8($5)
	lwc1	$f5, 8($4)
	lwc1	$f6, 4($3)
	lwc1	$f7, 0($3)
	mul.s	$f2, $f3, $f2
	mul.s	$f0, $f1, $f0
	lwc1	$f1, 12($5)
	lwc1	$f3, 12($4)
	mul.s	$f4, $f5, $f4
	lwc1	$f5, 8($3)
	addiu	$4, $4, 16
	addiu	$5, $5, 16
	mul.s	$f1, $f3, $f1
	lwc1	$f3, 12($3)
	add.s	$f0, $f0, $f7
	add.s	$f2, $f2, $f6
	add.s	$f4, $f4, $f5
	add.s	$f1, $f1, $f3
	swc1	$f4, 8($2)
	swc1	$f2, 4($2)
	swc1	$f0, 0($2)
	swc1	$f1, 12($2)
	addiu	$2, $2, 16
	bne	$2, $6, .LBB2_2
	addiu	$3, $3, 16
.LBB2_3:
	jr	$ra
	nop
	.set	at
	.set	macro
	.set	reorder
	.end	vpu_vector_madd
.Lfunc_end2:
	.size	vpu_vector_madd, .Lfunc_end2-vpu_vector_madd
                                        # -- End function
	.globl	vpu_matrix_vector_multiply      # -- Begin function vpu_matrix_vector_multiply
	.p2align	3
	.type	vpu_matrix_vector_multiply,@function
	.set	nomicromips
	.set	nomips16
	.ent	vpu_matrix_vector_multiply
vpu_matrix_vector_multiply:             # @vpu_matrix_vector_multiply
	.frame	$sp,96,$ra
	.mask 	0x00000000,0
	.fmask	0x55500000,-4
	.set	noreorder
	.set	nomacro
	.set	noat
# %bb.0:
	blez	$7, .LBB3_13
	nop
# %bb.1:
	sll	$1, $4, 0
	sll	$2, $6, 0
	lwc1	$f0, 60($1)
	lwc1	$f5, 56($1)
	lwc1	$f4, 52($1)
	lwc1	$f6, 48($1)
	lwc1	$f1, 44($1)
	lwc1	$f9, 40($1)
	lwc1	$f7, 36($1)
	lwc1	$f8, 32($1)
	lwc1	$f2, 28($1)
	lwc1	$f14, 24($1)
	lwc1	$f11, 20($1)
	lwc1	$f13, 16($1)
	lwc1	$f3, 12($1)
	lwc1	$f12, 8($1)
	lwc1	$f10, 4($1)
	lwc1	$f15, 0($1)
	andi	$1, $7, 1
	beqz	$1, .LBB3_4
	sll	$3, $5, 0
# %bb.2:
	lwc1	$f16, 0($3)
	lwc1	$f17, 4($3)
	lwc1	$f31, 12($3)
	addiu	$7, $7, -1
	mul.s	$f18, $f13, $f17
	mul.s	$f19, $f15, $f16
	mul.s	$f21, $f14, $f17
	mul.s	$f23, $f12, $f16
	mul.s	$f25, $f3, $f16
	mul.s	$f16, $f10, $f16
	add.s	$f18, $f19, $f18
	lwc1	$f19, 8($3)
	add.s	$f21, $f23, $f21
	mul.s	$f23, $f2, $f17
	mul.s	$f17, $f11, $f17
	mul.s	$f27, $f9, $f19
	mul.s	$f29, $f8, $f19
	add.s	$f16, $f16, $f17
	add.s	$f17, $f29, $f18
	add.s	$f18, $f27, $f21
	add.s	$f21, $f25, $f23
	mul.s	$f23, $f7, $f19
	mul.s	$f19, $f1, $f19
	mul.s	$f25, $f6, $f31
	mul.s	$f27, $f5, $f31
	add.s	$f19, $f19, $f21
	add.s	$f16, $f23, $f16
	mul.s	$f21, $f4, $f31
	add.s	$f18, $f27, $f18
	add.s	$f17, $f25, $f17
	add.s	$f16, $f21, $f16
	mul.s	$f21, $f0, $f31
	swc1	$f17, 0($2)
	swc1	$f18, 8($2)
	swc1	$f16, 4($2)
	add.s	$f16, $f21, $f19
	beqz	$7, .LBB3_13
	swc1	$f16, 12($2)
# %bb.3:
	addiu	$2, $2, 16
	addiu	$3, $3, 16
.LBB3_4:
	addiu	$sp, $sp, -96
	swc1	$f30, 92($sp)                   # 4-byte Folded Spill
	swc1	$f28, 88($sp)                   # 4-byte Folded Spill
	swc1	$f26, 84($sp)                   # 4-byte Folded Spill
	swc1	$f24, 80($sp)                   # 4-byte Folded Spill
	swc1	$f22, 76($sp)                   # 4-byte Folded Spill
	swc1	$f20, 72($sp)                   # 4-byte Folded Spill
	lwc1	$f17, 28($3)
	lwc1	$f19, 24($3)
	lwc1	$f27, 20($3)
	lwc1	$f25, 16($3)
	lwc1	$f16, 12($3)
	lwc1	$f18, 8($3)
	lwc1	$f21, 4($3)
	lwc1	$f23, 0($3)
	addiu	$1, $zero, 2
	bne	$7, $1, .LBB3_6
	nop
# %bb.5:
	mul.s	$f29, $f2, $f27
	mul.s	$f31, $f3, $f25
	mul.s	$f20, $f15, $f25
	mul.s	$f22, $f13, $f27
	mul.s	$f24, $f11, $f21
	mul.s	$f26, $f10, $f23
	mul.s	$f28, $f14, $f27
	mul.s	$f30, $f12, $f25
	mul.s	$f11, $f11, $f27
	mul.s	$f10, $f10, $f25
	mul.s	$f15, $f15, $f23
	mul.s	$f13, $f13, $f21
	mul.s	$f14, $f14, $f21
	mul.s	$f12, $f12, $f23
	mul.s	$f2, $f2, $f21
	mul.s	$f3, $f3, $f23
	mul.s	$f25, $f6, $f17
	mul.s	$f27, $f4, $f16
	mul.s	$f6, $f6, $f16
	mul.s	$f4, $f4, $f17
	add.s	$f12, $f12, $f14
	add.s	$f13, $f15, $f13
	add.s	$f15, $f26, $f24
	mul.s	$f24, $f8, $f18
	mul.s	$f26, $f7, $f18
	add.s	$f20, $f20, $f22
	mul.s	$f22, $f9, $f18
	mul.s	$f8, $f8, $f19
	add.s	$f29, $f31, $f29
	mul.s	$f31, $f1, $f19
	add.s	$f14, $f30, $f28
	mul.s	$f9, $f9, $f19
	add.s	$f10, $f10, $f11
	mul.s	$f7, $f7, $f19
	add.s	$f2, $f3, $f2
	mul.s	$f1, $f1, $f18
	add.s	$f8, $f8, $f20
	add.s	$f13, $f24, $f13
	add.s	$f9, $f9, $f14
	add.s	$f14, $f31, $f29
	mul.s	$f29, $f5, $f17
	mul.s	$f31, $f0, $f17
	add.s	$f7, $f7, $f10
	add.s	$f15, $f26, $f15
	add.s	$f12, $f22, $f12
	mul.s	$f5, $f5, $f16
	add.s	$f1, $f1, $f2
	mul.s	$f0, $f0, $f16
	add.s	$f5, $f5, $f12
	add.s	$f8, $f25, $f8
	add.s	$f4, $f4, $f7
	add.s	$f14, $f31, $f14
	add.s	$f9, $f29, $f9
	add.s	$f6, $f6, $f13
	add.s	$f12, $f27, $f15
	add.s	$f0, $f0, $f1
	swc1	$f4, 20($2)
	swc1	$f8, 16($2)
	swc1	$f12, 4($2)
	swc1	$f6, 0($2)
	swc1	$f9, 24($2)
	swc1	$f5, 8($2)
	swc1	$f14, 28($2)
	j	.LBB3_12
	swc1	$f0, 12($2)
.LBB3_6:
	mul.s	$f29, $f11, $f27
	mul.s	$f31, $f10, $f25
	mul.s	$f20, $f2, $f21
	mul.s	$f22, $f3, $f23
	mul.s	$f26, $f15, $f23
	mul.s	$f24, $f12, $f25
	mul.s	$f28, $f10, $f23
	swc1	$f2, 68($sp)                    # 4-byte Folded Spill
	mul.s	$f30, $f15, $f25
	mul.s	$f25, $f3, $f25
	swc1	$f1, 60($sp)                    # 4-byte Folded Spill
	swc1	$f7, 40($sp)                    # 4-byte Folded Spill
	swc1	$f8, 36($sp)                    # 4-byte Folded Spill
	swc1	$f3, 56($sp)                    # 4-byte Folded Spill
	swc1	$f9, 32($sp)                    # 4-byte Folded Spill
	mul.s	$f3, $f5, $f16
	sltiu	$1, $7, 5
	add.s	$f29, $f31, $f29
	mul.s	$f31, $f14, $f21
	add.s	$f20, $f22, $f20
	mul.s	$f22, $f13, $f27
	swc1	$f29, 48($sp)                   # 4-byte Folded Spill
	mul.s	$f29, $f1, $f18
	swc1	$f20, 64($sp)                   # 4-byte Folded Spill
	mul.s	$f20, $f7, $f19
	add.s	$f22, $f30, $f22
	mul.s	$f7, $f7, $f18
	mul.s	$f30, $f4, $f17
	swc1	$f29, 52($sp)                   # 4-byte Folded Spill
	mul.s	$f29, $f11, $f21
	mul.s	$f21, $f13, $f21
	swc1	$f29, 44($sp)                   # 4-byte Folded Spill
	mul.s	$f29, $f12, $f23
	mul.s	$f23, $f14, $f27
	mul.s	$f27, $f2, $f27
	add.s	$f2, $f26, $f21
	mul.s	$f26, $f1, $f19
	lwc1	$f1, 48($sp)                    # 4-byte Folded Reload
	mul.s	$f21, $f6, $f17
	swc1	$f5, 48($sp)                    # 4-byte Folded Spill
	mul.s	$f5, $f5, $f17
	add.s	$f20, $f20, $f1
	lwc1	$f1, 44($sp)                    # 4-byte Folded Reload
	swc1	$f6, 44($sp)                    # 4-byte Folded Spill
	swc1	$f2, 28($sp)                    # 4-byte Folded Spill
	add.s	$f2, $f24, $f23
	add.s	$f27, $f25, $f27
	mul.s	$f24, $f9, $f19
	add.s	$f23, $f29, $f31
	mul.s	$f29, $f8, $f18
	mul.s	$f18, $f9, $f18
	add.s	$f28, $f28, $f1
	mul.s	$f19, $f8, $f19
	mul.s	$f25, $f4, $f16
	lwc1	$f1, 64($sp)                    # 4-byte Folded Reload
	mul.s	$f8, $f6, $f16
	mul.s	$f31, $f0, $f17
	swc1	$f0, 64($sp)                    # 4-byte Folded Spill
	swc1	$f2, 24($sp)                    # 4-byte Folded Spill
	lwc1	$f2, 52($sp)                    # 4-byte Folded Reload
	swc1	$f4, 52($sp)                    # 4-byte Folded Spill
	add.s	$f6, $f26, $f27
	add.s	$f9, $f18, $f23
	add.s	$f23, $f19, $f22
	add.s	$f19, $f30, $f20
	lwc1	$f18, 40($3)
	lwc1	$f30, 52($3)
	lwc1	$f4, 24($sp)                    # 4-byte Folded Reload
	add.s	$f1, $f2, $f1
	mul.s	$f2, $f0, $f16
	lwc1	$f16, 44($3)
	add.s	$f27, $f24, $f4
	lwc1	$f4, 28($sp)                    # 4-byte Folded Reload
	lwc1	$f24, 36($3)
	add.s	$f20, $f21, $f23
	lwc1	$f23, 32($3)
	add.s	$f26, $f29, $f4
	add.s	$f29, $f7, $f28
	lwc1	$f28, 48($3)
	add.s	$f22, $f5, $f27
	add.s	$f27, $f3, $f9
	add.s	$f0, $f25, $f29
	add.s	$f21, $f8, $f26
	add.s	$f25, $f31, $f6
	lwc1	$f29, 60($3)
	lwc1	$f26, 56($3)
	bnez	$1, .LBB3_10
	add.s	$f31, $f2, $f1
# %bb.7:
	sll	$1, $7, 4
	swc1	$f11, 28($sp)                   # 4-byte Folded Spill
	addiu	$4, $zero, -32
	mov.s	$f11, $f0
	swc1	$f14, 16($sp)                   # 4-byte Folded Spill
	lwc1	$f14, 36($sp)                   # 4-byte Folded Reload
	addiu	$3, $3, 80
	swc1	$f12, 24($sp)                   # 4-byte Folded Spill
	swc1	$f13, 20($sp)                   # 4-byte Folded Spill
	addiu	$1, $1, -80
	and	$1, $1, $4
	addu	$1, $1, $2
	addiu	$4, $1, 32
.LBB3_8:                                # =>This Inner Loop Header: Depth=1
	lwc1	$f5, 20($sp)                    # 4-byte Folded Reload
	lwc1	$f4, 28($sp)                    # 4-byte Folded Reload
	mul.s	$f0, $f15, $f23
	mul.s	$f3, $f10, $f23
	lwc1	$f6, 16($sp)                    # 4-byte Folded Reload
	lwc1	$f7, 68($sp)                    # 4-byte Folded Reload
	lwc1	$f8, 56($sp)                    # 4-byte Folded Reload
	lwc1	$f17, 24($sp)                   # 4-byte Folded Reload
	mul.s	$f9, $f15, $f28
	lwc1	$f12, 40($sp)                   # 4-byte Folded Reload
	lwc1	$f13, 32($sp)                   # 4-byte Folded Reload
	swc1	$f20, 16($2)
	swc1	$f21, 0($2)
	lwc1	$f21, 60($sp)                   # 4-byte Folded Reload
	swc1	$f22, 24($2)
	swc1	$f19, 20($2)
	swc1	$f11, 4($2)
	swc1	$f27, 8($2)
	swc1	$f25, 28($2)
	swc1	$f31, 12($2)
	addiu	$1, $3, 32
	addiu	$2, $2, 32
	mul.s	$f2, $f4, $f24
	mul.s	$f1, $f5, $f24
	mul.s	$f4, $f4, $f30
	mul.s	$f20, $f21, $f26
	add.s	$f2, $f3, $f2
	mul.s	$f3, $f5, $f30
	add.s	$f0, $f0, $f1
	mul.s	$f1, $f6, $f30
	mul.s	$f5, $f6, $f24
	mul.s	$f30, $f7, $f30
	mul.s	$f6, $f8, $f28
	mul.s	$f24, $f7, $f24
	mul.s	$f7, $f8, $f23
	mul.s	$f8, $f17, $f28
	mul.s	$f23, $f17, $f23
	mul.s	$f28, $f10, $f28
	mov.s	$f17, $f15
	mov.s	$f15, $f10
	mul.s	$f10, $f14, $f18
	add.s	$f0, $f10, $f0
	mul.s	$f10, $f12, $f18
	add.s	$f1, $f8, $f1
	mul.s	$f8, $f13, $f18
	add.s	$f6, $f6, $f30
	add.s	$f3, $f9, $f3
	lwc1	$f30, 52($sp)                   # 4-byte Folded Reload
	add.s	$f7, $f7, $f24
	add.s	$f5, $f23, $f5
	add.s	$f4, $f28, $f4
	mul.s	$f24, $f12, $f26
	mul.s	$f28, $f14, $f26
	mul.s	$f18, $f21, $f18
	lwc1	$f12, 48($sp)                   # 4-byte Folded Reload
	mul.s	$f23, $f30, $f16
	mul.s	$f22, $f12, $f29
	add.s	$f2, $f10, $f2
	mul.s	$f10, $f13, $f26
	lwc1	$f13, 44($sp)                   # 4-byte Folded Reload
	mul.s	$f26, $f30, $f29
	add.s	$f4, $f24, $f4
	add.s	$f3, $f28, $f3
	add.s	$f7, $f18, $f7
	add.s	$f6, $f20, $f6
	lwc1	$f28, 0($3)
	lwc1	$f24, -12($3)
	lwc1	$f30, 4($3)
	lwc1	$f18, -8($3)
	mul.s	$f9, $f13, $f16
	add.s	$f11, $f23, $f2
	add.s	$f1, $f10, $f1
	mul.s	$f2, $f12, $f16
	mov.s	$f10, $f15
	add.s	$f19, $f26, $f4
	mov.s	$f15, $f17
	lwc1	$f23, -16($3)
	lwc1	$f26, 8($3)
	add.s	$f21, $f9, $f0
	lwc1	$f9, 64($sp)                    # 4-byte Folded Reload
	add.s	$f0, $f8, $f5
	mul.s	$f5, $f13, $f29
	mul.s	$f8, $f9, $f29
	mul.s	$f9, $f9, $f16
	add.s	$f22, $f22, $f1
	lwc1	$f16, -4($3)
	lwc1	$f29, 12($3)
	move	$3, $1
	add.s	$f20, $f5, $f3
	add.s	$f27, $f2, $f0
	add.s	$f25, $f8, $f6
	bne	$2, $4, .LBB3_8
	add.s	$f31, $f9, $f7
# %bb.9:
	swc1	$f11, 12($sp)                   # 4-byte Folded Spill
	lwc1	$f12, 24($sp)                   # 4-byte Folded Reload
	lwc1	$f13, 20($sp)                   # 4-byte Folded Reload
	lwc1	$f14, 16($sp)                   # 4-byte Folded Reload
	lwc1	$f11, 28($sp)                   # 4-byte Folded Reload
	j	.LBB3_11
	nop
.LBB3_10:
	swc1	$f0, 12($sp)                    # 4-byte Folded Spill
.LBB3_11:
	mul.s	$f1, $f12, $f23
	mul.s	$f2, $f11, $f24
	mul.s	$f4, $f11, $f30
	mov.s	$f11, $f12
	mov.s	$f12, $f10
	mul.s	$f0, $f14, $f24
	lwc1	$f6, 68($sp)                    # 4-byte Folded Reload
	mul.s	$f8, $f10, $f28
	mul.s	$f5, $f14, $f30
	lwc1	$f17, 56($sp)                   # 4-byte Folded Reload
	mul.s	$f9, $f15, $f28
	mul.s	$f10, $f13, $f30
	mul.s	$f3, $f13, $f24
	mul.s	$f13, $f15, $f23
	lwc1	$f15, 60($sp)                   # 4-byte Folded Reload
	swc1	$f20, 16($2)
	swc1	$f21, 0($2)
	swc1	$f19, 20($2)
	lwc1	$f14, 64($sp)                   # 4-byte Folded Reload
	mul.s	$f6, $f6, $f30
	mul.s	$f7, $f17, $f28
	mul.s	$f12, $f12, $f23
	mul.s	$f11, $f11, $f28
	add.s	$f0, $f1, $f0
	add.s	$f3, $f13, $f3
	add.s	$f4, $f8, $f4
	lwc1	$f13, 40($sp)                   # 4-byte Folded Reload
	add.s	$f6, $f7, $f6
	mul.s	$f7, $f13, $f26
	mul.s	$f13, $f13, $f18
	add.s	$f2, $f12, $f2
	add.s	$f1, $f11, $f5
	add.s	$f5, $f9, $f10
	lwc1	$f9, 32($sp)                    # 4-byte Folded Reload
	lwc1	$f12, 36($sp)                   # 4-byte Folded Reload
	mul.s	$f11, $f15, $f26
	mul.s	$f8, $f9, $f18
	mul.s	$f10, $f12, $f26
	mul.s	$f9, $f9, $f26
	mul.s	$f12, $f12, $f18
	add.s	$f4, $f7, $f4
	add.s	$f6, $f11, $f6
	lwc1	$f11, 52($sp)                   # 4-byte Folded Reload
	add.s	$f2, $f13, $f2
	mul.s	$f13, $f14, $f29
	add.s	$f3, $f12, $f3
	add.s	$f1, $f9, $f1
	lwc1	$f12, 44($sp)                   # 4-byte Folded Reload
	lwc1	$f9, 48($sp)                    # 4-byte Folded Reload
	add.s	$f5, $f10, $f5
	mul.s	$f10, $f11, $f29
	add.s	$f0, $f8, $f0
	mul.s	$f11, $f11, $f16
	mul.s	$f7, $f12, $f29
	mul.s	$f8, $f9, $f29
	mul.s	$f9, $f9, $f16
	mul.s	$f12, $f12, $f16
	add.s	$f6, $f13, $f6
	add.s	$f2, $f11, $f2
	add.s	$f4, $f10, $f4
	add.s	$f5, $f7, $f5
	add.s	$f3, $f12, $f3
	add.s	$f0, $f9, $f0
	add.s	$f1, $f8, $f1
	lwc1	$f7, 12($sp)                    # 4-byte Folded Reload
	swc1	$f7, 4($2)
	swc1	$f22, 24($2)
	swc1	$f27, 8($2)
	swc1	$f25, 28($2)
	swc1	$f31, 12($2)
	swc1	$f4, 52($2)
	swc1	$f5, 48($2)
	swc1	$f2, 36($2)
	swc1	$f3, 32($2)
	swc1	$f1, 56($2)
	swc1	$f0, 40($2)
	lwc1	$f0, 68($sp)                    # 4-byte Folded Reload
	mul.s	$f1, $f17, $f23
	swc1	$f6, 60($2)
	mul.s	$f0, $f0, $f24
	add.s	$f0, $f1, $f0
	mul.s	$f1, $f15, $f18
	add.s	$f0, $f1, $f0
	mul.s	$f1, $f14, $f16
	add.s	$f0, $f1, $f0
	swc1	$f0, 44($2)
.LBB3_12:
	lwc1	$f20, 72($sp)                   # 4-byte Folded Reload
	lwc1	$f22, 76($sp)                   # 4-byte Folded Reload
	lwc1	$f24, 80($sp)                   # 4-byte Folded Reload
	lwc1	$f26, 84($sp)                   # 4-byte Folded Reload
	lwc1	$f28, 88($sp)                   # 4-byte Folded Reload
	lwc1	$f30, 92($sp)                   # 4-byte Folded Reload
	addiu	$sp, $sp, 96
.LBB3_13:
	jr	$ra
	nop
	.set	at
	.set	macro
	.set	reorder
	.end	vpu_matrix_vector_multiply
.Lfunc_end3:
	.size	vpu_matrix_vector_multiply, .Lfunc_end3-vpu_matrix_vector_multiply
                                        # -- End function
	.globl	vpu_dot_product                 # -- Begin function vpu_dot_product
	.p2align	3
	.type	vpu_dot_product,@function
	.set	nomicromips
	.set	nomips16
	.ent	vpu_dot_product
vpu_dot_product:                        # @vpu_dot_product
	.frame	$sp,0,$ra
	.mask 	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	.set	noat
# %bb.0:
	blez	$7, .LBB4_3
	nop
# %bb.1:
	sll	$2, $6, 0
	sll	$1, $7, 2
	sll	$3, $5, 0
	sll	$4, $4, 0
	addu	$5, $2, $1
.LBB4_2:                                # =>This Inner Loop Header: Depth=1
	lwc1	$f0, 4($3)
	lwc1	$f1, 4($4)
	lwc1	$f2, 0($4)
	mul.s	$f0, $f1, $f0
	lwc1	$f1, 0($3)
	mul.s	$f1, $f2, $f1
	lwc1	$f2, 8($4)
	add.s	$f0, $f1, $f0
	lwc1	$f1, 8($3)
	mul.s	$f1, $f2, $f1
	lwc1	$f2, 12($4)
	addiu	$4, $4, 16
	add.s	$f0, $f1, $f0
	lwc1	$f1, 12($3)
	mul.s	$f1, $f2, $f1
	add.s	$f0, $f1, $f0
	swc1	$f0, 0($2)
	addiu	$2, $2, 4
	bne	$2, $5, .LBB4_2
	addiu	$3, $3, 16
.LBB4_3:
	jr	$ra
	nop
	.set	at
	.set	macro
	.set	reorder
	.end	vpu_dot_product
.Lfunc_end4:
	.size	vpu_dot_product, .Lfunc_end4-vpu_dot_product
                                        # -- End function
	.ident	"clang version 23.0.0git (https://github.com/rickgaiser/llvm-project 197832d0537e093c3115dcd38632f873ee548134)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.text
