	.file	1 "fpu_tests.c"
	.section .mdebug.abiN32
	.previous
	.nan	legacy
	.module	singlefloat
	.module	oddspreg
	.module	arch=r5900
	.abicalls
	.option	pic0
	.text
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align	3
.LC19:
	.ascii	"  FAIL %s: %.2f == %.2f: got %d, expected %d\012\000"
	.align	3
.LC20:
	.ascii	"  FAIL %s: %.2f < %.2f: got %d, expected %d\012\000"
	.align	3
.LC21:
	.ascii	"  FAIL %s: %.2f <= %.2f: got %d, expected %d\012\000"
	.align	3
.LC22:
	.ascii	"  FAIL %s: %.2f > %.2f: got %d, expected %d\012\000"
	.align	3
.LC23:
	.ascii	"  FAIL %s: %.2f >= %.2f: got %d, expected %d\012\000"
	.align	3
.LC24:
	.ascii	"  FAIL %s: %.2f != %.2f: got %d, expected %d\012\000"
	.text
	.align	2
	.align	3
	.set	nomips16
	.set	nomicromips
	.ent	validate_cmp
	.type	validate_cmp, @function
validate_cmp:
	.frame	$sp,64,$31		# vars= 16, regs= 6/0, args= 0, gp= 0
	.mask	0x801f0000,-8
	.fmask	0x00000000,0
	addiu	$sp,$sp,-64
	sd	$16,16($sp)
	move	$16,$8
	lw	$9,0($16)
	lw	$8,0($7)
	sd	$20,48($sp)
	sd	$19,40($sp)
	sd	$18,32($sp)
	move	$18,$4
	sd	$17,24($sp)
	move	$17,$7
	mfc1	$19,$f13
	mfc1	$20,$f14
	sd	$31,56($sp)
	bne	$8,$9,.L11
	li	$2,1			# 0x1
.L2:
	lw	$8,4($17)
	lw	$9,4($16)
	beq	$8,$9,.L3
	mtc1	$20,$f12
	sw	$8,8($sp)
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sw	$9,12($sp)
	.set	macro
	.set	reorder

	mtc1	$19,$f12
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sd	$2,0($sp)
	.set	macro
	.set	reorder

	lui	$4,%hi(.LC20)
	lw	$9,12($sp)
	move	$6,$2
	lw	$8,8($sp)
	move	$5,$18
	ld	$7,0($sp)
	.set	noreorder
	.set	nomacro
	jal	printf
	addiu	$4,$4,%lo(.LC20)
	.set	macro
	.set	reorder

	move	$2,$0
.L3:
	lw	$8,8($17)
	lw	$9,8($16)
	beq	$8,$9,.L4
	mtc1	$20,$f12
	sw	$8,8($sp)
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sw	$9,12($sp)
	.set	macro
	.set	reorder

	mtc1	$19,$f12
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sd	$2,0($sp)
	.set	macro
	.set	reorder

	lui	$4,%hi(.LC21)
	lw	$9,12($sp)
	move	$6,$2
	lw	$8,8($sp)
	move	$5,$18
	ld	$7,0($sp)
	.set	noreorder
	.set	nomacro
	jal	printf
	addiu	$4,$4,%lo(.LC21)
	.set	macro
	.set	reorder

	move	$2,$0
.L4:
	lw	$8,12($17)
	lw	$9,12($16)
	beq	$8,$9,.L5
	mtc1	$20,$f12
	sw	$8,8($sp)
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sw	$9,12($sp)
	.set	macro
	.set	reorder

	mtc1	$19,$f12
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sd	$2,0($sp)
	.set	macro
	.set	reorder

	lui	$4,%hi(.LC22)
	lw	$9,12($sp)
	move	$6,$2
	lw	$8,8($sp)
	move	$5,$18
	ld	$7,0($sp)
	.set	noreorder
	.set	nomacro
	jal	printf
	addiu	$4,$4,%lo(.LC22)
	.set	macro
	.set	reorder

	move	$2,$0
.L5:
	lw	$8,16($17)
	lw	$9,16($16)
	beq	$8,$9,.L6
	mtc1	$20,$f12
	sw	$8,8($sp)
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sw	$9,12($sp)
	.set	macro
	.set	reorder

	mtc1	$19,$f12
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sd	$2,0($sp)
	.set	macro
	.set	reorder

	lui	$4,%hi(.LC23)
	lw	$9,12($sp)
	move	$6,$2
	lw	$8,8($sp)
	move	$5,$18
	ld	$7,0($sp)
	.set	noreorder
	.set	nomacro
	jal	printf
	addiu	$4,$4,%lo(.LC23)
	.set	macro
	.set	reorder

	move	$2,$0
.L6:
	lw	$17,20($17)
	lw	$16,20($16)
	beq	$17,$16,.L1
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	mtc1	$20,$f12
	.set	macro
	.set	reorder

	mtc1	$19,$f12
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sd	$2,0($sp)
	.set	macro
	.set	reorder

	lui	$4,%hi(.LC24)
	move	$9,$16
	move	$8,$17
	ld	$7,0($sp)
	move	$6,$2
	move	$5,$18
	.set	noreorder
	.set	nomacro
	jal	printf
	addiu	$4,$4,%lo(.LC24)
	.set	macro
	.set	reorder

	move	$2,$0
.L1:
	ld	$31,56($sp)
	ld	$20,48($sp)
	ld	$19,40($sp)
	ld	$18,32($sp)
	ld	$17,24($sp)
	ld	$16,16($sp)
	.set	noreorder
	.set	nomacro
	jr	$31
	addiu	$sp,$sp,64
	.set	macro
	.set	reorder

	.align	3
