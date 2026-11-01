# Hand-written R5900 versions of the gfx_tests.c kernels (bench variant asm).
# n32 ABI. The kernels use the data of gfx_tests.c (gfx_mem, geom_mem,
# matrix, group_sum, spheres, sphere_visible: KERNEL_DATA), and return
# app_hash() of their output as the C kernels do. See int_tests.S for what
# "naive" means. Integer kernels use the GPRs, the scalar float kernels the
# FPU and the v4sf kernels VU0 macro instructions.

	.set	noreorder
	.set	noat

# gfx_mem: the offsets of its buffers
#define SPRITE		0x0000
#define FRAME		0x4400
#define INDEXED		0x8800
#define PALETTE		0x9c00
#define FRAME16		0xa000
#define SCALED		0xc400
# geom_mem
#define VERTS		0x0000
#define NORMALS		0x2400
#define SCALED_V	0x4800
#define FACE_NORMALS	0x6c00
#define SCREEN		0x9400
#define XFORMED		0xb800
#define COLORS		0xdc00
#define FACE_NORMALS_V	0xec00
#define VISIBLE		0x10c00

# reg = symbol + offset (any 32-bit offset), using $1
#define ADDR(reg, sym, off) \
	lui	$1, %hi(sym); addiu reg, $1, %lo(sym); \
	lui	$1, ((off) >> 16); ori $1, $1, ((off) & 0xffff); addu reg, reg, $1
# FPU register = the float constant at label
#define LOADF(freg, label) lui $1, %hi(label); lwc1 freg, %lo(label)($1)

	.section .rodata
	.p2align 4
f_0p125:	.float	0.125
f_0p5:		.float	0.5
f_1:		.float	1.0
f_16:		.float	16.0
f_64:		.float	64.0
f_128:		.float	128.0
f_2048:		.float	2048.0
f_65536:	.float	65536.0
	.p2align 4
# lighting: dir[3] and col[3] (vec4)
light_dir:	.float	0.5, 0.5, -0.5, 0.0, -0.75, 0.25, 0.0, 0.0, 0.0, -0.5, 0.75, 0.0
light_col:	.float	0.5, 0.375, 0.25, 0.0, 0.125, 0.25, 0.5, 0.0, 0.25, 0.25, 0.125, 0.0
# lighting_v4sf: dir with the intensity in w, col, the start color and 128
light_dir_v:	.float	0.5, 0.5, -0.5, 1.0, -0.75, 0.25, 0.0, 0.5, 0.0, -0.5, 0.75, 0.75
light_c0_v:	.float	0.125, 0.125, 0.125, 0.0
v_128:		.float	128.0, 128.0, 128.0, 128.0
# frustum_cull: 6 planes
planes:		.float	0.75, 0.0, 0.5, 0.0, -0.75, 0.0, 0.5, 0.0
		.float	0.0, 0.75, 0.5, 0.0, 0.0, -0.75, 0.5, 0.0
		.float	0.0, 0.0, 1.0, -1.0, 0.0, 0.0, -1.0, 100.0
	.text

# ---- 2D ----

# alpha_blend: frame[i] = blend of sprite[i] over frame[i] with its alpha
#   (r, g, b) = (s * a + d * (256 - a)) >> 8 per channel, d's alpha
# Optimized: no (naive: one pixel per iteration, mult per channel product).
	.p2align 3
	.globl	alpha_blend
	.ent	alpha_blend
alpha_blend:
	ADDR($8, gfx_mem, SPRITE)
	ADDR($9, gfx_mem, FRAME)
	addiu	$10, $8, 16384		# end of sprite
1:	lw	$11, 0($8)		# s
	lw	$12, 0($9)		# d
	srl	$13, $11, 24		# a
	addiu	$14, $0, 256
	subu	$14, $14, $13		# 256 - a
	andi	$15, $11, 0xff
	mult	$15, $15, $13
	andi	$24, $12, 0xff
	mult	$24, $24, $14
	addu	$15, $15, $24
	srl	$15, $15, 8		# r
	srl	$24, $11, 8
	andi	$24, $24, 0xff
	mult	$24, $24, $13
	srl	$25, $12, 8
	andi	$25, $25, 0xff
	mult	$25, $25, $14
	addu	$24, $24, $25
	srl	$24, $24, 8
	sll	$24, $24, 8
	or	$15, $15, $24		# | g << 8
	srl	$24, $11, 16
	andi	$24, $24, 0xff
	mult	$24, $24, $13
	srl	$25, $12, 16
	andi	$25, $25, 0xff
	mult	$25, $25, $14
	addu	$24, $24, $25
	srl	$24, $24, 8
	sll	$24, $24, 16
	or	$15, $15, $24		# | b << 16
	lui	$24, 0xff00
	and	$24, $12, $24
	or	$15, $15, $24		# | d's alpha
	sw	$15, 0($9)
	addiu	$8, $8, 4
	bne	$8, $10, 1b
	addiu	$9, $9, 4
	ADDR($4, gfx_mem, FRAME)
	j	app_hash
	ori	$5, $0, 16384
	.end	alpha_blend

