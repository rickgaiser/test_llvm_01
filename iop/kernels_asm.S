/* The hand-written kernels. n is a multiple of 16 here, as in the bench: a library
   version needs a tail. Why each is written this way: README.md. */

	.set	noreorder
	.set	noat
	.text

#define FN(n)	.globl asm_##n; .type asm_##n,@function; .balign 16; asm_##n:

/* ---- data ---- */

FN(cpy_byte)
	or	$t0, $a0, $a1
	andi	$t0, $t0, 3
	beqz	$t0, .Lcpy_word
	nop
1:	lbu	$t0, 0($a1)
	addiu	$a1, $a1, 1
	addiu	$a2, $a2, -1
	sb	$t0, 0($a0)
	bnez	$a2, 1b
	addiu	$a0, $a0, 1
	jr	$ra
	move	$v0, $zero

/* a line at a time: four loads, one miss, then the four stores */
FN(cpy_word)
.Lcpy_word:
	addu	$t9, $a1, $a2
1:	lw	$t0, 0($a1)
	lw	$t1, 4($a1)
	lw	$t2, 8($a1)
	lw	$t3, 12($a1)
	sw	$t0, 0($a0)
	sw	$t1, 4($a0)
	sw	$t2, 8($a0)
	addiu	$a1, $a1, 16
	sw	$t3, 12($a0)
	bne	$a1, $t9, 1b
	addiu	$a0, $a0, 16
	jr	$ra
	move	$v0, $zero

FN(cpy_unal)
	addu	$t9, $a1, $a2
1:	lwl	$t0, 3($a1)
	lwr	$t0, 0($a1)
	lwl	$t1, 7($a1)
	lwr	$t1, 4($a1)
	sw	$t0, 0($a0)
	lwl	$t2, 11($a1)
	lwr	$t2, 8($a1)
	sw	$t1, 4($a0)
	lwl	$t3, 15($a1)
	lwr	$t3, 12($a1)
	sw	$t2, 8($a0)
	addiu	$a1, $a1, 16
	sw	$t3, 12($a0)
	bne	$a1, $t9, 1b
	addiu	$a0, $a0, 16
	jr	$ra
	move	$v0, $zero

/* two lines per pass: the stores set the pace, at 2 cycles each */
FN(set_word)
	addu	$t9, $a0, $a2
1:	sw	$a3, 0($a0)
	sw	$a3, 4($a0)
	sw	$a3, 8($a0)
	sw	$a3, 12($a0)
	sw	$a3, 16($a0)
	sw	$a3, 20($a0)
	sw	$a3, 24($a0)
	addiu	$a0, $a0, 32
	bne	$a0, $t9, 1b
	sw	$a3, -4($a0)
	jr	$ra
	move	$v0, $zero

/* a word at a time: (w - 0x01010101) & ~w & 0x80808080 finds a zero byte */
FN(str_len)
	move	$v0, $a1
	li	$t8, 0x01010101
	li	$t9, 0x80808080
	lw	$t0, 0($a1)
1:	addiu	$a1, $a1, 4
	subu	$t1, $t0, $t8
	nor	$t2, $t0, $zero
	and	$t1, $t1, $t2
	and	$t1, $t1, $t9
	beqz	$t1, 1b
	lw	$t0, 0($a1)
	addiu	$a1, $a1, -4
	lbu	$t0, 0($a1)
2:	addiu	$a1, $a1, 1
	bnez	$t0, 2b
	lbu	$t0, 0($a1)
	addiu	$a1, $a1, -1
	jr	$ra
	subu	$v0, $a1, $v0

/* ---- network ---- */

/* words with the carries counted: 32 bits for 4 instructions */
FN(net_sum)
	addu	$t9, $a1, $a2
	move	$v0, $zero
	move	$v1, $zero
	lw	$t0, 0($a1)
