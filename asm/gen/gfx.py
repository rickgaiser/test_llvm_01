#!/usr/bin/env python3
"""Generates the gfx_tests.c kernels (2D graphics and 3D geometry) with
parameters, their loops ordered by sched.py. Used by asm/harness/gen_app.py
to measure variants on the PS2, and by gfx_file.py for asm/gfx_tests.S.

Every generator takes the symbol name first and returns the assembly of a
u32 (void) kernel as the bench runs it (the kernel, then app_hash of its
output). The code uses the macros of PRELUDE (ADDR, the buffer offsets)."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sched
from fpu_novu0 import loop

EST = {}

PRELUDE = r'''
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
'''

SAVED = ['$16', '$17', '$18', '$19', '$20', '$21', '$22', '$23']


def save(regs):
    """Prologue and epilogue lines that keep the callee-saved regs (sq)."""
    regs = [r for r in SAVED if r in regs]
    if not regs:
        return [], []
    n = 16 * len(regs)
    pro = [f'addiu\t$sp, $sp, -{n}'] + [f'sq\t{r}, {16 * k}($sp)' for k, r in enumerate(regs)]
    epi = [f'lq\t{r}, {16 * k}($sp)' for k, r in enumerate(regs)] + [f'addiu\t$sp, $sp, {n}']
    return pro, epi


def func(name, comment, pre, body, branch, post, label='1'):
    """A function: pre, the loop (body scheduled, delay slot filled), post."""
    lp = loop(body, branch, label)
    from fpu_novu0 import EST as E
    EST[name] = E[-1]
    return (f'\n{comment}\n\t.p2align 3\n\t.globl\t{name}\n\t.ent\t{name}\n{name}:\n' +
            ''.join(f'\t{l}\n' for l in pre) + lp +
            ''.join(f'\t{l}\n' for l in post) + f'\t.end\t{name}\n')


def hash_tail(addr_lines, nbytes, epi=()):
    """The tail: restore, then app_hash of the output (a tail call)."""
    return list(epi) + addr_lines + ['j\tapp_hash', f'ori\t$5, $0, {nbytes}']


def prefs(D, ptrs, U=1):
    """PREF D lines ahead of each pointer (offset added), once per 64 bytes."""
    if not D:
        return []
    return [f'pref\t0, {64 * D + off}({p})' for p, off in ptrs]


# ---- 2D ----

def alpha_blend(name, U=2, D=4, wpref=False):
    """frame = (s * a + d * (256 - a)) >> 8 per channel, d's alpha: MMI on
    halfwords, pmulth s * a, pmaddh d * (256 - a), pmfhl.lh."""
    # $8: sprite pointer (frame at +0x4400), $9: end, $10: mask of the
    # alpha halfwords, $11: 256 in each halfword
    regs = ['$2', '$3', '$4', '$5', '$6', '$7', '$12', '$13', '$14', '$15',
            '$24', '$25', '$16', '$17', '$18', '$19', '$20', '$21']
    body = []
    for k in range(U):
        s, d, a, al, au, sl, su, dl, du = regs[9 * (k % 2):9 * (k % 2) + 9]
        o = 16 * k
        body += [f'lq\t{s}, {o}($8)', f'lq\t{d}, {o + 0x4400}($8)',
                 f'psrlw\t{a}, {s}, 24', f'pextlw\t{al}, {a}, {a}', f'pextuw\t{au}, {a}, {a}',
                 f'pcpyh\t{al}, {al}', f'pcpyh\t{au}, {au}',
                 f'pand\t{al}, {al}, $10', f'pand\t{au}, {au}, $10',
                 f'pextlb\t{sl}, $0, {s}', f'pextub\t{su}, $0, {s}',
                 f'pextlb\t{dl}, $0, {d}', f'pextub\t{du}, $0, {d}',
                 f'psubh\t{a}, $11, {al}', f'psubh\t{s}, $11, {au}',
                 f'pmulth\t$0, {sl}, {al}', f'pmaddh\t$0, {dl}, {a}', f'pmfhl.lh\t{sl}',
                 f'pmulth\t$0, {su}, {au}', f'pmaddh\t$0, {du}, {s}', f'pmfhl.lh\t{su}',
                 f'psrlh\t{sl}, {sl}, 8', f'psrlh\t{su}, {su}, 8',
                 f'ppacb\t{d}, {su}, {sl}', f'sq\t{d}, {o + 0x4400}($8)']
    lines = max(1, U // 4)
    if D:
        # one PREF of each stream per iteration (U = 4: once per line)
        body += [f'pref\t0, {64 * D}($8)', f'pref\t0, {64 * D + 0x4400}($8)']
    body += [f'addiu\t$8, $8, {16 * U}']
    used = set(r for l in body for r in sched.parse(l)[1] + sched.parse(l)[2])
    pro, epi = save(used)
    pre = pro + ['ADDR($8, gfx_mem, SPRITE)', 'addiu\t$9, $8, 16384',
                 # 0xffff in the r, g, b halfwords, 0 in alpha
                 'daddiu\t$10, $0, -1', 'dsrl\t$10, $10, 16', 'pcpyld\t$10, $10, $10',
                 'addiu\t$11, $0, 256', 'pcpyh\t$11, $11', 'pcpyld\t$11, $11, $11']
    return func(name, f'# {name}: frame = sprite over frame by its alpha\n'
                f'# MMI per 4 pixels: bytes to halfwords, alpha broadcast with pcpyh,\n'
                f'# pmulth s * a + pmaddh d * (256 - a) (the alpha halfwords 0 and 256:\n'
                f'# the result keeps d\'s alpha), pmfhl.lh, psrlh 8, ppacb. {U} blocks of\n'
                f'# 4 pixels per iteration, PREF {D} lines ahead.',
                pre, body, 'bne\t$8, $9, 1b',
                hash_tail(['ADDR($4, gfx_mem, FRAME)'], 16384, epi))


def clut8(name, mode='mmi', D=0):
    """frame[i] = palette[indexed[i]], 16 pixels per iteration.
    mode 'alu': the palette addresses by srl/andi/addu per pixel;
    'mmi': the 16 indices to word addresses with MMI (zero-extend, psllw 2,
    paddw base), each moved to a GPR by sll/dsra32 (sign-extended)."""
    pre = ['ADDR($8, gfx_mem, INDEXED)', 'ADDR($9, gfx_mem, FRAME)',
           'ADDR($10, gfx_mem, PALETTE)', 'addiu\t$11, $8, 4096']
    body = []
    if mode == 'alu':
        body += ['lw\t$12, 0($8)', 'lw\t$13, 4($8)', 'lw\t$14, 8($8)', 'lw\t$15, 12($8)']
        T = ['$2', '$3', '$4', '$5', '$6', '$7', '$24', '$25']
        for i in range(16):
            w = ['$12', '$13', '$14', '$15'][i // 4]
            b = i % 4
            t = T[i % 8]
            if b == 0:
                body.append(f'sll\t{t}, {w}, 2')
            else:
                body.append(f'srl\t{t}, {w}, {8 * b - 2}')
            body += [f'andi\t{t}, {t}, 0x3fc', f'addu\t{t}, {t}, $10',
                     f'lw\t{t}, 0({t})', f'sw\t{t}, {4 * i}($9)']
        pro, epi = [], []
    else:
        # $10: the palette base in each word
        pre += ['pextlw\t$10, $10, $10', 'pcpyld\t$10, $10, $10']
        body += ['lq\t$12, 0($8)', 'pextlb\t$13, $0, $12', 'pextub\t$12, $0, $12',
                 'pextlh\t$14, $0, $13', 'pextuh\t$13, $0, $13',
                 'pextlh\t$15, $0, $12', 'pextuh\t$12, $0, $12']
        W = ['$14', '$13', '$15', '$12']   # pixels 0-3, 4-7, 8-11, 12-15
        for w in W:
            body += [f'psllw\t{w}, {w}, 2', f'paddw\t{w}, {w}, $10']
        T = ['$2', '$3', '$4', '$5', '$6', '$7', '$24', '$25', '$16', '$17', '$18', '$19']
        U = ['$20', '$21', '$22', '$23']
        for q, w in enumerate(W):
            u = U[q]
            body.append(f'pcpyud\t{u}, {w}, {w}')
            for j in range(4):
                t = T[(4 * q + j) % 12]
                src = w if j < 2 else u
                body.append(f'sll\t{t}, {src}, 0' if j % 2 == 0 else f'dsra32\t{t}, {src}, 0')
                body += [f'lw\t{t}, 0({t})', f'sw\t{t}, {4 * (4 * q + j)}($9)']
        pro, epi = save(SAVED)
    if D:
        body += [f'pref\t0, {64 * D}($9)']
    body += ['addiu\t$8, $8, 16', 'addiu\t$9, $9, 64']
    return func(name, f'# {name}: frame = palette[indexed], 16 pixels per iteration\n'
                f'# ({mode}: ' + ('palette addresses by srl, andi, addu' if mode == 'alu' else
                                 'the indices to palette addresses with MMI, each to a GPR by\n'
                                 '# sll or dsra32') + f'), lw and sw per pixel, PREF of frame {D} lines ahead.',
                pro + pre, body, 'bne\t$8, $11, 1b',
                hash_tail(['ADDR($4, gfx_mem, FRAME)'], 16384, epi))


def select2(name, what, U=8, D=2, wpref=True):
    """color_key (scaled = s == KEY ? d : s) and alpha_test (scaled = s >> 24
    >= 0x40 ? s : d): MMI per 4 pixels, the select by pxor, pand, pxor."""
    regs = ['$2', '$3', '$4', '$5', '$6', '$7', '$12', '$13', '$14', '$15', '$24', '$25',
            '$16', '$17', '$18', '$19', '$20', '$21', '$22', '$23']
    body = []
    for k in range(U):
        s, d, m = regs[3 * (k % 6):3 * (k % 6) + 3]
        o = 16 * k
        body += [f'lq\t{s}, {o}($8)', f'lq\t{d}, {o + 0x4400}($8)']
        if what == 'key':
            # m = s == KEY; scaled = s ^ ((s ^ d) & m)
            body += [f'pceqw\t{m}, {s}, $10', f'pxor\t{d}, {s}, {d}', f'pand\t{d}, {d}, {m}',
                     f'pxor\t{s}, {s}, {d}', f'sq\t{s}, {o}($11)']
        else:
            # m = a > 0x3f; scaled = d ^ ((s ^ d) & m)
            body += [f'psrlw\t{m}, {s}, 24', f'pcgtw\t{m}, {m}, $10', f'pxor\t{s}, {s}, {d}',
                     f'pand\t{s}, {s}, {m}', f'pxor\t{d}, {d}, {s}', f'sq\t{d}, {o}($11)']
    nl = max(1, U // 4)
    if D:
        for l in range(nl):
            body += [f'pref\t0, {64 * (D + l)}($8)', f'pref\t0, {64 * (D + l) + 0x4400}($8)']
            if wpref:
                body.append(f'pref\t0, {64 * (D + l)}($11)')
    body += [f'addiu\t$8, $8, {16 * U}', f'addiu\t$11, $11, {16 * U}']
    used = set(r for l in body for r in sched.parse(l)[1] + sched.parse(l)[2])
    pro, epi = save(used)
    if what == 'key':
        const = ['lui\t$10, 0x00ff', 'ori\t$10, $10, 0x00ff', 'pextlw\t$10, $10, $10',
                 'pcpyld\t$10, $10, $10']
        desc = 'pceqw with the key'
    else:
        const = ['addiu\t$10, $0, 0x3f', 'pextlw\t$10, $10, $10', 'pcpyld\t$10, $10, $10']
        desc = 'psrlw 24, pcgtw 0x3f'
    pre = pro + ['ADDR($8, gfx_mem, SPRITE)', 'ADDR($11, gfx_mem, SCALED)',
                 'addiu\t$9, $8, 16384'] + const
    return func(name, f'# {name}: MMI per 4 pixels: {desc}, the select by pxor, pand, pxor.\n'
                f'# {U} blocks per iteration, PREF {D} lines ahead (sprite, frame' +
                (', scaled' if wpref else '') + ').',
                pre, body, 'bne\t$8, $9, 1b',
                hash_tail(['ADDR($4, gfx_mem, SCALED)'], 16384, epi))


def to_ct16(name, U=2, D=4):
    """frame16 = RGBA5551 of sprite: ppac5 per 4 pixels (exactly the C
    formula), ppach packs 8 to halfwords."""
    regs = ['$2', '$3', '$4', '$5', '$6', '$7', '$12', '$13', '$14', '$15', '$24', '$25']
    body = []
    for k in range(U):
        a, b = regs[2 * (k % 6):2 * (k % 6) + 2]
        body += [f'lq\t{a}, {32 * k}($8)', f'lq\t{b}, {32 * k + 16}($8)',
                 f'ppac5\t{a}, {a}', f'ppac5\t{b}, {b}', f'ppach\t{a}, {b}, {a}',
                 f'sq\t{a}, {16 * k}($9)']
    nl = max(1, U // 2)
    if D:
        body += [f'pref\t0, {64 * (D + l)}($8)' for l in range(nl)]
    body += [f'addiu\t$8, $8, {32 * U}', f'addiu\t$9, $9, {16 * U}']
    pre = ['ADDR($8, gfx_mem, SPRITE)', 'ADDR($9, gfx_mem, FRAME16)', 'addiu\t$10, $8, 16384']
    return func(name, f'# {name}: ppac5 (RGBA8888 to 1555 per word, the C formula) and ppach\n'
                f'# per 8 pixels; {U} x 8 pixels per iteration, PREF {D} lines ahead.',
                pre, body, 'bne\t$8, $10, 1b',
                hash_tail(['ADDR($4, gfx_mem, FRAME16)'], 8192))


def scale_nn(name, D=0):
    """scaled = sprite scaled 64 -> 96 with nearest neighbor (16.16 steps of
    43690): the columns are fixed per row, so a row is unrolled: each source
    pixel loaded once and stored 1 or 2 times. A row with the same source
    row as the previous one is a copy of it (lq, sq)."""
    step = (64 << 16) // 96
    col = [(x * step) >> 16 for x in range(96)]
    row = [(y * step) >> 16 for y in range(96)]
    T = ['$2', '$3', '$4', '$5', '$6', '$7', '$12', '$13', '$14', '$15', '$24', '$25']
    build = []
    for x in range(96):
        c = col[x]
        if x == 0 or col[x - 1] != c:
            build.append(f'lw\t{T[c % 12]}, {4 * c}($8)')
        build.append(f'sw\t{T[c % 12]}, {4 * x}($9)')
    copy = []
    for q in range(24):
        r = T[q % 12]
        copy += [f'lq\t{r}, {16 * q - 384}($9)', f'sq\t{r}, {16 * q}($9)']
    if D:
        build += [f'pref\t0, {64 * D}($9)']
    # the rows: a bitmask of the copied rows in $10:$11 is simpler as a
    # table; the rows alternate build, copy in a fixed pattern
    b_order = sched.schedule(build)
    c_order = sched.schedule(copy)
    pat = ''.join('c' if y and row[y] == row[y - 1] else 'b' for y in range(96))
    # pattern per 3 rows after row 0: b c b ... check it
    out = [f'\n# {name}: scaled = sprite 64x64 scaled to 96x96, nearest neighbor (16.16\n'
           f'# steps of 43690). The columns are the same for every row, so a row is\n'
           f'# unrolled: each source pixel loaded once (lw) and stored once or twice\n'
           f'# (sw). A row with the source row of the one before is a copy of it\n'
           f'# (lq, sq). Rows: {pat}',
           '\t.p2align 3', f'\t.globl\t{name}', f'\t.ent\t{name}', f'{name}:',
           '\tADDR($8, gfx_mem, SPRITE)', '\tADDR($9, gfx_mem, SCALED)',
           '\tori\t$10, $0, 36864', '\taddu\t$10, $10, $9', f'\tlui\t$11, %hi({name}_rows)', f'\taddiu\t$11, $11, %lo({name}_rows)']
    # nn_rows: one byte per row, 1 = copy; the source pointer advances by
    # the row step after a build
    out += ['\t.p2align 3', '1:\tlbu\t$1, 0($11)', '\taddiu\t$11, $11, 1', '\tbnez\t$1, 3f', '\tnop']
    out += ['\t.p2align 3', '2:'] + [f'\t{l}' for l in b_order]
    out += ['\tb\t4f', '\taddiu\t$8, $8, 256']
    out += ['\t.p2align 3', '3:'] + [f'\t{l}' for l in c_order]
    out += ['4:\taddiu\t$9, $9, 384', '\tbne\t$9, $10, 1b', '\tnop',
            '\tADDR($4, gfx_mem, SCALED)', '\tj\tapp_hash', '\tori\t$5, $0, 36864',
            f'\t.end\t{name}']
    # the source row of a build is the one after the previous build: check
    src = -1
    for y in range(96):
        if pat[y] == 'b':
            src += 1
            assert row[y] == src, (y, row[y], src)
    tbl = f'\t.section .rodata\n{name}_rows:\t.byte\t' + ', '.join('1' if p == 'c' else '0' for p in pat) + '\n\t.text\n'
    EST[name] = (len(b_order), len(c_order))
    return '\n'.join(out) + '\n' + tbl


# ---- 3D ----
# VU0 kernels: vf0 = (0, 0, 0, 1). FPU kernels: $f0-$f19 and the odd
# $f21-$f31 are free; the even $f20-$f30 are callee-saved (savef).

FSAVED = ['$f20', '$f22', '$f24', '$f26', '$f28', '$f30']
FREE_F = [f'$f{i}' for i in range(20)] + [f'$f{i}' for i in range(21, 32, 2)]


def savef(used):
    regs = [r for r in FSAVED if r in used]
    if not regs:
        return [], []
    n = (4 * len(regs) + 15) & ~15
    return ([f'addiu\t$sp, $sp, -{n}'] + [f'swc1\t{r}, {4 * k}($sp)' for k, r in enumerate(regs)],
            [f'lwc1\t{r}, {4 * k}($sp)' for k, r in enumerate(regs)] + [f'addiu\t$sp, $sp, {n}'])


def used_regs(lines):
    return set(r for l in lines for r in sched.parse(l)[1] + sched.parse(l)[2])


def rodata(label, floats):
    return (f'\t.section .rodata\n\t.p2align 4\n{label}:\t.float\t' +
            ', '.join(repr(float(f)) for f in floats) + '\n\t.text\n')


def la(reg, label):
    return [f'lui\t$1, %hi({label})', f'addiu\t{reg}, $1, %lo({label})']


def transpose_vf(rows, cols):
    """cols = the transpose of the 4 rows (VF registers), by MMI in $2, $3,
    $12-$15, $24, $25."""
    a, b, c, d, t0, t1, t2, t3 = '$2', '$3', '$12', '$13', '$14', '$15', '$24', '$25'
    out = [f'qmfc2\t{g}, {r}' for g, r in zip((a, b, c, d), rows)]
    out += [f'pextlw\t{t0}, {b}, {a}', f'pextuw\t{t1}, {b}, {a}',
            f'pextlw\t{t2}, {d}, {c}', f'pextuw\t{t3}, {d}, {c}',
            f'pcpyld\t{a}, {t2}, {t0}', f'pcpyud\t{b}, {t0}, {t2}',
            f'pcpyld\t{c}, {t3}, {t1}', f'pcpyud\t{d}, {t1}, {t3}']
    out += [f'qmtc2\t{g}, {r}' for g, r in zip((a, b, c, d), cols)]
    return out


def vfunc(name, comment, pre, body, branch, post, data=''):
    return data + func(name, comment, pre, body, branch, post)


def transform_vu0(name, D=0, W=True):
    """screen = transform of verts (3 matrix rows), / z, to 12.4 in the GS
    window: the matrix as columns c0-c3 in vf20-vf23 with rows 0 and 1
    scaled by 1024 and the w lane row 2 scaled by 16: R = (1024 x, 1024 y,
    z, 16 z); ACC = (32768.5, 32768.5, 0.5); x, y += R * Q, z += 65536 * Q."""
    C = f'{name}_c'
    pre = la('$8', 'matrix') + [
        # transpose the 3 rows to columns: c_j = (1024 m0j, 1024 m1j, m2j, 16 m2j)
        'lqc2\t$vf1, 0($8)', 'lqc2\t$vf2, 16($8)', 'lqc2\t$vf3, 32($8)'] + la('$9', C) + [
        'lqc2\t$vf4, 0($9)',            # (1024, 1024, 1, 16)
        'lqc2\t$vf5, 16($9)',           # (32768.5, 32768.5, 0.5, 0)
        'lqc2\t$vf6, 32($9)',           # (0, 0, 65536, 0)
        'vmulx.xyzw\t$vf1, $vf1, $vf4x', 'vmuly.xyzw\t$vf2, $vf2, $vf4y',
        'vmulw.xyzw\t$vf7, $vf3, $vf4w']
    pre += transpose_vf(['$vf1', '$vf2', '$vf3', '$vf7'], ['$vf20', '$vf21', '$vf22', '$vf23'])
    pre += ['ADDR($8, geom_mem, VERTS)', 'ADDR($9, geom_mem, SCREEN)', 'addiu\t$10, $8, 8192']
    body = []
    for k in range(2):
        v, r = f'$vf{8 + 2 * k}', f'$vf{9 + 2 * k}'
        o = 16 * k
        body += [f'lqc2\t{v}, {o}($8)',
                 f'vmulax.xyzw\t$ACC, $vf20, {v}x', f'vmadday.xyzw\t$ACC, $vf21, {v}y',
                 f'vmaddaz.xyzw\t$ACC, $vf22, {v}z', f'vmaddw.xyzw\t{r}, $vf23, {v}w',
                 f'vdiv\t$Q, $vf0w, {v}z', 'vwaitq',
                 'vmulaw.xyz\t$ACC, $vf5, $vf0w',
                 f'vmaddq.xy\t{r}, {r}, $Q', f'vmaddq.z\t{r}, $vf6, $Q',
                 f'vftoi0.xyzw\t{r}, {r}', f'sqc2\t{r}, {o}($9)']
    if D:
        body += [f'pref\t0, {64 * D}($8)'] + ([f'pref\t0, {64 * D}($9)'] if W else [])
    body += ['addiu\t$8, $8, 32', 'addiu\t$9, $9, 32']
    return vfunc(name, f'# {name}: VU0, the 3 rows as columns scaled for the 12.4 window\n'
                 f'# (rows 0 and 1 by 1024 = 64 * 16, the w lane row 2 by 16), vdiv per\n'
                 f'# vertex, then (x, y) * Q and 65536 * Q added to ACC = (32768.5,\n'
                 f'# 32768.5, 0.5), vftoi0. 2 vertices per iteration, PREF {D} lines ahead.',
                 pre, body, 'bne\t$8, $10, 1b',
                 hash_tail(['ADDR($4, geom_mem, SCREEN)'], 8192),
                 rodata(C, [1024, 1024, 1, 16, 32768.5, 32768.5, 0.5, 0, 0, 0, 65536, 0]))


def transform_fpu(name, D=0, W=False):
    """The FPU version: rows 0 and 1 scaled by 1024 and row 2 by 16 (pad =
    16 z) in 12 registers; software-pipelined by 2: the loads and the div.s
    of the next vertex in the iteration of this one."""
    C = f'{name}_c'
    M = [f'$f{i}' for i in range(12)]
    Cc, H, B, ONE = '$f12', '$f13', '$f14', '$f15'
    S = [['$f16', '$f17', '$f18', '$f19', '$f21'], ['$f23', '$f25', '$f27', '$f29', '$f31']]
    X, Y, P = '$f20', '$f22', '$f24'
    pre = la('$8', 'matrix') + la('$9', C)
    for r in range(3):
        for j in range(4):
            pre.append(f'lwc1\t{M[4 * r + j]}, {16 * r + 4 * j}($8)')
    pre += [f'lwc1\t{Cc}, 0($9)', f'lwc1\t{H}, 4($9)', f'lwc1\t{B}, 8($9)', f'lwc1\t{ONE}, 12($9)',
            f'lwc1\t$f16, 16($9)', f'lwc1\t$f17, 20($9)']        # 1024, 16
    for j in range(4):
        pre += [f'mul.s\t{M[j]}, {M[j]}, $f16', f'mul.s\t{M[4 + j]}, {M[4 + j]}, $f16',
                f'mul.s\t{M[8 + j]}, {M[8 + j]}, $f17']

    def A(s, off):
        vx, vy, vz, vw, iw = s
        return [f'lwc1\t{vx}, {off}($10)', f'lwc1\t{vy}, {off + 4}($10)',
                f'lwc1\t{vz}, {off + 8}($10)', f'lwc1\t{vw}, {off + 12}($10)',
                f'div.s\t{iw}, {ONE}, {vz}']

    def Bst(s, off):
        vx, vy, vz, vw, iw = s
        out = []
        for r, d in enumerate((X, Y, P)):
            m = M[4 * r:4 * r + 4]
            out += [f'mula.s\t{m[0]}, {vx}', f'madda.s\t{m[1]}, {vy}', f'madda.s\t{m[2]}, {vz}',
                    f'madd.s\t{d}, {m[3]}, {vw}']
        out += [f'mula.s\t{Cc}, {ONE}', f'madd.s\t{X}, {X}, {iw}',
                f'mula.s\t{Cc}, {ONE}', f'madd.s\t{Y}, {Y}, {iw}',
                f'mula.s\t{H}, {ONE}', f'madd.s\t{vx}, {iw}, {B}']
        out += [f'cvt.w.s\t{d}, {d}' for d in (X, Y, vx, P)]
        out += [f'swc1\t{X}, {off}($11)', f'swc1\t{Y}, {off + 4}($11)',
                f'swc1\t{vx}, {off + 8}($11)', f'swc1\t{P}, {off + 12}($11)']
        return out
    body = A(S[1], 16) + Bst(S[0], 0) + A(S[0], 32) + Bst(S[1], 16)
    if D:
        body += [f'pref\t0, {64 * D}($10)'] + ([f'pref\t0, {64 * D}($11)'] if W else [])
    body += ['addiu\t$10, $10, 32', 'addiu\t$11, $11, 32']
    pro, epi = savef(used_regs(body))
    pre = pro + pre + ['ADDR($10, geom_mem, VERTS)', 'ADDR($11, geom_mem, SCREEN)',
                       'addiu\t$12, $10, 8192'] + A(S[0], 0)
    return vfunc(name, f'# {name}: FPU, the rows scaled for the 12.4 window (rows 0 and 1\n'
                 f'# by 1024 = 64 * 16, row 2 by 16 for pad): x, y and pad as chains through\n'
                 f'# ACC (mula.s, madda.s, madd.s), sx = 32768.5 + x * iw and sz = 0.5 +\n'
                 f'# 65536 * iw by mula.s and madd.s; 2 vertices per iteration, the loads and\n'
                 f'# div.s of the next vertex in the iteration before (software pipelined).\n'
                 f'# PREF {D} lines ahead.',
                 pre, body, 'bne\t$10, $12, 1b',
                 hash_tail(['ADDR($4, geom_mem, SCREEN)'], 8192, epi),
                 rodata(C, [32768.5, 0.5, 65536, 1, 1024, 16]))


LIGHT_DIR = [[0.5, 0.5, -0.5, 0.0], [-0.75, 0.25, 0.0, 0.0], [0.0, -0.5, 0.75, 0.0]]
LIGHT_DIR_V = [[0.5, 0.5, -0.5, 1.0], [-0.75, 0.25, 0.0, 0.5], [0.0, -0.5, 0.75, 0.75]]
LIGHT_COL = [[0.5, 0.375, 0.25, 0.0], [0.125, 0.25, 0.5, 0.0], [0.25, 0.25, 0.125, 0.0]]


def light_consts(v4sf):
    """The lights for both lighting kernels: dir (xyz) and col scaled by 128
    and, for lighting_v4sf, by the light's intensity (dir.w)."""
    dirs = LIGHT_DIR_V if v4sf else LIGHT_DIR
    cols = [[c * 128 * (d[3] if v4sf else 1.0) for c in col[:3]] for col, d in zip(LIGHT_COL, dirs)]
    return [d[:3] for d in dirs], cols