# clut8: frame[i] = palette[indexed[i]]
# Optimized: no (naive).
	.p2align 3
	.globl	clut8
	.ent	clut8
clut8:
	ADDR($8, gfx_mem, INDEXED)
	ADDR($9, gfx_mem, FRAME)
	ADDR($10, gfx_mem, PALETTE)
	addiu	$12, $8, 4096
1:	lbu	$11, 0($8)
	sll	$11, $11, 2
	addu	$11, $11, $10
	lw	$11, 0($11)
	sw	$11, 0($9)
	addiu	$8, $8, 1
	bne	$8, $12, 1b
	addiu	$9, $9, 4
	ADDR($4, gfx_mem, FRAME)
	j	app_hash
	ori	$5, $0, 16384
	.end	clut8

# color_key: scaled[i] = sprite[i] == 0x00ff00ff ? frame[i] : sprite[i]
# Optimized: no (naive: xor and movz per pixel).
	.p2align 3
	.globl	color_key
	.ent	color_key
color_key:
	ADDR($8, gfx_mem, SPRITE)
	ADDR($9, gfx_mem, FRAME)
	ADDR($10, gfx_mem, SCALED)
	lui	$13, 0x00ff
	ori	$13, $13, 0x00ff
	addiu	$15, $8, 16384
1:	lw	$11, 0($8)
	lw	$12, 0($9)
	xor	$14, $11, $13
	movz	$11, $12, $14
	sw	$11, 0($10)
	addiu	$8, $8, 4
	addiu	$9, $9, 4
	bne	$8, $15, 1b
	addiu	$10, $10, 4
	ADDR($4, gfx_mem, SCALED)
	j	app_hash
	ori	$5, $0, 16384
	.end	color_key

# alpha_test: scaled[i] = sprite[i] >> 24 >= 0x40 ? sprite[i] : frame[i]
# Optimized: no (naive: sltiu and movn per pixel).
	.p2align 3
	.globl	alpha_test
	.ent	alpha_test
alpha_test:
	ADDR($8, gfx_mem, SPRITE)
	ADDR($9, gfx_mem, FRAME)
	ADDR($10, gfx_mem, SCALED)
	addiu	$15, $8, 16384
1:	lw	$11, 0($8)
	lw	$12, 0($9)
	srl	$14, $11, 24
	sltiu	$14, $14, 0x40
	movn	$11, $12, $14
	sw	$11, 0($10)
	addiu	$8, $8, 4
	addiu	$9, $9, 4
	bne	$8, $15, 1b
	addiu	$10, $10, 4
	ADDR($4, gfx_mem, SCALED)
	j	app_hash
	ori	$5, $0, 16384
	.end	alpha_test

# to_ct16: frame16[i] = RGBA5551 of sprite[i]
# Optimized: no (naive: four shifts and masks per pixel).
	.p2align 3
	.globl	to_ct16
	.ent	to_ct16
to_ct16:
	ADDR($8, gfx_mem, SPRITE)
	ADDR($9, gfx_mem, FRAME16)
	addiu	$15, $8, 16384
1:	lw	$11, 0($8)
	srl	$12, $11, 3
	andi	$12, $12, 0x1f
	srl	$13, $11, 6
	andi	$13, $13, 0x3e0
	or	$12, $12, $13
	srl	$13, $11, 9
	andi	$13, $13, 0x7c00
	or	$12, $12, $13
	srl	$13, $11, 16
	andi	$13, $13, 0x8000
	or	$12, $12, $13
	sh	$12, 0($9)
	addiu	$8, $8, 4
	bne	$8, $15, 1b
	addiu	$9, $9, 2
	ADDR($4, gfx_mem, FRAME16)
	j	app_hash
	ori	$5, $0, 8192
	.end	to_ct16

# scale_nn: scaled (96x96) = sprite (64x64) scaled with nearest neighbor,
#   16.16 steps of (64 << 16) / 96 = 43690
# Optimized: no (naive: one pixel per iteration).
	.p2align 3
	.globl	scale_nn
	.ent	scale_nn
scale_nn:
	ADDR($8, gfx_mem, SPRITE)
	ADDR($9, gfx_mem, SCALED)
	ori	$24, $0, 43690		# step
	move	$10, $0			# sy
	addiu	$11, $0, 96		# rows left
1:	srl	$12, $10, 16
	sll	$12, $12, 8
	addu	$12, $12, $8		# row
	move	$13, $0			# sx
	addiu	$14, $0, 96