1:	lw	$t1, 4($a1)
	addu	$v0, $v0, $t0
	sltu	$t2, $v0, $t0
	addu	$v1, $v1, $t2
	lw	$t0, 8($a1)
	addu	$v0, $v0, $t1
	sltu	$t2, $v0, $t1
	addu	$v1, $v1, $t2
	lw	$t1, 12($a1)
	addu	$v0, $v0, $t0
	sltu	$t2, $v0, $t0
	addu	$v1, $v1, $t2
	addiu	$a1, $a1, 16
	addu	$v0, $v0, $t1
	lw	$t0, 0($a1)
	sltu	$t2, $v0, $t1
	bne	$a1, $t9, 1b
	addu	$v1, $v1, $t2
	/* v0 + v1 * 2^32, folded: 2^32 is 1 modulo 0xffff */
	srl	$t0, $v0, 16
	andi	$v0, $v0, 0xffff
	addu	$v0, $v0, $t0
	addu	$v0, $v0, $v1
	srl	$t0, $v0, 16
	andi	$v0, $v0, 0xffff
	addu	$v0, $v0, $t0
	srl	$t0, $v0, 16
	andi	$v0, $v0, 0xffff
	jr	$ra
	addu	$v0, $v0, $t0

/* two halfwords per word */
FN(net_swap)
	addu	$t9, $a1, $a2
	lui	$t8, 0x00ff
	ori	$t8, $t8, 0x00ff
1:	lw	$t0, 0($a1)
	lw	$t3, 4($a1)
	and	$t1, $t0, $t8
	srl	$t0, $t0, 8
	and	$t0, $t0, $t8
	sll	$t1, $t1, 8
	or	$t0, $t0, $t1
	sw	$t0, 0($a0)
	and	$t1, $t3, $t8
	srl	$t3, $t3, 8
	and	$t3, $t3, $t8
	sll	$t1, $t1, 8
	or	$t3, $t3, $t1
	addiu	$a1, $a1, 8
	sw	$t3, 4($a0)
	bne	$a1, $t9, 1b
	addiu	$a0, $a0, 8
	jr	$ra
	move	$v0, $zero

/* An uncached load waits for the write buffer: eight loads, then the eight stores */
FN(fifo_rd)
	addu	$t9, $a0, $a2
1:	lw	$t0, 0($a3)
	lw	$t1, 0($a3)
	lw	$t2, 0($a3)
	lw	$t3, 0($a3)
	lw	$t4, 0($a3)
	lw	$t5, 0($a3)
	lw	$t6, 0($a3)
	lw	$t7, 0($a3)
	sw	$t0, 0($a0)
	sw	$t1, 4($a0)
	sw	$t2, 8($a0)
	sw	$t3, 12($a0)
	sw	$t4, 16($a0)
	sw	$t5, 20($a0)
	sw	$t6, 24($a0)
	addiu	$a0, $a0, 32
	bne	$a0, $t9, 1b
	sw	$t7, -4($a0)
	jr	$ra
	move	$v0, $zero

/* ---- storage ---- */

/* xor is bytewise: a word at a time, folded at the end */
FN(mc_edc)
	addu	$t9, $a1, $a2
	move	$v0, $zero
	lw	$t0, 0($a1)
1:	lw	$t1, 4($a1)
	xor	$v0, $v0, $t0
	lw	$t0, 8($a1)
	xor	$v0, $v0, $t1
	lw	$t1, 12($a1)
	xor	$v0, $v0, $t0
	addiu	$a1, $a1, 16
	lw	$t0, 0($a1)
	bne	$a1, $t9, 1b
	xor	$v0, $v0, $t1
	srl	$t0, $v0, 16
	xor	$v0, $v0, $t0
	srl	$t0, $v0, 8
	xor	$v0, $v0, $t0
	jr	$ra
	andi	$v0, $v0, 0xff

/* a3 ends as t0 or ~t0: bit 7 of a2 counts how often the bit was set */
FN(mc_ecc)
	addu	$t9, $a1, $a2
	la	$t8, mc_xortable
	li	$t6, 0x80
2:	move	$t2, $zero			/* i */
	move	$t3, $zero			/* a2 */
	move	$t4, $zero			/* t0 */
	lbu	$t0, 0($a1)
	nop
