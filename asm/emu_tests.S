# Hand-written R5900 versions of the emu_tests.c kernels (bench variant asm).
# n32 ABI. The kernels use the data of emu_tests.c (KERNEL_DATA). See
# int_tests.S for what "naive" means. The interpreters dispatch through jump
# tables in .rodata.

	.set	noreorder
	.set	noat

#define ADDR(reg, sym) lui $1, %hi(sym); addiu reg, $1, %lo(sym)

	.section .rodata
	.p2align 2
cpu8_table:
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_asl_a
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_clc
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_and_imm
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_sec
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_eor_imm
	.word	c8_lsr_a
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_adc_zp
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_adc_imm
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_sta_zp
	.word	c8_brk
	.word	c8_brk
	.word	c8_dey
	.word	c8_brk
	.word	c8_txa
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_bcc
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_sta_zpx
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_ldy_imm
	.word	c8_brk
	.word	c8_ldx_imm
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_lda_imm
	.word	c8_tax
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_bcs
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_lda_zpx
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_iny
	.word	c8_cmp_imm
	.word	c8_dex
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_bne
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_cpx_imm
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_inx
	.word	c8_sbc_imm
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
	.word	c8_brk
r3k_table:
	.word	r3k_special
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_beq
	.word	r3k_bne
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_addiu
	.word	r3k_slti
	.word	r3k_next
	.word	r3k_andi
	.word	r3k_ori
	.word	r3k_next
	.word	r3k_lui
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_lw
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_sw
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
r3k_special_table:
	.word	r3k_f_sll
	.word	r3k_next
	.word	r3k_f_srl
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_f_syscall
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_f_addu
	.word	r3k_next
	.word	r3k_f_subu
	.word	r3k_f_and
	.word	r3k_f_or
	.word	r3k_f_xor
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_f_slt
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.word	r3k_next
	.text

# cpu8: a 6502 subset interpreter on mem8, from 0x200 to BRK; returns
#   app_hash(mem8, 256) ^ a ^ x << 8 ^ y << 16 ^ c << 24 ^ v << 25
# a $16, x $17, y $18, c $19, v $20, nz $21, pc $22, mem8 $23; op $9, m $10.
# Optimized: no (naive: a jump table per instruction, the operand byte read
# for every instruction as the C does).
	.p2align 3
	.globl	cpu8
	.ent	cpu8
cpu8:
	addiu	$sp, $sp, -80
	sd	$ra, 0($sp)
	sd	$16, 8($sp)
	sd	$17, 16($sp)
	sd	$18, 24($sp)
	sd	$19, 32($sp)
	sd	$20, 40($sp)
	sd	$21, 48($sp)
	sd	$22, 56($sp)
	sd	$23, 64($sp)
	move	$16, $0
	move	$17, $0
	move	$18, $0
	move	$19, $0
	move	$20, $0
	addiu	$21, $0, 1
	addiu	$22, $0, 0x200
	ADDR($23, mem8)
c8_loop:
	addu	$8, $23, $22
	lbu	$9, 0($8)		# op
	addiu	$10, $22, 1
	andi	$10, $10, 0xfff
	addu	$10, $10, $23
	lbu	$10, 0($10)		# m
	addiu	$22, $22, 1
	sll	$8, $9, 2
	ADDR($11, cpu8_table)
	addu	$8, $8, $11
	lw	$8, 0($8)
	jr	$8
	nop
c8_next:
	b	c8_loop
	andi	$22, $22, 0xfff
c8_lda_imm:
	move	$16, $10
	move	$21, $10
	b	c8_next
	addiu	$22, $22, 1
c8_lda_zpx:
	addu	$8, $10, $17
	andi	$8, $8, 0xff
	addu	$8, $8, $23
	lbu	$16, 0($8)
	move	$21, $16
	b	c8_next
	addiu	$22, $22, 1
c8_sta_zp:
	addu	$8, $23, $10
	sb	$16, 0($8)
	b	c8_next
	addiu	$22, $22, 1
c8_sta_zpx:
	addu	$8, $10, $17
	andi	$8, $8, 0xff
	addu	$8, $8, $23
	sb	$16, 0($8)
	b	c8_next
	addiu	$22, $22, 1
c8_ldx_imm:
	move	$17, $10
	move	$21, $10
	b	c8_next
	addiu	$22, $22, 1
c8_ldy_imm:
	move	$18, $10
	move	$21, $10
	b	c8_next
	addiu	$22, $22, 1
c8_adc_zp:
	addu	$8, $23, $10
	b	c8_adc_imm
	lbu	$10, 0($8)
c8_sbc_imm:
	xori	$10, $10, 0xff