2:	srl	$15, $13, 16
	sll	$15, $15, 2
	addu	$15, $15, $12
	lw	$15, 0($15)
	sw	$15, 0($9)
	addiu	$9, $9, 4
	addu	$13, $13, $24
	addiu	$14, $14, -1
	bnez	$14, 2b
	nop
	addu	$10, $10, $24
	addiu	$11, $11, -1
	bnez	$11, 1b
	nop
	ADDR($4, gfx_mem, SCALED)
	j	app_hash
	ori	$5, $0, 36864
	.end	scale_nn

# ---- 3D ----

# transform: screen[i] of verts[i] by matrix (3 rows), divided by z, to the
#   GS window in 12.4 fixed point; pad = z * 16
# Optimized: no (naive: the matrix in $f0-$f11, mul.s and add.s in source
# order, the constants loaded from memory where they are used).
	.p2align 3
	.globl	transform
	.ent	transform
transform:
	lui	$1, %hi(matrix)
	addiu	$8, $1, %lo(matrix)
	lwc1	$f0, 0($8)
	lwc1	$f1, 4($8)
	lwc1	$f2, 8($8)
	lwc1	$f3, 12($8)
	lwc1	$f4, 16($8)
	lwc1	$f5, 20($8)
	lwc1	$f6, 24($8)
	lwc1	$f7, 28($8)
	lwc1	$f8, 32($8)
	lwc1	$f9, 36($8)
	lwc1	$f10, 40($8)
	lwc1	$f11, 44($8)
	ADDR($9, geom_mem, VERTS)
	ADDR($10, geom_mem, SCREEN)
	addiu	$11, $9, 8192
1:	lwc1	$f12, 0($9)
	lwc1	$f13, 4($9)
	lwc1	$f14, 8($9)
	lwc1	$f15, 12($9)
	mul.s	$f16, $f0, $f12		# x
	mul.s	$f19, $f1, $f13
	add.s	$f16, $f16, $f19
	mul.s	$f19, $f2, $f14
	add.s	$f16, $f16, $f19
	mul.s	$f19, $f3, $f15
	add.s	$f16, $f16, $f19
	mul.s	$f17, $f4, $f12		# y
	mul.s	$f19, $f5, $f13
	add.s	$f17, $f17, $f19
	mul.s	$f19, $f6, $f14
	add.s	$f17, $f17, $f19
	mul.s	$f19, $f7, $f15
	add.s	$f17, $f17, $f19
	mul.s	$f18, $f8, $f12		# z
	mul.s	$f19, $f9, $f13
	add.s	$f18, $f18, $f19
	mul.s	$f19, $f10, $f14
	add.s	$f18, $f18, $f19
	mul.s	$f19, $f11, $f15
	add.s	$f18, $f18, $f19
	LOADF($f13, f_1)
	div.s	$f12, $f13, $f14	# iw = 1 / v.z
	mul.s	$f19, $f16, $f12	# screen.x
	LOADF($f13, f_64)
	mul.s	$f19, $f19, $f13
	LOADF($f13, f_2048)
	add.s	$f19, $f19, $f13
	LOADF($f13, f_16)
	mul.s	$f19, $f19, $f13
	LOADF($f13, f_0p5)
	add.s	$f19, $f19, $f13
	cvt.w.s	$f19, $f19
	swc1	$f19, 0($10)
	mul.s	$f19, $f17, $f12	# screen.y
	LOADF($f13, f_64)
	mul.s	$f19, $f19, $f13
	LOADF($f13, f_2048)
	add.s	$f19, $f19, $f13
	LOADF($f13, f_16)
	mul.s	$f19, $f19, $f13
	LOADF($f13, f_0p5)
	add.s	$f19, $f19, $f13
	cvt.w.s	$f19, $f19
	swc1	$f19, 4($10)
	LOADF($f13, f_65536)		# screen.z
	mul.s	$f19, $f12, $f13
	LOADF($f13, f_0p5)
	add.s	$f19, $f19, $f13
	cvt.w.s	$f19, $f19
	swc1	$f19, 8($10)
	LOADF($f13, f_16)		# screen.pad
	mul.s	$f19, $f18, $f13
	cvt.w.s	$f19, $f19
	swc1	$f19, 12($10)
	addiu	$9, $9, 16
	bne	$9, $11, 1b
	addiu	$10, $10, 16
	ADDR($4, geom_mem, SCREEN)
	j	app_hash
	ori	$5, $0, 8192
	.end	transform

# One component of a matrix times a vector: $f0 = sum of the four products of
# the floats at m + o0, o1, o2, o3 with $f16..$f19, in that order.
#define DOT4(m, o0, o1, o2, o3) \
	lwc1 $f1, o0(m); mul.s $f0, $f1, $f16; \
	lwc1 $f1, o1(m); mul.s $f1, $f1, $f17; add.s $f0, $f0, $f1; \
	lwc1 $f1, o2(m); mul.s $f1, $f1, $f18; add.s $f0, $f0, $f1; \
	lwc1 $f1, o3(m); mul.s $f1, $f1, $f19; add.s $f0, $f0, $f1
