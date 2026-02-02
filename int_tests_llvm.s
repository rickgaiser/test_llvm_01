	.section	.mdebug.abiN32,"",@progbits
	.nan	legacy
	.text
	.file	"int_tests.c"
	.globl	int_scalar_multiply             # -- Begin function int_scalar_multiply
	.p2align	3
	.type	int_scalar_multiply,@function
	.set	nomicromips
	.set	nomips16
	.ent	int_scalar_multiply
int_scalar_multiply:                    # @int_scalar_multiply
	.frame	$sp,0,$ra
	.mask 	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	.set	noat
# %bb.0:
	blez	$6, .LBB0_8
	nop
# %bb.1:
	sll	$2, $5, 0
	sll	$4, $4, 0
	andi	$3, $6, 3
	sltiu	$1, $6, 4
	bnez	$1, .LBB0_6
	addiu	$5, $zero, 0
# %bb.2:
	lui	$1, 32767
	addiu	$5, $zero, 0
	addiu	$8, $2, 8
	addiu	$9, $4, 8
	ori	$1, $1, 65532
	and	$1, $6, $1
	negu	$6, $1
.LBB0_3:                                # =>This Inner Loop Header: Depth=1
	lw	$1, -8($9)
	addiu	$10, $8, 16
	addiu	$5, $5, -4
	mult	$1, $7
	mflo	$1
	sw	$1, -8($8)
	lw	$1, -4($9)
	mult	$1, $7
	mflo	$1
	sw	$1, -4($8)
	lw	$1, 0($9)
	mult	$1, $7
	mflo	$1
	sw	$1, 0($8)
	lw	$1, 4($9)
	addiu	$9, $9, 16
	mult	$1, $7
	mflo	$1
	sw	$1, 4($8)
	bne	$6, $5, .LBB0_3
	move	$8, $10
# %bb.4:
	beqz	$3, .LBB0_8
	nop
# %bb.5:
	negu	$5, $5
.LBB0_6:
	sll	$1, $5, 2
	sll	$3, $3, 2
	addu	$4, $4, $1
	addu	$5, $2, $1
	addu	$1, $1, $3
	addu	$2, $2, $1
.LBB0_7:                                # =>This Inner Loop Header: Depth=1
	lw	$1, 0($4)
	mult	$1, $7
	mflo	$1
	sw	$1, 0($5)
	addiu	$5, $5, 4
	bne	$5, $2, .LBB0_7
	addiu	$4, $4, 4
.LBB0_8:
	jr	$ra
	nop
	.set	at
	.set	macro
	.set	reorder
	.end	int_scalar_multiply
.Lfunc_end0:
	.size	int_scalar_multiply, .Lfunc_end0-int_scalar_multiply
                                        # -- End function
	.globl	transform_vector_list_fp        # -- Begin function transform_vector_list_fp
	.p2align	3
	.type	transform_vector_list_fp,@function
	.set	nomicromips
	.set	nomips16
	.ent	transform_vector_list_fp
transform_vector_list_fp:               # @transform_vector_list_fp
	.frame	$sp,288,$ra
	.mask 	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	.set	noat
# %bb.0:
	blez	$7, .LBB1_9
	nop
# %bb.1:
	addiu	$sp, $sp, -288
	sd	$ra, 280($sp)                   # 8-byte Folded Spill
	sd	$fp, 272($sp)                   # 8-byte Folded Spill
	sd	$23, 264($sp)                   # 8-byte Folded Spill
	sd	$22, 256($sp)                   # 8-byte Folded Spill
	sd	$21, 248($sp)                   # 8-byte Folded Spill
	sd	$20, 240($sp)                   # 8-byte Folded Spill
	sd	$19, 232($sp)                   # 8-byte Folded Spill
	sd	$18, 224($sp)                   # 8-byte Folded Spill
	sd	$17, 216($sp)                   # 8-byte Folded Spill
	sd	$16, 208($sp)                   # 8-byte Folded Spill
	sll	$1, $4, 0
	sll	$8, $6, 0
	sll	$5, $5, 0
	andi	$16, $7, 3
	addiu	$18, $zero, 0
	lw	$3, 52($1)
	lw	$10, 48($1)
	lw	$19, 56($1)
	lw	$24, 44($1)
	lw	$11, 40($1)
	lw	$9, 36($1)
	lw	$6, 24($1)
	lw	$20, 20($1)
	lw	$4, 16($1)
	lw	$15, 0($1)
	lw	$2, 60($1)
	sd	$3, 192($sp)                    # 8-byte Folded Spill
	lw	$3, 32($1)
	move	$17, $10
	sd	$19, 120($sp)                   # 8-byte Folded Spill
	sd	$10, 184($sp)                   # 8-byte Folded Spill
	sd	$20, 56($sp)                    # 8-byte Folded Spill
	sd	$24, 160($sp)                   # 8-byte Folded Spill
	sd	$11, 152($sp)                   # 8-byte Folded Spill
	sd	$9, 200($sp)                    # 8-byte Folded Spill
	sd	$6, 144($sp)                    # 8-byte Folded Spill
	sd	$4, 136($sp)                    # 8-byte Folded Spill
	sd	$15, 128($sp)                   # 8-byte Folded Spill
	sd	$3, 112($sp)                    # 8-byte Folded Spill
	lw	$3, 28($1)
	sd	$3, 104($sp)                    # 8-byte Folded Spill
	lw	$3, 12($1)
	sd	$3, 176($sp)                    # 8-byte Folded Spill
	lw	$3, 8($1)
	sd	$3, 96($sp)                     # 8-byte Folded Spill
	lw	$3, 4($1)
	sltiu	$1, $7, 4
	bnez	$1, .LBB1_6
	sd	$3, 168($sp)
# %bb.2:
	lui	$1, 32767
	addiu	$3, $zero, 0
	addiu	$21, $8, 32
	addiu	$22, $5, 32
	sw	$16, 92($sp)                    # 4-byte Folded Spill
	sw	$8, 16($sp)                     # 4-byte Folded Spill
	sw	$5, 12($sp)                     # 4-byte Folded Spill
	sd	$2, 64($sp)                     # 8-byte Folded Spill
	ori	$1, $1, 65532
	sw	$3, 80($sp)                     # 4-byte Folded Spill
	and	$1, $7, $1
	negu	$1, $1
	sw	$1, 20($sp)                     # 4-byte Folded Spill