.L11:
	mov.s	$f12,$f14
	sw	$8,8($sp)
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sw	$9,12($sp)
	.set	macro
	.set	reorder

	mtc1	$19,$f12
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sd	$2,0($sp)
	.set	macro
	.set	reorder

	lui	$4,%hi(.LC19)
	lw	$9,12($sp)
	move	$6,$2
	lw	$8,8($sp)
	move	$5,$18
	ld	$7,0($sp)
	.set	noreorder
	.set	nomacro
	jal	printf
	addiu	$4,$4,%lo(.LC19)
	.set	macro
	.set	reorder

	move	$2,$0
	b	.L2
	.end	validate_cmp
	.size	validate_cmp, .-validate_cmp
	.section	.rodata.str1.8
	.align	3
.LC25:
	.ascii	"%s:\012\000"
	.align	3
.LC26:
	.ascii	"  [\000"
	.align	3
.LC27:
	.ascii	"%7.2f\000"
	.align	3
.LC28:
	.ascii	"]\000"
	.align	3
.LC29:
	.ascii	", \000"
	.text
	.align	2
	.align	3
	.globl	print_matrix
	.set	nomips16
	.set	nomicromips
	.ent	print_matrix
	.type	print_matrix, @function
print_matrix:
	.frame	$sp,64,$31		# vars= 0, regs= 8/0, args= 0, gp= 0
	.mask	0x807f0000,-8
	.fmask	0x00000000,0
	addiu	$sp,$sp,-64
	sd	$21,40($sp)
	lui	$21,%hi(.LC26)
	sd	$20,32($sp)
	addiu	$20,$5,80
	sd	$19,24($sp)
	lui	$19,%hi(.LC27)
	sd	$18,16($sp)
	lui	$18,%hi(.LC29)
	sd	$17,8($sp)
	addiu	$17,$5,16
	move	$5,$4
	lui	$4,%hi(.LC25)
	sd	$22,48($sp)
	addiu	$4,$4,%lo(.LC25)
	lui	$22,%hi(.LC28)
	addiu	$21,$21,%lo(.LC26)
	addiu	$19,$19,%lo(.LC27)
	addiu	$18,$18,%lo(.LC29)
	sd	$31,56($sp)
	.set	noreorder
	.set	nomacro
	jal	printf
	sd	$16,0($sp)
	.set	macro
	.set	reorder

	.align	3
.L14:
	move	$4,$21
	.set	noreorder
	.set	nomacro
	jal	printf
	addiu	$16,$17,-16
	.set	macro
	.set	reorder

	lwc1	$f12,0($16)
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	addiu	$16,$16,4
	.set	macro
	.set	reorder

	move	$4,$19
	.set	noreorder
	.set	nomacro
	jal	printf
	move	$5,$2
	.set	macro
	.set	reorder

	move	$4,$18
	beq	$16,$17,.L20
.L13:
	jal	printf
	lwc1	$f12,0($16)
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	addiu	$16,$16,4
	.set	macro
	.set	reorder

	move	$4,$19
	.set	noreorder
	.set	nomacro
	jal	printf
	move	$5,$2
	.set	macro
	.set	reorder

	move	$4,$18
	bne	$16,$17,.L13
.L20:
	addiu	$4,$22,%lo(.LC28)
	.set	noreorder
	.set	nomacro
	jal	puts
	addiu	$17,$16,16
	.set	macro
	.set	reorder

	bne	$17,$20,.L14
	ld	$31,56($sp)
	ld	$22,48($sp)
	ld	$21,40($sp)
	ld	$20,32($sp)
	ld	$19,24($sp)
	ld	$18,16($sp)
	ld	$17,8($sp)
	ld	$16,0($sp)
	.set	noreorder
	.set	nomacro
	jr	$31
	addiu	$sp,$sp,64
	.set	macro
	.set	reorder

	.end	print_matrix
	.size	print_matrix, .-print_matrix
	.section	.rodata.str1.8
	.align	3
.LC30:
	.ascii	"%s: [\000"
	.text
	.align	2
	.align	3
	.globl	print_vector
	.set	nomips16
	.set	nomicromips
	.ent	print_vector
	.type	print_vector, @function
print_vector:
	.frame	$sp,48,$31		# vars= 0, regs= 5/0, args= 0, gp= 0
	.mask	0x800f0000,-8
	.fmask	0x00000000,0
	addiu	$sp,$sp,-48
	sd	$16,8($sp)
	move	$16,$5
	move	$5,$4
	lui	$4,%hi(.LC30)
	addiu	$4,$4,%lo(.LC30)
	sd	$31,40($sp)
	sd	$19,32($sp)
	addiu	$19,$16,16
	sd	$18,24($sp)
	lui	$18,%hi(.LC27)
	.set	noreorder
	.set	nomacro
	jal	printf
	sd	$17,16($sp)
	.set	macro
	.set	reorder

	lwc1	$f12,0($16)
	addiu	$18,$18,%lo(.LC27)
	lui	$17,%hi(.LC29)
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	addiu	$16,$16,4
	.set	macro
	.set	reorder

	addiu	$17,$17,%lo(.LC29)
	move	$4,$18
	.set	noreorder
	.set	nomacro
	jal	printf
	move	$5,$2
	.set	macro
	.set	reorder

	move	$4,$17
	beq	$16,$19,.L25
.L22:
	jal	printf
	lwc1	$f12,0($16)
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	addiu	$16,$16,4
	.set	macro
	.set	reorder

	move	$4,$18
	.set	noreorder
	.set	nomacro
	jal	printf
	move	$5,$2
	.set	macro
	.set	reorder

	move	$4,$17
	bne	$16,$19,.L22