# The same with three products and a fourth term added: the translation.
#define DOT3T(m, o0, o1, o2, o3) \
	lwc1 $f1, o0(m); mul.s $f0, $f1, $f16; \
	lwc1 $f1, o1(m); mul.s $f1, $f1, $f17; add.s $f0, $f0, $f1; \
	lwc1 $f1, o2(m); mul.s $f1, $f1, $f18; add.s $f0, $f0, $f1; \
	lwc1 $f1, o3(m); add.s $f0, $f0, $f1

# mxv_row4(m $4, in $5, out $6, n $7): out[i] = rows of m dotted with in[i]
# Optimized: no (naive: the matrix loaded for every product).
	.p2align 3
	.globl	mxv_row4
	.ent	mxv_row4
mxv_row4:
	blez	$7, 2f
	nop
1:	lwc1	$f16, 0($5)
	lwc1	$f17, 4($5)
	lwc1	$f18, 8($5)
	lwc1	$f19, 12($5)
	DOT4($4, 0, 4, 8, 12)
	swc1	$f0, 0($6)
	DOT4($4, 16, 20, 24, 28)
	swc1	$f0, 4($6)
	DOT4($4, 32, 36, 40, 44)
	swc1	$f0, 8($6)
	DOT4($4, 48, 52, 56, 60)
	swc1	$f0, 12($6)
	addiu	$5, $5, 16
	addiu	$6, $6, 16
	addiu	$7, $7, -1
	bnez	$7, 1b
	nop
2:	jr	$ra
	nop
	.end	mxv_row4

# mxv_col4(m $4, in $5, out $6, n $7): out[i] = columns of m scaled by in[i]
# Optimized: no (naive).
	.p2align 3
	.globl	mxv_col4
	.ent	mxv_col4
mxv_col4:
	blez	$7, 2f
	nop
1:	lwc1	$f16, 0($5)
	lwc1	$f17, 4($5)
	lwc1	$f18, 8($5)
	lwc1	$f19, 12($5)
	DOT4($4, 0, 16, 32, 48)
	swc1	$f0, 0($6)
	DOT4($4, 4, 20, 36, 52)
	swc1	$f0, 4($6)
	DOT4($4, 8, 24, 40, 56)
	swc1	$f0, 8($6)
	DOT4($4, 12, 28, 44, 60)
	swc1	$f0, 12($6)
	addiu	$5, $5, 16
	addiu	$6, $6, 16
	addiu	$7, $7, -1
	bnez	$7, 1b
	nop
2:	jr	$ra
	nop
	.end	mxv_col4

# x, y and z of screen[i] = (int)(component * 16)
#define TO_SCREEN(off) \
	LOADF($f1, f_16); mul.s $f0, $f0, $f1; cvt.w.s $f0, $f0; swc1 $f0, off($8)

# mxv_row3(m $4, in $5, n $6): screen[i] = 12.4 of the 3x4 rows of m times
#   in[i] (translation in w)
# Optimized: no (naive).
	.p2align 3
	.globl	mxv_row3
	.ent	mxv_row3
mxv_row3:
	ADDR($8, geom_mem, SCREEN)
	blez	$6, 2f
	nop
1:	lwc1	$f16, 0($5)
	lwc1	$f17, 4($5)
	lwc1	$f18, 8($5)
	DOT3T($4, 0, 4, 8, 12)
	TO_SCREEN(0)
	DOT3T($4, 16, 20, 24, 28)
	TO_SCREEN(4)
	DOT3T($4, 32, 36, 40, 44)
	TO_SCREEN(8)
	addiu	$5, $5, 16
	addiu	$8, $8, 16
	addiu	$6, $6, -1
	bnez	$6, 1b
	nop
2:	jr	$ra
	nop
	.end	mxv_row3

# mxv_col3(m $4, in $5, n $6): the same with the matrix in columns
# Optimized: no (naive).
	.p2align 3
	.globl	mxv_col3
	.ent	mxv_col3
mxv_col3:
	ADDR($8, geom_mem, SCREEN)
	blez	$6, 2f
	nop
1:	lwc1	$f16, 0($5)
	lwc1	$f17, 4($5)
	lwc1	$f18, 8($5)
	DOT3T($4, 0, 16, 32, 48)
	TO_SCREEN(0)
	DOT3T($4, 4, 20, 36, 52)
	TO_SCREEN(4)
	DOT3T($4, 8, 24, 40, 56)
	TO_SCREEN(8)
	addiu	$5, $5, 16
	addiu	$8, $8, 16
	addiu	$6, $6, -1
	bnez	$6, 1b
	nop