c8_adc_imm:
	addu	$8, $16, $10
	addu	$8, $8, $19		# t = a + m + c
	xor	$9, $16, $10
	nor	$9, $9, $0		# ~(a ^ m)
	xor	$11, $16, $8		# a ^ t
	and	$9, $9, $11
	andi	$20, $9, 0x80		# v
	srl	$19, $8, 8		# c
	andi	$16, $8, 0xff
	move	$21, $16
	b	c8_next
	addiu	$22, $22, 1
c8_and_imm:
	and	$16, $16, $10
	move	$21, $16
	b	c8_next
	addiu	$22, $22, 1
c8_eor_imm:
	xor	$16, $16, $10
	move	$21, $16
	b	c8_next
	addiu	$22, $22, 1
c8_asl_a:
	srl	$19, $16, 7
	sll	$16, $16, 1
	andi	$16, $16, 0xff
	b	c8_next
	move	$21, $16
c8_lsr_a:
	andi	$19, $16, 1
	srl	$16, $16, 1
	b	c8_next
	move	$21, $16
c8_cmp_imm:
	subu	$8, $16, $10
	sltu	$19, $16, $10
	xori	$19, $19, 1		# c = a >= m
	andi	$21, $8, 0xff
	b	c8_next
	addiu	$22, $22, 1
c8_cpx_imm:
	subu	$8, $17, $10
	sltu	$19, $17, $10
	xori	$19, $19, 1
	andi	$21, $8, 0xff
	b	c8_next
	addiu	$22, $22, 1
c8_inx:
	addiu	$17, $17, 1
	andi	$17, $17, 0xff
	b	c8_next
	move	$21, $17
c8_dex:
	addiu	$17, $17, -1
	andi	$17, $17, 0xff
	b	c8_next
	move	$21, $17
c8_iny:
	addiu	$18, $18, 1
	andi	$18, $18, 0xff
	b	c8_next
	move	$21, $18
c8_dey:
	addiu	$18, $18, -1
	andi	$18, $18, 0xff
	b	c8_next
	move	$21, $18
c8_tax:
	move	$17, $16
	b	c8_next
	move	$21, $16
c8_txa:
	move	$16, $17
	b	c8_next
	move	$21, $17
c8_clc:
	b	c8_next
	move	$19, $0
c8_sec:
	b	c8_next
	addiu	$19, $0, 1
c8_bne:
	beqz	$21, c8_next
	addiu	$22, $22, 1
	b	c8_branch
	nop
c8_bcc:
	bnez	$19, c8_next
	addiu	$22, $22, 1
	b	c8_branch
	nop
c8_bcs:
	beqz	$19, c8_next
	addiu	$22, $22, 1
c8_branch:
	sll	$8, $10, 24
	sra	$8, $8, 24		# (s8)m
	b	c8_next
	addu	$22, $22, $8
c8_brk:
	move	$4, $23
	jal	app_hash
	ori	$5, $0, 256
	xor	$2, $2, $16
	sll	$8, $17, 8
	xor	$2, $2, $8
	sll	$8, $18, 16
	xor	$2, $2, $8
	sll	$8, $19, 24
	xor	$2, $2, $8
	sll	$8, $20, 25
	xor	$2, $2, $8
	ld	$ra, 0($sp)
	ld	$16, 8($sp)
	ld	$17, 16($sp)
	ld	$18, 24($sp)
	ld	$19, 32($sp)
	ld	$20, 40($sp)
	ld	$21, 48($sp)
	ld	$22, 56($sp)
	ld	$23, 64($sp)
	jr	$ra
	addiu	$sp, $sp, 80
	.end	cpu8

# r3000: a MIPS R3000 subset interpreter on ram, with branch delay slots,
#   from 0 to SYSCALL; returns app_hash(&ram[0x600], 2048) ^ r[2]
# The guest registers r[32] are on the stack at 48($sp) ($19 points to
# them), ram $16, pc $17, npc $18; in $9, &r[rs] $10, &r[rt] $11, &r[rd] $12,
# the sign-extended immediate $13.
# Optimized: no (naive: jump tables, the guest registers in memory).
	.p2align 3
	.globl	r3000
	.ent	r3000
r3000:
	addiu	$sp, $sp, -176
	sd	$ra, 0($sp)
	sd	$16, 8($sp)
	sd	$17, 16($sp)
	sd	$18, 24($sp)
	sd	$19, 32($sp)
	addiu	$19, $sp, 48		# r[0..31] at 48..175
	move	$8, $19
	addiu	$9, $19, 128
1:	sw	$0, 0($8)
	addiu	$8, $8, 4
	bne	$8, $9, 1b
	nop
	ADDR($16, ram)
	move	$17, $0			# pc
	addiu	$18, $0, 4		# npc
