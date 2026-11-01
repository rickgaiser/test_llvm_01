#!/usr/bin/env python3
"""Generates the game_tests.c kernels (ps2-llvm-benchmarks) with parameters,
for asm/game_tests.S (app_files.py) and the variants of the app harness
(asm/harness/gen_app.py, set game). Every generator takes the symbol name
first and returns the assembly text of one function (n32 ABI). The kernels
use the data of game_tests.c (KERNEL_DATA) and end with app_hash of their
output as the C kernels do."""
import os
import struct
import sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import sched
from gfx import la


def f32(x):
    """x rounded to a float, as the C code computes its constants."""
    return struct.unpack('<f', struct.pack('<f', x))[0]


def f32bits(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def frame():
    """Prologue and epilogue that keep $16-$23 and $30 (all 128 bits, sq) and
    $31, for a kernel that uses them all."""
    regs = ['$16', '$17', '$18', '$19', '$20', '$21', '$22', '$23', '$30']
    pro = ['addiu\t$sp, $sp, -160'] + [f'sq\t{r}, {16 * k}($sp)' for k, r in enumerate(regs)] + [
        'sd\t$31, 144($sp)']
    epi = [f'lq\t{r}, {16 * k}($sp)' for k, r in enumerate(regs)] + ['ld\t$31, 144($sp)',
                                                                    'addiu\t$sp, $sp, 160']
    return pro, epi


def head(name, comment):
    return [f'\n{comment}', '\t.p2align 3', f'\t.globl\t{name}', f'\t.ent\t{name}', f'{name}:']


def text(lines):
    return '\n'.join(l if l.endswith(':') or l.startswith(('\n', '#', '\t.')) else f'\t{l}'
                     for l in lines) + '\n'


# ---- Copy Loop: copy_dst = copy_src, 64 KB ----

def copy(name, U=1, D=2):
    """64 bytes per lq/sq group, U groups per iteration, PREF of the source
    D lines ahead (none for D = 0); no write PREF (it costs a fetch, see
    [ee-mmi-stream-loop])."""
    out = head(name, f'# {name}: copy_dst = copy_src (64 KB): lq/sq, {64 * U} bytes per iteration,\n'
               f'# PREF of the source {D} lines ahead.')
    out += la('$8', 'copy_src') + la('$9', 'copy_dst') + ['lui\t$1, 1', 'addu\t$10, $8, $1']
    regs = ['$2', '$3', '$4', '$5', '$6', '$7', '$11', '$12', '$13', '$14', '$15', '$24', '$25']
    body = []
    for g in range(U):
        if D:
            body.append(f'pref\t0, {64 * (D + g)}($8)')
        r = [regs[(4 * g + k) % len(regs)] for k in range(4)]
        body += [f'lq\t{r[k]}, {64 * g + 16 * k}($8)' for k in range(4)]
        body += [f'sq\t{r[k]}, {64 * g + 16 * k}($9)' for k in range(4)]
    out += ['.p2align 3', '1:'] + body + [f'addiu\t$8, $8, {64 * U}', 'bne\t$8, $10, 1b',
                                          f'addiu\t$9, $9, {64 * U}']
    out += la('$4', 'copy_dst') + ['j\tapp_hash', 'lui\t$5, 1', f'.end\t{name}']
    return text(out)


# ---- Packet Build: 2000 vertices of RGBAQ, ST and XYZ2 after a tag ----

def packet_scalar(name):
    """Per vertex: the six products i*k as counters, the fields by andi, sll,
    or (64-bit, so that bit 31 of RGBAQ does not sign-extend), 4 sd."""
    out = head(name, f'# {name}: GIF packet, scalar: the i*k products are counters, 4 sd per vertex.')
    out += la('$8', 'packets')
    out += ['lui\t$9, 0x1000', 'dsll32\t$9, $9, 0', 'ori\t$9, $9, 0x87d1', 'sd\t$9, 0($8)',
            'ori\t$9, $0, 0x5551', 'sd\t$9, 8($8)',
            'addiu\t$8, $8, 16', 'ori\t$1, $0, 64000', 'addu\t$7, $8, $1',
            'move\t$10, $0', 'move\t$11, $0', 'move\t$12, $0', 'move\t$13, $0', 'move\t$14, $0',
            'move\t$15, $0',
            'ori\t$24, $0, 0x8000', 'dsll\t$24, $24, 16',   # 0x80000000, zero-extended
            'lui\t$25, 0x3f80', 'lui\t$6, 0xff']
    body = ['andi\t$2, $13, 0xff', 'sll\t$3, $14, 8', 'andi\t$3, $3, 0xff00', 'sll\t$4, $15, 16',
            'and\t$4, $4, $6', 'or\t$2, $2, $3', 'or\t$2, $2, $4', 'or\t$2, $2, $24',
            'sd\t$2, 0($8)', 'sd\t$25, 8($8)',
            'andi\t$3, $10, 0x1ff', 'sll\t$3, $3, 4', 'andi\t$4, $11, 0x1ff', 'sll\t$4, $4, 20',
            'or\t$3, $3, $4', 'dsll32\t$5, $12, 0', 'or\t$3, $3, $5',
            'sd\t$3, 16($8)', 'sd\t$0, 24($8)',
            'addiu\t$10, $10, 37', 'addiu\t$11, $11, 91', 'addiu\t$12, $12, 3',
            'addiu\t$13, $13, 5', 'addiu\t$14, $14, 11', 'addiu\t$15, $15, 17',
            'addiu\t$8, $8, 32', 'bne\t$8, $7, 1b']
    order = sched.schedule(body[:-2], memdeps=True) + body[-2:]
    out += ['.p2align 3', '1:'] + order + ['nop']
    out += la('$4', 'packets') + ['j\tapp_hash', 'ori\t$5, $0, 64016', f'.end\t{name}']
    return text(out)


def packet_mmi(name):
    """Four vertices per iteration in the lanes of MMI registers: the fields
    by pand, psllw, por; the quadwords by pextlw/pextuw and pcpyld/pcpyud;
    8 sq."""
    pro, epi = frame()
    out = head(name, f'# {name}: GIF packet, MMI: 4 vertices per iteration in the lanes, 8 sq.')
    out += pro
    out += la('$1', f'{name}_k')
    # rodata: start lanes (r g b x y z), steps, masks, constants
    names = ['$4', '$5', '$6', '$7', '$8', '$9',        # VR VG VB VX VY VZ
             '$10', '$11', '$12', '$13', '$14', '$15',  # steps
             '$16', '$17', '$18', '$19', '$20', '$21']  # 0xff, 0xff00, 0xff0000, 0x1ff, 0x80000000, K3F
    out += [f'lq\t{r}, {16 * k}($1)' for k, r in enumerate(names)]
    out += la('$2', 'packets')
    out += ['lui\t$1, 0x1000', 'dsll32\t$1, $1, 0', 'ori\t$1, $1, 0x87d1', 'sd\t$1, 0($2)',
            'ori\t$1, $0, 0x5551', 'sd\t$1, 8($2)',
            'addiu\t$2, $2, 16', 'ori\t$1, $0, 64000', 'addu\t$3, $2, $1']
    t0, t1, t2, t3, t4, a, b, c, d = '$1', '$24', '$25', '$22', '$23', '$30', '$31', '$1', '$24'
    body = [f'pand\t{t0}, $4, $16', f'psllw\t{t1}, $5, 8', f'pand\t{t1}, {t1}, $17',
            f'psllw\t{t2}, $6, 16', f'pand\t{t2}, {t2}, $18', f'por\t{t0}, {t0}, {t1}',
            f'por\t{t0}, {t0}, {t2}', f'por\t{t0}, {t0}, $20',
            f'pand\t{t3}, $7, $19', f'psllw\t{t3}, {t3}, 4', f'pand\t{t4}, $8, $19',
            f'psllw\t{t4}, {t4}, 20', f'por\t{t3}, {t3}, {t4}',
            f'pextlw\t{a}, $0, {t0}', f'pextuw\t{b}, $0, {t0}',
            f'pextlw\t{t2}, $9, {t3}', f'pextuw\t{t4}, $9, {t3}',
            f'pcpyld\t{t1}, $21, {a}', f'sq\t{t1}, 0($2)', f'pcpyld\t{t1}, $0, {t2}', f'sq\t{t1}, 16($2)',
            f'pcpyud\t{t1}, {a}, $21', f'sq\t{t1}, 32($2)', f'pcpyud\t{t1}, {t2}, $0', f'sq\t{t1}, 48($2)',
            f'pcpyld\t{t1}, $21, {b}', f'sq\t{t1}, 64($2)', f'pcpyld\t{t1}, $0, {t4}', f'sq\t{t1}, 80($2)',
            f'pcpyud\t{t1}, {b}, $21', f'sq\t{t1}, 96($2)', f'pcpyud\t{t1}, {t4}, $0', f'sq\t{t1}, 112($2)',
            'paddw\t$4, $4, $10', 'paddw\t$5, $5, $11', 'paddw\t$6, $6, $12',
            'paddw\t$7, $7, $13', 'paddw\t$8, $8, $14', 'paddw\t$9, $9, $15']
    order = sched.schedule(body, memdeps=True)
    out += ['.p2align 3', '1:'] + order + ['addiu\t$2, $2, 128', 'bne\t$2, $3, 1b', 'nop']
    out += epi
    out += la('$4', 'packets') + ['j\tapp_hash', 'ori\t$5, $0, 64016', f'.end\t{name}']
    k = [[0, 5, 10, 15], [0, 11, 22, 33], [0, 17, 34, 51], [0, 37, 74, 111], [0, 91, 182, 273],
         [0, 3, 6, 9], [20] * 4, [44] * 4, [68] * 4, [148] * 4, [364] * 4, [12] * 4,
         [0xff] * 4, [0xff00] * 4, [0xff0000] * 4, [0x1ff] * 4, [0x80000000] * 4,
         [0x3f800000, 0, 0x3f800000, 0]]
    data = ['\t.section .rodata', '\t.p2align 4', f'{name}_k:'] + [
        '\t.word\t' + ', '.join(f'0x{w:x}' for w in v) for v in k] + ['\t.text']
    return text(out) + '\n'.join(data) + '\n'


# ---- LZ Decompress ----

def lz(name):
    """Literal runs and matches with a distance of 8 or more are copied 8
    bytes at a time (ldl/ldr, sdl/sdr), up to 7 bytes too many, which the next
    copy overwrites (both buffers have room); a closer match byte by byte."""
    out = head(name, f'# {name}: LZ77 decoder: copies of 8 bytes (ldl/ldr, sdl/sdr) that may\n'
               f'# write up to 7 bytes past the end, which the next copy overwrites; matches\n'
               f'# closer than 8 bytes byte by byte.')
    out += ['addiu\t$sp, $sp, -16', 'sd\t$31, 0($sp)', 'sd\t$16, 8($sp)']
    out += la('$8', 'lz_in') + ['lui\t$1, %hi(lz_in_len)', 'lw\t$1, %lo(lz_in_len)($1)',
                                'addu\t$9, $8, $1'] + la('$10', 'lz_out') + ['move\t$11, $10']
    out += ['.p2align 3',
            '1:', 'lbu\t$2, 0($8)', 'addiu\t$8, $8, 1', 'andi\t$3, $2, 0x80', 'bnez\t$3, 4f',
            'addiu\t$3, $2, 1',
            # literal run of $3 bytes
            '2:', 'ldl\t$12, 7($8)', 'ldr\t$12, 0($8)', 'addiu\t$3, $3, -8', 'addiu\t$8, $8, 8',
            'sdl\t$12, 7($10)', 'sdr\t$12, 0($10)', 'bgtz\t$3, 2b', 'addiu\t$10, $10, 8',
            'addu\t$8, $8, $3', 'addu\t$10, $10, $3',
            '3:', 'sltu\t$1, $8, $9', 'bnez\t$1, 1b', 'nop', 'b\t9f', 'nop',
            # match: length (tag & 0x7f) + 3, distance (in[0] << 8 | in[1]) + 1
            '4:', 'lbu\t$12, 0($8)', 'lbu\t$13, 1($8)', 'andi\t$3, $2, 0x7f', 'addiu\t$8, $8, 2',
            'sll\t$12, $12, 8', 'addiu\t$3, $3, 3', 'or\t$12, $12, $13', 'addiu\t$12, $12, 1',
            'sltiu\t$1, $12, 8', 'bnez\t$1, 6f', 'subu\t$14, $10, $12',
            '5:', 'ldl\t$12, 7($14)', 'ldr\t$12, 0($14)', 'addiu\t$3, $3, -8', 'addiu\t$14, $14, 8',
            'sdl\t$12, 7($10)', 'sdr\t$12, 0($10)', 'bgtz\t$3, 5b', 'addiu\t$10, $10, 8',
            'b\t3b', 'addu\t$10, $10, $3',
            '6:', 'lbu\t$12, 0($14)', 'addiu\t$3, $3, -1', 'addiu\t$14, $14, 1', 'sb\t$12, 0($10)',
            'bgtz\t$3, 6b', 'addiu\t$10, $10, 1', 'b\t3b', 'nop',
            # app_hash(lz_out, n & ~3) ^ n
            '9:', 'subu\t$16, $10, $11', 'move\t$4, $11', 'addiu\t$5, $0, -4', 'jal\tapp_hash',
            'and\t$5, $16, $5', 'xor\t$2, $2, $16', 'ld\t$31, 0($sp)', 'ld\t$16, 8($sp)',
            'jr\t$31', 'addiu\t$sp, $sp, 16', f'.end\t{name}']
    return text(out)


# ---- Particles: one frame of 1024 particles ----

PSTRIDE = 4 * 1040
PX, PY, PZ, VX, VY, VZ = (k * PSTRIDE for k in range(6))


def particles(name, U=4, D=0, G=1, madd=False):
    """G groups of U particles per iteration. A group: the loads, updates
    and stores of its particles scheduled together, then the floor and wall
    tests in order, each a branch to code after the loop (rarely taken). One
    pointer into part[][] with the arrays at fixed offsets. With D, a PREF D
    lines ahead of each array: once per iteration of 16 particles (U * G =
    16), the six spread over the groups (the PS2 serves one miss at a time),
    else of every array every iteration. madd: d2 by mula.s/madd.s."""
    N = U * G
    assert 1024 % N == 0
    dt = f32(1.0 / 60.0)
    g = f32(f32(9.8) * dt)
    consts = {'G': g, 'DT': dt, 'K08': f32(0.8), 'H': 100.0, 'ONE': 1.0}
    C = {'G': '$f1', 'DT': '$f2', 'K08': '$f3', 'H': '$f4', 'ONE': '$f5', 'ZERO': '$f6'}
    free = [f'$f{i}' for i in range(7, 32)]
    per = []
    for k in range(U):
        regs = free[5 * k:5 * k + 5]
        assert len(regs) == 5, 'too many particles per group'
        per.append(dict(zip(['vy', 'px', 'py', 'pz', 'd2'], regs)))
    used = set(r for p in per for r in p.values())
    fs = [r for r in ['$f20', '$f22', '$f24', '$f26', '$f28', '$f30'] if r in used]
    out = head(name, f'# {name}: particles, {G} group(s) of {U} per iteration (loads, updates and\n'
               f'# stores of a group scheduled together, then its tests: branches to code\n'
               f'# after the loop), PREF {D} lines ahead{", d2 by mula.s/madd.s" if madd else ""}.')
    out += [f'addiu\t$sp, $sp, -32'] + [f'swc1\t{r}, {4 * k}($sp)' for k, r in enumerate(fs)]
    out += la('$8', 'part') + [f'addiu\t$9, $8, {4 * 1024}', 'move\t$2, $0', 'move\t$3, $0']
    out += la('$1', f'{name}_k') + [f'lwc1\t{C[n]}, {4 * k}($1)' for k, n in enumerate(consts)]
    out += ['mtc1\t$0, $f6']
    bases = (PX, PY, PZ, VX, VY, VZ)
    loop, fix = [], []
    for gi in range(G):
        main, tests = [], []
        for k, p in enumerate(per):
            o = 4 * (gi * U + k)
            main += [f'lwc1\t{p["vy"]}, {VY + o}($8)', f'lwc1\t{p["px"]}, {PX + o}($8)',
                     f'lwc1\t{p["py"]}, {PY + o}($8)', f'lwc1\t{p["pz"]}, {PZ + o}($8)',
                     f'lwc1\t{p["d2"]}, {VX + o}($8)',
                     f'sub.s\t{p["vy"]}, {p["vy"]}, {C["G"]}',
                     f'mul.s\t{p["d2"]}, {p["d2"]}, {C["DT"]}',
                     f'add.s\t{p["px"]}, {p["px"]}, {p["d2"]}',
                     f'lwc1\t{p["d2"]}, {VZ + o}($8)',
                     f'mul.s\t{p["d2"]}, {p["d2"]}, {C["DT"]}',
                     f'add.s\t{p["pz"]}, {p["pz"]}, {p["d2"]}',
                     f'mul.s\t{p["d2"]}, {p["vy"]}, {C["DT"]}',
                     f'add.s\t{p["py"]}, {p["py"]}, {p["d2"]}',
                     f'swc1\t{p["vy"]}, {VY + o}($8)', f'swc1\t{p["px"]}, {PX + o}($8)',
                     f'swc1\t{p["py"]}, {PY + o}($8)', f'swc1\t{p["pz"]}, {PZ + o}($8)']
        for k, p in enumerate(per):
            if madd:
                main += [f'mula.s\t{p["px"]}, {p["px"]}', f'madd.s\t{p["d2"]}, {p["pz"]}, {p["pz"]}']
            else:
                main += [f'mul.s\t$f0, {p["pz"]}, {p["pz"]}', f'mul.s\t{p["d2"]}, {p["px"]}, {p["px"]}',
                         f'add.s\t{p["d2"]}, {p["d2"]}, $f0']
        order = sched.schedule(main, memdeps=True)
        if D:
            if N * 4 == 64:
                mine = [b for s, b in enumerate(bases) if s * G // 6 == gi]
            else:
                mine = list(bases)
            step = len(order) // (len(mine) + 1)
            for m, base in reversed(list(enumerate(mine))):
                order.insert(step * (m + 1), f'pref\t0, {base + 64 * D}($8)')
        for k, p in enumerate(per):
            j = gi * U + k
            o = 4 * j
            tests += [f'c.lt.s\t{p["py"]}, {C["ZERO"]}', f'bc1t\t7{j}f', 'nop', f'8{j}:',
                      f'c.lt.s\t{C["H"]}, {p["d2"]}', f'bc1t\t5{j}f', 'nop', f'6{j}:']
            fix += [f'7{j}:', f'neg.s\t{p["py"]}, {p["py"]}', f'neg.s\t$f0, {p["vy"]}',
                    f'mul.s\t$f0, $f0, {C["K08"]}', f'swc1\t{p["py"]}, {PY + o}($8)',
                    f'swc1\t$f0, {VY + o}($8)', f'b\t8{j}b', 'addiu\t$2, $2, 1',
                    f'5{j}:', f'rsqrt.s\t$f0, {C["ONE"]}, {p["d2"]}', f'mul.s\t{p["px"]}, {p["px"]}, $f0',
                    f'mul.s\t{p["pz"]}, {p["pz"]}, $f0', f'neg.s\t{p["px"]}, {p["px"]}',
                    f'neg.s\t{p["pz"]}, {p["pz"]}', f'swc1\t{p["px"]}, {VX + o}($8)',
                    f'swc1\t{p["pz"]}, {VZ + o}($8)', f'b\t6{j}b', 'addiu\t$3, $3, 1']
        loop += order + tests
    out += ['.p2align 3', '1:'] + loop + [f'addiu\t$8, $8, {4 * N}', 'bne\t$8, $9, 1b', 'nop']
    out += [f'lwc1\t{r}, {4 * k}($sp)' for k, r in enumerate(fs)]
    out += ['sll\t$2, $2, 16', 'or\t$2, $2, $3', 'jr\t$31', 'addiu\t$sp, $sp, 32']
    out += fix + [f'.end\t{name}']
    data = ['\t.section .rodata', '\t.p2align 4', f'{name}_k:'] + [
        '\t.word\t' + ', '.join(f'0x{f32bits(v):08x}' for v in consts.values())] + ['\t.text']
    return text(out) + '\n'.join(data) + '\n'


# ---- Tri Raster: 200 triangles into the 128x128 frame ----

def raster(name, U=1):
    """MMI, 4 pixels per step: the three edge functions of 4 neighbouring
    pixels in the lanes (paddw per step), inside when (w0 | w1 | w2) >= 0
    (pcgtw against -1; for a triangle with negative area the edge functions
    are negated, which makes w <= 0 into -w >= 0), the pixels written by a
    select with the frame (lq, pxor, pand, pxor, sq). The quads of a row are
    aligned; the first and the last are masked to the bounding box."""
    pro, epi = frame()
    out = head(name, f'# {name}: triangle rasterizer, MMI: the edge functions of 4 pixels in the\n'
               f'# lanes, the quads of a row aligned, the first and last masked.')
    out += pro
    out += la('$2', 'tri') + ['addiu\t$3, $2, 4800'] + la('$25', 'tri_color')
    out += ['addiu\t$16, $0, -1', 'pcpyld\t$16, $16, $16']   # NEG1
    bc = lambda r, s: [f'pextlw\t{r}, {s}, {s}', f'pcpyld\t{r}, {r}, {r}']
    quad = lambda mask: (['lq\t$13, 0($1)', 'por\t$11, $8, $9', 'por\t$11, $11, $10',
                          'pcgtw\t$12, $11, $16'] + ([f'pand\t$12, $12, {mask}'] if mask else []) +
                         ['pxor\t$11, $13, $4', 'pand\t$11, $11, $12', 'pxor\t$11, $11, $13',
                          'sq\t$11, 0($1)'])
    step = ['paddw\t$8, $8, $21', 'paddw\t$9, $9, $22', 'paddw\t$10, $10, $23', 'addiu\t$1, $1, 16']
    out += ['.p2align 3', '1:',
            'lw\t$8, 0($2)', 'lw\t$9, 4($2)', 'lw\t$10, 8($2)', 'lw\t$11, 12($2)', 'lw\t$12, 16($2)',
            'lw\t$13, 20($2)', 'lw\t$4, 0($25)', 'addiu\t$2, $2, 24', 'addiu\t$25, $25, 4',
            # area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)
            'subu\t$1, $10, $8', 'subu\t$14, $13, $9', 'mult\t$1, $1, $14',
            'subu\t$14, $12, $8', 'subu\t$15, $11, $9', 'mult\t$14, $14, $15',
            'subu\t$1, $1, $14', 'beqz\t$1, 9f',
            # the edges: w = b * (Y - yk) + a * (X - xk)
            'subu\t$21, $9, $11', 'subu\t$30, $10, $8', 'subu\t$22, $11, $13', 'subu\t$31, $12, $10',
            'subu\t$23, $13, $9', 'bgtz\t$1, 2f', 'subu\t$7, $8, $12',
            'subu\t$21, $0, $21', 'subu\t$30, $0, $30', 'subu\t$22, $0, $22', 'subu\t$31, $0, $31',
            'subu\t$23, $0, $23', 'subu\t$7, $0, $7',
            '2:',
            # minx $14, maxx $15, miny $24, maxy $17
            'move\t$14, $8', 'slt\t$1, $10, $14', 'movn\t$14, $10, $1', 'slt\t$1, $12, $14',
            'movn\t$14, $12, $1',
            'move\t$15, $8', 'slt\t$1, $15, $10', 'movn\t$15, $10, $1', 'slt\t$1, $15, $12',
            'movn\t$15, $12, $1',
            'move\t$24, $9', 'slt\t$1, $11, $24', 'movn\t$24, $11, $1', 'slt\t$1, $13, $24',
            'movn\t$24, $13, $1',
            'move\t$17, $9', 'slt\t$1, $17, $11', 'movn\t$17, $11, $1', 'slt\t$1, $17, $13',
            'movn\t$17, $13, $1',
            'subu\t$17, $17, $24', 'addiu\t$17, $17, 1',   # rows
            'addiu\t$5, $0, -4', 'and\t$5, $14, $5',        # xq
            # w at (xq, miny)
            'subu\t$1, $24, $9', 'mult\t$1, $30, $1', 'subu\t$6, $5, $8', 'mult\t$6, $21, $6',
            'addu\t$18, $1, $6',
            'subu\t$1, $24, $11', 'mult\t$1, $31, $1', 'subu\t$6, $5, $10', 'mult\t$6, $22, $6',
            'addu\t$19, $1, $6',
            'subu\t$1, $24, $13', 'mult\t$1, $7, $1', 'subu\t$6, $5, $12', 'mult\t$6, $23, $6',
            'addu\t$20, $1, $6',
            # first row: fb + miny * 512 + xq * 4
            'sll\t$1, $24, 9', 'sll\t$6, $5, 2', 'addu\t$1, $1, $6', 'lui\t$6, %hi(fb)',
            'addiu\t$6, $6, %lo(fb)', 'addu\t$24, $1, $6',
            # the lane masks of the first and the last quad
            f'lui\t$1, %hi({name}_lanes)', f'lq\t$9, %lo({name}_lanes)($1)',
            'andi\t$1, $14, 3', 'addiu\t$1, $1, -1'] + bc('$8', '$1') + ['pcgtw\t$5, $9, $8',
            'andi\t$1, $15, 3', 'addiu\t$1, $1, 1'] + bc('$8', '$1') + ['pcgtw\t$6, $8, $9',
            # 16 * (quads - 1)
            'sra\t$1, $15, 2', 'sra\t$14, $14, 2', 'subu\t$14, $1, $14', 'sll\t$14, $14, 4',
            'bnez\t$14, 3f', 'move\t$15, $17', 'pand\t$5, $5, $6',
            '3:']
    # the lanes of each edge: [w, w + a, w + 2a, w + 3a]; steps 4a per quad, b per row
    for w, a, b in (('$18', '$21', '$30'), ('$19', '$22', '$31'), ('$20', '$23', '$7')):
        out += [f'addu\t$8, {w}, {a}', f'addu\t$9, $8, {a}', f'addu\t$10, $9, {a}',
                f'pextlw\t$11, $8, {w}', f'pextlw\t$12, $10, $9', f'pcpyld\t{w}, $12, $11',
                f'sll\t$1, {a}, 2'] + bc(a, '$1') + bc(b, b)
    out += bc('$4', '$4')
    # the rows
    out += ['.p2align 3', '4:', 'por\t$8, $18, $0', 'por\t$9, $19, $0', 'por\t$10, $20, $0',
            'move\t$1, $24', 'addu\t$17, $24, $14'] + quad('$5') + ['beq\t$1, $17, 7f', 'nop'] + step
    out += ['beq\t$1, $17, 6f', 'nop', '.p2align 3', '5:']
    mid = quad(None)
    if U == 1:
        out += mid[:1] + mid[1:] + step[:2] + ['addiu\t$1, $1, 16', 'bne\t$1, $17, 5b', step[2]]
    else:
        out += mid + step[:3] + ['addiu\t$1, $1, 16', 'bne\t$1, $17, 5b', 'nop']
    out += ['6:'] + quad('$6')
    out += ['7:', 'paddw\t$18, $18, $30', 'paddw\t$19, $19, $31', 'paddw\t$20, $20, $7',
            'addiu\t$15, $15, -1', 'bnez\t$15, 4b', 'addiu\t$24, $24, 512',
            '9:', 'bne\t$2, $3, 1b', 'nop']
    out += epi
    out += la('$4', 'fb') + ['j\tapp_hash', 'lui\t$5, 1', f'.end\t{name}']
    data = ['\t.section .rodata', '\t.p2align 4', f'{name}_lanes:', '\t.word\t0, 1, 2, 3', '\t.text']
    return text(out) + '\n'.join(data) + '\n'
