#!/usr/bin/env python3
"""Generates the emu_tests.c kernels with parameters, ordered by sched.py:
for asm/harness/gen_app.py (variants on the PS2) and emu_file.py
(asm/emu_tests.S). Every generator takes the symbol name first and returns
a u32 (void) kernel as the C kernels are."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sched
from gfx import func, save, used_regs, la


def slot_fill(order, branch):
    """The branch with the last instruction of order that the branch and no
    later instruction depends on in its delay slot."""
    bsrcs = set(sched.parse(branch)[2])
    for k in range(len(order) - 1, -1, -1):
        op, dests, srcs = sched.parse(order[k])
        later = order[k + 1:]
        if op in sched.BRANCHES or op in sched.STORES and any(
                sched.parse(l)[0] in sched.STORES | sched.LOADS for l in later):
            continue
        if not set(dests) & bsrcs and all(
                not set(dests) & set(sched.parse(l)[2]) and
                not set(sched.parse(l)[1]) & (set(srcs) | set(dests)) for l in later):
            return order[:k] + order[k + 1:] + [branch, order[k]]
    return order + [branch, 'nop']


def block(lines, branch, noalias=()):
    """lines scheduled (loads kept after stores, but those from a base in
    noalias), then branch + delay slot."""
    return slot_fill(sched.schedule(lines, memdeps=True, noalias=noalias), branch)


def text(name, comment, parts, data=''):
    out = f'\n{comment}\n\t.p2align 3\n\t.globl\t{name}\n\t.ent\t{name}\n{name}:\n'
    for p in parts:
        if isinstance(p, str):
            out += p + '\n'
        else:
            out += ''.join(f'\t{l}\n' for l in p)
    return data + out + f'\t.end\t{name}\n'


# ---- 6502 ----

C8_OPS = {
    0xa9: 'lda_imm', 0xb5: 'lda_zpx', 0x85: 'sta_zp', 0x95: 'sta_zpx', 0xa2: 'ldx_imm',
    0xa0: 'ldy_imm', 0x65: 'adc_zp', 0x69: 'adc_imm', 0xe9: 'sbc_imm', 0x29: 'and_imm',
    0x49: 'eor_imm', 0x0a: 'asl_a', 0x4a: 'lsr_a', 0xc9: 'cmp_imm', 0xe0: 'cpx_imm',
    0xe8: 'inx', 0xca: 'dex', 0xc8: 'iny', 0x88: 'dey', 0xaa: 'tax', 0x8a: 'txa',
    0x18: 'clc', 0x38: 'sec', 0xd0: 'bne', 0x90: 'bcc', 0xb0: 'bcs'}


def cpu8(name):
    """Threaded dispatch: each handler ends with the fetch of the next
    opcode and operand byte (pc masked to 12 bits as the C does) and the
    jump through the table; the branches are branchless (movn/movz). a $16,
    x $17, y $18, c $19, v $20, nz $21, pc $22 (the opcode's address + 1),
    mem8 $23, the table $24; the operand byte m in $10."""
    A, X, Y, Cf, V, NZ, PC, MEM, TBL, M = '$16', '$17', '$18', '$19', '$20', '$21', '$22', '$23', '$24', '$10'

    def nxt(k):
        """pc += k, then the dispatch of the next instruction."""
        return ([f'addiu\t{PC}, {PC}, {k}'] if k else []) + [
            f'andi\t{PC}, {PC}, 0xfff', f'addu\t$8, {MEM}, {PC}', 'lbu\t$9, 0($8)',
            f'addiu\t$11, {PC}, 1', 'andi\t$11, $11, 0xfff', f'addu\t$11, $11, {MEM}', f'lbu\t{M}, 0($11)',
            'sll\t$9, $9, 2', f'addu\t$9, $9, {TBL}', 'lw\t$9, 0($9)', f'addiu\t{PC}, {PC}, 1']

    def adc():
        return [f'addu\t$12, {A}, {M}', f'addu\t$12, $12, {Cf}', f'xor\t$13, {A}, {M}', 'nor\t$13, $13, $0',
                f'xor\t$14, {A}, $12', 'and\t$13, $13, $14', f'andi\t{V}, $13, 0x80', f'srl\t{Cf}, $12, 8',
                f'andi\t{A}, $12, 0xff', f'move\t{NZ}, {A}']

    def br(cond):
        # pc += 1, then if cond: pc += (s8)m
        return [f'addiu\t{PC}, {PC}, 1', f'sll\t$12, {M}, 24', 'sra\t$12, $12, 24', f'addu\t$12, $12, {PC}',
                cond.replace('T', '$12')]
    H = {
        'lda_imm': ([f'move\t{A}, {M}', f'move\t{NZ}, {M}'], 1),
        'lda_zpx': ([f'addu\t$12, {M}, {X}', 'andi\t$12, $12, 0xff', f'addu\t$12, $12, {MEM}',
                     f'lbu\t{A}, 0($12)', f'move\t{NZ}, {A}'], 1),
        'sta_zp': ([f'addu\t$12, {MEM}, {M}', f'sb\t{A}, 0($12)'], 1),
        'sta_zpx': ([f'addu\t$12, {M}, {X}', 'andi\t$12, $12, 0xff', f'addu\t$12, $12, {MEM}',
                     f'sb\t{A}, 0($12)'], 1),
        'ldx_imm': ([f'move\t{X}, {M}', f'move\t{NZ}, {M}'], 1),
        'ldy_imm': ([f'move\t{Y}, {M}', f'move\t{NZ}, {M}'], 1),
        'adc_zp': ([f'addu\t$12, {MEM}, {M}', f'lbu\t{M}, 0($12)'] + adc(), 1),
        'adc_imm': (adc(), 1),
        'sbc_imm': ([f'xori\t{M}, {M}, 0xff'] + adc(), 1),
        'and_imm': ([f'and\t{A}, {A}, {M}', f'move\t{NZ}, {A}'], 1),
        'eor_imm': ([f'xor\t{A}, {A}, {M}', f'move\t{NZ}, {A}'], 1),
        'asl_a': ([f'srl\t{Cf}, {A}, 7', f'sll\t{A}, {A}, 1', f'andi\t{A}, {A}, 0xff', f'move\t{NZ}, {A}'], 0),
        'lsr_a': ([f'andi\t{Cf}, {A}, 1', f'srl\t{A}, {A}, 1', f'move\t{NZ}, {A}'], 0),
        'cmp_imm': ([f'subu\t$12, {A}, {M}', f'sltu\t{Cf}, {A}, {M}', f'xori\t{Cf}, {Cf}, 1',
                     f'andi\t{NZ}, $12, 0xff'], 1),
        'cpx_imm': ([f'subu\t$12, {X}, {M}', f'sltu\t{Cf}, {X}, {M}', f'xori\t{Cf}, {Cf}, 1',
                     f'andi\t{NZ}, $12, 0xff'], 1),
        'inx': ([f'addiu\t{X}, {X}, 1', f'andi\t{X}, {X}, 0xff', f'move\t{NZ}, {X}'], 0),
        'dex': ([f'addiu\t{X}, {X}, -1', f'andi\t{X}, {X}, 0xff', f'move\t{NZ}, {X}'], 0),
        'iny': ([f'addiu\t{Y}, {Y}, 1', f'andi\t{Y}, {Y}, 0xff', f'move\t{NZ}, {Y}'], 0),
        'dey': ([f'addiu\t{Y}, {Y}, -1', f'andi\t{Y}, {Y}, 0xff', f'move\t{NZ}, {Y}'], 0),
        'tax': ([f'move\t{X}, {A}', f'move\t{NZ}, {A}'], 0),
        'txa': ([f'move\t{A}, {X}', f'move\t{NZ}, {X}'], 0),
        'clc': ([f'move\t{Cf}, $0'], 0),
        'sec': ([f'addiu\t{Cf}, $0, 1'], 0),
        'bne': (br(f'movn\t{PC}, T, {NZ}'), 0),
        'bcc': (br(f'movz\t{PC}, T, {Cf}'), 0),
        'bcs': (br(f'movn\t{PC}, T, {Cf}'), 0),
    }
    parts = [['addiu\t$sp, $sp, -80', 'sd\t$ra, 0($sp)'] + [f'sd\t${16 + k}, {8 + 8 * k}($sp)' for k in range(9)] +
             [f'move\t{A}, $0', f'move\t{X}, $0', f'move\t{Y}, $0', f'move\t{Cf}, $0', f'move\t{V}, $0',
              f'addiu\t{NZ}, $0, 1', f'addiu\t{PC}, $0, 0x200'] + la(MEM, 'mem8') + la(TBL, f'{name}_t')]
    parts.append(block(nxt(0), 'jr\t$9', {'$9'}))
    for h, (body, k) in H.items():
        parts += ['\t.p2align 3', f'{name}_{h}:', block(body + nxt(k), 'jr\t$9', {'$9'})]
    parts += [f'{name}_brk:', [f'move\t$4, {MEM}', 'jal\tapp_hash', 'ori\t$5, $0, 256', f'xor\t$2, $2, {A}',
                               f'sll\t$8, {X}, 8', 'xor\t$2, $2, $8', f'sll\t$8, {Y}, 16', 'xor\t$2, $2, $8',
                               f'sll\t$8, {Cf}, 24', 'xor\t$2, $2, $8', f'sll\t$8, {V}, 25', 'xor\t$2, $2, $8',
                               'ld\t$ra, 0($sp)'] + [f'ld\t${16 + k}, {8 + 8 * k}($sp)' for k in range(9)] +
              ['jr\t$ra', 'addiu\t$sp, $sp, 80']]
    tbl = [f'{name}_{C8_OPS[o]}' if o in C8_OPS else f'{name}_brk' for o in range(256)]
    data = f'\t.section .rodata\n\t.p2align 2\n{name}_t:\n' + ''.join(
        f'\t.word\t{", ".join(tbl[i:i + 4])}\n' for i in range(0, 256, 4)) + '\t.text\n'
    return text(name, f'# {name}: threaded dispatch: each handler ends with the fetch of the next\n'
                '# opcode and operand byte (pc masked to 12 bits as in the C) and the jump\n'
                '# through the table, ordered by sched.py; branches without branches (movn,\n'
                '# movz). a $16, x $17, y $18, c $19, v $20, nz $21, pc $22, mem8 $23, table $24.',
                parts, data)


# ---- R3000 ----

def r3000(name):
    """Threaded dispatch: the next instruction (at pc) is fetched at the
    start of each handler (after the store for sw), its handler found by one
    table of 128 entries (the opcode, or 64 + funct for SPECIAL: movz); the
    branches set npc with movn/movz. ram $16, pc $17, npc $18, the guest
    registers $19 (on the stack), the table $20, in $9, the next in $25."""
    RAM, PC, NPC, G, TBL, IN, NIN = '$16', '$17', '$18', '$19', '$20', '$9', '$25'
    fetch = [f'andi\t$8, {PC}, 0x3ffc', f'addu\t$8, $8, {RAM}', f'lw\t{NIN}, 0($8)']
    tail = [f'move\t{IN}, {NIN}', f'move\t{PC}, {NPC}', f'addiu\t{NPC}, {NPC}, 4',
            f'srl\t$10, {NIN}, 26', f'andi\t$11, {NIN}, 63', 'addiu\t$11, $11, 64', 'movz\t$10, $11, $10',
            'sll\t$10, $10, 2', f'addu\t$10, $10, {TBL}', 'lw\t$10, 0($10)']
    rs = [f'srl\t$12, {IN}, 19', 'andi\t$12, $12, 0x7c', f'addu\t$12, $12, {G}', 'lw\t$12, 0($12)']
    rt = [f'srl\t$13, {IN}, 14', 'andi\t$13, $13, 0x7c', f'addu\t$13, $13, {G}', 'lw\t$13, 0($13)']
    rta = [f'srl\t$14, {IN}, 14', 'andi\t$14, $14, 0x7c', f'addu\t$14, $14, {G}']   # &r[rt]
    rda = [f'srl\t$14, {IN}, 9', 'andi\t$14, $14, 0x7c', f'addu\t$14, $14, {G}']    # &r[rd]
    imm = [f'sll\t$15, {IN}, 16', 'sra\t$15, $15, 16']
    wr = ['sw\t$2, 0($14)', f'sw\t$0, 0({G})']

    def rr(op):
        return fetch + rs + rt + rda + [f'{op}\t$2, $12, $13'] + wr

    def ri(op, zext=False):
        return fetch + rs + rta + ([f'andi\t$15, {IN}, 0xffff'] if zext else imm) + [f'{op}\t$2, $12, $15'] + wr

    def br(mov):
        return fetch + rs + rt + [f'sll\t$15, {IN}, 16', 'sra\t$15, $15, 14', f'addu\t$15, $15, {PC}',
                                  'xor\t$12, $12, $13', f'{mov}\t{NPC}, $15, $12']
    H = {
        64 + 0x00: ('sll', fetch + rt + rda + [f'srl\t$15, {IN}, 6', 'andi\t$15, $15, 31', 'sllv\t$2, $13, $15'] + wr),
        64 + 0x02: ('srl', fetch + rt + rda + [f'srl\t$15, {IN}, 6', 'andi\t$15, $15, 31', 'srlv\t$2, $13, $15'] + wr),
        64 + 0x21: ('addu', rr('addu')), 64 + 0x23: ('subu', rr('subu')), 64 + 0x24: ('and', rr('and')),
        64 + 0x25: ('or', rr('or')), 64 + 0x26: ('xor', rr('xor')), 64 + 0x2a: ('slt', rr('slt')),
        0x04: ('beq', br('movz')), 0x05: ('bne', br('movn')),
        0x09: ('addiu', ri('addu')), 0x0a: ('slti', ri('slt')), 0x0c: ('andi', ri('and', True)),
        0x0d: ('ori', ri('or', True)),
        0x0f: ('lui', fetch + rta + [f'sll\t$2, {IN}, 16'] + wr),
        0x23: ('lw', fetch + rs + rta + imm + ['addu\t$12, $12, $15', 'andi\t$12, $12, 0x3ffc',
                                               f'addu\t$12, $12, {RAM}', 'lw\t$2, 0($12)'] + wr),
        0x2b: ('sw', rs + rt + imm + ['addu\t$12, $12, $15', 'andi\t$12, $12, 0x3ffc',
                                      f'addu\t$12, $12, {RAM}', 'sw\t$13, 0($12)'] + fetch),
    }
    parts = [['addiu\t$sp, $sp, -176', 'sd\t$ra, 0($sp)'] + [f'sd\t${16 + k}, {8 + 8 * k}($sp)' for k in range(5)] +
             [f'addiu\t{G}, $sp, 48'] +
             [f'sw\t$0, {48 + 4 * k}($sp)' for k in range(32)] + la(RAM, 'ram') + la(TBL, f'{name}_t') +
             [f'move\t{PC}, $0', f'addiu\t{NPC}, $0, 4']]
    # the first fetch and dispatch: in = ram[0], pc = 4, npc = 8
    parts.append(block(fetch + tail, 'jr\t$10', {'$10'}))
    for idx, (h, body) in sorted(H.items()):
        parts += ['\t.p2align 3', f'{name}_{h}:', block(body + tail, 'jr\t$10', {'$10'})]
    parts += ['\t.p2align 3', f'{name}_nop:', block(fetch + [f'sw\t$0, 0({G})'] + tail, 'jr\t$10', {'$10'})]
    parts += [f'{name}_syscall:', [f'addiu\t$4, {RAM}, 0x1800', 'jal\tapp_hash', 'ori\t$5, $0, 2048',
                                   f'lw\t$8, 8({G})', 'xor\t$2, $2, $8', 'ld\t$ra, 0($sp)'] +
              [f'ld\t${16 + k}, {8 + 8 * k}($sp)' for k in range(5)] + ['jr\t$ra', 'addiu\t$sp, $sp, 176']]
    tbl = []
    for i in range(128):
        if i in H:
            tbl.append(f'{name}_{H[i][0]}')
        elif i == 64 + 0x0c:
            tbl.append(f'{name}_syscall')
        else:
            tbl.append(f'{name}_nop')
    data = f'\t.section .rodata\n\t.p2align 2\n{name}_t:\n' + ''.join(
        f'\t.word\t{", ".join(tbl[i:i + 4])}\n' for i in range(0, 128, 4)) + '\t.text\n'
    return text(name, f'# {name}: threaded dispatch: each handler fetches the next instruction (at pc;\n'
                '# for sw after the store) first and ends with the jump to its handler: one table\n'
                '# of 128 (the opcode, or 64 + funct for SPECIAL, by movz); the branches set npc\n'
                '# by movn/movz. ram $16, pc $17, npc $18, the guest registers at $19 (stack),\n'
                '# the table $20, in $9, the next in $25; ordered by sched.py.',
                parts, data)


# ---- SNES tiles ----

def tile_decode_mmi(name):
    """MMI, a tile per iteration: halfword y of the quadword Q at 0 is
    p1[y] << 8 | p0[y], of R at 16 p3[y] << 8 | p2[y]. For bit b of pixel
    x = 7 - b, T = (Q >> b) & 0x0101 | (R >> (b - 2)) & 0x0404 has the plane
    bits at bits 0, 8, 2 and 10, and T | T >> 7 the pixel in bits 0-3. The
    8x8 bytes are transposed into rows: pairs of pixels in halfwords (pand,
    psllh 8, por), pextlh/pextuh, pextlw/pextuw; 4 sq."""
    Q, R, K1, K4, KF = '$2', '$3', '$4', '$5', '$6'
    T = [['$7', '$10', '$12'], ['$13', '$14', '$15']]
    W = ['$24', '$25', '$16', '$17']
    pre = la('$8', 'tiles') + la('$9', 'chunky') + ['addiu\t$11, $8, 4096',
           f'ori\t{K1}, $0, 0x0101', f'pcpyld\t{K1}, {K1}, {K1}', f'pcpyh\t{K1}, {K1}',
           f'psllh\t{K4}, {K1}, 2', f'pnor\t{KF}, $0, $0', f'psrlh\t{KF}, {KF}, 8']
    body = [f'lq\t{Q}, 0($8)', f'lq\t{R}, 16($8)']
    for x in range(8):
        b = 7 - x
        qs, rs, t = T[x % 2]
        if b:
            body += [f'psrlh\t{qs}, {Q}, {b}', f'pand\t{qs}, {qs}, {K1}']
        else:
            body += [f'pand\t{qs}, {Q}, {K1}']
        if b != 2:
            body += [f'psrlh\t{rs}, {R}, {b - 2}' if b > 2 else f'psllh\t{rs}, {R}, {2 - b}',
                     f'pand\t{rs}, {rs}, {K4}']
        else:
            body += [f'pand\t{rs}, {R}, {K4}']
        body += [f'por\t{t}, {qs}, {rs}', f'psrlh\t{rs}, {t}, 7', f'por\t{t}, {t}, {rs}']
        if x % 2:
            e = T[0][2]
            body += [f'pand\t{e}, {e}, {KF}', f'psllh\t{t}, {t}, 8', f'por\t{W[x // 2]}, {e}, {t}']
    X = ['$7', '$10', '$13', '$14']
    body += [f'pextlh\t{X[0]}, {W[1]}, {W[0]}', f'pextuh\t{X[1]}, {W[1]}, {W[0]}',
             f'pextlh\t{X[2]}, {W[3]}, {W[2]}', f'pextuh\t{X[3]}, {W[3]}, {W[2]}',
             f'pextlw\t$12, {X[2]}, {X[0]}', f'pextuw\t$15, {X[2]}, {X[0]}',
             f'pextlw\t{Q}, {X[3]}, {X[1]}', f'pextuw\t{R}, {X[3]}, {X[1]}',
             'sq\t$12, 0($9)', 'sq\t$15, 16($9)', f'sq\t{Q}, 32($9)', f'sq\t{R}, 48($9)',
             'addiu\t$8, $8, 32', 'addiu\t$9, $9, 64']
    pro, epi = save(used_regs(pre + body))
    return func(name, f'# {name}: MMI, a tile per iteration: T = (Q >> b) & 0x0101 | (R >> (b - 2)) & 0x0404\n'
                '# per pixel x = 7 - b (Q, R: the two quadwords of the tile), T | T >> 7 its 4 bits;\n'
                '# the 8x8 bytes transposed into rows (psllh, por, pextlh/uh, pextlw/uw), 4 sq.',
                pro + pre, body, 'bne\t$8, $11, 1b',
                epi + la('$4', 'chunky') + ['j\tapp_hash', 'ori\t$5, $0, 8192'])


def tile_decode(name):
    """A table of 256 doublewords: byte x of T[p] = bit 7 - x of p. A row of
    8 pixels is T[p0] | T[p1] << 1 | T[p2] << 2 | T[p3] << 3 (ld, dsll, or),
    one sd. A tile (8 rows) per iteration."""
    T = [sum(((p >> (7 - x)) & 1) << (8 * x) for x in range(8)) for p in range(256)]
    data = f'\t.section .rodata\n\t.p2align 3\n{name}_t:\n' + ''.join(
        f'\t.8byte\t{", ".join(hex(t) for t in T[i:i + 4])}\n' for i in range(0, 256, 4)) + '\t.text\n'
    pre = la('$8', 'tiles') + la('$9', 'chunky') + la('$10', f'{name}_t') + ['addiu\t$11, $8, 4096']
    body = []
    R = [['$2', '$3', '$4', '$5'], ['$6', '$7', '$12', '$13'], ['$14', '$15', '$24', '$25']]
    for y in range(8):
        r = R[y % 3]
        offs = [2 * y, 2 * y + 1, 16 + 2 * y, 17 + 2 * y]
        for k in range(4):
            body += [f'lbu\t{r[k]}, {offs[k]}($8)', f'sll\t{r[k]}, {r[k]}, 3', f'addu\t{r[k]}, {r[k]}, $10',
                     f'ld\t{r[k]}, 0({r[k]})']
        body += [f'dsll\t{r[1]}, {r[1]}, 1', f'dsll\t{r[2]}, {r[2]}, 2', f'dsll\t{r[3]}, {r[3]}, 3',
                 f'or\t{r[0]}, {r[0]}, {r[1]}', f'or\t{r[2]}, {r[2]}, {r[3]}', f'or\t{r[0]}, {r[0]}, {r[2]}',
                 f'sd\t{r[0]}, {8 * y}($9)']
    body += ['addiu\t$8, $8, 32', 'addiu\t$9, $9, 64']
    return data + func(name, f'# {name}: a table of 256 doublewords (byte x of T[p] = bit 7 - x of p): a\n'
                       '# row of 8 pixels is T[p0] | T[p1] << 1 | T[p2] << 2 | T[p3] << 3 (ld, dsll, or),\n'
                       '# one sd; a tile per iteration.',
                       pre, body, 'bne\t$8, $11, 1b', la('$4', 'chunky') + ['j\tapp_hash', 'ori\t$5, $0, 8192'])


# ---- N64 To CT16 (the hand/gen_n64.py study: seq, PREF 2 lines ahead after
# MMI op 30 of 48, no write PREF, 4 blocks per iteration) ----

def n64_to_ct16(name, U=4, pref_at=30, D=2):
    M1, M2 = '$2', '$3'
    REGS = ['$1', '$6', '$7', '$8', '$9', '$10', '$11', '$12', '$13', '$14',
            '$16', '$17', '$18', '$19', '$20', '$21', '$22', '$23', '$24', '$25']

    def blk(v, t, r, g, b):
        return [f'psrlh {t}, {v}, 8', f'psllh {v}, {v}, 8', f'por {v}, {v}, {t}',
                f'psrlh {r}, {v}, 11', f'psrlh {g}, {v}, 1', f'pand {g}, {g}, {M1}',
                f'psllh {b}, {v}, 9', f'pand {b}, {b}, {M2}', f'psllh {t}, {v}, 15',
                f'por {r}, {r}, {g}', f'por {r}, {r}, {b}', f'por {r}, {r}, {t}']

    def schedule(mmi, ls):
        out, done = [], 0
        pend = sorted(ls, key=lambda x: (x[2] if x[2] is not None else 1 << 30, x[1]))

        def due():
            return [x for x in pend if x[2] is not None and x[2] <= done]
        while done < len(mmi):
            for x in due():
                out.append(x[0]); pend.remove(x)
            out.append(mmi[done]); done += 1
            ready = [x for x in pend if x[1] <= done and (x[2] is None or done <= x[2])]
            if ready:
                out.append(ready[0][0]); pend.remove(ready[0])
            elif done < len(mmi) and not due():
                out.append(mmi[done]); done += 1
        return out, [x[0] for x in pend]
    regs = [REGS[5 * j:5 * j + 5] for j in range(U)]
    mmi, ls = [], [(f'pref 0, {64 * D}($4)', pref_at, None)]
    for j in range(U):
        v, t, r, g, b = regs[j]
        start = len(mmi)
        ls.append((f'lq {v}, {16 * j}($4)', max(0, start - 3), start))
        mmi += blk(v, t, r, g, b)
        ls.append((f'sq {r}, {16 * j}($5)', start + 12, None))
    body, rest = schedule(mmi, ls)
    tail = rest + [f'addiu $4, $4, {16 * U}']
    if len(body) % 2:
        body.append(tail.pop(0))
    L = ['lui $1, %hi(' + name + '_m)', 'lq $2, %lo(' + name + '_m)($1)', 'lq $3, %lo(' + name + '_m+16)($1)',
         'addiu $sp, $sp, -128'] + [f'sq ${16 + k}, {16 * k}($sp)' for k in range(8)]
    L += la('$4', 'n64_tex') + la('$5', 'ct16') + ['addiu $15, $4, 8192']
    data = (f'\t.section .rodata\n\t.p2align 4\n{name}_m:\n\t.word 0x03e003e0, 0x03e003e0, 0x03e003e0, 0x03e003e0\n'
            '\t.word 0x7c007c00, 0x7c007c00, 0x7c007c00, 0x7c007c00\n\t.text\n')
    parts = [L, '\t.p2align 3', '1:', body + tail + ['bne $4, $15, 1b', f'addiu $5, $5, {16 * U}'],
             [f'lq ${16 + k}, {16 * k}($sp)' for k in range(8)] + ['addiu $sp, $sp, 128'] +
             la('$4', 'ct16') + ['j app_hash', 'ori $5, $0, 8192']]
    return text(name, f'# {name}: MMI per 8 pixels: the bytes swapped (psrlh, psllh, por), the\n'
                '# fields moved by psrlh/psllh/pand and ORed; each lq, sq and PREF in an aligned\n'
                f'# pair with an MMI op (they pair only with a load or store); {U} blocks per\n'
                f'# iteration, PREF {D} lines ahead after MMI op {pref_at} (spaced from the store\n'
                '# that starts a line), no write PREF. From the hand-written study of this loop\n'
                '# (build/ps2-hw-test/src/hand/gen_n64.py): 21901 cycles per run, compiled 24657.',
                parts, data)


# ---- Guest loads ----

def guest_loads(name, mode='scalar', U=4, D=0, K=0):
    """sum = rotl1(sum) + bswap(page_table[a >> 12][(a & 4095) >> 2]) over
    guest_addr: the page offset (a >> 10) & 0x1c, the word offset a & 0xffc;
    the byte swap by shifts and masks (scalar) or by MMI on 4 words (mmi)."""
    pre = la('$8', 'guest_addr') + la('$9', 'page_table') + ['ori\t$10, $0, 16384', 'addu\t$10, $10, $8',
                                                            'move\t$2, $0', 'ori\t$11, $0, 0xff00', 'lui\t$12, 0xff']
    body = []
    W = ['$3', '$4', '$5', '$6']
    T = [['$7', '$13'], ['$14', '$15'], ['$24', '$25'], ['$16', '$17']]
    for k in range(U):
        a, p = T[k % 4]
        w = W[k % 4]
        body += [f'lw\t{a}, {4 * k}($8)', f'srl\t{p}, {a}, 10', f'andi\t{p}, {p}, 0x1c', f'addu\t{p}, {p}, $9',
                 f'lw\t{p}, 0({p})', f'andi\t{a}, {a}, 0xffc', f'addu\t{p}, {p}, {a}', f'lw\t{w}, 0({p})']
    if mode == 'scalar':
        for k in range(U):
            w = W[k % 4]
            a, p = T[k % 4]
            body += [f'srl\t{a}, {w}, 24', f'sll\t{p}, {w}, 24', f'or\t{a}, {a}, {p}',
                     f'srl\t{p}, {w}, 8', f'and\t{p}, {p}, $11', f'or\t{a}, {a}, {p}',
                     f'sll\t{p}, {w}, 8', f'and\t{p}, {p}, $12', f'or\t{w}, {a}, {p}']
    else:
        # 4 words into one register, swap the bytes of each halfword, then
        # the halfwords of each word; back to 4 GPRs (sign-extended)
        body += ['pextlw\t$18, $4, $3', 'pextlw\t$19, $6, $5', 'pcpyld\t$18, $19, $18',
                 'psrlh\t$19, $18, 8', 'psllh\t$18, $18, 8', 'por\t$18, $18, $19',
                 'psrlw\t$19, $18, 16', 'psllw\t$18, $18, 16', 'por\t$18, $18, $19',
                 'sll\t$3, $18, 0', 'dsra32\t$4, $18, 0', 'pcpyud\t$19, $18, $18', 'sll\t$5, $19, 0',
                 'dsra32\t$6, $19, 0']
    for k in range(U):
        w = W[k % 4]
        body += ['sll\t$20, $2, 1', 'srl\t$21, $2, 31', 'or\t$2, $20, $21', f'addu\t$2, $2, {w}']
    if D:
        body += [f'pref\t0, {64 * D}($8)']
    if K:
        # the host word of the load K ahead, translated and prefetched
        body += [f'lw\t$22, {4 * K}($8)', 'srl\t$23, $22, 10', 'andi\t$23, $23, 0x1c', 'addu\t$23, $23, $9',
                 'lw\t$23, 0($23)', 'andi\t$22, $22, 0xffc', 'addu\t$23, $23, $22', 'pref\t0, 0($23)']
    body += [f'addiu\t$8, $8, {4 * U}']
    sv, ep = save(used_regs(body))
    return func(name, f'# {name}: sum = rotl1(sum) + the byte-swapped word through page_table, {U}\n'
                '# loads per iteration: the page entry (a >> 10) & 0x1c, the word a & 0xffc; the\n'
                '# byte swap ' + ('by shifts and masks' if mode == 'scalar' else
                                  'by MMI on the 4 words (psrlh/psllh/por, then psrlw/psllw/por)') +
                f'.\n# PREF of guest_addr {D} lines ahead' + (f', of the host word of the load {K} ahead' if K else '') + '.',
                sv + pre, body, 'bne\t$8, $10, 1b', ep + ['jr\t$ra', 'nop'])