.L25:
	lui	$4,%hi(.LC28)
	ld	$31,40($sp)
	ld	$19,32($sp)
	addiu	$4,$4,%lo(.LC28)
	ld	$18,24($sp)
	ld	$17,16($sp)
	ld	$16,8($sp)
	.set	noreorder
	.set	nomacro
	j	puts
	addiu	$sp,$sp,48
	.set	macro
	.set	reorder

	.end	print_vector
	.size	print_vector, .-print_vector
	.align	2
	.align	3
	.globl	matrix_vector_multiply
	.set	nomips16
	.set	nomicromips
	.ent	matrix_vector_multiply
	.type	matrix_vector_multiply, @function
matrix_vector_multiply:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	lwc1	$f0,0($5)
	lwc1	$f1,4($5)
	lwc1	$f4,4($4)
	lwc1	$f3,20($4)
	lwc1	$f2,32($4)
	lwc1	$f12,52($4)
	lwc1	$f7,0($4)
	lwc1	$f6,16($4)
	lwc1	$f5,36($4)
	lwc1	$f13,48($4)
	mul.s	$f11,$f1,$f4
	mul.s	$f7,$f0,$f7
	mul.s	$f10,$f1,$f3
	mul.s	$f6,$f0,$f6
	mul.s	$f9,$f0,$f2
	mul.s	$f5,$f1,$f5
	mul.s	$f2,$f0,$f13
	mul.s	$f1,$f1,$f12
	lwc1	$f0,8($5)
	lwc1	$f4,24($4)
	lwc1	$f3,40($4)
	lwc1	$f12,56($4)
	lwc1	$f8,8($4)
	add.s	$f11,$f11,$f7
	mul.s	$f8,$f0,$f8
	mul.s	$f7,$f0,$f4
	add.s	$f10,$f10,$f6
	add.s	$f9,$f9,$f5
	mul.s	$f6,$f0,$f3
	add.s	$f2,$f2,$f1
	mul.s	$f1,$f0,$f12
	lwc1	$f5,12($4)
	lwc1	$f0,12($5)
	lwc1	$f4,28($4)
	lwc1	$f3,44($4)
	lwc1	$f12,60($4)
	mul.s	$f5,$f0,$f5
	mul.s	$f4,$f0,$f4
	mul.s	$f3,$f0,$f3
	add.s	$f8,$f8,$f11
	add.s	$f7,$f7,$f10
	add.s	$f6,$f6,$f9
	add.s	$f1,$f1,$f2
	mul.s	$f0,$f0,$f12
	add.s	$f5,$f5,$f8
	add.s	$f4,$f4,$f7
	add.s	$f3,$f3,$f6
	add.s	$f0,$f0,$f1
	swc1	$f5,0($6)
	swc1	$f4,4($6)
	swc1	$f3,8($6)
	.set	noreorder
	.set	nomacro
	jr	$31
	swc1	$f0,12($6)
	.set	macro
	.set	reorder

	.end	matrix_vector_multiply
	.size	matrix_vector_multiply, .-matrix_vector_multiply
	.align	2
	.align	3
	.globl	matrix_matrix_multiply
	.set	nomips16
	.set	nomicromips
	.ent	matrix_matrix_multiply
	.type	matrix_matrix_multiply, @function