2:	jr	$ra
	nop
	.end	mxv_col3

# lighting: colors[i] = ambient 0.125 plus 3 directional lights on normals[i]
#   (a light adds d * col where d = n . dir > 0), clamped to 1, * 128 per
#   channel, alpha 0x80
# Optimized: no (naive: the lights loaded per vertex, branches for d > 0 and
# the clamps).
	.p2align 3
	.globl	lighting
	.ent	lighting
lighting:
	ADDR($8, geom_mem, NORMALS)
	ADDR($9, geom_mem, COLORS)
	addiu	$10, $8, 8192
1:	lwc1	$f16, 0($8)
	lwc1	$f17, 4($8)
	lwc1	$f18, 8($8)
	LOADF($f2, f_0p125)
	mov.s	$f3, $f2		# r
	mov.s	$f4, $f2		# g
	mov.s	$f5, $f2		# b
	lui	$1, %hi(light_dir)
	addiu	$11, $1, %lo(light_dir)
	lui	$1, %hi(light_col)
	addiu	$12, $1, %lo(light_col)
	addiu	$13, $0, 3
2:	lwc1	$f6, 0($11)
	mul.s	$f0, $f16, $f6
	lwc1	$f6, 4($11)
	mul.s	$f6, $f17, $f6
	add.s	$f0, $f0, $f6
	lwc1	$f6, 8($11)
	mul.s	$f6, $f18, $f6
	add.s	$f0, $f0, $f6		# d
	mtc1	$0, $f7
	c.lt.s	$f7, $f0
	bc1f	3f
	nop
	lwc1	$f6, 0($12)
	mul.s	$f6, $f0, $f6
	add.s	$f3, $f3, $f6
	lwc1	$f6, 4($12)
	mul.s	$f6, $f0, $f6
	add.s	$f4, $f4, $f6
	lwc1	$f6, 8($12)
	mul.s	$f6, $f0, $f6
	add.s	$f5, $f5, $f6
3:	addiu	$11, $11, 16
	addiu	$12, $12, 16
	addiu	$13, $13, -1
	bnez	$13, 2b
	nop
	LOADF($f6, f_1)
	c.lt.s	$f6, $f3
	bc1f	4f
	nop
	mov.s	$f3, $f6
4:	c.lt.s	$f6, $f4
	bc1f	5f
	nop
	mov.s	$f4, $f6
5:	c.lt.s	$f6, $f5
	bc1f	6f
	nop
	mov.s	$f5, $f6
6:	LOADF($f6, f_128)
	mul.s	$f3, $f3, $f6
	cvt.w.s	$f3, $f3
	mfc1	$14, $f3
	mul.s	$f4, $f4, $f6
	cvt.w.s	$f4, $f4
	mfc1	$15, $f4
	sll	$15, $15, 8
	or	$14, $14, $15
	mul.s	$f5, $f5, $f6
	cvt.w.s	$f5, $f5
	mfc1	$15, $f5
	sll	$15, $15, 16
	or	$14, $14, $15
	lui	$15, 0x8000
	or	$14, $14, $15
	sw	$14, 0($9)
	addiu	$8, $8, 16
	bne	$8, $10, 1b
	addiu	$9, $9, 4
	ADDR($4, geom_mem, COLORS)
	j	app_hash
	ori	$5, $0, 2048
	.end	lighting

# backface_cull: visible = the corners of the counter-clockwise triangles of
#   the strip of screen, by the sign of the 64-bit 2D cross product; returns
#   app_hash(visible, n * 2) ^ n for n corners
# Optimized: no (naive: the products moved from HI:LO to 64-bit registers).
	.p2align 3
	.globl	backface_cull
	.ent	backface_cull
backface_cull:
	addiu	$sp, $sp, -16
	sd	$ra, 0($sp)
	sd	$16, 8($sp)
	ADDR($8, geom_mem, SCREEN)
	ADDR($9, geom_mem, VISIBLE)
	move	$16, $0			# n
	move	$10, $0			# i
1:	andi	$11, $10, 1
	addu	$12, $10, $11
	addiu	$12, $12, 1		# b = i + 1 + (i & 1)
	subu	$13, $10, $11
	addiu	$13, $13, 2		# c = i + 2 - (i & 1)
	sll	$14, $10, 4
	addu	$14, $14, $8		# &screen[a]
	sll	$15, $12, 4
	addu	$15, $15, $8		# &screen[b]
	sll	$24, $13, 4
	addu	$24, $24, $8		# &screen[c]
	lw	$2, 0($14)		# a.x
	lw	$3, 4($14)		# a.y
	lw	$4, 0($15)
	subu	$4, $4, $2		# ex
	lw	$5, 4($15)
	subu	$5, $5, $3		# ey
	lw	$6, 0($24)
	subu	$6, $6, $2		# fx
	lw	$7, 4($24)
	subu	$7, $7, $3		# fy
	mult	$4, $7			# ex * fy
	mflo	$2
	mfhi	$3
	dsll32	$3, $3, 0
	dsll32	$2, $2, 0
	dsrl32	$2, $2, 0
	or	$25, $3, $2
	mult	$5, $6			# ey * fx
	mflo	$2
	mfhi	$3
	dsll32	$3, $3, 0
	dsll32	$2, $2, 0
	dsrl32	$2, $2, 0
	or	$3, $3, $2
	dsubu	$25, $25, $3
	blez	$25, 2f
	nop
	sll	$2, $16, 1
	addu	$2, $2, $9
	sh	$10, 0($2)
	sh	$12, 2($2)
	sh	$13, 4($2)
	addiu	$16, $16, 3
