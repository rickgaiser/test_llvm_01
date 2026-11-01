# Hand-written R5900 versions of the int_tests.c kernels (bench variant asm).
# n32 ABI: arguments in $4..$11, $16..$23 and $28..$31 callee-saved (all 128
# bits are saved here where MMI uses them).
#
# Each kernel says whether it has been optimized, and how. A naive kernel is
# a direct translation: one element per iteration in source order, branch
# delay slots filled with nop, no scheduling for dual issue.
#
# The R5900 issues the two instructions of an aligned 8-byte pair together
# when they use different pipes: loads and stores only use pipe 1, mult/madd
# MAC0 (pipe 0), mult1/madd1 MAC1 (pipe 1), and an MMI operation both pipes
# (it only pairs with a load or store). The multiplier and the dividers run
# asynchronously; reading HI/LO before they finish interlocks. From the EE
# Core User's Manual (2.12): mult, madd, mult1, madd1 and the MMI multiplies
# have latency 4 and throughput 2 (a new one every second cycle per MAC).
# Measured (bench uarch tests): a chain of madd or pmaddw into one accumulator
# runs at the throughput (2 cycles each), lw and lq take 2 cycles to their
# consumer, and MAC1 (mult1, madd1) is in pipe 1 with the loads and stores,
# so only MAC0 pairs with a load or store.

	.set	noreorder
	.set	noat
	.text

# int_scalar_multiply(in $4, out $5, count $6, scalar $7):
#   out[i] = in[i] * scalar
# Optimized: modulo scheduled, 8 elements in 16 cycles (the bound of the
# load/store pipe: 8 loads and 8 stores). In each cycle a load or store pairs
# with a mult on MAC0 every second cycle (its throughput; MAC1 is in the
# load/store pipe). Element i is loaded in cycle i, multiplied in cycle
# 2i + 2 (2 after its load) and stored at least 4 cycles later: elements 0-4
# in the same iteration, 5-7 (results in $24, $25, $2) and the mult of
# element 7 in the next one. A first iteration without them, and an exit
# that finishes the last group. count % 8 elements first go through a loop
# of one.
# Measured: this 128 MOPS (2.3 cycles per element); naive 33 MOPS; 8 per
# iteration with each mult right after its load, MAC0 only: 98 MOPS (each
# mult waited a cycle); with mult and mult1 alternating: 106 MOPS (mult1 does
# not pair with a load or store).
	.p2align 3
	.globl	int_scalar_multiply
	.ent	int_scalar_multiply
int_scalar_multiply:
	andi	$3, $6, 7
	beqz	$3, 2f
	subu	$6, $6, $3
1:	lw	$8, 0($4)		# count % 8 elements
	mult	$8, $8, $7
	sw	$8, 0($5)
	addiu	$4, $4, 4
	addiu	$3, $3, -1
	bnez	$3, 1b
	addiu	$5, $5, 4
2:	blez	$6, 9f
	nop
	# the first group: loads, mults 0-6, stores 0-4
	.p2align 3
	lw	$8, 0($4)
	lw	$9, 4($4)
	lw	$10, 8($4)
	mult	$8, $8, $7
	lw	$11, 12($4)
	lw	$12, 16($4)
	mult	$9, $9, $7
	lw	$13, 20($4)
	lw	$14, 24($4)
	mult	$10, $10, $7
	lw	$15, 28($4)
	nop
	mult	$11, $11, $7
	sw	$8, 0($5)
	nop
	mult	$12, $12, $7
	sw	$9, 4($5)
	nop
	mult	$24, $13, $7
	sw	$10, 8($5)
	nop
	mult	$25, $14, $7
	sw	$11, 12($5)
	nop
	sw	$12, 16($5)
	addiu	$6, $6, -8
	addiu	$4, $4, 32
	blez	$6, 4f
	addiu	$5, $5, 32
	# steady state: the mult of element 7 and the stores 5-7 of the group
	# before, the loads, mults 0-6 and stores 0-4 of this one
	.p2align 3