matrix_matrix_multiply:
	.frame	$sp,32,$31		# vars= 0, regs= 0/6, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x55500000,-4
	lwc1	$f9,0($5)
	addiu	$sp,$sp,-32
	lwc1	$f8,4($5)
	lwc1	$f7,8($5)
	lwc1	$f1,4($4)
	lwc1	$f19,16($5)
	lwc1	$f18,20($5)
	lwc1	$f16,24($5)
	lwc1	$f3,0($4)
	swc1	$f30,28($sp)
	swc1	$f28,24($sp)
	swc1	$f22,12($sp)
	lwc1	$f27,12($5)
	lwc1	$f25,28($5)
	lwc1	$f0,20($4)
	mul.s	$f23,$f9,$f3
	mul.s	$f22,$f8,$f3
	mul.s	$f12,$f7,$f3
	mul.s	$f29,$f19,$f1
	mul.s	$f30,$f18,$f1
	mul.s	$f28,$f16,$f1
	swc1	$f26,20($sp)
	lwc1	$f6,32($5)
	lwc1	$f5,36($5)
	lwc1	$f4,40($5)
	lwc1	$f2,16($4)
	lwc1	$f21,44($5)
	mul.s	$f11,$f0,$f18
	mul.s	$f10,$f0,$f16
	mul.s	$f14,$f1,$f25
	mul.s	$f3,$f27,$f3
	lwc1	$f1,8($4)
	mul.s	$f26,$f0,$f19
	mul.s	$f13,$f0,$f25
	lwc1	$f0,24($4)
	swc1	$f24,16($sp)
	mul.s	$f15,$f2,$f8
	add.s	$f29,$f29,$f23
	mul.s	$f24,$f6,$f1
	mul.s	$f23,$f5,$f1
	add.s	$f30,$f30,$f22
	add.s	$f28,$f28,$f12
	mul.s	$f22,$f4,$f1
	mul.s	$f12,$f1,$f21
	mul.s	$f1,$f0,$f5
	mul.s	$f17,$f2,$f7
	add.s	$f14,$f14,$f3
	add.s	$f15,$f11,$f15
	mfc1	$2,$f1
	swc1	$f20,8($sp)
	add.s	$f1,$f10,$f17
	mul.s	$f20,$f2,$f9
	add.s	$f12,$f12,$f14
	mul.s	$f2,$f2,$f27
	mtc1	$2,$f14
	lwc1	$f3,48($5)
	add.s	$f14,$f14,$f15
	lwc1	$f17,60($5)
	lwc1	$f10,12($4)
	mul.s	$f31,$f0,$f4
	add.s	$f13,$f13,$f2
	add.s	$f26,$f26,$f20
	lwc1	$f2,52($5)
	mul.s	$f20,$f0,$f6
	mul.s	$f11,$f0,$f21
	mfc1	$3,$f1
	lwc1	$f1,56($5)
	lwc1	$f0,28($4)
	add.s	$f24,$f24,$f29
	add.s	$f30,$f23,$f30
	mul.s	$f29,$f3,$f10
	mul.s	$f23,$f2,$f10
	add.s	$f28,$f22,$f28
	mfc1	$2,$f14
	mul.s	$f22,$f1,$f10
	mtc1	$3,$f14
	mul.s	$f10,$f10,$f17
	add.s	$f26,$f20,$f26
	mul.s	$f15,$f0,$f2
	mul.s	$f20,$f0,$f3
	add.s	$f31,$f31,$f14
	add.s	$f11,$f11,$f13
	mul.s	$f14,$f0,$f1
	mul.s	$f0,$f0,$f17
	add.s	$f10,$f10,$f12
	mtc1	$2,$f12
	add.s	$f29,$f29,$f24
	add.s	$f15,$f15,$f12
	add.s	$f0,$f0,$f11
	add.s	$f23,$f23,$f30
	add.s	$f22,$f22,$f28
	add.s	$f20,$f20,$f26
	add.s	$f14,$f14,$f31
	swc1	$f29,0($6)
	lwc1	$f11,36($4)
	swc1	$f10,12($6)
	swc1	$f15,20($6)
	lwc1	$f10,32($4)
	lwc1	$f15,52($4)
	swc1	$f0,28($6)
	lwc1	$f0,48($4)
	mul.s	$f28,$f11,$f19
	mul.s	$f26,$f11,$f18
	mul.s	$f24,$f11,$f16
	mul.s	$f31,$f10,$f7
	mul.s	$f13,$f0,$f9
	mul.s	$f12,$f0,$f8
	mul.s	$f18,$f15,$f18
	mul.s	$f16,$f15,$f16
	mul.s	$f19,$f15,$f19
	swc1	$f23,4($6)
	mul.s	$f15,$f15,$f25
	mul.s	$f23,$f11,$f25
	swc1	$f22,8($6)
	mul.s	$f11,$f0,$f7
	mul.s	$f22,$f10,$f9
	swc1	$f20,16($6)
	swc1	$f14,24($6)
	mul.s	$f20,$f10,$f8
	mul.s	$f14,$f10,$f27
	mul.s	$f10,$f0,$f27
	lwc1	$f7,40($4)
	lwc1	$f0,56($4)
	add.s	$f22,$f22,$f28
	mul.s	$f29,$f7,$f6
	add.s	$f20,$f20,$f26
	mul.s	$f27,$f7,$f5
	mul.s	$f25,$f7,$f4
	mul.s	$f8,$f0,$f6
	add.s	$f31,$f31,$f24
	mul.s	$f6,$f0,$f4
	mul.s	$f9,$f7,$f21
	add.s	$f12,$f12,$f18
	mul.s	$f7,$f0,$f5
	add.s	$f11,$f11,$f16
	add.s	$f10,$f10,$f15
	mul.s	$f5,$f0,$f21
	add.s	$f14,$f14,$f23
	add.s	$f13,$f13,$f19
	lwc1	$f4,44($4)
	lwc1	$f0,60($4)
	mul.s	$f18,$f4,$f3
	mul.s	$f16,$f4,$f2
	mul.s	$f15,$f4,$f1
	add.s	$f29,$f29,$f22
	add.s	$f27,$f27,$f20
	mul.s	$f3,$f0,$f3
	mul.s	$f2,$f0,$f2
	mul.s	$f1,$f0,$f1
	add.s	$f25,$f25,$f31
	add.s	$f9,$f9,$f14
	mul.s	$f4,$f4,$f17
	add.s	$f8,$f8,$f13
	add.s	$f7,$f7,$f12
	add.s	$f6,$f6,$f11
	add.s	$f5,$f5,$f10
	mul.s	$f0,$f0,$f17
	add.s	$f18,$f18,$f29
	add.s	$f16,$f16,$f27
	add.s	$f15,$f15,$f25
	add.s	$f4,$f4,$f9
	add.s	$f3,$f3,$f8
	add.s	$f2,$f2,$f7
	add.s	$f1,$f1,$f6
	add.s	$f0,$f0,$f5
	lwc1	$f30,28($sp)
	lwc1	$f28,24($sp)
	lwc1	$f26,20($sp)
	lwc1	$f24,16($sp)
	lwc1	$f22,12($sp)
	lwc1	$f20,8($sp)
	addiu	$sp,$sp,32
	swc1	$f18,32($6)
	swc1	$f16,36($6)
	swc1	$f15,40($6)
	swc1	$f4,44($6)
	swc1	$f3,48($6)
	swc1	$f2,52($6)
	swc1	$f1,56($6)
	.set	noreorder
	.set	nomacro
	jr	$31
	swc1	$f0,60($6)
	.set	macro
	.set	reorder

	.end	matrix_matrix_multiply
	.size	matrix_matrix_multiply, .-matrix_matrix_multiply
	.align	2
	.align	3
	.globl	transform_vector_list
	.set	nomips16
	.set	nomicromips
	.ent	transform_vector_list
	.type	transform_vector_list, @function
transform_vector_list:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	blez	$7,.L33
	sll	$2,$7,4
	lwc1	$f29,0($4)
	lwc1	$f27,4($4)
	addu	$2,$5,$2
	lwc1	$f25,8($4)
	lwc1	$f23,12($4)
	lwc1	$f21,16($4)
	lwc1	$f19,20($4)
	lwc1	$f18,24($4)
	lwc1	$f17,28($4)
	lwc1	$f16,32($4)
	lwc1	$f15,36($4)
	lwc1	$f14,40($4)
	lwc1	$f13,44($4)
	lwc1	$f12,48($4)
	lwc1	$f11,52($4)
	lwc1	$f10,56($4)
	lwc1	$f9,60($4)
	.align	3