2:	addiu	$10, $10, 1
	addiu	$1, $0, 510
	bne	$10, $1, 1b
	nop
	ADDR($4, geom_mem, VISIBLE)
	jal	app_hash
	sll	$5, $16, 1
	xor	$2, $2, $16
	ld	$ra, 0($sp)
	ld	$16, 8($sp)
	jr	$ra
	addiu	$sp, $sp, 16
	.end	backface_cull

# face_normal: face_normals[i] = (b - a) x (c - a) of verts i, i + 1, i + 2
# Optimized: no (naive: the FPU, in source order).
	.p2align 3
	.globl	face_normal
	.ent	face_normal
face_normal:
	ADDR($8, geom_mem, VERTS)
	ADDR($9, geom_mem, FACE_NORMALS)
	addiu	$10, $0, 510
1:	lwc1	$f0, 0($8)		# a
	lwc1	$f1, 4($8)
	lwc1	$f2, 8($8)
	lwc1	$f3, 16($8)		# b
	lwc1	$f4, 20($8)
	lwc1	$f5, 24($8)
	lwc1	$f6, 32($8)		# c
	lwc1	$f7, 36($8)
	lwc1	$f8, 40($8)
	sub.s	$f3, $f3, $f0		# ex
	sub.s	$f4, $f4, $f1		# ey
	sub.s	$f5, $f5, $f2		# ez
	sub.s	$f6, $f6, $f0		# fx
	sub.s	$f7, $f7, $f1		# fy
	sub.s	$f8, $f8, $f2		# fz
	mul.s	$f9, $f4, $f8
	mul.s	$f10, $f5, $f7
	sub.s	$f9, $f9, $f10		# ey * fz - ez * fy
	swc1	$f9, 0($9)
	mul.s	$f9, $f5, $f6
	mul.s	$f10, $f3, $f8
	sub.s	$f9, $f9, $f10		# ez * fx - ex * fz
	swc1	$f9, 4($9)
	mul.s	$f9, $f3, $f7
	mul.s	$f10, $f4, $f6
	sub.s	$f9, $f9, $f10		# ex * fy - ey * fx
	swc1	$f9, 8($9)
	sw	$0, 12($9)
	addiu	$8, $8, 16
	addiu	$9, $9, 16
	addiu	$10, $10, -1
	bnez	$10, 1b
	nop
	ADDR($4, geom_mem, FACE_NORMALS)
	j	app_hash
	ori	$5, $0, 8160
	.end	face_normal

# face_normal_v4sf: face_normals_v[i] = (v[i+1] - v[i]) x (v[i+2] - v[i]), w 0
# Optimized: no (naive: VU0 vsub, the cross product with vopmula/vopmsub, one
# triangle per iteration).
	.p2align 3
	.globl	face_normal_v4sf
	.ent	face_normal_v4sf
face_normal_v4sf:
	ADDR($8, geom_mem, VERTS)
	ADDR($9, geom_mem, FACE_NORMALS_V)
	addiu	$10, $0, 510
1:	lqc2	$vf1, 0($8)
	lqc2	$vf2, 16($8)
	lqc2	$vf3, 32($8)
	vsub.xyzw	$vf4, $vf2, $vf1	# e
	vsub.xyzw	$vf5, $vf3, $vf1	# f
	vopmula.xyz	$ACC, $vf4, $vf5	# e.yzx * f.zxy
	vopmsub.xyz	$vf6, $vf5, $vf4	# - e.zxy * f.yzx
	vsub.w	$vf6, $vf0, $vf0		# w = 0
	sqc2	$vf6, 0($9)
	addiu	$8, $8, 16
	addiu	$9, $9, 16
	addiu	$10, $10, -1
	bnez	$10, 1b
	nop
	ADDR($4, geom_mem, FACE_NORMALS_V)
	j	app_hash
	ori	$5, $0, 8160
	.end	face_normal_v4sf