def lighting_vu0(name, v4sf, D=0):
    """colors from normals with 3 lights on VU0: the 3 dots at once (the
    directions as columns), max with 0, the color as a chain through ACC
    from (16, 16, 16, 128) = 128 * the ambient and alpha, the lights' colors
    scaled by 128 (and the intensity), min with 128, vftoi0; 4 vertices
    packed to bytes with ppach, ppacb."""
    dirs, cols = light_consts(v4sf)
    C = f'{name}_c'
    data = []
    for j in range(3):                       # Lx, Ly, Lz: lane l = dir[l][j]
        data += [dirs[0][j], dirs[1][j], dirs[2][j], 0.0]
    for l in range(3):
        data += cols[l] + [0.0]
    data += [16, 16, 16, 128, 128, 128, 128, 128]
    pre = la('$8', C) + [f'lqc2\t$vf{20 + k}, {16 * k}($8)' for k in range(8)]
    pre += ['ADDR($8, geom_mem, NORMALS)', 'ADDR($9, geom_mem, COLORS)', 'addiu\t$10, $8, 8192']
    body = []
    G = ['$2', '$3', '$4', '$5']
    for k in range(4):
        n, d, r = f'$vf{1 + 3 * k}', f'$vf{2 + 3 * k}', f'$vf{3 + 3 * k}'
        body += [f'lqc2\t{n}, {16 * k}($8)',
                 f'vmulax.xyz\t$ACC, $vf20, {n}x', f'vmadday.xyz\t$ACC, $vf21, {n}y',
                 f'vmaddz.xyz\t{d}, $vf22, {n}z', f'vmax.xyz\t{d}, {d}, $vf0',
                 'vmulaw.xyzw\t$ACC, $vf26, $vf0w', f'vmaddax.xyzw\t$ACC, $vf23, {d}x',
                 f'vmadday.xyzw\t$ACC, $vf24, {d}y', f'vmaddz.xyzw\t{r}, $vf25, {d}z',
                 f'vmini.xyzw\t{r}, {r}, $vf27', f'vftoi0.xyzw\t{r}, {r}', f'qmfc2\t{G[k]}, {r}']
    body += ['ppach\t$6, $3, $2', 'ppach\t$7, $5, $4', 'ppacb\t$6, $7, $6', 'sq\t$6, 0($9)']
    if D:
        body += [f'pref\t0, {64 * D}($8)']
    body += ['addiu\t$8, $8, 64', 'addiu\t$9, $9, 16']
    return vfunc(name, f'# {name}: VU0 per vertex: the 3 dots with the directions as columns\n'
                 f'# (vmulax, vmadday, vmaddz), vmax 0, the color as a chain through ACC from\n'
                 f'# (16, 16, 16, 128) with the colors scaled by 128' + (' and the intensity' if v4sf else '') +
                 f',\n# vmini 128, vftoi0; 4 colors packed by ppach and ppacb, one sq. PREF {D} lines ahead.',
                 pre, body, 'bne\t$8, $10, 1b',
                 hash_tail(['ADDR($4, geom_mem, COLORS)'], 2048), rodata(C, data))