.LBB1_3:                                # =>This Inner Loop Header: Depth=1
	lw	$fp, -32($22)
	ld	$1, 136($sp)                    # 8-byte Folded Reload
	ld	$4, 192($sp)                    # 8-byte Folded Reload
	lw	$23, -24($22)
	ld	$7, 144($sp)                    # 8-byte Folded Reload
	mult	$fp, $1
	lw	$1, -28($22)
	mfhi	$2
	mflo	$3
	nop
	nop
	mult	$1, $4
	mfhi	$4
	mflo	$5
	nop
	nop
	mult	$fp, $17
	mflo	$6
	mfhi	$8
	nop
	nop
	mult	$23, $19
	mfhi	$9
	mflo	$10
	nop
	nop
	mult	$23, $7
	ld	$7, 200($sp)                    # 8-byte Folded Reload
	mfhi	$12
	mflo	$11
	dsll	$2, $2, 32
	mult	$1, $7
	dsll	$5, $5, 32
	dsll	$3, $3, 32
	mfhi	$13
	mflo	$14
	nop
	nop
	mult	$1, $20
	dsll	$4, $4, 32
	dsll	$6, $6, 32
	ld	$18, 112($sp)                   # 8-byte Folded Reload
	mflo	$15
	mfhi	$24
	dsrl	$5, $5, 32
	dsll	$8, $8, 32
	dsrl	$3, $3, 32
	dsrl	$6, $6, 32
	or	$4, $5, $4
	dsll	$9, $9, 32
	or	$2, $3, $2
	mult	$fp, $18
	or	$6, $6, $8
	dsll	$12, $12, 32
	mflo	$16
	mfhi	$25
	daddu	$4, $4, $6
	dsll	$3, $13, 32
	dsll	$5, $15, 32
	dsll	$8, $11, 32
	dsll	$10, $10, 32
	dsll	$11, $14, 32
	dsll	$14, $24, 32
	ld	$24, 152($sp)                   # 8-byte Folded Reload
	dsrl	$5, $5, 32
	dsrl	$10, $10, 32
	dsrl	$11, $11, 32
	dsrl	$8, $8, 32
	or	$5, $5, $14
	mult	$23, $24
	or	$9, $10, $9
	or	$3, $11, $3
	or	$8, $8, $12
	dsll	$14, $16, 32
	lw	$ra, -20($22)
	ld	$7, 64($sp)                     # 8-byte Folded Reload
	mfhi	$6
	mflo	$10
	dsll	$13, $25, 32
	daddu	$4, $4, $9
	daddu	$2, $5, $2
	move	$25, $24
	dsrl	$14, $14, 32
	daddu	$2, $2, $8
	mult	$ra, $7
	or	$13, $14, $13
	mflo	$7
	mfhi	$14
	daddu	$3, $3, $13
	dsll	$6, $6, 32
	sw	$7, 72($sp)                     # 8-byte Folded Spill
	mfhi	$7
	dsll	$12, $14, 32
	sw	$7, 76($sp)                     # 8-byte Folded Spill
	dsll	$10, $10, 32
	ld	$7, 160($sp)                    # 8-byte Folded Reload
	ld	$9, 104($sp)                    # 8-byte Folded Reload
	dsrl	$10, $10, 32
	mult	$ra, $7
	or	$6, $10, $6
	mfhi	$11
	mflo	$5
	nop
	nop
	mult	$ra, $9
	daddu	$3, $3, $6
	mflo	$9
	dsll	$11, $11, 32
	sw	$9, 48($sp)                     # 8-byte Folded Spill
	mfhi	$9
	sw	$9, 52($sp)                     # 8-byte Folded Spill
	dsll	$5, $5, 32
	lw	$13, 72($sp)                    # 8-byte Folded Reload
	mfhi	$9
	dsrl	$5, $5, 32
	mtlo	$13
	lw	$13, 76($sp)                    # 8-byte Folded Reload
	or	$5, $5, $11
	daddu	$3, $3, $5
	mthi	$13
	dsrl	$3, $3, 16
	dsll	$9, $9, 32
	mflo	$13
	dsll	$13, $13, 32
	ld	$17, 128($sp)                   # 8-byte Folded Reload
	lw	$8, 48($sp)                     # 8-byte Folded Reload
	dsrl	$13, $13, 32
	mult	$fp, $17
	or	$12, $13, $12
	mfhi	$10
	mflo	$13
	mtlo	$8
	lw	$8, 52($sp)                     # 8-byte Folded Reload
	daddu	$4, $4, $12
	dsrl	$4, $4, 16
	mthi	$8
	mflo	$8
	dsll	$10, $10, 32
	dsll	$8, $8, 32
	dsrl	$8, $8, 32
	or	$8, $8, $9
	dsll	$9, $13, 32
	ld	$6, 168($sp)                    # 8-byte Folded Reload
	ld	$15, 96($sp)                    # 8-byte Folded Reload
	daddu	$2, $2, $8
	ld	$8, 176($sp)                    # 8-byte Folded Reload
	dsrl	$9, $9, 32
	mult	$1, $6
	dsrl	$2, $2, 16
	or	$9, $9, $10
	mflo	$1
	mfhi	$6
	nop
	nop
	mult	$23, $15
	mflo	$5
	mfhi	$11
	nop
	nop
	mult	$ra, $8
	mflo	$8
	mfhi	$12
	dsll	$1, $1, 32
	dsll	$6, $6, 32
	dsll	$5, $5, 32
	dsrl	$1, $1, 32
	sw	$4, -20($21)
	sw	$3, -24($21)
	sw	$2, -28($21)
	dsll	$3, $8, 32
	or	$1, $1, $6
	dsll	$6, $11, 32
	dsrl	$5, $5, 32
	dsll	$2, $12, 32
	lw	$23, -12($22)
	ld	$16, 200($sp)                   # 8-byte Folded Reload
	ld	$14, 136($sp)                   # 8-byte Folded Reload
	ld	$13, 192($sp)                   # 8-byte Folded Reload
	dsrl	$3, $3, 32
	daddu	$1, $1, $9
	or	$5, $5, $6
	daddu	$1, $1, $5
	or	$2, $3, $2
	daddu	$1, $1, $2
	dsrl	$1, $1, 16
	sw	$1, -32($21)
	lw	$1, -16($22)
	mult	$1, $18
	mfhi	$2
	mflo	$3
	nop
	nop
	mult	$23, $16
	mflo	$4
	dsll	$2, $2, 32
	sw	$4, 72($sp)                     # 8-byte Folded Spill
	mfhi	$4
	sw	$4, 76($sp)                     # 8-byte Folded Spill
	mfhi	$4
	nop
	nop
	mult	$1, $14
	mflo	$5
	dsll	$4, $4, 32
	sw	$5, 24($sp)                     # 8-byte Folded Spill
	mfhi	$5
	sw	$5, 28($sp)                     # 8-byte Folded Spill
	mfhi	$5
	nop
	nop
	mult	$23, $20
	mflo	$6
	dsll	$5, $5, 32
	sw	$6, 40($sp)                     # 8-byte Folded Spill
	mfhi	$6
	sw	$6, 44($sp)                     # 8-byte Folded Spill
	mfhi	$6
	nop
	nop
	mult	$23, $13
	mflo	$8
	dsll	$6, $6, 32
	sw	$8, 48($sp)                     # 8-byte Folded Spill
	mfhi	$8
	sw	$8, 52($sp)                     # 8-byte Folded Spill
	dsll	$3, $3, 32
	ld	$9, 184($sp)                    # 8-byte Folded Reload
	mfhi	$8
	lw	$10, 24($sp)                    # 8-byte Folded Reload
	dsrl	$3, $3, 32
	mult	$1, $9
	or	$2, $3, $2
	mflo	$9
	dsll	$8, $8, 32
	sw	$9, 32($sp)                     # 8-byte Folded Spill
	mfhi	$9
	sw	$9, 36($sp)                     # 8-byte Folded Spill
	mfhi	$9
	mtlo	$10
	lw	$10, 28($sp)                    # 8-byte Folded Reload
	mthi	$10
	mflo	$10
	dsll	$9, $9, 32
	dsll	$10, $10, 32
	dsrl	$10, $10, 32
	or	$5, $10, $5
	lw	$10, 40($sp)                    # 8-byte Folded Reload
	mtlo	$10
	lw	$10, 44($sp)                    # 8-byte Folded Reload
	mthi	$10
	mflo	$10
	dsll	$10, $10, 32
	lw	$3, 72($sp)                     # 8-byte Folded Reload
	dsrl	$10, $10, 32
	mtlo	$3
	lw	$3, 76($sp)                     # 8-byte Folded Reload
	or	$6, $10, $6
	daddu	$5, $6, $5
	mthi	$3
	mflo	$3
	dsll	$3, $3, 32
	dsrl	$3, $3, 32
	or	$3, $3, $4
	lw	$4, 32($sp)                     # 8-byte Folded Reload
	daddu	$2, $3, $2
	mtlo	$4
	lw	$4, 36($sp)                     # 8-byte Folded Reload
	mthi	$4
	mflo	$4
	dsll	$4, $4, 32
	dsrl	$4, $4, 32
	or	$4, $4, $9
	lw	$9, 48($sp)                     # 8-byte Folded Reload
	mtlo	$9
	lw	$9, 52($sp)                     # 8-byte Folded Reload
	mthi	$9
	mflo	$9
	dsll	$9, $9, 32
	lw	$fp, -8($22)
	ld	$12, 144($sp)                   # 8-byte Folded Reload
	dsrl	$9, $9, 32
	mult	$fp, $25
	or	$8, $9, $8
	mflo	$9
	daddu	$4, $8, $4
	sw	$9, 72($sp)                     # 8-byte Folded Spill
	mfhi	$9
	sw	$9, 76($sp)                     # 8-byte Folded Spill
	mfhi	$9
	nop
	nop
	mult	$fp, $12
	mflo	$6
	mfhi	$8
	dsll	$9, $9, 32
	dsll	$6, $6, 32
	lw	$3, 72($sp)                     # 8-byte Folded Reload
	dsll	$8, $8, 32
	dsrl	$6, $6, 32
	mtlo	$3
	lw	$3, 76($sp)                     # 8-byte Folded Reload
	or	$6, $6, $8
	daddu	$5, $5, $6
	mthi	$3
	mflo	$3
	nop
	nop
	mult	$fp, $19
	dsll	$3, $3, 32
	dsrl	$3, $3, 32
	or	$3, $3, $9
	mfhi	$9
	daddu	$2, $2, $3
	dsll	$8, $9, 32
	mflo	$9
	dsll	$9, $9, 32
	ld	$24, 64($sp)                    # 8-byte Folded Reload
	dsrl	$9, $9, 32
	or	$8, $9, $8
	lw	$9, -4($22)
	daddu	$4, $4, $8
	mult	$9, $7
	mflo	$6
	sw	$6, 72($sp)                     # 8-byte Folded Spill
	mfhi	$6
	sw	$6, 76($sp)                     # 8-byte Folded Spill
	mfhi	$6
	nop
	nop
	mult	$9, $24
	mflo	$3
	lw	$7, 72($sp)                     # 8-byte Folded Reload
	dsll	$6, $6, 32
	sw	$3, 48($sp)                     # 8-byte Folded Spill
	mfhi	$3
	sw	$3, 52($sp)                     # 8-byte Folded Spill
	mfhi	$3
	mtlo	$7
	lw	$7, 76($sp)                     # 8-byte Folded Reload
	mthi	$7
	mflo	$8
	dsll	$3, $3, 32
	dsll	$8, $8, 32
	lw	$7, 48($sp)                     # 8-byte Folded Reload
	dsrl	$8, $8, 32
	mtlo	$7
	lw	$7, 52($sp)                     # 8-byte Folded Reload
	or	$6, $8, $6
	daddu	$2, $2, $6
	mthi	$7
	dsrl	$2, $2, 16
	mflo	$8
	dsll	$8, $8, 32
	ld	$ra, 104($sp)                   # 8-byte Folded Reload
	dsrl	$8, $8, 32
	mult	$9, $ra
	or	$3, $8, $3
	mfhi	$8
	mflo	$10
	nop
	nop
	mult	$1, $17
	daddu	$3, $4, $3
	mflo	$1
	dsrl	$3, $3, 16
	dsll	$10, $10, 32
	ld	$7, 168($sp)                    # 8-byte Folded Reload
	dsll	$8, $8, 32
	sw	$1, 72($sp)                     # 8-byte Folded Spill
	mfhi	$1
	dsrl	$10, $10, 32
	or	$8, $10, $8
	daddu	$5, $5, $8
	sw	$1, 76($sp)                     # 8-byte Folded Spill
	mfhi	$1
	nop
	nop
	mult	$23, $7
	lw	$6, 72($sp)                     # 8-byte Folded Reload
	mfhi	$10
	mflo	$4
	mtlo	$6
	lw	$6, 76($sp)                     # 8-byte Folded Reload
	mthi	$6
	dsll	$1, $1, 32
	mflo	$6
	nop
	nop
	mult	$fp, $15
	mflo	$8
	mfhi	$11
	dsll	$6, $6, 32
	dsll	$4, $4, 32
	ld	$17, 176($sp)                   # 8-byte Folded Reload
	dsrl	$6, $6, 32
	dsrl	$4, $4, 32
	mult	$9, $17
	or	$1, $6, $1
	dsll	$6, $10, 32
	mflo	$9
	mfhi	$10
	or	$4, $4, $6
	dsll	$6, $8, 32
	sw	$3, -4($21)
	sw	$2, -8($21)
	daddu	$1, $4, $1
	dsrl	$4, $5, 16
	dsll	$5, $11, 32
	dsrl	$6, $6, 32
	sw	$4, -12($21)
	or	$5, $6, $5
	daddu	$1, $1, $5
	dsll	$3, $9, 32
	dsll	$2, $10, 32
	lw	$23, 4($22)
	dsrl	$3, $3, 32
	or	$2, $3, $2
	daddu	$1, $1, $2
	dsrl	$1, $1, 16
	sw	$1, -16($21)
	lw	$1, 0($22)
	mult	$1, $18
	move	$18, $14
	mfhi	$2
	mflo	$3
	nop
	nop
	mult	$23, $16
	mflo	$4
	dsll	$2, $2, 32
	sw	$4, 72($sp)                     # 8-byte Folded Spill
	mfhi	$4
	sw	$4, 76($sp)                     # 8-byte Folded Spill
	mfhi	$4
	nop
	nop
	mult	$1, $14
	move	$14, $12
	mflo	$5
	dsll	$4, $4, 32
	sw	$5, 24($sp)                     # 8-byte Folded Spill
	mfhi	$5
	sw	$5, 28($sp)                     # 8-byte Folded Spill
	mfhi	$5
	nop
	nop
	mult	$23, $20
	mflo	$6
	dsll	$5, $5, 32
	sw	$6, 40($sp)                     # 8-byte Folded Spill
	mfhi	$6
	sw	$6, 44($sp)                     # 8-byte Folded Spill
	mfhi	$6
	nop
	nop
	mult	$23, $13
	mflo	$8
	dsll	$6, $6, 32
	sw	$8, 48($sp)                     # 8-byte Folded Spill
	mfhi	$8
	sw	$8, 52($sp)                     # 8-byte Folded Spill
	dsll	$3, $3, 32
	ld	$13, 184($sp)                   # 8-byte Folded Reload
	mfhi	$8
	lw	$10, 24($sp)                    # 8-byte Folded Reload
	dsrl	$3, $3, 32
	mult	$1, $13
	or	$2, $3, $2
	mflo	$9
	dsll	$8, $8, 32
	sw	$9, 32($sp)                     # 8-byte Folded Spill
	mfhi	$9
	sw	$9, 36($sp)                     # 8-byte Folded Spill
	mfhi	$9
	mtlo	$10
	lw	$10, 28($sp)                    # 8-byte Folded Reload
	mthi	$10
	mflo	$10
	dsll	$9, $9, 32
	dsll	$10, $10, 32
	dsrl	$10, $10, 32
	or	$5, $10, $5
	lw	$10, 40($sp)                    # 8-byte Folded Reload
	mtlo	$10
	lw	$10, 44($sp)                    # 8-byte Folded Reload
	mthi	$10
	mflo	$10
	dsll	$10, $10, 32
	lw	$3, 72($sp)                     # 8-byte Folded Reload
	dsrl	$10, $10, 32
	mtlo	$3
	lw	$3, 76($sp)                     # 8-byte Folded Reload
	or	$6, $10, $6
	daddu	$5, $6, $5
	mthi	$3
	mflo	$3
	dsll	$3, $3, 32
	dsrl	$3, $3, 32
	or	$3, $3, $4
	lw	$4, 32($sp)                     # 8-byte Folded Reload
	daddu	$2, $3, $2
	mtlo	$4
	lw	$4, 36($sp)                     # 8-byte Folded Reload
	mthi	$4
	mflo	$4
	dsll	$4, $4, 32
	dsrl	$4, $4, 32
	or	$4, $4, $9
	lw	$9, 48($sp)                     # 8-byte Folded Reload
	mtlo	$9
	lw	$9, 52($sp)                     # 8-byte Folded Reload
	mthi	$9
	mflo	$9
	dsll	$9, $9, 32
	lw	$fp, 8($22)
	dsrl	$9, $9, 32
	mult	$fp, $25
	or	$8, $9, $8
	move	$25, $ra
	mflo	$9
	daddu	$4, $8, $4
	sw	$9, 72($sp)                     # 8-byte Folded Spill
	mfhi	$9
	sw	$9, 76($sp)                     # 8-byte Folded Spill
	mfhi	$9
	nop
	nop
	mult	$fp, $12
	mflo	$6
	mfhi	$8
	dsll	$9, $9, 32
	dsll	$6, $6, 32
	lw	$3, 72($sp)                     # 8-byte Folded Reload
	dsll	$8, $8, 32
	dsrl	$6, $6, 32
	mtlo	$3
	lw	$3, 76($sp)                     # 8-byte Folded Reload
	or	$6, $6, $8
	daddu	$5, $5, $6
	mthi	$3
	mflo	$3
	nop
	nop
	mult	$fp, $19
	dsll	$3, $3, 32
	dsrl	$3, $3, 32
	or	$3, $3, $9
	mfhi	$9
	daddu	$2, $2, $3
	dsll	$8, $9, 32
	mflo	$9
	dsll	$9, $9, 32
	ld	$15, 160($sp)                   # 8-byte Folded Reload
	dsrl	$9, $9, 32
	or	$8, $9, $8
	lw	$9, 12($22)
	daddu	$4, $4, $8
	mult	$9, $15
	mflo	$6
	sw	$6, 72($sp)                     # 8-byte Folded Spill
	mfhi	$6
	sw	$6, 76($sp)                     # 8-byte Folded Spill
	mfhi	$6
	nop
	nop
	mult	$9, $24
	mflo	$3
	lw	$8, 72($sp)                     # 8-byte Folded Reload
	dsll	$6, $6, 32
	sw	$3, 48($sp)                     # 8-byte Folded Spill
	mfhi	$3
	sw	$3, 52($sp)                     # 8-byte Folded Spill
	mfhi	$3
	mtlo	$8
	lw	$8, 76($sp)                     # 8-byte Folded Reload
	mthi	$8
	mflo	$8
	dsll	$3, $3, 32
	dsll	$8, $8, 32
	dsrl	$8, $8, 32
	or	$6, $8, $6
	lw	$8, 48($sp)                     # 8-byte Folded Reload
	daddu	$2, $2, $6
	mtlo	$8
	lw	$8, 52($sp)                     # 8-byte Folded Reload
	dsrl	$2, $2, 16
	mthi	$8
	mflo	$8
	nop
	nop
	mult	$9, $ra
	mflo	$10
	dsll	$8, $8, 32
	dsll	$10, $10, 32
	ld	$16, 128($sp)                   # 8-byte Folded Reload
	dsrl	$8, $8, 32
	or	$3, $8, $3
	mfhi	$8
	dsrl	$10, $10, 32
	nop
	mult	$1, $16
	daddu	$3, $4, $3
	mflo	$1
	dsrl	$3, $3, 16
	dsll	$8, $8, 32
	or	$8, $10, $8
	sw	$1, 72($sp)                     # 8-byte Folded Spill
	mfhi	$1
	daddu	$5, $5, $8
	sw	$1, 76($sp)                     # 8-byte Folded Spill
	mfhi	$1
	nop
	nop
	mult	$23, $7
	lw	$6, 72($sp)                     # 8-byte Folded Reload
	mfhi	$10
	mflo	$4
	mtlo	$6
	lw	$6, 76($sp)                     # 8-byte Folded Reload
	mthi	$6
	dsll	$1, $1, 32
	mflo	$6
	dsll	$6, $6, 32
	ld	$7, 96($sp)                     # 8-byte Folded Reload
	dsll	$4, $4, 32
	dsrl	$6, $6, 32
	dsrl	$4, $4, 32
	mult	$fp, $7
	or	$1, $6, $1
	dsll	$6, $10, 32
	mflo	$8
	mfhi	$11
	nop
	nop
	mult	$9, $17
	or	$4, $4, $6
	mflo	$9
	mfhi	$10
	daddu	$1, $4, $1
	dsrl	$4, $5, 16
	dsll	$6, $8, 32
	sw	$3, 12($21)
	sw	$2, 8($21)
	sw	$4, 4($21)
	dsll	$5, $11, 32
	dsll	$3, $9, 32
	dsrl	$6, $6, 32
	dsll	$2, $10, 32
	lw	$ra, 16($22)
	lw	$fp, 20($22)
	lw	$23, 80($sp)                    # 4-byte Folded Reload
	or	$5, $6, $5
	dsrl	$3, $3, 32
	daddu	$1, $1, $5
	or	$2, $3, $2
	mult	$ra, $18
	addiu	$23, $23, -4
	daddu	$1, $1, $2
	mfhi	$2
	mflo	$3
	nop
	nop
	mult	$fp, $20
	dsrl	$1, $1, 16
	mfhi	$4
	mflo	$5
	sw	$1, 0($21)
	ld	$1, 200($sp)                    # 8-byte Folded Reload
	mult	$fp, $1
	ld	$1, 112($sp)                    # 8-byte Folded Reload
	mfhi	$8
	mflo	$6
	dsll	$2, $2, 32
	mult	$ra, $1
	ld	$1, 192($sp)                    # 8-byte Folded Reload
	dsll	$4, $4, 32
	mfhi	$10
	mflo	$9
	nop
	nop
	mult	$fp, $1
	addiu	$1, $21, 64
	mfhi	$11
	mflo	$12
	nop
	nop
	mult	$ra, $13
	sw	$1, 72($sp)                     # 4-byte Folded Spill
	addiu	$1, $22, 64
	sw	$1, 80($sp)                     # 4-byte Folded Spill
	mflo	$1
	dsll	$3, $3, 32
	mfhi	$13
	dsll	$8, $8, 32
	dsrl	$3, $3, 32
	or	$19, $3, $2
	dsll	$10, $10, 32
	dsll	$11, $11, 32
	sw	$1, 48($sp)                     # 8-byte Folded Spill
	mfhi	$1
	dsll	$13, $13, 32
	sw	$1, 52($sp)                     # 8-byte Folded Spill
	dsll	$5, $5, 32
	dsll	$9, $9, 32
	dsll	$6, $6, 32
	lw	$18, 24($22)
	lw	$22, 28($22)
	dsll	$12, $12, 32
	ld	$1, 120($sp)                    # 8-byte Folded Reload
	dsrl	$3, $5, 32
	dsrl	$9, $9, 32
	dsrl	$6, $6, 32
	dsrl	$12, $12, 32
	mult	$18, $14
	or	$3, $3, $4
	or	$9, $9, $10
	or	$6, $6, $8
	or	$11, $12, $11
	mfhi	$2
	mflo	$14
	nop
	nop
	mult	$18, $1
	daddu	$6, $6, $9
	daddu	$3, $3, $19
	mflo	$4
	mfhi	$5
	nop
	nop
	mult	$22, $15
	mfhi	$15
	mflo	$10
	dsll	$2, $2, 32
	dsll	$4, $4, 32
	dsll	$14, $14, 32
	ld	$1, 152($sp)                    # 8-byte Folded Reload
	dsll	$5, $5, 32
	dsrl	$12, $14, 32
	dsrl	$4, $4, 32
	mult	$18, $1
	or	$4, $4, $5
	dsll	$5, $15, 32
	or	$2, $12, $2
	mflo	$1
	mflo	$24
	daddu	$2, $3, $2
	sw	$1, 40($sp)                     # 8-byte Folded Spill
	mfhi	$1
	sw	$1, 44($sp)                     # 8-byte Folded Spill
	dsll	$24, $24, 32
	lw	$1, 48($sp)                     # 8-byte Folded Reload
	dsrl	$24, $24, 32
	mtlo	$1
	lw	$1, 52($sp)                     # 8-byte Folded Reload
	mthi	$1
	mflo	$8
	dsll	$8, $8, 32
	lw	$1, 40($sp)                     # 8-byte Folded Reload
	dsrl	$8, $8, 32
	mtlo	$1
	lw	$1, 44($sp)                     # 8-byte Folded Reload
	or	$8, $8, $13
	dsll	$10, $10, 32
	ld	$19, 120($sp)                   # 8-byte Folded Reload
	dsrl	$9, $10, 32
	daddu	$8, $11, $8
	mthi	$1
	ld	$1, 64($sp)                     # 8-byte Folded Reload
	or	$5, $9, $5
	daddu	$4, $8, $4
	mfhi	$13
	nop
	nop
	mult	$22, $25
	mfhi	$25
	mflo	$14
	nop
	nop
	mult	$ra, $16
	mflo	$16
	mfhi	$17
	nop
	nop
	mult	$22, $1
	ld	$1, 168($sp)                    # 8-byte Folded Reload
	mfhi	$ra
	dsll	$13, $13, 32
	or	$13, $24, $13
	mflo	$24
	nop
	nop
	mult	$fp, $1
	ld	$1, 176($sp)                    # 8-byte Folded Reload
	dsll	$11, $14, 32
	dsll	$10, $25, 32
	mflo	$20
	mfhi	$fp
	nop
	nop
	mult	$18, $7
	dsrl	$11, $11, 32
	daddu	$3, $6, $13
	mflo	$18
	mfhi	$7
	nop
	nop
	mult	$22, $1
	dsll	$8, $ra, 32
	or	$6, $11, $10
	daddu	$3, $3, $5
	mflo	$22
	mfhi	$1
	daddu	$2, $2, $6
	dsrl	$3, $3, 16
	dsrl	$2, $2, 16
	dsll	$9, $24, 32
	dsrl	$9, $9, 32
	dsll	$6, $fp, 32
	or	$8, $9, $8
	dsll	$9, $17, 32
	ld	$17, 184($sp)                   # 8-byte Folded Reload
	dsll	$10, $16, 32
	daddu	$4, $4, $8
	dsll	$8, $20, 32
	dsrl	$10, $10, 32
	ld	$20, 56($sp)                    # 8-byte Folded Reload
	dsll	$1, $1, 32
	dsrl	$8, $8, 32
	or	$5, $10, $9
	dsrl	$4, $4, 16
	or	$6, $8, $6
	daddu	$5, $6, $5
	dsll	$6, $7, 32
	dsll	$7, $18, 32
	sw	$4, 28($21)
	sw	$3, 24($21)
	sw	$2, 20($21)
	dsll	$3, $22, 32
	lw	$22, 80($sp)                    # 4-byte Folded Reload
	sw	$23, 80($sp)                    # 4-byte Folded Spill
	dsrl	$7, $7, 32
	dsrl	$3, $3, 32
	or	$6, $7, $6
	or	$1, $3, $1
	daddu	$2, $5, $6
	daddu	$1, $2, $1
	dsrl	$1, $1, 16
	sw	$1, 16($21)
	lw	$21, 72($sp)                    # 4-byte Folded Reload
	lw	$1, 20($sp)                     # 4-byte Folded Reload
	bne	$1, $23, .LBB1_3
	nop