1:	addu	$t0, $t8, $t0
	lbu	$t1, 0($t0)
	lbu	$t0, 1($a1)
	xor	$t3, $t3, $t1
	andi	$t1, $t1, 0x80
	beqz	$t1, 3f
	addiu	$t2, $t2, 1
	addiu	$t5, $t2, -1
	xor	$t4, $t4, $t5
3:	bne	$t2, $t6, 1b
	addiu	$a1, $a1, 1
	andi	$t5, $t3, 0x80
	beqz	$t5, 4f
	move	$t5, $t4
	nor	$t5, $t4, $zero
4:	nor	$t3, $t3, $zero
	andi	$t3, $t3, 0x77
	sb	$t3, 0($a0)
	nor	$t5, $t5, $zero
	andi	$t5, $t5, 0x7f
	sb	$t5, 1($a0)
	nor	$t4, $t4, $zero
	andi	$t4, $t4, 0x7f
	sb	$t4, 2($a0)
	bne	$a1, $t9, 2b
	addiu	$a0, $a0, 3
	jr	$ra
	move	$v0, $zero

/* shift first: three masks, not six. Four words per pass */
#define BIT_REV(r)				\
	srl	$t4, r, 4;			\
	and	$t4, $t4, $t5;			\
	and	r, r, $t5;			\
	sll	r, r, 4;			\
	or	r, r, $t4;			\
	srl	$t4, r, 2;			\
	and	$t4, $t4, $t6;			\
	and	r, r, $t6;			\
	sll	r, r, 2;			\
	or	r, r, $t4;			\
	srl	$t4, r, 1;			\
	and	$t4, $t4, $t7;			\
	and	r, r, $t7;			\
	sll	r, r, 1;			\
	or	r, r, $t4
FN(bit_rev)
	addu	$t9, $a0, $a2
	li	$t5, 0x0f0f0f0f
	li	$t6, 0x33333333
	li	$t7, 0x55555555
1:	lw	$t0, 0($a0)
	lw	$t1, 4($a0)
	lw	$t2, 8($a0)
	lw	$t3, 12($a0)
	BIT_REV($t0)
	sw	$t0, 0($a0)
	BIT_REV($t1)
	sw	$t1, 4($a0)
	BIT_REV($t2)
	sw	$t2, 8($a0)
	BIT_REV($t3)
	addiu	$a0, $a0, 16
	bne	$a0, $t9, 1b
	sw	$t3, -4($a0)
	jr	$ra
	move	$v0, $zero

/* four bytes from one load, the shift of the index folded into the mask */
FN(crc32)
	addu	$t9, $a1, $a2
	la	$t8, crc_table
	li	$v0, -1
1:	lw	$t0, 0($a1)
	addiu	$a1, $a1, 4
	xor	$v0, $v0, $t0			/* all four bytes at once */
	.rept 4
	andi	$t1, $v0, 0xff
	sll	$t1, $t1, 2
	addu	$t1, $t8, $t1
	lw	$t1, 0($t1)
	srl	$v0, $v0, 8
	xor	$v0, $v0, $t1
	.endr
	bne	$a1, $t9, 1b
	nop
	jr	$ra
	nor	$v0, $v0, $zero

/* two words per branch, the test of each in the shadow of the next load */
FN(fat_scan)
	move	$v0, $a1
	lw	$t0, 0($a1)
	lw	$t1, 4($a1)
1:	beqz	$t0, 2f
	lw	$t2, 8($a1)
	beqz	$t1, 3f
	lw	$t3, 12($a1)
	beqz	$t2, 4f
	lw	$t0, 16($a1)
	beqz	$t3, 5f
	lw	$t1, 20($a1)
	b	1b
	addiu	$a1, $a1, 16
5:	addiu	$a1, $a1, 4
4:	addiu	$a1, $a1, 4
3:	addiu	$a1, $a1, 4
2:	subu	$v0, $a1, $v0
	jr	$ra
	srl	$v0, $v0, 2