.L31:
	lwc1	$f2,0($5)
	addiu	$6,$6,16
	addiu	$5,$5,16
	lwc1	$f1,-12($5)
	mul.s	$f5,$f29,$f2
	mul.s	$f8,$f27,$f1
	mul.s	$f7,$f1,$f19
	mul.s	$f6,$f1,$f15
	mul.s	$f4,$f2,$f21
	mul.s	$f3,$f2,$f16
	mul.s	$f1,$f1,$f11
	mul.s	$f2,$f2,$f12
	lwc1	$f0,-8($5)
	add.s	$f5,$f5,$f8
	add.s	$f4,$f4,$f7
	add.s	$f3,$f3,$f6
	mul.s	$f8,$f25,$f0
	mul.s	$f7,$f0,$f18
	mul.s	$f6,$f0,$f14
	add.s	$f2,$f2,$f1
	mul.s	$f1,$f0,$f10
	lwc1	$f0,-4($5)
	add.s	$f8,$f8,$f5
	add.s	$f7,$f7,$f4
	mul.s	$f5,$f23,$f0
	mul.s	$f4,$f0,$f17
	add.s	$f6,$f6,$f3
	add.s	$f1,$f1,$f2
	mul.s	$f3,$f0,$f13
	mul.s	$f0,$f0,$f9
	add.s	$f5,$f5,$f8
	add.s	$f4,$f4,$f7
	add.s	$f3,$f3,$f6
	add.s	$f0,$f0,$f1
	swc1	$f5,-16($6)
	swc1	$f4,-12($6)
	swc1	$f3,-8($6)
	swc1	$f0,-4($6)
	bne	$2,$5,.L31
.L33:
	jr	$31
	.end	transform_vector_list
	.size	transform_vector_list, .-transform_vector_list
	.align	2
	.align	3
	.globl	fpu_compare_test
	.set	nomips16
	.set	nomicromips
	.ent	fpu_compare_test
	.type	fpu_compare_test, @function
fpu_compare_test:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	c.eq.s	$f12,$f13
	li	$2,1			# 0x1
	bc1t	.L35
	move	$2,$0
.L35:
	c.lt.s	$f12,$f13
	sw	$2,0($6)
	li	$2,1			# 0x1
	bc1t	.L36
	move	$2,$0
.L36:
	c.le.s	$f12,$f13
	sw	$2,4($6)
	li	$2,1			# 0x1
	bc1t	.L37
	move	$2,$0
.L37:
	c.lt.s	$f13,$f12
	sw	$2,8($6)
	li	$2,1			# 0x1
	bc1t	.L38
	move	$2,$0
.L38:
	c.le.s	$f13,$f12
	sw	$2,12($6)
	li	$2,1			# 0x1
	bc1t	.L39
	move	$2,$0
.L39:
	c.eq.s	$f12,$f13
	sw	$2,16($6)
	li	$2,1			# 0x1
	bc1f	.L40
	move	$2,$0
.L40:
	.set	noreorder
	.set	nomacro
	jr	$31
	sw	$2,20($6)
	.set	macro
	.set	reorder

	.end	fpu_compare_test
	.size	fpu_compare_test, .-fpu_compare_test
	.section	.rodata.str1.8
	.align	3
.LC32:
	.ascii	"--- FPU Compare Tests ---\000"
	.align	3
.LC33:
	.ascii	"--- Infinity Tests ---\000"
	.align	3
.LC35:
	.ascii	"+inf vs +inf\000"
	.align	3
.LC37:
	.ascii	"+inf vs 1\000"
	.align	3
.LC39:
	.ascii	"-inf vs 1\000"
	.align	3
.LC40:
	.ascii	"+inf vs -inf\000"
	.align	3
.LC41:
	.ascii	"--- NaN Tests ---\000"
	.align	3
.LC43:
	.ascii	"NaN vs 1\000"
	.align	3
.LC44:
	.ascii	"1 vs NaN\000"
	.align	3
.LC45:
	.ascii	"NaN vs NaN\000"
	.align	3
.LC46:
	.ascii	"FPU Compare: %d/%d tests passed\012\000"
	.align	3
.LC0:
	.ascii	"1 vs 2\000"
	.align	3
.LC1:
	.ascii	"2 vs 1\000"
	.align	3
.LC2:
	.ascii	"1 vs 1\000"
	.align	3
.LC3:
	.ascii	"0 vs 0\000"
	.align	3
.LC4:
	.ascii	"-0 vs +0\000"
	.align	3
.LC5:
	.ascii	"+0 vs -0\000"
	.align	3
.LC6:
	.ascii	"-1 vs -2\000"
	.align	3
.LC7:
	.ascii	"-2 vs -1\000"
	.align	3
.LC8:
	.ascii	"-1 vs 1\000"
	.align	3
.LC9:
	.ascii	"1 vs 1+eps\000"
	.align	3
.LC10:
	.ascii	"1+eps vs 1\000"
	.align	3
.LC11:
	.ascii	"1e30 vs 1e30\000"
	.align	3
.LC12:
	.ascii	"1e30 vs -1e30\000"
	.align	3
.LC13:
	.ascii	"1e-30 vs 1e-30\000"
	.align	3
.LC14:
	.ascii	"1e-30 vs 2e-30\000"
	.rdata
	.align	3