# lighting_v4sf: colors[i] from normals[i] with 3 lights: c = (0.125, 0.125,
#   0.125) + sum of col * (max(n . dir, 0) * dir.w), min(c, 1) * 128 per
#   channel, alpha 0x80
# Optimized: no (naive: VU0, the lights loaded per vertex; vf0 = (0, 0, 0, 1)
# gives the 0 of the max and the 1 of the min).
	.p2align 3
	.globl	lighting_v4sf
	.ent	lighting_v4sf
lighting_v4sf:
	ADDR($8, geom_mem, NORMALS)
	ADDR($9, geom_mem, COLORS)
	addiu	$10, $8, 8192
1:	lqc2	$vf1, 0($8)		# n
	lui	$1, %hi(light_c0_v)
	lqc2	$vf2, %lo(light_c0_v)($1)	# c
	lui	$1, %hi(light_dir_v)
	addiu	$11, $1, %lo(light_dir_v)
	lui	$1, %hi(light_col)
	addiu	$12, $1, %lo(light_col)
	addiu	$13, $0, 3
2:	lqc2	$vf3, 0($11)		# dir
	lqc2	$vf4, 0($12)		# col
	vmul.xyz	$vf5, $vf1, $vf3	# p
	vaddy.x	$vf6, $vf5, $vf5y
	vaddz.x	$vf6, $vf6, $vf5z	# p.x + p.y + p.z
	vmax.x	$vf6, $vf6, $vf0	# max(, 0)
	vmulw.x	$vf6, $vf6, $vf3w	# * dir.w
	vmulx.xyz	$vf7, $vf4, $vf6x	# col * d
	vadd.xyz	$vf2, $vf2, $vf7
	addiu	$11, $11, 16
	addiu	$12, $12, 16
	addiu	$13, $13, -1
	bnez	$13, 2b
	nop
	vminiw.xyz	$vf2, $vf2, $vf0w	# min(c, 1)
	lui	$1, %hi(v_128)
	lqc2	$vf3, %lo(v_128)($1)
	vmul.xyz	$vf2, $vf2, $vf3
	vftoi0.xyz	$vf2, $vf2
	qmfc2	$14, $vf2
	andi	$15, $14, 0xff		# r
	dsrl32	$24, $14, 0
	andi	$24, $24, 0xff
	sll	$24, $24, 8
	or	$15, $15, $24		# | g << 8
	pcpyud	$24, $14, $14
	andi	$24, $24, 0xff
	sll	$24, $24, 16
	or	$15, $15, $24		# | b << 16
	lui	$24, 0x8000
	or	$15, $15, $24
	sw	$15, 0($9)
	addiu	$8, $8, 16
	bne	$8, $10, 1b
	addiu	$9, $9, 4
	ADDR($4, geom_mem, COLORS)
	j	app_hash
	ori	$5, $0, 2048
	.end	lighting_v4sf

# field_scale_v4sf: scaled_v[i] = normals[i] * (verts[i].y * normals[i].z)
# Optimized: no (naive: VU0 with broadcast fields).
	.p2align 3
	.globl	field_scale_v4sf
	.ent	field_scale_v4sf
field_scale_v4sf:
	ADDR($8, geom_mem, VERTS)
	ADDR($9, geom_mem, NORMALS)
	ADDR($10, geom_mem, SCALED_V)
	addiu	$11, $8, 8192
1:	lqc2	$vf1, 0($8)		# v
	lqc2	$vf2, 0($9)		# n
	vmulz.y	$vf3, $vf1, $vf2z	# v.y * n.z
	vmuly.xyzw	$vf4, $vf2, $vf3y
	sqc2	$vf4, 0($10)
	addiu	$8, $8, 16
	addiu	$9, $9, 16
	bne	$8, $11, 1b
	addiu	$10, $10, 16
	ADDR($4, geom_mem, SCALED_V)
	j	app_hash
	ori	$5, $0, 8192
	.end	field_scale_v4sf

# field_scale_bc_v4sf: scaled_v[i] = normals[i] + verts[i].y * normals[i].z
# Optimized: no (naive: VU0 with broadcast fields).
	.p2align 3
	.globl	field_scale_bc_v4sf
	.ent	field_scale_bc_v4sf
field_scale_bc_v4sf:
	ADDR($8, geom_mem, VERTS)
	ADDR($9, geom_mem, NORMALS)
	ADDR($10, geom_mem, SCALED_V)
	addiu	$11, $8, 8192
1:	lqc2	$vf1, 0($8)
	lqc2	$vf2, 0($9)
	vmulz.y	$vf3, $vf1, $vf2z
	vaddy.xyzw	$vf4, $vf2, $vf3y
	sqc2	$vf4, 0($10)
	addiu	$8, $8, 16
	addiu	$9, $9, 16
	bne	$8, $11, 1b
	addiu	$10, $10, 16
	ADDR($4, geom_mem, SCALED_V)
	j	app_hash
	ori	$5, $0, 8192
	.end	field_scale_bc_v4sf