/* ---- sound ---- */

/* one load per stereo pair, one word store per channel: stores are the limit */
FN(up_24000)
	addu	$t9, $a1, $a2
	addu	$t8, $a0, $a2			/* right */
	lui	$t7, 0xffff
	lw	$t0, 0($a1)
1:	addiu	$a1, $a1, 4
	andi	$t1, $t0, 0xffff
	sll	$t2, $t0, 16
	or	$t1, $t1, $t2
	sw	$t1, 0($a0)
	srl	$t1, $t0, 16
	and	$t2, $t0, $t7
	or	$t1, $t1, $t2
	lw	$t0, 0($a1)
	addiu	$a0, $a0, 4
	sw	$t1, 0($t8)
	bne	$a1, $t9, 1b
	addiu	$t8, $t8, 4
	jr	$ra
	move	$v0, $zero

/* Four outputs per pass, halfword stores. Packing two samples into a word store
   costs more in shifts than it saves: the source stays in the D-cache, and two
   chains in turn leave no load delay empty */
#define UP_GEN2(o)				\
	lh	$t0, o($t8);			\
	lh	$t1, o + 2($t8);		\
	sll	$t0, $t0, 2;			\
	sll	$t1, $t1, 2;			\
	addu	$t0, $a1, $t0;			\
	addu	$t1, $a1, $t1;			\
	lhu	$t2, 0($t0);			\
	lhu	$t3, 2($t0);			\
	lhu	$t4, 0($t1);			\
	sh	$t2, o($a0);			\
	lhu	$t5, 2($t1);			\
	sh	$t3, o + 1024($a0);		\
	sh	$t4, o + 2($a0);		\
	sh	$t5, o + 1026($a0)
FN(up_gen)
	la	$t8, up_lut
	addiu	$t9, $t8, 1024
1:	UP_GEN2(0)
	UP_GEN2(4)
	addiu	$t8, $t8, 8
	bne	$t8, $t9, 1b
	addiu	$a0, $a0, 8
	jr	$ra
	move	$v0, $zero

/* the next multiply runs while this sample is finished: mflo does not wait */
FN(mix_vol)
	addu	$t9, $a1, $a2
	li	$t8, 0x8000
	lh	$t0, 0($a1)
	nop
	mult	$t0, $a3
1:	lh	$t1, 0($a0)
	addiu	$a1, $a1, 2
	lh	$t0, 0($a1)
	mflo	$t2
	sra	$t2, $t2, 15
	addu	$t2, $t1, $t2
	mult	$t0, $a3
	addu	$t3, $t2, $t8
	srl	$t3, $t3, 16
	beqz	$t3, 2f
	slt	$t3, $t2, $zero
	li	$t2, 0x7fff
	addu	$t2, $t2, $t3
2:	sh	$t2, 0($a0)
	bne	$a1, $t9, 1b
	addiu	$a0, $a0, 2
	jr	$ra
	move	$v0, $zero

/* the sample, 16 bits, in rs: 11 cycles, not the 15 of a large gain */
FN(gain_q16)
	addu	$t9, $a1, $a2
	lh	$t0, 0($a1)
	addiu	$a1, $a1, 2
	mult	$t0, $a3
1:	lh	$t0, 0($a1)
	addiu	$a0, $a0, 4
	mflo	$t2
	sra	$t2, $t2, 16
	sw	$t2, -4($a0)
	mult	$t0, $a3
	bne	$a1, $t9, 1b
	addiu	$a1, $a1, 2
	jr	$ra
	move	$v0, $zero

/* two straight copies by words: no test of the position per byte */
#define RING_COPY(end)				\
	lw	$t5, 0($a1);			\
	lw	$t6, 4($a1);			\
	sw	$t5, 0($a0);			\
	lw	$t5, 8($a1);			\
	sw	$t6, 4($a0);			\
	lw	$t6, 12($a1);			\
	sw	$t5, 8($a0);			\
	addiu	$a1, $a1, 16;			\
	sw	$t6, 12($a0);			\
	bne	$a1, end, 2b;			\
	addiu	$a0, $a0, 16
