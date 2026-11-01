# Hand-written R5900 versions of the sound_tests.c kernels (bench variant
# asm). n32 ABI. The kernels use the data of sound_tests.c (KERNEL_DATA) and
# return app_hash() of their output. See int_tests.S for what "naive" means.

	.set	noreorder
	.set	noat

# reg = the address of sym
#define ADDR(reg, sym) lui $1, %hi(sym); addiu reg, $1, %lo(sym)
# r = sat16(r): clamp to -32768..32767, using t and u
#define SAT16(r, t, u) \
	slti t, r, 32767; addiu u, $0, 32767; movz r, u, t; \
	slti t, r, -32768; addiu u, $0, -32768; movn r, u, t

	.section .rodata
	.p2align 3
adpcm_coef:	.word	0, 0, 60, 0, 115, -52, 98, -55, 122, -60
	.text

# mix: mix_out = 8 voices of voice_pcm with their Q15 left and right volume,
#   (s * vol) >> 15 per voice, saturated to 16 bits
# Optimized: no (naive: per frame the 8 voices one by one, mult per product).
	.p2align 3
	.globl	mix
	.ent	mix
mix:
	ADDR($8, voice_pcm)
	ADDR($9, mix_out)
	addiu	$10, $0, 256		# frames
1:	move	$11, $0			# l
	move	$12, $0			# r
	move	$13, $8			# &voice_pcm[0][i]
	ADDR($14, voice_vol)
	addiu	$15, $0, 8
2:	lh	$24, 0($13)		# s
	lh	$25, 0($14)
	mult	$25, $24, $25
	sra	$25, $25, 15
	addu	$11, $11, $25
	lh	$25, 2($14)
	mult	$25, $24, $25
	sra	$25, $25, 15
	addu	$12, $12, $25
	addiu	$13, $13, 512		# next voice
	addiu	$14, $14, 4
	addiu	$15, $15, -1
	bnez	$15, 2b
	nop
	SAT16($11, $24, $25)
	SAT16($12, $24, $25)
	sh	$11, 0($9)
	sh	$12, 2($9)
	addiu	$8, $8, 2
	addiu	$9, $9, 4
	addiu	$10, $10, -1
	bnez	$10, 1b
	nop
	ADDR($4, mix_out)
	j	app_hash
	ori	$5, $0, 1024
	.end	mix

# adpcm_decode: pcm = the 64 PS-ADPCM blocks of adpcm, 28 samples each:
#   s = (s16)(nibble << 12) >> shift, + (old * f0 + older * f1 + 32) >> 6,
#   saturated; old and older carry over the blocks
# Optimized: no (naive: one sample per iteration).
	.p2align 3
	.globl	adpcm_decode
	.ent	adpcm_decode
adpcm_decode:
	ADDR($8, adpcm)
	ADDR($9, pcm)
	addiu	$10, $8, 1024		# end of the blocks
	move	$2, $0			# old
	move	$3, $0			# older
1:	lbu	$11, 0($8)
	andi	$12, $11, 0xf		# shift
	srl	$11, $11, 4		# filter
	sll	$11, $11, 3
	ADDR($13, adpcm_coef)
	addu	$13, $13, $11
	lw	$14, 0($13)		# f0
	lw	$15, 4($13)		# f1
	move	$13, $0			# i
2:	srl	$24, $13, 1
	addu	$24, $24, $8
	lbu	$24, 2($24)
	andi	$25, $13, 1
	sll	$25, $25, 2
	srlv	$24, $24, $25
	andi	$24, $24, 0xf		# nibble
	sll	$24, $24, 28
	sra	$24, $24, 16		# (s16)(nibble << 12)
	srav	$24, $24, $12		# >> shift
	mult	$25, $2, $14		# old * f0
	mult	$4, $3, $15		# older * f1
	addu	$25, $25, $4
	addiu	$25, $25, 32
	sra	$25, $25, 6
	addu	$24, $24, $25
	SAT16($24, $4, $5)
	sh	$24, 0($9)
	addiu	$9, $9, 2
	move	$3, $2
	move	$2, $24
	addiu	$13, $13, 1
	addiu	$1, $0, 28
	bne	$13, $1, 2b
	nop
	addiu	$8, $8, 16
	bne	$8, $10, 1b
	nop
	ADDR($4, pcm)
	j	app_hash
	ori	$5, $0, 3584
	.end	adpcm_decode