# %bb.4:
	lw	$16, 92($sp)                    # 4-byte Folded Reload
	ld	$2, 64($sp)                     # 8-byte Folded Reload
	ld	$24, 160($sp)                   # 8-byte Folded Reload
	ld	$11, 152($sp)                   # 8-byte Folded Reload
	ld	$9, 200($sp)                    # 8-byte Folded Reload
	ld	$6, 144($sp)                    # 8-byte Folded Reload
	ld	$4, 136($sp)                    # 8-byte Folded Reload
	ld	$15, 128($sp)                   # 8-byte Folded Reload
	beqz	$16, .LBB1_8
	nop
# %bb.5:
	lw	$1, 80($sp)                     # 4-byte Folded Reload
	lw	$8, 16($sp)                     # 4-byte Folded Reload
	lw	$5, 12($sp)                     # 4-byte Folded Reload
	negu	$18, $1
.LBB1_6:
	sll	$1, $18, 4
	ori	$1, $1, 8
	addu	$7, $8, $1
	addu	$18, $5, $1
.LBB1_7:                                # =>This Inner Loop Header: Depth=1
	lw	$1, -4($18)
	lw	$3, -8($18)
	lw	$21, 4($18)
	sw	$16, 92($sp)                    # 4-byte Folded Spill
	ld	$16, 112($sp)                   # 8-byte Folded Reload
	mult	$1, $20
	lw	$20, 0($18)
	addiu	$18, $18, 16
	mfhi	$fp
	mflo	$ra
	nop
	nop
	mult	$3, $4
	mfhi	$5
	mflo	$4
	nop
	nop
	mult	$20, $6
	mfhi	$8
	mflo	$6
	nop
	nop
	mult	$1, $9
	mfhi	$9
	mflo	$10
	nop
	nop
	mult	$20, $11
	mflo	$11
	mfhi	$12
	nop
	nop
	mult	$3, $17
	mfhi	$13
	mflo	$14
	nop
	nop
	mult	$3, $15
	dsll	$fp, $fp, 32
	mfhi	$15
	mflo	$22
	nop
	nop
	mult	$21, $2
	dsll	$5, $5, 32
	mfhi	$23
	dsll	$8, $8, 32
	dsll	$9, $9, 32
	sd	$15, 80($sp)                    # 8-byte Folded Spill
	mflo	$15
	nop
	nop
	mult	$21, $24
	mfhi	$24
	mflo	$25
	nop
	nop
	mult	$3, $16
	mflo	$3
	dsll	$24, $24, 32
	sw	$3, 72($sp)                     # 8-byte Folded Spill
	mfhi	$3
	sw	$3, 76($sp)                     # 8-byte Folded Spill
	dsll	$ra, $ra, 32
	dsll	$10, $10, 32
	ld	$16, 104($sp)                   # 8-byte Folded Reload
	dsll	$4, $4, 32
	dsll	$6, $6, 32
	dsll	$11, $11, 32
	dsll	$14, $14, 32
	mfhi	$3
	dsrl	$ra, $ra, 32
	dsrl	$4, $4, 32
	dsrl	$6, $6, 32
	dsrl	$10, $10, 32
	dsrl	$11, $11, 32
	or	$fp, $ra, $fp
	lw	$ra, 72($sp)                    # 8-byte Folded Reload
	mult	$21, $16
	or	$4, $4, $5
	or	$9, $10, $9
	dsll	$10, $12, 32
	dsll	$12, $13, 32
	dsrl	$13, $14, 32
	or	$6, $6, $8
	mfhi	$16
	mflo	$17
	or	$12, $13, $12
	or	$10, $11, $10
	daddu	$4, $fp, $4
	mtlo	$ra
	lw	$ra, 76($sp)                    # 8-byte Folded Reload
	daddu	$4, $4, $6
	dsll	$3, $3, 32
	mthi	$ra
	mflo	$ra
	dsll	$16, $16, 32
	dsll	$ra, $ra, 32
	ld	$5, 192($sp)                    # 8-byte Folded Reload
	dsll	$17, $17, 32
	dsll	$25, $25, 32
	dsll	$15, $15, 32
	dsll	$8, $22, 32
	dsll	$22, $23, 32
	dsrl	$ra, $ra, 32
	dsrl	$8, $8, 32
	mult	$1, $5
	or	$3, $ra, $3
	mflo	$5
	mfhi	$14
	nop
	nop
	mult	$20, $19
	ld	$19, 168($sp)                   # 8-byte Folded Reload
	daddu	$3, $9, $3
	dsrl	$9, $15, 32
	dsrl	$15, $25, 32
	dsrl	$25, $17, 32
	ld	$17, 184($sp)                   # 8-byte Folded Reload
	mfhi	$11
	mflo	$13
	or	$15, $15, $24
	daddu	$3, $3, $10
	or	$9, $9, $22
	mult	$1, $19
	ld	$19, 96($sp)                    # 8-byte Folded Reload
	daddu	$3, $3, $15
	mflo	$1
	mfhi	$23
	dsrl	$3, $3, 16
	mult	$20, $19
	ld	$19, 176($sp)                   # 8-byte Folded Reload
	dsll	$5, $5, 32
	dsll	$14, $14, 32
	ld	$24, 80($sp)                    # 8-byte Folded Reload
	mflo	$20
	mfhi	$ra
	dsrl	$5, $5, 32
	dsll	$6, $11, 32
	ld	$11, 152($sp)                   # 8-byte Folded Reload
	mult	$21, $19
	move	$19, $2
	or	$5, $5, $14
	or	$14, $25, $16
	lw	$16, 92($sp)                    # 4-byte Folded Reload
	dsll	$10, $13, 32
	dsll	$24, $24, 32
	mflo	$21
	ld	$15, 128($sp)                   # 8-byte Folded Reload
	mfhi	$2
	addiu	$25, $7, 16
	dsrl	$10, $10, 32
	daddu	$5, $5, $12
	daddu	$4, $4, $14
	or	$6, $10, $6
	dsrl	$4, $4, 16
	addiu	$16, $16, -1
	daddu	$5, $5, $6
	or	$6, $8, $24
	ld	$24, 160($sp)                   # 8-byte Folded Reload
	dsll	$8, $23, 32
	daddu	$5, $5, $9
	ld	$9, 200($sp)                    # 8-byte Folded Reload
	dsll	$1, $1, 32
	dsrl	$1, $1, 32
	dsrl	$5, $5, 16
	dsll	$2, $2, 32
	or	$1, $1, $8
	dsll	$8, $20, 32
	sw	$5, 4($7)
	sw	$3, 0($7)
	ld	$20, 56($sp)                    # 8-byte Folded Reload
	sw	$4, -4($7)
	daddu	$1, $1, $6
	dsll	$6, $ra, 32
	dsrl	$8, $8, 32
	or	$6, $8, $6
	daddu	$1, $1, $6
	ld	$6, 144($sp)                    # 8-byte Folded Reload
	dsll	$3, $21, 32
	ld	$4, 136($sp)                    # 8-byte Folded Reload
	dsrl	$3, $3, 32
	or	$2, $3, $2
	daddu	$1, $1, $2
	move	$2, $19
	ld	$19, 120($sp)                   # 8-byte Folded Reload
	dsrl	$1, $1, 16
	sw	$1, -8($7)
	bnez	$16, .LBB1_7
	move	$7, $25