r3k_loop:
	srl	$8, $17, 2
	andi	$8, $8, 4095
	sll	$8, $8, 2
	addu	$8, $8, $16
	lw	$9, 0($8)		# in
	srl	$10, $9, 21
	andi	$10, $10, 31
	sll	$10, $10, 2
	addu	$10, $10, $19		# &r[rs]
	srl	$11, $9, 16
	andi	$11, $11, 31
	sll	$11, $11, 2
	addu	$11, $11, $19		# &r[rt]
	srl	$12, $9, 11
	andi	$12, $12, 31
	sll	$12, $12, 2
	addu	$12, $12, $19		# &r[rd]
	sll	$13, $9, 16
	sra	$13, $13, 16		# imm
	move	$17, $18		# pc = npc
	addiu	$18, $18, 4		# npc += 4
	srl	$8, $9, 26
	sll	$8, $8, 2
	ADDR($14, r3k_table)
	addu	$8, $8, $14
	lw	$8, 0($8)
	jr	$8
	nop
r3k_next:
	b	r3k_loop
	sw	$0, 0($19)		# r[0] = 0
r3k_special:
	andi	$8, $9, 63
	sll	$8, $8, 2
	ADDR($14, r3k_special_table)
	addu	$8, $8, $14
	lw	$8, 0($8)
	jr	$8
	nop
r3k_f_sll:
	lw	$14, 0($11)
	srl	$15, $9, 6
	andi	$15, $15, 31
	sllv	$14, $14, $15
	b	r3k_next
	sw	$14, 0($12)
r3k_f_srl:
	lw	$14, 0($11)
	srl	$15, $9, 6
	andi	$15, $15, 31
	srlv	$14, $14, $15
	b	r3k_next
	sw	$14, 0($12)
r3k_f_addu:
	lw	$14, 0($10)
	lw	$15, 0($11)
	addu	$14, $14, $15
	b	r3k_next
	sw	$14, 0($12)
r3k_f_subu:
	lw	$14, 0($10)
	lw	$15, 0($11)
	subu	$14, $14, $15
	b	r3k_next
	sw	$14, 0($12)
r3k_f_and:
	lw	$14, 0($10)
	lw	$15, 0($11)
	and	$14, $14, $15
	b	r3k_next
	sw	$14, 0($12)
r3k_f_or:
	lw	$14, 0($10)
	lw	$15, 0($11)
	or	$14, $14, $15
	b	r3k_next
	sw	$14, 0($12)
r3k_f_xor:
	lw	$14, 0($10)
	lw	$15, 0($11)
	xor	$14, $14, $15
	b	r3k_next
	sw	$14, 0($12)
r3k_f_slt:
	lw	$14, 0($10)
	lw	$15, 0($11)
	slt	$14, $14, $15
	b	r3k_next
	sw	$14, 0($12)
r3k_beq:
	lw	$14, 0($10)
	lw	$15, 0($11)
	bne	$14, $15, r3k_next
	sll	$14, $13, 2
	b	r3k_next
	addu	$18, $17, $14		# npc = pc + (imm << 2)
r3k_bne:
	lw	$14, 0($10)
	lw	$15, 0($11)
	beq	$14, $15, r3k_next
	sll	$14, $13, 2
	b	r3k_next
	addu	$18, $17, $14
r3k_addiu:
	lw	$14, 0($10)
	addu	$14, $14, $13
	b	r3k_next
	sw	$14, 0($11)
r3k_slti:
	lw	$14, 0($10)
	slt	$14, $14, $13
	b	r3k_next
	sw	$14, 0($11)
r3k_andi:
	lw	$14, 0($10)
	andi	$15, $9, 0xffff
	and	$14, $14, $15
	b	r3k_next
	sw	$14, 0($11)
r3k_ori:
	lw	$14, 0($10)
	andi	$15, $9, 0xffff
	or	$14, $14, $15
	b	r3k_next
	sw	$14, 0($11)
r3k_lui:
	sll	$14, $9, 16
	b	r3k_next
	sw	$14, 0($11)
r3k_lw:
	lw	$14, 0($10)
	addu	$14, $14, $13
	srl	$14, $14, 2
	andi	$14, $14, 4095
	sll	$14, $14, 2
	addu	$14, $14, $16
	lw	$14, 0($14)
	b	r3k_next
	sw	$14, 0($11)
r3k_sw:
	lw	$14, 0($10)
	addu	$14, $14, $13
	srl	$14, $14, 2
	andi	$14, $14, 4095
	sll	$14, $14, 2
	addu	$14, $14, $16
	lw	$15, 0($11)
	b	r3k_next
	sw	$15, 0($14)