# fir: fir_out[i] = sat16((1 << 14 + sum of fir_in[i + k] * fir_coef[k]) >> 15)
#   over 32 taps
# Optimized: no (naive: mult and addu per tap).
	.p2align 3
	.globl	fir
	.ent	fir
fir:
	ADDR($8, fir_in)
	ADDR($9, fir_out)
	addiu	$10, $0, 512
1:	addiu	$11, $0, 16384		# acc
	move	$12, $8
	ADDR($13, fir_coef)
	addiu	$14, $13, 64
2:	lh	$15, 0($12)
	lh	$24, 0($13)
	mult	$15, $15, $24
	addu	$11, $11, $15
	addiu	$12, $12, 2
	addiu	$13, $13, 2
	bne	$13, $14, 2b
	nop
	sra	$11, $11, 15
	SAT16($11, $15, $24)
	sh	$11, 0($9)
	addiu	$8, $8, 2
	addiu	$9, $9, 2
	addiu	$10, $10, -1
	bnez	$10, 1b
	nop
	ADDR($4, fir_out)
	j	app_hash
	ori	$5, $0, 1024
	.end	fir

# resample: rs_out[i] = linear interpolation of rs_in at pos = i * step,
#   step = (44100 << 16) / 48000 = 60211, Q15 fraction (pos >> 1) & 0x7fff
# Optimized: no (naive).
	.p2align 3
	.globl	resample
	.ent	resample
resample:
	ADDR($8, rs_in)
	ADDR($9, rs_out)
	ori	$10, $0, 60211		# step
	move	$11, $0			# pos
	addiu	$12, $9, 2048		# end of the output
1:	srl	$13, $11, 16
	sll	$13, $13, 1
	addu	$13, $13, $8
	lh	$14, 0($13)		# a
	lh	$15, 2($13)		# b
	srl	$24, $11, 1
	andi	$24, $24, 0x7fff	# frac
	subu	$15, $15, $14
	mult	$15, $15, $24
	sra	$15, $15, 15
	addu	$14, $14, $15
	sh	$14, 0($9)
	addiu	$9, $9, 2
	bne	$9, $12, 1b
	addu	$11, $11, $10
	ADDR($4, rs_out)
	j	app_hash
	ori	$5, $0, 2048
	.end	resample

# correlate: corr_out[lag] = sum over i < 512 of corr_in[i] * corr_in[i + lag],
#   lag 0..31
# Optimized: no (naive: mult and addu per product; see TODO.md, the sum can
# stay in HI:LO with madd).
	.p2align 3
	.globl	correlate
	.ent	correlate
correlate:
	ADDR($8, corr_in)
	ADDR($9, corr_out)
	move	$10, $0			# lag
1:	move	$11, $0			# acc
	move	$12, $8			# &corr_in[i]
	sll	$13, $10, 1
	addu	$13, $13, $8		# &corr_in[i + lag]
	addiu	$14, $8, 1024		# end
2:	lh	$15, 0($12)
	lh	$24, 0($13)
	mult	$15, $15, $24
	addu	$11, $11, $15
	addiu	$12, $12, 2
	bne	$12, $14, 2b
	addiu	$13, $13, 2
	sw	$11, 0($9)
	addiu	$9, $9, 4
	addiu	$10, $10, 1
	addiu	$1, $0, 32
	bne	$10, $1, 1b
	nop
	ADDR($4, corr_out)
	j	app_hash
	ori	$5, $0, 128
	.end	correlate

# energy: energy_out[b] = sum of the squares of the 256 floats of block b
# Optimized: no (naive: mul.s and add.s per sample).
	.p2align 3
	.globl	energy
	.ent	energy
energy:
	ADDR($8, energy_in)
	ADDR($9, energy_out)
	addiu	$10, $9, 64		# 16 blocks
1:	mtc1	$0, $f0
	addiu	$11, $8, 1024
2:	lwc1	$f1, 0($8)
	mul.s	$f1, $f1, $f1
	add.s	$f0, $f0, $f1
	addiu	$8, $8, 4
	bne	$8, $11, 2b
	nop
	swc1	$f0, 0($9)
	addiu	$9, $9, 4
	bne	$9, $10, 1b
	nop
	ADDR($4, energy_out)
	j	app_hash
	ori	$5, $0, 64
	.end	energy