.LBB1_8:
	ld	$16, 208($sp)                   # 8-byte Folded Reload
	ld	$17, 216($sp)                   # 8-byte Folded Reload
	ld	$18, 224($sp)                   # 8-byte Folded Reload
	ld	$19, 232($sp)                   # 8-byte Folded Reload
	ld	$20, 240($sp)                   # 8-byte Folded Reload
	ld	$21, 248($sp)                   # 8-byte Folded Reload
	ld	$22, 256($sp)                   # 8-byte Folded Reload
	ld	$23, 264($sp)                   # 8-byte Folded Reload
	ld	$fp, 272($sp)                   # 8-byte Folded Reload
	ld	$ra, 280($sp)                   # 8-byte Folded Reload
	addiu	$sp, $sp, 288
.LBB1_9:
	jr	$ra
	nop
	.set	at
	.set	macro
	.set	reorder
	.end	transform_vector_list_fp
.Lfunc_end1:
	.size	transform_vector_list_fp, .Lfunc_end1-transform_vector_list_fp
                                        # -- End function
	.globl	test_dual_pipeline_int          # -- Begin function test_dual_pipeline_int
	.p2align	3
	.type	test_dual_pipeline_int,@function
	.set	nomicromips
	.set	nomips16
	.ent	test_dual_pipeline_int