.LC31:
	.word	1065353216
	.word	1073741824
	.word	0
	.word	1
	.word	1
	.word	0
	.word	0
	.word	1
	.word	.LC0
	.word	1073741824
	.word	1065353216
	.word	0
	.word	0
	.word	0
	.word	1
	.word	1
	.word	1
	.word	.LC1
	.word	1065353216
	.word	1065353216
	.word	1
	.word	0
	.word	1
	.word	0
	.word	1
	.word	0
	.word	.LC2
	.word	0
	.word	0
	.word	1
	.word	0
	.word	1
	.word	0
	.word	1
	.word	0
	.word	.LC3
	.word	-2147483648
	.word	0
	.word	1
	.word	0
	.word	1
	.word	0
	.word	1
	.word	0
	.word	.LC4
	.word	0
	.word	-2147483648
	.word	1
	.word	0
	.word	1
	.word	0
	.word	1
	.word	0
	.word	.LC5
	.word	-1082130432
	.word	-1073741824
	.word	0
	.word	0
	.word	0
	.word	1
	.word	1
	.word	1
	.word	.LC6
	.word	-1073741824
	.word	-1082130432
	.word	0
	.word	1
	.word	1
	.word	0
	.word	0
	.word	1
	.word	.LC7
	.word	-1082130432
	.word	1065353216
	.word	0
	.word	1
	.word	1
	.word	0
	.word	0
	.word	1
	.word	.LC8
	.word	1065353216
	.word	1065353216
	.word	0
	.word	1
	.word	1
	.word	0
	.word	0
	.word	1
	.word	.LC9
	.word	1065353216
	.word	1065353216
	.word	0
	.word	0
	.word	0
	.word	1
	.word	1
	.word	1
	.word	.LC10
	.word	1900671689
	.word	1900671689
	.word	1
	.word	0
	.word	1
	.word	0
	.word	1
	.word	0
	.word	.LC11
	.word	1900671689
	.word	-246811959
	.word	0
	.word	0
	.word	0
	.word	1
	.word	1
	.word	1
	.word	.LC12
	.word	228737631
	.word	228737631
	.word	1
	.word	0
	.word	1
	.word	0
	.word	1
	.word	0
	.word	.LC13
	.word	228737631
	.word	237126239
	.word	0
	.word	1
	.word	1
	.word	0
	.word	0
	.word	1
	.word	.LC14
	.text
	.align	2
	.align	3
	.globl	fpu_compare_validate_all
	.set	nomips16
	.set	nomicromips
	.ent	fpu_compare_validate_all
	.type	fpu_compare_validate_all, @function
fpu_compare_validate_all:
	.frame	$sp,752,$31		# vars= 656, regs= 10/2, args= 0, gp= 0
	.mask	0xc0ff0000,-24
	.fmask	0x00500000,-4
	addiu	$sp,$sp,-752
	lui	$2,%hi(.LC31)
	addiu	$2,$2,%lo(.LC31)
	swc1	$f22,748($sp)
	swc1	$f20,744($sp)
	sd	$31,728($sp)
	sd	$fp,720($sp)
	addiu	$3,$2,512
	sd	$23,712($sp)
	sd	$22,704($sp)
	sd	$21,696($sp)
	sd	$20,688($sp)
	sd	$19,680($sp)
	sd	$18,672($sp)
	sd	$17,664($sp)
	sd	$16,656($sp)
	sw	$sp,592($sp)
.L42:
	lw	$4,592($sp)
	addiu	$2,$2,32
	addiu	$4,$4,32
	ld	$7,-32($2)
	sw	$4,592($sp)
	lw	$8,592($sp)
	sd	$7,-32($8)
	ld	$6,-24($2)
	lw	$7,592($sp)
	sd	$6,-24($7)
	ld	$5,-16($2)
	lw	$6,592($sp)
	sd	$5,-16($6)
	ld	$4,-8($2)
	lw	$5,592($sp)
	sd	$4,-8($5)
	bne	$2,$3,.L42
	lw	$7,592($sp)
	lui	$4,%hi(.LC32)
	ld	$6,0($2)
	addiu	$4,$4,%lo(.LC32)
	sd	$6,0($7)
	ld	$5,8($2)
	lw	$6,592($sp)
	ld	$3,16($2)
	sd	$5,8($6)
	lw	$5,592($sp)
	sd	$3,16($5)
	lw	$2,24($2)
	lw	$3,592($sp)
	.set	noreorder
	.set	nomacro
	jal	puts
	sw	$2,24($3)
	.set	macro
	.set	reorder

	li	$2,1			# 0x1
	sw	$2,624($sp)
	lui	$2,%hi(.LC19)
	addiu	$2,$2,%lo(.LC19)
	sw	$sp,592($sp)
	sw	$2,628($sp)
	lui	$2,%hi(.LC20)
	addiu	$2,$2,%lo(.LC20)
	sw	$0,596($sp)
	sw	$2,632($sp)
	lui	$2,%hi(.LC24)
	addiu	$2,$2,%lo(.LC24)
	sw	$2,636($sp)
	lui	$2,%hi(.LC23)
	sw	$2,640($sp)
	.align	3
.L60:
	lw	$2,592($sp)
	li	$8,1			# 0x1
	lwc1	$f22,0($2)
	lwc1	$f20,4($2)
	c.eq.s	$f22,$f20
	bc1t	.L43
	move	$8,$0
.L43:
	c.lt.s	$f22,$f20
	li	$3,1			# 0x1
	bc1t	.L44
	move	$3,$0
.L44:
	c.le.s	$f22,$f20
	li	$19,1			# 0x1
	bc1t	.L45
	move	$19,$0
.L45:
	c.lt.s	$f20,$f22
	li	$18,1			# 0x1
	bc1t	.L46
	move	$18,$0
.L46:
	c.le.s	$f20,$f22
	li	$17,1			# 0x1
	bc1t	.L47
	move	$17,$0
.L47:
	c.eq.s	$f22,$f20
	li	$20,1			# 0x1
	bc1f	.L48
	move	$20,$0