r3k_f_syscall:
	addiu	$4, $16, 0x1800
	jal	app_hash
	ori	$5, $0, 2048
	lw	$8, 8($19)		# r[2]
	xor	$2, $2, $8
	ld	$ra, 0($sp)
	ld	$16, 8($sp)
	ld	$17, 16($sp)
	ld	$18, 24($sp)
	ld	$19, 32($sp)
	jr	$ra
	addiu	$sp, $sp, 176
	.end	r3000


# tile_decode: chunky = the 128 SNES 4bpp planar tiles of tiles, 8 bits per
#   pixel: bit b of the 4 planes of a row, b = 7 - x
# Optimized: no (naive: one pixel per iteration).
	.p2align 3
	.globl	tile_decode
	.ent	tile_decode
tile_decode:
	ADDR($8, tiles)
	ADDR($9, chunky)
	addiu	$10, $8, 4096		# end of the tiles
1:	move	$11, $0			# y
2:	sll	$12, $11, 1
	addu	$12, $12, $8
	lbu	$13, 0($12)		# p0
	lbu	$14, 1($12)		# p1
	lbu	$15, 16($12)		# p2
	lbu	$24, 17($12)		# p3
	addiu	$25, $0, 7		# b
3:	srlv	$2, $13, $25
	andi	$2, $2, 1
	srlv	$3, $14, $25
	andi	$3, $3, 1
	sll	$3, $3, 1
	or	$2, $2, $3
	srlv	$3, $15, $25
	andi	$3, $3, 1
	sll	$3, $3, 2
	or	$2, $2, $3
	srlv	$3, $24, $25
	andi	$3, $3, 1
	sll	$3, $3, 3
	or	$2, $2, $3
	sb	$2, 0($9)
	addiu	$9, $9, 1
	bgtz	$25, 3b
	addiu	$25, $25, -1
	addiu	$11, $11, 1
	addiu	$1, $0, 8
	bne	$11, $1, 2b
	nop
	addiu	$8, $8, 32
	bne	$8, $10, 1b
	nop
	ADDR($4, chunky)
	j	app_hash
	ori	$5, $0, 8192
	.end	tile_decode

# n64_to_ct16: ct16[i] = PSMCT16 of the big-endian N64 RGBA5551 n64_tex[i]
# Optimized: no (naive: one pixel per iteration in GPRs).
	.p2align 3
	.globl	n64_to_ct16
	.ent	n64_to_ct16
n64_to_ct16:
	ADDR($8, n64_tex)
	ADDR($9, ct16)
	addiu	$10, $8, 8192
1:	lhu	$11, 0($8)
	andi	$12, $11, 0xff
	sll	$12, $12, 8
	srl	$11, $11, 8
	or	$11, $11, $12		# p, byte-swapped
	srl	$12, $11, 11
	andi	$12, $12, 0x1f
	srl	$13, $11, 6
	andi	$13, $13, 0x1f
	sll	$13, $13, 5
	or	$12, $12, $13
	srl	$13, $11, 1
	andi	$13, $13, 0x1f
	sll	$13, $13, 10
	or	$12, $12, $13
	andi	$13, $11, 1
	sll	$13, $13, 15
	or	$12, $12, $13
	sh	$12, 0($9)
	addiu	$8, $8, 2
	bne	$8, $10, 1b
	addiu	$9, $9, 2
	ADDR($4, ct16)
	j	app_hash
	ori	$5, $0, 8192
	.end	n64_to_ct16

# guest_loads: returns the sum (rotated left by 1 before each add) of the
#   byte-swapped words at the 4096 guest_addr through page_table (4 KB pages)
# Optimized: no (naive).
	.p2align 3
	.globl	guest_loads
	.ent	guest_loads
guest_loads:
	ADDR($8, guest_addr)
	ADDR($9, page_table)
	addiu	$10, $8, 16384
	move	$2, $0			# sum
1:	lw	$11, 0($8)		# addr
	srl	$12, $11, 12
	sll	$12, $12, 2
	addu	$12, $12, $9
	lw	$12, 0($12)		# page
	andi	$13, $11, 4092
	addu	$12, $12, $13
	lw	$11, 0($12)		# w
	srl	$12, $11, 24
	srl	$13, $11, 8
	andi	$13, $13, 0xff00
	or	$12, $12, $13
	sll	$13, $11, 8
	lui	$14, 0xff
	and	$13, $13, $14
	or	$12, $12, $13
	sll	$13, $11, 24
	or	$12, $12, $13		# byte-swapped
	sll	$13, $2, 1
	srl	$14, $2, 31
	or	$2, $13, $14
	addu	$2, $2, $12
	addiu	$8, $8, 4
	bne	$8, $10, 1b
	nop
	jr	$ra
	nop
	.end	guest_loads