3:	mult	$2, $15, $7		# element 7 of the group before
	lw	$8, 0($4)
	lw	$9, 4($4)
	nop
	mult	$8, $8, $7
	lw	$10, 8($4)
	lw	$11, 12($4)
	nop
	mult	$9, $9, $7
	lw	$12, 16($4)
	lw	$13, 20($4)
	nop
	mult	$10, $10, $7
	lw	$14, 24($4)
	lw	$15, 28($4)
	nop
	mult	$11, $11, $7
	sw	$24, -12($5)		# 5-7 of the group before
	sw	$25, -8($5)
	nop
	mult	$12, $12, $7
	sw	$2, -4($5)
	sw	$8, 0($5)
	nop
	mult	$24, $13, $7
	sw	$9, 4($5)
	sw	$10, 8($5)
	addiu	$6, $6, -8
	mult	$25, $14, $7
	sw	$11, 12($5)
	sw	$12, 16($5)
	addiu	$4, $4, 32
	bgtz	$6, 3b
	addiu	$5, $5, 32
4:	mult	$2, $15, $7		# finish the last group
	sw	$24, -12($5)
	sw	$25, -8($5)
	nop
	nop
	sw	$2, -4($5)
9:	jr	$ra
	nop
	.end	int_scalar_multiply

# transform_vector_list_fp(mat $4, in $5, out $6, count $7):
#   out[v][r] = (sum over c of (s64)mat[r][c] * in[v][c]) >> 16, Q16.16
# Optimized: MMI. PMADDW multiplies words 0 and 2 of two registers and adds
# the 64-bit products to the two halves of HI:LO, so a row pair (0, 1), then
# (2, 3), takes one PMULTW and three PMADDW per vector, with the matrix in
# columns of a row pair ({m[r][c], m[r+1][c]} as sign-extended doublewords,
# $16..$23) and each lane of the vector in both doublewords (sign-extended:
# PSRAW gives the signs, PEXTLW/PEXTUW interleave them, PCPYLD/PCPYUD copy
# a doubleword to both halves). The results are (LO >> 16) | (HI << 16) per
# word, and PPACW packs the four into the output vector.
# Measured: naive 93 MOPS, this 225 MOPS. Bound by the chains of PMADDW (4
# cycles each, both row pairs share HI:LO): about 42 cycles per vector, of
# which the multiplies need 16 (throughput 2). To do: fill the latency with
# the work of the next and the previous vector (software pipelining).
	.p2align 3
	.globl	transform_vector_list_fp
	.ent	transform_vector_list_fp
transform_vector_list_fp:
	blez	$7, 3f
	nop
	addiu	$sp, $sp, -128
	sq	$16, 0($sp)
	sq	$17, 16($sp)
	sq	$18, 32($sp)
	sq	$19, 48($sp)
	sq	$20, 64($sp)
	sq	$21, 80($sp)
	sq	$22, 96($sp)
	sq	$23, 112($sp)
	# M01_c = {m[0][c], m[1][c]}, M23_c = {m[2][c], m[3][c]}
	lw	$8, 0($4)
	lw	$9, 16($4)
	pcpyld	$16, $9, $8
	lw	$8, 4($4)
	lw	$9, 20($4)
	pcpyld	$17, $9, $8
	lw	$8, 8($4)
	lw	$9, 24($4)
	pcpyld	$18, $9, $8
	lw	$8, 12($4)
	lw	$9, 28($4)
	pcpyld	$19, $9, $8
	lw	$8, 32($4)
	lw	$9, 48($4)
	pcpyld	$20, $9, $8
	lw	$8, 36($4)
	lw	$9, 52($4)
	pcpyld	$21, $9, $8
	lw	$8, 40($4)
	lw	$9, 56($4)
	pcpyld	$22, $9, $8
	lw	$8, 44($4)
	lw	$9, 60($4)
	pcpyld	$23, $9, $8
	.p2align 3