test_dual_pipeline_int:                 # @test_dual_pipeline_int
	.frame	$sp,0,$ra
	.mask 	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	.set	noat
# %bb.0:
	blez	$5, .LBB2_4
	sll	$2, $7, 0
# %bb.1:
	sll	$3, $5, 2
	sll	$1, $4, 0
	addiu	$4, $zero, -16
	addiu	$5, $zero, 0
	addiu	$7, $zero, 0
	addiu	$3, $3, -4
	addiu	$9, $1, 8
	and	$3, $3, $4
	addiu	$4, $zero, 0
	addu	$3, $3, $1
	addiu	$8, $3, 24
	addiu	$3, $zero, 0
.LBB2_2:                                # =>This Inner Loop Header: Depth=1
	lw	$1, -8($9)
	lw	$10, -4($9)
	lw	$11, 0($9)
	lw	$12, 4($9)
	addiu	$9, $9, 16
	mult	$1, $6
	mflo	$1
	nop
	nop
	mult	$10, $6
	mflo	$10
	nop
	nop
	mult	$11, $6
	mflo	$11
	nop
	nop
	mult	$12, $6
	mflo	$12
	addu	$3, $1, $3
	addu	$4, $10, $4
	addu	$5, $11, $5
	bne	$9, $8, .LBB2_2
	addu	$7, $12, $7