FN(ring_put)
	addu	$t9, $a1, $a2
	li	$t0, 2048
	subu	$t1, $t0, $a3			/* room to the end */
	sltu	$t2, $a2, $t1
	beqz	$t2, 1f
	move	$t3, $a0			/* the ring */
	move	$t1, $a2
1:	addu	$t2, $a1, $t1			/* end of the first part */
	addu	$a0, $a0, $a3
2:	RING_COPY($t2)
	beq	$a1, $t9, 3f
	move	$a0, $t3
2:	RING_COPY($t9)
3:	addu	$v0, $a3, $a2
	jr	$ra
	andi	$v0, $v0, 2047

/* ---- system ---- */

/* the key of the next node in the delay slot: of node 0 it is read and dropped */
FN(list_walk)
	move	$v0, $zero
	lw	$t0, 4($a1)
1:	lw	$a1, 0($a1)
	addu	$v0, $v0, $t0
	bnez	$a1, 1b
	lw	$t0, 4($a1)
	jr	$ra
	nop

/* the value in rs: a mult gets faster as it shrinks */
FN(itoa10)
	move	$v0, $a0
	addu	$t9, $a1, $a2
	li	$t8, 0xcccccccd
1:	lw	$t0, 0($a1)
	addiu	$a1, $a1, 4
2:	multu	$t0, $t8
	addiu	$a0, $a0, 1
	mfhi	$t1
	srl	$t1, $t1, 3			/* v / 10 */
	sll	$t2, $t1, 3
	sll	$t3, $t1, 1
	addu	$t2, $t2, $t3
	subu	$t2, $t0, $t2			/* v % 10 */
	addiu	$t2, $t2, '0'
	sb	$t2, -1($a0)
	bnez	$t1, 2b
	move	$t0, $t1
	bne	$a1, $t9, 1b
	nop
	jr	$ra
	subu	$v0, $a0, $v0

/* x * 1000 = ((x << 7) - x - (x << 1)) << 3. Four words per pass */
#define MUL1000(r)				\
	sll	$t4, r, 7;			\
	subu	$t4, $t4, r;			\
	sll	r, r, 1;			\
	subu	r, $t4, r;			\
	sll	r, r, 3
FN(mul_const)
	addu	$t9, $a1, $a2
1:	lw	$t0, 0($a1)
	lw	$t1, 4($a1)
	lw	$t2, 8($a1)
	lw	$t3, 12($a1)
	MUL1000($t0)
	sw	$t0, 0($a0)
	MUL1000($t1)
	sw	$t1, 4($a0)
	MUL1000($t2)
	sw	$t2, 8($a0)
	MUL1000($t3)
	addiu	$a1, $a1, 16
	sw	$t3, 12($a0)
	bne	$a1, $t9, 1b
	addiu	$a0, $a0, 16
	jr	$ra
	move	$v0, $zero

/* the loads of the next pair run under the last multiply of this one */
FN(mul64)
	addu	$t9, $a1, $a2
	lw	$t0, 0($a1)			/* a lo */
	lw	$t2, 8($a1)			/* b lo */
	lw	$t1, 4($a1)			/* a hi */
	lw	$t3, 12($a1)			/* b hi */
1:	multu	$t1, $t2
	addiu	$a1, $a1, 16
	addiu	$a0, $a0, 8
	mflo	$t4
	nop
	nop
	multu	$t0, $t3
	nop
	nop
	mflo	$t5
	addu	$t4, $t4, $t5
	nop
	multu	$t0, $t2
	lw	$t0, 0($a1)
	lw	$t2, 8($a1)
	lw	$t1, 4($a1)
	lw	$t3, 12($a1)
	mflo	$t5
	mfhi	$t6
	addu	$t4, $t4, $t6
	sw	$t5, -8($a0)
	bne	$a1, $t9, 1b
	sw	$t4, -4($a0)
	jr	$ra
	move	$v0, $zero