.L48:
	lw	$2,592($sp)
	lw	$9,8($2)
	lw	$10,12($2)
	lw	$23,16($2)
	lw	$22,20($2)
	lw	$21,24($2)
	lw	$fp,28($2)
	lw	$16,32($2)
	beq	$9,$8,.L68
	mov.s	$f12,$f20
	sw	$3,612($sp)
	sw	$10,608($sp)
	sw	$9,616($sp)
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sw	$8,620($sp)
	.set	macro
	.set	reorder

	mov.s	$f12,$f22
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sd	$2,600($sp)
	.set	macro
	.set	reorder

	lw	$9,616($sp)
	lw	$8,620($sp)
	move	$6,$2
	ld	$7,600($sp)
	move	$5,$16
	.set	noreorder
	.set	nomacro
	jal	printf
	lw	$4,628($sp)
	.set	macro
	.set	reorder

	move	$2,$0
	lw	$10,608($sp)
	lw	$3,612($sp)
.L49:
	beq	$10,$3,.L50
	mov.s	$f12,$f20
	sw	$10,608($sp)
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sw	$3,612($sp)
	.set	macro
	.set	reorder

	mov.s	$f12,$f22
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sd	$2,600($sp)
	.set	macro
	.set	reorder

	lw	$9,608($sp)
	lw	$8,612($sp)
	move	$6,$2
	ld	$7,600($sp)
	move	$5,$16
	.set	noreorder
	.set	nomacro
	jal	printf
	lw	$4,632($sp)
	.set	macro
	.set	reorder

	move	$2,$0
.L50:
	beq	$23,$19,.L51
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	mov.s	$f12,$f22
	.set	macro
	.set	reorder

	mov.s	$f12,$f20
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sd	$2,600($sp)
	.set	macro
	.set	reorder

	lui	$4,%hi(.LC21)
	move	$9,$23
	move	$8,$19
	move	$7,$2
	ld	$6,600($sp)
	move	$5,$16
	addiu	$4,$4,%lo(.LC21)
	.set	noreorder
	.set	nomacro
	jal	printf
	move	$23,$2
	.set	macro
	.set	reorder

	beq	$22,$18,.L97
.L52:
	lui	$4,%hi(.LC22)
	move	$9,$22
	move	$8,$18
	move	$7,$23
	ld	$6,600($sp)
	move	$5,$16
	.set	noreorder
	.set	nomacro
	jal	printf
	addiu	$4,$4,%lo(.LC22)
	.set	macro
	.set	reorder

.L97:
	beq	$21,$17,.L98
.L53:
	lw	$2,640($sp)
	move	$9,$21
	move	$8,$17
	move	$7,$23
	ld	$6,600($sp)
	move	$5,$16
	.set	noreorder
	.set	nomacro
	jal	printf
	addiu	$4,$2,%lo(.LC23)
	.set	macro
	.set	reorder

.L98:
	beq	$fp,$20,.L57
.L54:
	move	$9,$fp
	move	$8,$20
	move	$7,$23
	ld	$6,600($sp)
	move	$5,$16
	.set	noreorder
	.set	nomacro
	jal	printf
	lw	$4,636($sp)
	.set	macro
	.set	reorder

.L57:
	sw	$0,624($sp)
.L55:
	lw	$2,592($sp)
	addiu	$2,$2,36
	sw	$2,592($sp)
	addiu	$2,$sp,540
	lw	$3,592($sp)
	bne	$2,$3,.L60
	lui	$4,%hi(.LC33)
	lui	$19,%hi(.LC34)
	.set	noreorder
	.set	nomacro
	jal	puts
	addiu	$4,$4,%lo(.LC33)
	.set	macro
	.set	reorder

	lwc1	$f14,%lo(.LC34)($19)
	lui	$4,%hi(.LC35)
	li	$2,1			# 0x1
	mov.s	$f13,$f14
	addiu	$8,$sp,544
	addiu	$7,$sp,568
	addiu	$4,$4,%lo(.LC35)
	sd	$2,568($sp)
	sd	$2,576($sp)
	sd	$2,584($sp)
	sd	$2,544($sp)
	sd	$2,552($sp)
	.set	noreorder
	.set	nomacro
	jal	validate_cmp
	sd	$2,560($sp)
	.set	macro
	.set	reorder

	move	$16,$2
	beq	$2,$0,.L61
	lw	$2,596($sp)
	addiu	$2,$2,1
	lw	$16,624($sp)
	sw	$2,596($sp)
.L61:
	lui	$17,%hi(.LC36)
	li	$2,1			# 0x1
	lwc1	$f14,%lo(.LC36)($17)
	lui	$4,%hi(.LC37)
	lwc1	$f13,%lo(.LC34)($19)
	dsll	$3,$2,32
	addiu	$8,$sp,544
	addiu	$7,$sp,568
	addiu	$4,$4,%lo(.LC37)
	sw	$0,568($sp)
	sd	$3,576($sp)
	sw	$2,588($sp)
	sw	$0,544($sp)
	sd	$3,552($sp)
	.set	noreorder
	.set	nomacro
	jal	validate_cmp
	sw	$2,564($sp)
	.set	macro
	.set	reorder

	move	$18,$2
	beq	$2,$0,.L62
	lw	$2,596($sp)
	move	$18,$16
	addiu	$2,$2,1
	sw	$2,596($sp)
.L62:
	lui	$20,%hi(.LC38)
	lwc1	$f14,%lo(.LC36)($17)
	lwc1	$f13,%lo(.LC38)($20)
	lui	$4,%hi(.LC39)
	li	$2,1			# 0x1
	li	$3,1			# 0x1
	addiu	$8,$sp,544
	addiu	$7,$sp,568
	addiu	$4,$4,%lo(.LC39)
	sw	$3,572($sp)
	sd	$2,576($sp)
	sw	$0,584($sp)
	sw	$2,548($sp)
	sd	$2,552($sp)
	.set	noreorder
	.set	nomacro
	jal	validate_cmp
	sw	$0,560($sp)
	.set	macro
	.set	reorder

	move	$16,$2
	beq	$2,$0,.L63
	lw	$2,596($sp)
	move	$16,$18
	addiu	$2,$2,1
	sw	$2,596($sp)