# %bb.3:
	j	.LBB2_5
	nop
.LBB2_4:
	addiu	$7, $zero, 0
	addiu	$5, $zero, 0
	addiu	$4, $zero, 0
	addiu	$3, $zero, 0
.LBB2_5:
	sw	$7, 12($2)
	sw	$5, 8($2)
	sw	$4, 4($2)
	jr	$ra
	sw	$3, 0($2)
	.set	at
	.set	macro
	.set	reorder
	.end	test_dual_pipeline_int
.Lfunc_end2:
	.size	test_dual_pipeline_int, .Lfunc_end2-test_dual_pipeline_int
                                        # -- End function
	.globl	test_dual_pipeline_madd         # -- Begin function test_dual_pipeline_madd
	.p2align	3
	.type	test_dual_pipeline_madd,@function
	.set	nomicromips
	.set	nomips16
	.ent	test_dual_pipeline_madd
test_dual_pipeline_madd:                # @test_dual_pipeline_madd
	.frame	$sp,0,$ra
	.mask 	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	.set	noat
# %bb.0:
	blez	$5, .LBB3_8
	sll	$2, $7, 0
# %bb.1:
	addiu	$1, $5, -1
	sll	$7, $4, 0
	addiu	$4, $zero, 0
	addiu	$8, $zero, 0
	srl	$1, $1, 1
	addiu	$9, $1, 1
	sltiu	$1, $5, 7
	addiu	$5, $zero, 0
	bnez	$1, .LBB3_5
	andi	$3, $9, 3