def lighting_fpu(name, v4sf, D=0):
    """The FPU version: per light a dot (mula.s, madda.s, madd.s) and max.s
    with 0; per channel a chain through ACC from 16 (mula.s 16, 1), min.s
    128, cvt.w.s and mfc1; the channels packed in GPRs."""
    dirs, cols = light_consts(v4sf)
    C = f'{name}_c'
    data = [x for d in dirs for x in d] + [x for c in cols for x in c] + [16, 1, 0, 128]
    Dr = [f'$f{i}' for i in range(9)]                 # dirs
    Cr = [f'$f{i}' for i in range(9, 18)]             # cols
    B16, ONE, Z, K = '$f18', '$f19', '$f21', '$f23'
    N = ['$f25', '$f27', '$f29']
    Dd = ['$f31', '$f20', '$f22']
    R = ['$f24', '$f26', '$f28']
    pre = la('$8', C) + [f'lwc1\t{r}, {4 * k}($8)' for k, r in enumerate(Dr + Cr + [B16, ONE, Z, K])]
    body = [f'lwc1\t{N[j]}, {4 * j}($8)' for j in range(3)]
    for l in range(3):
        d = Dr[3 * l:3 * l + 3]
        body += [f'mula.s\t{N[0]}, {d[0]}', f'madda.s\t{N[1]}, {d[1]}', f'madd.s\t{Dd[l]}, {N[2]}, {d[2]}',
                 f'max.s\t{Dd[l]}, {Dd[l]}, {Z}']
    for ch in range(3):
        body += [f'mula.s\t{B16}, {ONE}'] + [f'madda.s\t{Dd[l]}, {Cr[3 * l + ch]}' for l in range(2)] + \
                [f'madd.s\t{R[ch]}, {Dd[2]}, {Cr[6 + ch]}', f'min.s\t{R[ch]}, {R[ch]}, {K}',
                 f'cvt.w.s\t{R[ch]}, {R[ch]}', f'mfc1\t${2 + ch}, {R[ch]}']
    body += ['sll\t$3, $3, 8', 'sll\t$4, $4, 16', 'or\t$2, $2, $3', 'or\t$2, $2, $4',
             'or\t$2, $2, $11', 'sw\t$2, 0($9)'] + ([f'pref\t0, {64 * D}($8)'] if D else []) + [
             'addiu\t$8, $8, 16', 'addiu\t$9, $9, 4']
    pro, epi = savef(used_regs(body))
    pre = pro + pre + ['ADDR($8, geom_mem, NORMALS)', 'ADDR($9, geom_mem, COLORS)',
                       'addiu\t$10, $8, 8192', 'lui\t$11, 0x8000']
    return vfunc(name, f'# {name}: FPU per vertex: a dot per light (mula.s, madda.s, madd.s)\n'
                 f'# and max.s 0; per channel a chain through ACC from 16 = 128 * the ambient,\n'
                 f'# the colors scaled by 128' + (' and the intensity' if v4sf else '') +
                 f', min.s 128, cvt.w.s, mfc1; packed by sll and or.\n# PREF {D} lines ahead.',
                 pre, body, 'bne\t$8, $10, 1b',
                 hash_tail(['ADDR($4, geom_mem, COLORS)'], 2048, epi), rodata(C, data))