# project_v4sf: scaled_v[i] = verts[i] * (1 / verts[i].z)
# Optimized: no (naive: VU0 vdiv by vf0.w = 1, waiting for Q each vertex).
	.p2align 3
	.globl	project_v4sf
	.ent	project_v4sf
project_v4sf:
	ADDR($8, geom_mem, VERTS)
	ADDR($10, geom_mem, SCALED_V)
	addiu	$11, $8, 8192
1:	lqc2	$vf1, 0($8)
	vdiv	$Q, $vf0w, $vf1z
	vwaitq
	vmulq.xyzw	$vf2, $vf1, $Q
	sqc2	$vf2, 0($10)
	addiu	$8, $8, 16
	bne	$8, $11, 1b
	addiu	$10, $10, 16
	ADDR($4, geom_mem, SCALED_V)
	j	app_hash
	ori	$5, $0, 8192
	.end	project_v4sf

# weighted_sum_v4sf: group_sum[g] = sum of verts[i] * normals[i] over the 16
#   groups of geom_group_len vertices
# Optimized: no (naive: VU0 vmul and vadd per vertex).
	.p2align 3
	.globl	weighted_sum_v4sf
	.ent	weighted_sum_v4sf
weighted_sum_v4sf:
	ADDR($8, geom_mem, VERTS)
	ADDR($9, geom_mem, NORMALS)
	lui	$1, %hi(group_sum)
	addiu	$10, $1, %lo(group_sum)
	lui	$1, %hi(geom_group_len)
	lw	$12, %lo(geom_group_len)($1)
	addiu	$11, $0, 16		# groups
1:	vsub.xyzw	$vf3, $vf0, $vf0	# s = 0
	blez	$12, 3f
	move	$13, $12
2:	lqc2	$vf1, 0($8)
	lqc2	$vf2, 0($9)
	vmul.xyzw	$vf1, $vf1, $vf2
	vadd.xyzw	$vf3, $vf3, $vf1
	addiu	$8, $8, 16
	addiu	$9, $9, 16
	addiu	$13, $13, -1
	bnez	$13, 2b
	nop
3:	sqc2	$vf3, 0($10)
	addiu	$10, $10, 16
	addiu	$11, $11, -1
	bnez	$11, 1b
	nop
	lui	$1, %hi(group_sum)
	addiu	$4, $1, %lo(group_sum)
	j	app_hash
	ori	$5, $0, 256
	.end	weighted_sum_v4sf

# frustum_cull: sphere_visible[i] = spheres[i] is not completely outside one
#   of the 6 planes (d = p . center + p.w < -radius); returns
#   app_hash(sphere_visible, 1024) ^ the number of visible spheres
# Optimized: no (naive: the FPU, the planes loaded per test, a branch out at
# the first plane that rejects).
	.p2align 3
	.globl	frustum_cull
	.ent	frustum_cull
frustum_cull:
	addiu	$sp, $sp, -16
	sd	$ra, 0($sp)
	sd	$16, 8($sp)
	lui	$1, %hi(spheres)
	addiu	$8, $1, %lo(spheres)
	lui	$1, %hi(sphere_visible)
	addiu	$9, $1, %lo(sphere_visible)
	addiu	$10, $9, 1024
	move	$16, $0			# count
1:	lwc1	$f16, 0($8)
	lwc1	$f17, 4($8)
	lwc1	$f18, 8($8)
	lwc1	$f19, 12($8)
	neg.s	$f19, $f19		# -radius
	lui	$1, %hi(planes)
	addiu	$11, $1, %lo(planes)
	addiu	$12, $11, 96
	addiu	$13, $0, 1		# in
2:	lwc1	$f1, 0($11)
	mul.s	$f0, $f1, $f16
	lwc1	$f1, 4($11)
	mul.s	$f1, $f1, $f17
	add.s	$f0, $f0, $f1
	lwc1	$f1, 8($11)
	mul.s	$f1, $f1, $f18
	add.s	$f0, $f0, $f1
	lwc1	$f1, 12($11)
	add.s	$f0, $f0, $f1		# d
	c.lt.s	$f0, $f19
	bc1t	3f
	addiu	$11, $11, 16
	bne	$11, $12, 2b
	nop
	b	4f
	nop
3:	move	$13, $0
4:	sb	$13, 0($9)
	addu	$16, $16, $13
	addiu	$8, $8, 16
	addiu	$9, $9, 1
	bne	$9, $10, 1b
	nop
	lui	$1, %hi(sphere_visible)
	addiu	$4, $1, %lo(sphere_visible)
	jal	app_hash
	ori	$5, $0, 1024
	xor	$2, $2, $16
	ld	$ra, 0($sp)
	ld	$16, 8($sp)
	jr	$ra
	addiu	$sp, $sp, 16
	.end	frustum_cull