# %bb.2:
	lui	$1, 8191
	addiu	$4, $zero, 0
	addiu	$5, $zero, 0
	addiu	$8, $zero, 0
	ori	$1, $1, 65532
	and	$1, $9, $1
	addiu	$9, $7, 16
	sll	$1, $1, 3
	addu	$10, $9, $1
.LBB3_3:                                # =>This Inner Loop Header: Depth=1
	lw	$1, 8($9)
	lw	$11, 0($9)
	lw	$12, -8($9)
	lw	$13, -16($9)
	lw	$14, 12($9)
	lw	$15, 4($9)
	lw	$24, -4($9)
	lw	$25, -12($9)
	addiu	$9, $9, 32
	addiu	$8, $8, 8
	mult	$1, $6
	mflo	$1
	nop
	nop
	mult	$11, $6
	mflo	$11
	nop
	nop
	mult	$12, $6
	mflo	$12
	nop
	nop
	mult	$13, $6
	mflo	$13
	nop
	nop
	mult	$14, $6
	mflo	$14
	nop
	nop
	mult	$15, $6
	mflo	$15
	nop
	nop
	mult	$24, $6
	mflo	$24
	nop
	nop
	mult	$25, $6
	mflo	$25
	addu	$4, $13, $4
	addu	$4, $12, $4
	addu	$4, $11, $4
	addu	$4, $1, $4
	addu	$5, $25, $5
	addu	$5, $24, $5
	addu	$5, $15, $5
	bne	$9, $10, .LBB3_3
	addu	$5, $14, $5