1:	lq	$8, 0($5)		# v
	psraw	$9, $8, 31		# signs
	pextlw	$10, $9, $8		# {v0, v1} sign-extended
	pextuw	$11, $9, $8		# {v2, v3}
	pcpyld	$12, $10, $10		# v0 in both halves
	pcpyud	$13, $10, $10		# v1
	pcpyld	$14, $11, $11		# v2
	pcpyud	$15, $11, $11		# v3
	pmultw	$0, $16, $12		# rows 0 and 1
	pmaddw	$0, $17, $13
	pmaddw	$0, $18, $14
	pmaddw	$0, $19, $15
	pmflo	$24
	pmfhi	$25
	psrlw	$24, $24, 16
	psllw	$25, $25, 16
	por	$10, $24, $25		# r0 in word 0, r1 in word 2
	pmultw	$0, $20, $12		# rows 2 and 3
	pmaddw	$0, $21, $13
	pmaddw	$0, $22, $14
	pmaddw	$0, $23, $15
	pmflo	$24
	pmfhi	$25
	psrlw	$24, $24, 16
	psllw	$25, $25, 16
	por	$11, $24, $25		# r2, r3
	ppacw	$8, $11, $10		# {r0, r1, r2, r3}
	sq	$8, 0($6)
	addiu	$5, $5, 16
	addiu	$7, $7, -1
	bgtz	$7, 1b
	addiu	$6, $6, 16
	lq	$16, 0($sp)
	lq	$17, 16($sp)
	lq	$18, 32($sp)
	lq	$19, 48($sp)
	lq	$20, 64($sp)
	lq	$21, 80($sp)
	lq	$22, 96($sp)
	lq	$23, 112($sp)
	addiu	$sp, $sp, 128
3:	jr	$ra
	nop
	.end	transform_vector_list_fp

# test_dual_pipeline_int(data $4, count $5, scalar $6, result $7):
#   result[k] = sum over i of data[4 * i + k] * scalar, k = 0..3 (mod 2^32)
# Optimized: MMI, two passes over the data (it stays in the data cache): the
# two halves of HI:LO hold two sums, PMADDW adds two products per
# instruction, and a chain of PMADDW runs at its throughput (2 cycles). Pass 1
# sums words 0 and 1 of each vector, pass 2 words 2 and 3 (PSRAW and
# PEXTLW/PEXTUW make them sign-extended doublewords, as PMADDW requires); the
# scalar is in both doublewords. 2 vectors per iteration, loaded a vector
# ahead (lq takes 2 cycles to its use). count is a multiple of 8.
# Measured: this 294 MOPS; naive 85 MOPS; one vector per iteration: 148
# MOPS; mult/mult1 and addu into GPR sums: 236 MOPS.
	.p2align 3
	.globl	test_dual_pipeline_int
	.ent	test_dual_pipeline_int
test_dual_pipeline_int:
	pcpyld	$6, $6, $6		# scalar in both halves
	sll	$5, $5, 2
	addu	$5, $5, $4		# end of the data
	addiu	$5, $5, -32		# the last iteration starts here
	move	$3, $4
	pmthi	$0
	pmtlo	$0
	lq	$8, 0($4)
	.p2align 3
1:	lq	$9, 16($4)		# pass 1: words 0 and 1
	psraw	$10, $8, 31
	pextlw	$10, $10, $8
	lq	$8, 32($4)		# the next iteration (past the end at the last)
	pmaddw	$0, $10, $6
	psraw	$11, $9, 31
	pextlw	$11, $11, $9
	pmaddw	$0, $11, $6
	bne	$4, $5, 1b
	addiu	$4, $4, 32
	mflo	$11
	mflo1	$12
	sw	$11, 0($7)
	sw	$12, 4($7)
	pmthi	$0
	pmtlo	$0
	lq	$8, 0($3)
	.p2align 3
2:	lq	$9, 16($3)		# pass 2: words 2 and 3
	psraw	$10, $8, 31
	pextuw	$10, $10, $8
	lq	$8, 32($3)
	pmaddw	$0, $10, $6
	psraw	$11, $9, 31
	pextuw	$11, $11, $9
	pmaddw	$0, $11, $6
	bne	$3, $5, 2b
	addiu	$3, $3, 32
	mflo	$11
	mflo1	$12
	sw	$11, 8($7)
	jr	$ra
	sw	$12, 12($7)
	.end	test_dual_pipeline_int