def stream_v(name, what, unit, U=4, D=0, W=False):
    """field_scale (n * (v.y * n.z)), field_offset (n + v.y * n.z), project
    (v * (1 / v.z)), on VU0 ('vu0') or the FPU ('fpu'). U vertices per
    iteration, PREF D lines ahead of the inputs."""
    pre = ['ADDR($8, geom_mem, VERTS)', 'ADDR($9, geom_mem, NORMALS)',
           'ADDR($10, geom_mem, SCALED_V)', 'addiu\t$11, $8, 8192']
    body = []
    ins = [('$8', 0)] if what == 'project' else [('$8', 0), ('$9', 0)]
    for k in range(U):
        o = 16 * k
        if unit == 'vu0':
            v, n, t = f'$vf{1 + 3 * k}', f'$vf{2 + 3 * k}', f'$vf{3 + 3 * k}'
            if what == 'project':
                body += [f'lqc2\t{v}, {o}($8)', f'vdiv\t$Q, $vf0w, {v}z', 'vwaitq',
                         f'vmulq.xyzw\t{t}, {v}, $Q', f'sqc2\t{t}, {o}($10)']
            else:
                op = 'vmuly' if what == 'scale' else 'vaddy'
                body += [f'lqc2\t{v}, {o}($8)', f'lqc2\t{n}, {o}($9)', f'vmulz.y\t{t}, {v}, {n}z',
                         f'{op}.xyzw\t{t}, {n}, {t}y', f'sqc2\t{t}, {o}($10)']
        else:
            F = [f'$f{i}' for i in range(20)] + [f'$f{i}' for i in range(21, 32, 2)]
            s = F[6 * (k % 4):6 * (k % 4) + 6]
            if what == 'project':
                v, t = s[:4], s[4]
                body += [f'lwc1\t{v[j]}, {o + 4 * j}($8)' for j in range(4)]
                body += [f'div.s\t{t}, {ONE_F}, {v[2]}']
                body += [f'mul.s\t{v[j]}, {v[j]}, {t}' for j in range(4)]
                body += [f'swc1\t{v[j]}, {o + 4 * j}($10)' for j in range(4)]
            else:
                n, vy, t = s[:4], s[4], s[5]
                op = 'mul.s' if what == 'scale' else 'add.s'
                body += [f'lwc1\t{vy}, {o + 4}($8)'] + [f'lwc1\t{n[j]}, {o + 4 * j}($9)' for j in range(4)]
                body += [f'mul.s\t{t}, {vy}, {n[2]}'] + [f'{op}\t{n[j]}, {n[j]}, {t}' for j in range(4)]
                body += [f'swc1\t{n[j]}, {o + 4 * j}($10)' for j in range(4)]
    nl = max(1, U // 4)
    if D:
        for l in range(nl):
            body += [f'pref\t0, {64 * (D + l)}({p})' for p, _ in ins + ([('$10', 0)] if W else [])]
    body += [f'addiu\t$8, $8, {16 * U}', f'addiu\t$9, $9, {16 * U}', f'addiu\t$10, $10, {16 * U}']
    pro, epi = savef(used_regs(body))
    if unit == 'fpu' and what == 'project':
        pre = ['lui\t$1, 0x3f80', f'mtc1\t$1, {ONE_F}'] + pre
    desc = {'scale': 'n * (v.y * n.z)', 'offset': 'n + v.y * n.z', 'project': 'v * (1 / v.z)'}[what]
    out = {'scale': 'SCALED_V', 'offset': 'SCALED_V', 'project': 'SCALED_V'}[what]
    return func(name, f'# {name}: {desc} on {"VU0" if unit == "vu0" else "the FPU"}, {U} vertices per\n'
                f'# iteration, PREF {D} lines ahead.',
                pro + pre, body, 'bne\t$8, $11, 1b',
                hash_tail([f'ADDR($4, geom_mem, {out})'], 8192, epi))


ONE_F = '$f31'


def weighted_sum(name, unit, U=4, D=0):
    """group_sum[g] = sum of verts[i] * normals[i] over each group of
    geom_group_len vertices (a multiple of U). VU0: one chain through ACC
    (vsuba to clear it, vmadda per vertex: 1 cycle a step); FPU: x through
    ACC (madda.s), y, z, w with mul.s and add.s."""
    pre = ['ADDR($8, geom_mem, VERTS)', 'ADDR($9, geom_mem, NORMALS)'] + la('$10', 'group_sum') + [
        'lui\t$1, %hi(geom_group_len)', 'lw\t$12, %lo(geom_group_len)($1)', 'addiu\t$11, $0, 16']
    body = []
    if unit == 'vu0':
        for k in range(U):
            v, n = f'$vf{1 + 2 * k}', f'$vf{2 + 2 * k}'
            body += [f'lqc2\t{v}, {16 * k}($8)', f'lqc2\t{n}, {16 * k}($9)',
                     f'vmadda.xyzw\t$ACC, {v}, {n}']
        start = ['vsuba.xyzw\t$ACC, $vf0, $vf0']
        end = ['vmaddx.xyzw\t$vf20, $vf0, $vf0x', 'sqc2\t$vf20, 0($10)']
        pro, epi = [], []
    else:
        S = ['$f16', '$f17', '$f18']          # y, z, w sums
        for k in range(U):
            r = [f'$f{(8 * k + j) % 16}' for j in range(8)]
            body += [f'lwc1\t{r[j]}, {16 * k + 4 * j}($8)' for j in range(4)]
            body += [f'lwc1\t{r[4 + j]}, {16 * k + 4 * j}($9)' for j in range(4)]
            body += [f'madda.s\t{r[0]}, {r[4]}']
            body += [f'mul.s\t{r[1 + j]}, {r[1 + j]}, {r[5 + j]}' for j in range(3)]
            body += [f'add.s\t{S[j]}, {S[j]}, {r[1 + j]}' for j in range(3)]
        start = ['mtc1\t$0, $f19', 'mula.s\t$f19, $f19', 'mov.s\t$f16, $f19', 'mov.s\t$f17, $f19',
                 'mov.s\t$f18, $f19']
        end = ['madd.s\t$f19, $f19, $f19', 'swc1\t$f19, 0($10)', 'swc1\t$f16, 4($10)',
               'swc1\t$f17, 8($10)', 'swc1\t$f18, 12($10)']
        pro, epi = [], []
    if D:
        body += [f'pref\t0, {64 * D}($8)', f'pref\t0, {64 * D}($9)']
    body += [f'addiu\t$8, $8, {16 * U}', f'addiu\t$9, $9, {16 * U}', f'addiu\t$13, $13, -{U}']
    lp = loop(body, 'bgtz\t$13, 2b', '2')
    from fpu_novu0 import EST as E
    EST[name] = E[-1]
    lines = pre + ['.p2align 3', '1:'] + start + ['blez\t$12, 3f', 'move\t$13, $12']
    out = f'\n# {name}: the sum of verts * normals per group, ' + (
        'VU0: a chain through ACC (vsuba clears it, vmadda per vertex)' if unit == 'vu0' else
        'FPU: x through ACC (madda.s),\n# y, z and w by mul.s and add.s') + \
        f', {U} vertices per\n# iteration (the group length a multiple of {U}), PREF {D} lines ahead.\n'
    out += f'\t.p2align 3\n\t.globl\t{name}\n\t.ent\t{name}\n{name}:\n'
    out += ''.join(f'\t{l}\n' if not l.endswith(':') else f'{l}' for l in lines)
    out += lp
    out += '3:' + ''.join(f'\t{l}\n' for l in end + ['addiu\t$11, $11, -1', 'bnez\t$11, 1b',
                                                      'addiu\t$10, $10, 16'] +
                          la('$4', 'group_sum') + ['j\tapp_hash', 'ori\t$5, $0, 256'])
    return out + f'\t.end\t{name}\n'


def mxv(name, form, unit, D=0, W=False):
    """mxv_row4 / col4 (m $4, in $5, out $6, n $7) and mxv_row3 / col3 (m $4,
    in $5, n $6; out screen, 12.4: x, y, z written, pad kept)."""
    four = form.endswith('4')
    rows = form.startswith('row')
    nreg = '$7' if four else '$6'
    out_p = '$6' if four else '$8'
    pre = [] if four else ['ADDR($8, geom_mem, SCREEN)']
    body = []
    if unit == 'vu0':
        pre += [f'lqc2\t$vf{1 + j}, {16 * j}($4)' for j in range(4)]
        if rows:
            pre += transpose_vf(['$vf1', '$vf2', '$vf3', '$vf4'], ['$vf20', '$vf21', '$vf22', '$vf23'])
        else:
            pre += [f'vmove.xyzw\t$vf{20 + j}, $vf{1 + j}' for j in range(4)]
        if not four:
            # 12.4: the columns scaled by 16
            pre += ['lui\t$1, 0x4180', 'qmtc2\t$1, $vf5'] + \
                   [f'vmulx.xyzw\t$vf{20 + j}, $vf{20 + j}, $vf5x' for j in range(4)]
        for k in range(4):
            v, r = f'$vf{1 + 2 * k}', f'$vf{2 + 2 * k}'
            o = 16 * k
            if four:
                body += [f'lqc2\t{v}, {o}($5)', f'vmulax.xyzw\t$ACC, $vf20, {v}x',
                         f'vmadday.xyzw\t$ACC, $vf21, {v}y', f'vmaddaz.xyzw\t$ACC, $vf22, {v}z',
                         f'vmaddw.xyzw\t{r}, $vf23, {v}w', f'sqc2\t{r}, {o}($6)']
            else:
                body += [f'lqc2\t{r}, {o}($8)', f'lqc2\t{v}, {o}($5)', f'vmulax.xyz\t$ACC, $vf20, {v}x',
                         f'vmadday.xyz\t$ACC, $vf21, {v}y', f'vmaddaz.xyz\t$ACC, $vf22, {v}z',
                         f'vmaddw.xyz\t{r}, $vf23, $vf0w', f'vftoi0.xyz\t{r}, {r}', f'sqc2\t{r}, {o}($8)']
        U = 4
    else:
        # the matrix in $f0-$f15 as m[r][j] = row r: for row forms out_r =
        # sum_j m[r][j] v_j; column forms out_r = sum_j m[j][r] v_j
        M = [f'$f{i}' for i in range(16)]
        pre += [f'lwc1\t{M[i]}, {4 * i}($4)' for i in range(16 if four else 16)]
        if not four:
            pre += ['lui\t$1, 0x4180', 'mtc1\t$1, $f16'] + [f'mul.s\t{M[i]}, {M[i]}, $f16' for i in range(16)] + \
                   ['lui\t$1, 0x3f80', 'mtc1\t$1, $f17']
        ONE = '$f17'

        def m(r, j):
            return M[4 * r + j] if rows else M[4 * j + r]
        if four:
            SETS = [['$f16', '$f17', '$f18', '$f19', '$f21', '$f23', '$f25', '$f27'],
                    ['$f20', '$f22', '$f24', '$f26', '$f28', '$f30', '$f29', '$f31']]
            U = 2
        else:
            SETS = [['$f18', '$f19', '$f21', '$f23', '$f25', '$f27'],
                    ['$f20', '$f22', '$f24', '$f26', '$f28', '$f29']]
            U = 2
        for k in range(U):
            s = SETS[k]
            o = 16 * k
            nin = 4 if four else 3
            v = s[:nin]
            res = s[nin:nin + (4 if four else 3)] if four else s[3:6]
            body += [f'lwc1\t{v[j]}, {o + 4 * j}($5)' for j in range(nin)]
            for r in range(4 if four else 3):
                if four:
                    body += [f'mula.s\t{m(r, 0)}, {v[0]}', f'madda.s\t{m(r, 1)}, {v[1]}',
                             f'madda.s\t{m(r, 2)}, {v[2]}', f'madd.s\t{res[r]}, {m(r, 3)}, {v[3]}']
                else:
                    body += [f'mula.s\t{m(r, 0)}, {v[0]}', f'madda.s\t{m(r, 1)}, {v[1]}',
                             f'madda.s\t{m(r, 2)}, {v[2]}', f'madd.s\t{res[r]}, {m(r, 3)}, {ONE}',
                             f'cvt.w.s\t{res[r]}, {res[r]}']
            body += [f'swc1\t{res[r]}, {o + 4 * r}({out_p})' for r in range(4 if four else 3)]
    if D:
        body += [f'pref\t0, {64 * D}($5)']
        if unit == 'vu0' and not four or W:
            body += [f'pref\t0, {64 * D}({out_p})']
    body += [f'addiu\t$5, $5, {16 * U}', f'addiu\t{out_p}, {out_p}, {16 * U}', f'addiu\t{nreg}, {nreg}, -{U}']
    pro, epi = savef(used_regs(body)) if unit == 'fpu' else ([], [])
    what = {'row4': 'the 4 rows of m dotted with in[i]', 'col4': 'the 4 columns of m scaled by in[i]',
            'row3': 'the 3x4 rows of m (translation in w), 12.4', 'col3': 'the columns of m, translation in column 3, 12.4'}[form]
    how = ('VU0: the matrix as columns (rows transposed by MMI), vmulax, vmadday, vmaddaz, vmaddw'
           if unit == 'vu0' else 'FPU: each output a chain through ACC (mula.s, madda.s, madd.s)')
    if not four:
        how += ', the matrix scaled by 16'
        if unit == 'vu0':
            how += '; the old pad loaded with the output, the xyz lanes written'
    f = func(name, f'# {name}: {what}\n# {how}; {U} vertices per iteration, PREF {D} lines ahead.\n# n is a multiple of {U}.',
             pro + pre + [f'blez\t{nreg}, 2f', 'nop'], body, f'bgtz\t{nreg}, 1b', [])
    # the exit (also for n <= 0) after the loop
    return f.replace(f'\t.end\t{name}\n', '') + '2:' + ''.join(f'\t{l}\n' for l in epi + ['jr\t$ra', 'nop']) + \
        f'\t.end\t{name}\n'


def mxv_wrapper(name, kernel, form):
    """u32 (void) for the harness: kernel(geom_model_view, verts, ...), then
    app_hash of the output."""
    four = form.endswith('4')
    out = ['addiu\t$sp, $sp, -16', 'sd\t$ra, 0($sp)'] + la('$4', 'geom_model_view') + \
          ['ADDR($5, geom_mem, VERTS)']
    if four:
        out += ['ADDR($6, geom_mem, XFORMED)', 'jal\t' + kernel, 'addiu\t$7, $0, 512']
    else:
        out += ['jal\t' + kernel, 'addiu\t$6, $0, 512']
    out += ['ld\t$ra, 0($sp)', 'addiu\t$sp, $sp, 16',
            f'ADDR($4, geom_mem, {"XFORMED" if four else "SCREEN"})', 'j\tapp_hash', 'ori\t$5, $0, 8192']
    return f'\t.p2align 3\n\t.globl\t{name}\n\t.ent\t{name}\n{name}:\n' + ''.join(f'\t{l}\n' for l in out) + f'\t.end\t{name}\n'


def backface_cull(name, D=0):
    """The strip's triangles in pairs (i even: a, b, c = i, i+1, i+2; i+1:
    i+1, i+3, i+2): per triangle the edges by psubw, each sign-extended to
    doublewords for pmultw (psraw, pextlw; f rotated to fy, fx by prot3w,
    pexcw), one pmultw gives ex * fy and ey * fx (64 bits), dsubu and slt.
    The corners are always stored; the output pointer advances by 6 bytes
    when the triangle is kept (movn)."""
    pre = ['ADDR($8, geom_mem, SCREEN)', 'ADDR($9, geom_mem, VISIBLE)', 'move\t$2, $9',
           'move\t$10, $0', 'addiu\t$11, $0, 1', 'addiu\t$12, $0, 2', 'addiu\t$13, $0, 3',
           'addiu\t$3, $8, 8160']   # 255 pairs
    I = ['$10', '$11', '$12', '$13']
    body = [f'lq\t$14, 0($8)', 'lq\t$15, 16($8)', 'lq\t$24, 32($8)', 'lq\t$25, 48($8)']
    tris = [('$14', '$15', '$24', 'gt', (0, 1, 2)), ('$15', '$24', '$25', 'lt', (1, 3, 2))]
    T = [['$4', '$5', '$6', '$7', '$16', '$17'], ['$18', '$19', '$20', '$21', '$22', '$23']]
    for k, (A, B, Cc, sign, idx) in enumerate(tris):
        e, f, se, sf, p, q = T[k]
        body += [f'psubw\t{e}, {B}, {A}', f'psubw\t{f}, {Cc}, {A}',
                 f'psraw\t{se}, {e}, 31', f'pextlw\t{e}, {se}, {e}',
                 f'prot3w\t{f}, {f}', f'pexcw\t{f}, {f}', f'psraw\t{sf}, {f}, 31', f'pextlw\t{f}, {sf}, {f}',
                 f'pmultw\t{p}, {e}, {f}', f'pcpyud\t{q}, {p}, {p}', f'dsubu\t{p}, {p}, {q}',
                 (f'slt\t{p}, $0, {p}' if sign == 'gt' else f'slt\t{p}, {p}, $0'),
                 f'sh\t{I[idx[0]]}, 0($2)', f'sh\t{I[idx[1]]}, 2($2)', f'sh\t{I[idx[2]]}, 4($2)',
                 f'addiu\t{q}, $2, 6', f'movn\t$2, {q}, {p}']
    body += [f'addiu\t{r}, {r}, 2' for r in I]
    if D:
        body += [f'pref\t0, {64 * D}($8)']
    body += ['addiu\t$8, $8, 32']
    # the stores and the pointer of triangle 1 after those of triangle 0
    pro, epi = save(used_regs(body))
    post = ['subu\t$16, $2, $9', 'srl\t$16, $16, 1']   # n corners
    return func(name, f'# {name}: MMI per triangle: the edges (psubw), sign-extended to\n'
                f'# doublewords (psraw, pextlw; f rotated by prot3w, pexcw), pmultw gives ex * fy\n'
                f'# and ey * fx, dsubu, slt; the corners always stored, the pointer advanced\n'
                f'# by movn. 2 triangles per iteration (the odd one with b and c swapped).',
                pro + ['addiu\t$sp, $sp, -16', 'sd\t$ra, 0($sp)'] + pre, body, 'bne\t$8, $3, 1b',
                post + ['move\t$4, $9', 'jal\tapp_hash', 'sll\t$5, $16, 1', 'xor\t$2, $2, $16',
                        'ld\t$ra, 0($sp)', 'addiu\t$sp, $sp, 16'] + epi + ['jr\t$ra', 'nop'])


def face_normal(name, unit, vec, D=0, W=False):
    """face_normals[i] = (v[i+1] - v[i]) x (v[i+2] - v[i]), w = 0: 3
    triangles per iteration, each vertex loaded once (the registers rotate).
    VU0: vsub, vopmula, vopmsub (the w lane of the outputs stays 0); FPU:
    sub.s, then mula.s and msub.s per component."""
    out = 'FACE_NORMALS_V' if vec else 'FACE_NORMALS'
    pre = ['ADDR($8, geom_mem, VERTS)', f'ADDR($9, geom_mem, {out})', 'addiu\t$10, $0, 170']
    body = []
    if unit == 'vu0':
        V = ['$vf1', '$vf2', '$vf3']
        pre += ['lqc2\t$vf1, 0($8)', 'lqc2\t$vf2, 16($8)'] + \
               [f'vsub.xyzw\t$vf{10 + k}, $vf0, $vf0' for k in range(3)]
        for k in range(3):
            a, b, c = V[k % 3], V[(k + 1) % 3], V[(k + 2) % 3]
            e, f, n = f'$vf{4 + 2 * k}', f'$vf{5 + 2 * k}', f'$vf{10 + k}'
            body += [f'lqc2\t{c}, {16 * k + 32}($8)', f'vsub.xyz\t{e}, {b}, {a}', f'vsub.xyz\t{f}, {c}, {a}',
                     f'vopmula.xyz\t$ACC, {e}, {f}', f'vopmsub.xyz\t{n}, {f}, {e}', f'sqc2\t{n}, {16 * k}($9)']
    else:
        V = [['$f0', '$f1', '$f2'], ['$f3', '$f4', '$f5'], ['$f6', '$f7', '$f8']]
        pre += [f'lwc1\t{V[0][j]}, {4 * j}($8)' for j in range(3)] + \
               [f'lwc1\t{V[1][j]}, {16 + 4 * j}($8)' for j in range(3)]
        T = [['$f9', '$f10', '$f11', '$f12', '$f13', '$f14', '$f15', '$f16', '$f17'],
             ['$f18', '$f19', '$f21', '$f23', '$f25', '$f27', '$f29', '$f31', '$f9']]
        for k in range(3):
            a, b, c = V[k % 3], V[(k + 1) % 3], V[(k + 2) % 3]
            t = T[k % 2] if k < 2 else T[0]
            ex, ey, ez, fx, fy, fz, nx, ny, nz = t
            body += [f'lwc1\t{c[j]}, {16 * k + 32 + 4 * j}($8)' for j in range(3)]
            body += [f'sub.s\t{ex}, {b[0]}, {a[0]}', f'sub.s\t{ey}, {b[1]}, {a[1]}', f'sub.s\t{ez}, {b[2]}, {a[2]}',
                     f'sub.s\t{fx}, {c[0]}, {a[0]}', f'sub.s\t{fy}, {c[1]}, {a[1]}', f'sub.s\t{fz}, {c[2]}, {a[2]}',
                     f'mula.s\t{ey}, {fz}', f'msub.s\t{nx}, {ez}, {fy}',
                     f'mula.s\t{ez}, {fx}', f'msub.s\t{ny}, {ex}, {fz}',
                     f'mula.s\t{ex}, {fy}', f'msub.s\t{nz}, {ey}, {fx}',
                     f'swc1\t{nx}, {16 * k}($9)', f'swc1\t{ny}, {16 * k + 4}($9)', f'swc1\t{nz}, {16 * k + 8}($9)',
                     f'sw\t$0, {16 * k + 12}($9)']
    if D:
        body += [f'pref\t0, {64 * D}($8)'] + ([f'pref\t0, {64 * D}($9)'] if W else [])
    body += ['addiu\t$8, $8, 48', 'addiu\t$9, $9, 48', 'addiu\t$10, $10, -1']
    return func(name, f'# {name}: (b - a) x (c - a) of a strip, 3 triangles per iteration with\n'
                f'# each vertex loaded once; ' + ('VU0 vsub, vopmula, vopmsub (the w lanes of the\n# outputs stay 0)'
                                                  if unit == 'vu0' else 'FPU sub.s, then mula.s and msub.s per component') +
                f'. PREF {D} lines ahead.',
                pre, body, 'bnez\t$10, 1b', hash_tail([f'ADDR($4, geom_mem, {out})'], 8160))


PLANES = [[0.75, 0.0, 0.5, 0.0], [-0.75, 0.0, 0.5, 0.0], [0.0, 0.75, 0.5, 0.0],
          [0.0, -0.75, 0.5, 0.0], [0.0, 0.0, 1.0, -1.0], [0.0, 0.0, -1.0, 100.0]]


def frustum_cull(name, unit, D=0):
    """All 6 planes per sphere, no early exit: visible when d + r >= 0 for
    every plane (d = p . center + p.w). VU0: planes 0-3 and 4-5 (with two
    planes that keep everything) as columns, d + r as chains through ACC,
    vmini of the two, the sign bits ORed in GPRs. FPU: the minimum of the 6
    d (min.s), + r, the sign by mfc1."""
    C = f'{name}_c'
    pre = la('$8', 'spheres') + la('$9', 'sphere_visible') + ['addiu\t$10, $9, 1024', 'move\t$2, $0']
    body = []
    if unit == 'vu0':
        P2 = PLANES[4:] + [[0.0, 0.0, 0.0, 1.0], [0.0, 0.0, 0.0, 1.0]]
        data = []
        for grp in (PLANES[:4], P2):
            for j in range(4):
                data += [grp[l][j] for l in range(4)]
        data += [1, 1, 1, 1]
        pre += la('$11', C) + [f'lqc2\t$vf{20 + k}, {16 * k}($11)' for k in range(9)]
        G = ['$12', '$13']
        for k in range(2):
            s, d1, d2 = f'$vf{1 + 3 * k}', f'$vf{2 + 3 * k}', f'$vf{3 + 3 * k}'
            g, h = f'${12 + 2 * k}', f'${13 + 2 * k}'
            body += [f'lqc2\t{s}, {16 * k}($8)']
            for d, base in ((d1, 20), (d2, 24)):
                body += [f'vmulax.xyzw\t$ACC, $vf{base}, {s}x', f'vmadday.xyzw\t$ACC, $vf{base + 1}, {s}y',
                         f'vmaddaz.xyzw\t$ACC, $vf{base + 2}, {s}z', f'vmaddaw.xyzw\t$ACC, $vf{base + 3}, $vf0w',
                         f'vmaddw.xyzw\t{d}, $vf28, {s}w']
            body += [f'vmini.xyzw\t{d1}, {d1}, {d2}', f'qmfc2\t{g}, {d1}', f'pcpyud\t{h}, {g}, {g}',
                     f'or\t{h}, {h}, {g}', f'dsrl32\t{g}, {h}, 0', f'or\t{g}, {g}, {h}', f'srl\t{g}, {g}, 31',
                     f'xori\t{g}, {g}, 1', f'sb\t{g}, {k}($9)', f'addu\t$2, $2, {g}']
        U = 2
        pro, epi = [], []
    else:
        data = [x for p in PLANES for x in p] + [1.0]
        Pr = [f'$f{i}' for i in range(24)]
        ONE = '$f24'
        S = ['$f25', '$f26', '$f27', '$f28']
        Dd = ['$f29', '$f30', '$f31']
        pre += la('$11', C) + [f'lwc1\t{r}, {4 * k}($11)' for k, r in enumerate(Pr + [ONE])]
        body += [f'lwc1\t{S[j]}, {4 * j}($8)' for j in range(4)]
        for p in range(6):
            pp = Pr[4 * p:4 * p + 4]
            d = Dd[0] if p == 0 else Dd[1 + (p % 2)]
            body += [f'mula.s\t{pp[0]}, {S[0]}', f'madda.s\t{pp[1]}, {S[1]}', f'madda.s\t{pp[2]}, {S[2]}',
                     f'madd.s\t{d}, {pp[3]}, {ONE}']
            if p:
                body.append(f'min.s\t{Dd[0]}, {Dd[0]}, {d}')
        body += [f'add.s\t{Dd[0]}, {Dd[0]}, {S[3]}', f'mfc1\t$12, {Dd[0]}', 'srl\t$12, $12, 31',
                 'xori\t$12, $12, 1', 'sb\t$12, 0($9)', 'addu\t$2, $2, $12']
        U = 1
        pro, epi = savef(used_regs(body))
    if D:
        body += [f'pref\t0, {64 * D}($8)']
    body += [f'addiu\t$8, $8, {16 * U}', f'addiu\t$9, $9, {U}']
    post = ['move\t$16, $2'] + la('$4', 'sphere_visible') + ['jal\tapp_hash', 'ori\t$5, $0, 1024',
                                                            'xor\t$2, $2, $16', 'ld\t$ra, 0($sp)', 'lq\t$16, 16($sp)',
                                                            'addiu\t$sp, $sp, 32'] + epi + ['jr\t$ra', 'nop']
    return vfunc(name, f'# {name}: all 6 planes, no early exit: visible when d + r >= 0 for each\n# plane; ' +
                 ('VU0: the planes as columns (4 + 2 and 2 that keep everything), d + r as\n'
                  '# chains through ACC, vmini, the sign bits ORed in GPRs; 2 spheres per iteration'
                  if unit == 'vu0' else 'FPU: each d a chain through ACC, the minimum by min.s, + r, the sign by mfc1') +
                 f'. PREF {D} lines ahead.',
                 pro + ['addiu\t$sp, $sp, -32', 'sd\t$ra, 0($sp)', 'sq\t$16, 16($sp)'] + pre, body,
                 'bne\t$9, $10, 1b', post, rodata(C, data))