# %bb.4:
	beqz	$3, .LBB3_7
	nop
.LBB3_5:
	sll	$1, $8, 2
	addu	$1, $1, $7
	addiu	$7, $1, 4
.LBB3_6:                                # =>This Inner Loop Header: Depth=1
	lw	$1, -4($7)
	lw	$8, 0($7)
	addiu	$3, $3, -1
	addiu	$7, $7, 8
	mult	$1, $6
	mflo	$1
	nop
	nop
	mult	$8, $6
	mflo	$8
	addu	$4, $1, $4
	bnez	$3, .LBB3_6
	addu	$5, $8, $5
.LBB3_7:
	sw	$5, 4($2)
	jr	$ra
	sw	$4, 0($2)
.LBB3_8:
	addiu	$5, $zero, 0
	addiu	$4, $zero, 0
	sw	$5, 4($2)
	jr	$ra
	sw	$4, 0($2)
	.set	at
	.set	macro
	.set	reorder
	.end	test_dual_pipeline_madd
.Lfunc_end3:
	.size	test_dual_pipeline_madd, .Lfunc_end3-test_dual_pipeline_madd
                                        # -- End function
	.ident	"clang version 23.0.0git (https://github.com/rickgaiser/llvm-project 197832d0537e093c3115dcd38632f873ee548134)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.text