# test_dual_pipeline_madd(data $4, count $5, scalar $6, result $7):
#   result[k] = sum over i of data[2 * i + k] * scalar, k = 0..1 (mod 2^32)
# Optimized: MMI, one pass: PMADDW on {w0, w1} then on {w2, w3} of each
# vector adds the even elements to one half of HI:LO and the odd ones to the
# other (sign-extended as PMADDW requires; a chain of PMADDW runs at 2
# cycles). 2 vectors per iteration, loaded a vector ahead. count is a
# multiple of 8.
# Measured: this 391 MOPS (1.5 cycles per element); naive 80 MOPS; one
# vector per iteration, used right after its lq: 262 MOPS; mult/mult1 and
# addu into GPR sums: 237 MOPS.
	.p2align 3
	.globl	test_dual_pipeline_madd
	.ent	test_dual_pipeline_madd
test_dual_pipeline_madd:
	pcpyld	$6, $6, $6
	sll	$5, $5, 2
	addu	$5, $5, $4
	addiu	$5, $5, -32
	pmthi	$0
	pmtlo	$0
	lq	$8, 0($4)
	.p2align 3
1:	lq	$9, 16($4)
	psraw	$10, $8, 31
	pextlw	$11, $10, $8		# {w0, w1}
	pmaddw	$0, $11, $6
	pextuw	$11, $10, $8		# {w2, w3}
	lq	$8, 32($4)		# the next iteration (past the end at the last)
	pmaddw	$0, $11, $6
	psraw	$10, $9, 31
	pextlw	$11, $10, $9
	pmaddw	$0, $11, $6
	pextuw	$11, $10, $9
	pmaddw	$0, $11, $6
	bne	$4, $5, 1b
	addiu	$4, $4, 32
	mflo	$11
	mflo1	$12
	sw	$11, 0($7)
	jr	$ra
	sw	$12, 4($7)
	.end	test_dual_pipeline_madd

# int_divide(in $4, out $5, count $6):
#   out[i] = in[i] / (((in[i] >> 4) & 7) + 1)
# Optimized: the two dividers (div, div1) run all the time: two elements per
# iteration, the divisions of the next pair start right after the mflo and
# mflo1 of the current one, and the loads and divisors of the next pair are
# computed while they run. count is even and at least 2.
# Measured: naive 6.5 MOPS (one div at a time), this 16.2 MOPS (about 18.5
# cycles per element: two divisions per 37 cycles).
	.p2align 3
	.globl	int_divide
	.ent	int_divide
int_divide:
	lw	$8, 0($4)
	lw	$9, 4($4)
	sra	$10, $8, 4
	andi	$10, $10, 7
	addiu	$10, $10, 1
	sra	$11, $9, 4
	andi	$11, $11, 7
	addiu	$11, $11, 1
	div	$0, $8, $10
	div1	$0, $9, $11
	addiu	$6, $6, -2
	blez	$6, 2f
	addiu	$4, $4, 8
	.p2align 3
1:	lw	$8, 0($4)		# the next pair
	lw	$9, 4($4)
	sra	$10, $8, 4
	andi	$10, $10, 7
	addiu	$10, $10, 1
	sra	$11, $9, 4
	andi	$11, $11, 7
	addiu	$11, $11, 1
	mflo	$12			# waits for div
	div	$0, $8, $10
	mflo1	$13			# waits for div1
	div1	$0, $9, $11
	sw	$12, 0($5)
	sw	$13, 4($5)
	addiu	$4, $4, 8
	addiu	$6, $6, -2
	bgtz	$6, 1b
	addiu	$5, $5, 8
2:	mflo	$12
	mflo1	$13
	sw	$12, 0($5)
	jr	$ra
	sw	$13, 4($5)
	.end	int_divide