.L63:
	li	$2,1			# 0x1
	lwc1	$f14,%lo(.LC38)($20)
	lwc1	$f13,%lo(.LC34)($19)
	lui	$4,%hi(.LC40)
	dsll	$3,$2,32
	addiu	$8,$sp,544
	addiu	$7,$sp,568
	addiu	$4,$4,%lo(.LC40)
	sw	$0,572($sp)
	sd	$3,576($sp)
	sw	$2,584($sp)
	sw	$0,548($sp)
	sd	$3,552($sp)
	.set	noreorder
	.set	nomacro
	jal	validate_cmp
	sw	$2,560($sp)
	.set	macro
	.set	reorder

	move	$18,$2
	beq	$2,$0,.L64
	lw	$2,596($sp)
	move	$18,$16
	addiu	$2,$2,1
	sw	$2,596($sp)
.L64:
	lui	$4,%hi(.LC41)
	lui	$19,%hi(.LC42)
	.set	noreorder
	.set	nomacro
	jal	puts
	addiu	$4,$4,%lo(.LC41)
	.set	macro
	.set	reorder

	lwc1	$f14,%lo(.LC36)($17)
	lwc1	$f13,%lo(.LC42)($19)
	lui	$4,%hi(.LC43)
	addiu	$8,$sp,544
	addiu	$7,$sp,568
	addiu	$4,$4,%lo(.LC43)
	sw	$0,556($sp)
	.set	noreorder
	.set	nomacro
	jal	validate_cmp
	sw	$0,560($sp)
	.set	macro
	.set	reorder

	move	$16,$2
	beq	$2,$0,.L65
	lw	$2,596($sp)
	move	$16,$18
	addiu	$2,$2,1
	sw	$2,596($sp)
.L65:
	lwc1	$f13,%lo(.LC36)($17)
	li	$2,1			# 0x1
	lwc1	$f14,%lo(.LC42)($19)
	lui	$4,%hi(.LC44)
	sw	$2,572($sp)
	addiu	$8,$sp,544
	li	$2,1			# 0x1
	addiu	$7,$sp,568
	addiu	$4,$4,%lo(.LC44)
	sd	$2,576($sp)
	.set	noreorder
	.set	nomacro
	jal	validate_cmp
	sw	$0,584($sp)
	.set	macro
	.set	reorder

	move	$17,$2
	beq	$2,$0,.L66
	lw	$2,596($sp)
	move	$17,$16
	addiu	$2,$2,1
	sw	$2,596($sp)
.L66:
	lwc1	$f14,%lo(.LC42)($19)
	lui	$4,%hi(.LC45)
	li	$2,1			# 0x1
	addiu	$8,$sp,544
	mov.s	$f13,$f14
	addiu	$7,$sp,568
	addiu	$4,$4,%lo(.LC45)
	sd	$2,568($sp)
	.set	noreorder
	.set	nomacro
	jal	validate_cmp
	sd	$2,584($sp)
	.set	macro
	.set	reorder

	move	$16,$2
	beq	$2,$0,.L67
	lw	$2,596($sp)
	move	$16,$17
	addiu	$2,$2,1
	sw	$2,596($sp)
.L67:
	lui	$4,%hi(.LC46)
	lw	$5,596($sp)
	li	$6,22			# 0x16
	.set	noreorder
	.set	nomacro
	jal	printf
	addiu	$4,$4,%lo(.LC46)
	.set	macro
	.set	reorder

	ld	$31,728($sp)
	lwc1	$f22,748($sp)
	move	$2,$16
	lwc1	$f20,744($sp)
	ld	$fp,720($sp)
	ld	$23,712($sp)
	ld	$22,704($sp)
	ld	$21,696($sp)
	ld	$20,688($sp)
	ld	$19,680($sp)
	ld	$18,672($sp)
	ld	$17,664($sp)
	ld	$16,656($sp)
	.set	noreorder
	.set	nomacro
	jr	$31
	addiu	$sp,$sp,752
	.set	macro
	.set	reorder

	.align	3
.L51:
	beq	$22,$18,.L56
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	mov.s	$f12,$f22
	.set	macro
	.set	reorder

	mov.s	$f12,$f20
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sd	$2,600($sp)
	.set	macro
	.set	reorder

	move	$23,$2
	b	.L52
	.align	3
.L68:
	li	$2,1			# 0x1
	b	.L49
	.align	3
.L56:
	beq	$21,$17,.L58
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	mov.s	$f12,$f22
	.set	macro
	.set	reorder

	mov.s	$f12,$f20
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sd	$2,600($sp)
	.set	macro
	.set	reorder

	move	$23,$2
	b	.L53
	.align	3
.L58:
	beq	$fp,$20,.L59
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	mov.s	$f12,$f22
	.set	macro
	.set	reorder

	mov.s	$f12,$f20
	.set	noreorder
	.set	nomacro
	jal	__extendsfdf2
	sd	$2,600($sp)
	.set	macro
	.set	reorder

	move	$23,$2
	b	.L54
	.align	3
.L59:
	beq	$2,$0,.L57
	lw	$2,596($sp)
	addiu	$2,$2,1
	sw	$2,596($sp)
	b	.L55
	.end	fpu_compare_validate_all
	.size	fpu_compare_validate_all, .-fpu_compare_validate_all
	.section	.rodata.cst4,"aM",@progbits,4
	.align	2
.LC34:
	.word	2139095040
	.align	2
.LC36:
	.word	1065353216
	.align	2
.LC38:
	.word	-8388608
	.align	2
.LC42:
	.word	2143289344
	.globl	__extendsfdf2
	.ident	"GCC: (GNU) 15.2.0"
