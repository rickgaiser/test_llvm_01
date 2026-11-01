#!/usr/bin/env python3
"""Generates the sound_tests.c kernels with parameters, their loops ordered
by sched.py: for asm/harness/gen_app.py (variants on the PS2) and
sound_file.py (asm/sound_tests.S). Every generator takes the symbol name
first and returns a u32 (void) kernel that ends with app_hash of its output,
as the C kernels do."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sched
from gfx import func, save, savef, used_regs, la, rodata, EST

PRELUDE = ''


def words(reg, value):
    """reg = value (sign-extended) in each of the 4 words."""
    return [f'addiu\t{reg}, $0, {value}' if -32768 <= value < 32768 else
            f'lui\t{reg}, {(value >> 16) & 0xffff}\n\tori\t{reg}, {reg}, {value & 0xffff}',
            f'pextlw\t{reg}, {reg}, {reg}', f'pcpyld\t{reg}, {reg}, {reg}']


def mix(name):
    """mix_out: 8 frames per iteration, MMI. Per voice: pmulth of the 8
    samples and the left (then right) volume in 8 halfwords; the products
    of the even frames are in rd, those of the odd frames by pmfhl.uw; each
    psraw 15 and paddw into the sums of the even and odd frames. Then pmaxw,
    pminw (sat16), pinteh pairs left and right, pextlw/pextuw put the frames
    in order."""
    # the volumes as halfword vectors on the stack: 16 x 16 bytes at 0($sp)
    pro = ['addiu\t$sp, $sp, -256'] + la('$8', 'voice_vol')
    for v in range(8):
        for c in range(2):
            pro += [f'lh\t$9, {4 * v + 2 * c}($8)', 'pcpyh\t$9, $9', 'pcpyld\t$9, $9, $9',
                    f'sq\t$9, {32 * v + 16 * c}($sp)']
    sv, ep = save(['$16', '$17', '$18', '$19', '$20', '$21'])
    pro = sv + pro
    pre = la('$8', 'voice_pcm') + la('$9', 'mix_out') + ['addiu\t$10, $8, 512'] + \
        words('$24', -32768) + words('$25', 32767)
    EL, OL, ER, OR = '$2', '$3', '$4', '$5'
    T = [['$6', '$7', '$11', '$12', '$13'], ['$14', '$15', '$16', '$17', '$18']]
    body = []
    for v in range(8):
        s, VL, VR, e, o = T[v % 2]
        body += [f'lq\t{s}, {512 * v}($8)', f'lq\t{VL}, {32 * v}($sp)', f'lq\t{VR}, {32 * v + 16}($sp)']
        for vol, ae, ao in ((VL, EL, OL), (VR, ER, OR)):
            body += [f'pmulth\t{e}, {s}, {vol}', f'pmfhl.uw\t{o}']
            if v == 0:
                body += [f'psraw\t{ae}, {e}, 15', f'psraw\t{ao}, {o}, 15']
            else:
                body += [f'psraw\t{e}, {e}, 15', f'psraw\t{o}, {o}, 15',
                         f'paddw\t{ae}, {ae}, {e}', f'paddw\t{ao}, {ao}, {o}']
    for r in (EL, OL, ER, OR):
        body += [f'pmaxw\t{r}, {r}, $24', f'pminw\t{r}, {r}, $25']
    body += [f'pinteh\t{EL}, {ER}, {EL}', f'pinteh\t{OL}, {OR}, {OL}',
             f'pextlw\t$19, {OL}, {EL}', f'pextuw\t$20, {OL}, {EL}',
             'sq\t$19, 0($9)', 'sq\t$20, 16($9)']
    body += ['addiu\t$8, $8, 16', 'addiu\t$9, $9, 32']
    post = ['addiu\t$sp, $sp, 256'] + ep + la('$4', 'mix_out') + ['j\tapp_hash', 'ori\t$5, $0, 1024']
    return func(name, '# mix: MMI, 8 frames per iteration. Per voice pmulth of the samples and the\n'
                '# left (then right) volume in 8 halfwords: the even frames\' products in rd, the\n'
                '# odd ones\' by pmfhl.uw; each psraw 15, paddw into the sums of the even and odd\n'
                '# frames. pmaxw/pminw saturate, pinteh pairs left and right, pextlw/pextuw\n'
                '# order the frames. The volume vectors are on the stack.',
                pro + pre, body, 'bne\t$8, $10, 1b', post)


def adpcm(name):
    """Scalar: the chain from one sample to the next is mult (f0 * old),
    addu, sra 6, pmaxw, pminw: x * 64 + f1 * older + 32 is added before the
    shift (x + (y >> 6) = (64 x + y) >> 6), off the chain. The samples
    rotate through 4 registers (28 per block); 64 x = (n << 28) >> (10 +
    shift)."""
    R = ['$11', '$12', '$13', '$14']   # s[n % 4]
    pre = la('$8', 'adpcm') + la('$9', 'pcm') + la('$24', f'{name}_coef') + [
        'addiu\t$10, $8, 1024', 'move\t$11, $0', 'move\t$12, $0', 'move\t$13, $0', 'move\t$14, $0'] + \
        ['addiu\t$2, $0, -32768', 'addiu\t$3, $0, 32767']
    # block header: shift amount 10 + shift in $15, f0 $4, f1 $5
    head = ['lbu\t$6, 0($8)', 'andi\t$15, $6, 0xf', 'addiu\t$15, $15, 10', 'srl\t$6, $6, 4',
            'sll\t$6, $6, 3', 'addu\t$6, $6, $24', 'lw\t$4, 0($6)', 'lw\t$5, 4($6)']
    body = list(head)
    # samples: the previous two are R[(n-1) % 4] and R[(n-2) % 4]; at the
    # start of a block n = 0, so old is R[3] and older R[2]
    X = ['$16', '$17', '$18', '$19', '$20', '$21', '$22', '$23']
    Y = ['$6', '$25']
    for n in range(28):
        old, older, sm = R[(n - 1) % 4], R[(n - 2) % 4], R[n % 4]
        x, y = X[n % 8], Y[n % 2]
        if n % 2 == 0:
            body += [f'lbu\t$7, {2 + n // 2}($8)', f'sll\t{x}, $7, 28']
        else:
            body += [f'andi\t{x}, $7, 0xf0', f'sll\t{x}, {x}, 24']
        body += [f'srav\t{x}, {x}, $15',                 # 64 x
                 f'addiu\t{x}, {x}, 32', f'mult1\t{y}, $5, {older}', f'addu\t{x}, {x}, {y}',
                 f'mult\t$1, $4, {old}', f'addu\t{sm}, {x}, $1', f'sra\t{sm}, {sm}, 6',
                 f'pmaxw\t{sm}, {sm}, $2', f'pminw\t{sm}, {sm}, $3', f'sh\t{sm}, {2 * n}($9)']
    body += ['addiu\t$8, $8, 16', 'addiu\t$9, $9, 56']
    pro, epi = save(['$16', '$17', '$18', '$19', '$20', '$21', '$22', '$23'])
    data = rodata(f'{name}_coef', []).replace('.float\t', '.word\t0, 0, 60, 0, 115, -52, 98, -55, 122, -60')
    return data + func(name, '# adpcm_decode: scalar, the chain from a sample to the next is mult (f0 *\n'
                       '# old), addu, sra 6, pmaxw, pminw (sat16): 64 x + f1 * older + 32 is added\n'
                       '# before the shift, off the chain (x + (y >> 6) = (64 x + y) >> 6), with\n'
                       '# 64 x = (n << 28) >> (10 + shift) and f1 * older by mult1. A block (28\n'
                       '# samples) per iteration, the samples rotating through 4 registers.',
                       pro + pre, body, 'bne\t$8, $10, 1b',
                       epi + la('$4', 'pcm') + ['j\tapp_hash', 'ori\t$5, $0, 3584'])


def fir(name):
    """MMI, 8 outputs per iteration in the 8 lanes of HI:LO: for each tap
    k = 8q + r, pmaddh of the inputs in[i + k .. i + k + 7] (qfsrv of the
    aligned blocks q and q + 1 by r halfwords: mtsah once per r) and c[k]
    in 8 halfwords (a table on the stack). Then pmfhl.lw/uw, + 16384,
    psraw 15, pmaxw/pminw, pinteh, sq."""
    pro = ['addiu\t$sp, $sp, -512'] + la('$8', 'fir_coef')
    for k in range(32):
        pro += [f'lh\t$9, {2 * k}($8)', 'pcpyh\t$9, $9', 'pcpyld\t$9, $9, $9', f'sq\t$9, {16 * k}($sp)']
    pre = la('$8', 'fir_in') + la('$9', 'fir_out') + ['addiu\t$10, $9, 1024'] + \
        words('$11', 16384) + words('$24', -32768) + words('$25', 32767)
    B = ['$2', '$3', '$4', '$5', '$6']
    V = ['$7', '$12', '$13', '$14']
    Cr = ['$15', '$16', '$17', '$18']
    body = [f'lq\t{B[q]}, {16 * q}($8)' for q in range(5)]
    first = True
    n = 0
    for r in range(8):
        if r:
            body.append(f'mtsah\t$0, {r}')
        for q in range(4):
            k = 8 * q + r
            c = Cr[n % 4]
            body.append(f'lq\t{c}, {16 * k}($sp)')
            if r:
                v = V[n % 4]
                body.append(f'qfsrv\t{v}, {B[q + 1]}, {B[q]}')
            else:
                v = B[q]
            body.append(f'{"pmulth" if first else "pmaddh"}\t$0, {v}, {c}')
            first = False
            n += 1
    body += ['pmfhl.lw\t$19', 'pmfhl.uw\t$20']
    for x in ('$19', '$20'):
        body += [f'paddw\t{x}, {x}, $11', f'psraw\t{x}, {x}, 15', f'pmaxw\t{x}, {x}, $24',
                 f'pminw\t{x}, {x}, $25']
    body += ['pinteh\t$19, $20, $19', 'sq\t$19, 0($9)', 'addiu\t$8, $8, 16', 'addiu\t$9, $9, 16']
    sv, ep = save(['$16', '$17', '$18', '$19', '$20'])
    return func(name, '# fir: MMI, 8 outputs per iteration in the 8 lanes of HI:LO: per tap k = 8q + r\n'
                '# pmaddh of in[i + k ..] (qfsrv of the aligned blocks q and q + 1 by r\n'
                '# halfwords, mtsah once per r) and c[k] in 8 halfwords (a table on the\n'
                '# stack); then pmfhl.lw/uw, + 16384, psraw 15, pmaxw/pminw, pinteh.',
                sv + pro + pre, body, 'bne\t$9, $10, 1b',
                ['addiu\t$sp, $sp, 512'] + ep + la('$4', 'fir_out') + ['j\tapp_hash', 'ori\t$5, $0, 1024'])


def resample(name, U=4):
    """Scalar, U outputs per iteration: j * 2 = (pos >> 15) & ~1, frac =
    (pos >> 1) & 0x7fff, a + ((b - a) * frac >> 15) by mult (rd form)."""
    step = (44100 << 16) // 48000
    pre = la('$8', 'rs_in') + la('$9', 'rs_out') + ['ori\t$10, $0, 2048', 'addu\t$10, $10, $9',
                                                    'move\t$11, $0', f'ori\t$1, $0, {step}']
    body = []
    T = [['$2', '$3', '$4', '$5', '$6'], ['$7', '$12', '$13', '$14', '$15'],
         ['$16', '$17', '$18', '$19', '$20'], ['$21', '$22', '$23', '$24', '$25']]
    prev = '$11'
    for k in range(U):
        p, j, f, a, b = T[k % 4]
        body += [f'move\t{p}, $11' if k == 0 else f'addu\t{p}, {prev}, $1',
                 f'srl\t{j}, {p}, 15', f'andi\t{j}, {j}, 0xfffe', f'addu\t{j}, {j}, $8',
                 f'lh\t{a}, 0({j})', f'lh\t{b}, 2({j})', f'srl\t{f}, {p}, 1', f'andi\t{f}, {f}, 0x7fff',
                 f'subu\t{b}, {b}, {a}', f'mult\t{b}, {b}, {f}', f'sra\t{b}, {b}, 15', f'addu\t{a}, {a}, {b}',
                 f'sh\t{a}, {2 * k}($9)']
        prev = p
    body += [f'addu\t$11, {prev}, $1', f'addiu\t$9, $9, {2 * U}']
    sv, ep = save(used_regs(body))
    return func(name, f'# resample: scalar, {U} outputs per iteration: j * 2 = (pos >> 15) & ~1,\n'
                '# frac = (pos >> 1) & 0x7fff, a + ((b - a) * frac >> 15) with mult (rd form).',
                sv + pre, body, 'bne\t$9, $10, 1b',
                ep + la('$4', 'rs_out') + ['j\tapp_hash', 'ori\t$5, $0, 2048'])


def correlate(name):
    """MMI: the lags in groups of r = lag % 8: the input shifted by r
    halfwords (qfsrv, mtsah once) into a buffer on the stack, then for the
    4 lags 8q + r: pmaddh of 64 blocks of the input and of the shifted
    buffer from block q, into HI:LO; the 8 lanes summed."""
    out = [f'\n# {name}: MMI, the lags in groups by r = lag % 8: the input shifted by r\n'
           '# halfwords (qfsrv, mtsah once per r) into a buffer on the stack (r = 0: the input\n'
           '# itself), then for the 4 lags 8q + r: pmaddh of the 64 blocks of the input with\n'
           '# the shifted blocks from q on, into HI:LO (2 lq per pmaddh); the 8 lanes summed\n'
           '# by pmfhl.lw/uw, paddw, pcpyud, addu.',
           '\t.p2align 3', f'\t.globl\t{name}', f'\t.ent\t{name}', f'{name}:']
    L = ['addiu\t$sp, $sp, -1088'] + la('$8', 'corr_in') + la('$9', 'corr_out') + ['move\t$10, $0']
    # $10 = r (0..7)
    out += [f'\t{l}' for l in L]
    # per r: the shifted buffer S (68 blocks, the last one unused) at 0($sp);
    # r = 0 uses the input directly
    out += ['\t.p2align 3', '1:\tbeqz\t$10, 3f', '\tmove\t$11, $8',
            '\tsll\t$1, $10, 1', '\tmtsab\t$1, 0',
            '\tmove\t$12, $8', '\tmove\t$11, $sp', '\taddiu\t$13, $sp, 1088', '\tlq\t$2, 0($12)']
    sh = ['lq\t$3, 16($12)', 'qfsrv\t$4, $3, $2', 'sq\t$4, 0($11)', 'lq\t$2, 32($12)',
          'qfsrv\t$5, $2, $3', 'sq\t$5, 16($11)', 'addiu\t$12, $12, 32']
    lp = sched_loop(sh + ['addiu\t$11, $11, 32'], 'bne\t$11, $13, 2b', '2')
    out += [lp.rstrip('\n'), '\tmove\t$11, $sp']
    # $11: the shifted blocks; 4 lags
    out += ['3:\tmove\t$14, $0']
    # per q: $15 = S + 16 q
    out += ['4:\tsll\t$15, $14, 4', '\taddu\t$15, $15, $11', '\tmove\t$12, $8',
            '\tlq\t$2, 0($12)', '\tlq\t$3, 0($15)', '\tpmulth\t$0, $2, $3',
            '\taddiu\t$12, $12, 16', '\taddiu\t$15, $15, 16']
    body = []
    A = ['$2', '$4', '$6', '$24']
    Bv = ['$3', '$5', '$7', '$25']
    for k in range(8):
        a, b = A[k % 4], Bv[k % 4]
        body += [f'lq\t{a}, {16 * k}($12)', f'lq\t{b}, {16 * k}($15)', f'pmaddh\t$0, {a}, {b}']
    body += ['addiu\t$12, $12, 128', 'addiu\t$15, $15, 128']
    # 63 more blocks: 7 iterations of 8 and 7 single
    lp = sched_loop(body, 'bne\t$12, $13, 5b', '5')
    out += ['\taddiu\t$13, $8, 912']   # 16 + 7 * 128
    out += [lp.rstrip('\n')]
    rest = []
    for k in range(7):
        a, b = A[k % 4], Bv[k % 4]
        rest += [f'lq\t{a}, {16 * k}($12)', f'lq\t{b}, {16 * k}($15)', f'pmaddh\t$0, {a}, {b}']
    rest += ['pmfhl.lw\t$2', 'pmfhl.uw\t$3', 'paddw\t$2, $2, $3', 'pcpyud\t$3, $2, $2', 'paddw\t$2, $2, $3', 'dsra32\t$3, $2, 0', 'sll\t$2, $2, 0', 'addu\t$2, $2, $3']
    out += [f'\t{l}' for l in sched.schedule(rest)]
    # corr_out[8q + r]
    out += ['\tsll\t$1, $14, 5', '\tsll\t$4, $10, 2', '\taddu\t$1, $1, $4', '\taddu\t$1, $1, $9',
            '\tsw\t$2, 0($1)', '\taddiu\t$14, $14, 1', '\taddiu\t$1, $0, 4', '\tbne\t$14, $1, 4b', '\tnop',
            '\taddiu\t$10, $10, 1', '\taddiu\t$1, $0, 8', '\tbne\t$10, $1, 1b', '\tnop',
            '\taddiu\t$sp, $sp, 1088']
    out += [f'\t{l}' for l in la('$4', 'corr_out') + ['j\tapp_hash', 'ori\t$5, $0, 128']]
    out.append(f'\t.end\t{name}')
    return '\n'.join(out) + '\n'


def sched_loop(body, branch, label):
    from fpu_novu0 import loop
    return loop(body, branch, label)


def energy(name, unit, U=8, D=4):
    """Sums of squares of 16 blocks of 256 floats. VU0: vmadda of each
    vector with itself into ACC (1 cycle a step), the 4 lanes added at the
    end of a block (exact: the inputs are multiples of 1/16). FPU: a chain
    through ACC (madda.s), one lwc1 and one madda.s per sample."""
    pre = la('$8', 'energy_in') + la('$9', 'energy_out') + ['ori\t$10, $0, 16384', 'addu\t$10, $10, $8']
    out = [f'\n# {name}: the sums of squares of 16 blocks of 256 floats; ' +
           ('VU0: vmadda of each vector with\n# itself into ACC (1 cycle a step), the 4 lanes added at the end of a block\n'
            '# (exact: the inputs are multiples of 1/16)' if unit == 'vu0' else
            'FPU: a chain through ACC\n# (madda.s), one lwc1 and one madda.s per sample') +
           f'; {U} {"vectors" if unit == "vu0" else "samples"} per iteration,\n# PREF {D} lines ahead.',
           '\t.p2align 3', f'\t.globl\t{name}', f'\t.ent\t{name}', f'{name}:'] + [f'\t{l}' for l in pre]
    body = []
    if unit == 'vu0':
        for k in range(U):
            body += [f'lqc2\t$vf{1 + k}, {16 * k}($8)', f'vmadda.xyzw\t$ACC, $vf{1 + k}, $vf{1 + k}']
        per = 16 * U
        start = ['vsuba.xyzw\t$ACC, $vf0, $vf0']
        end = ['vmaddx.xyzw\t$vf20, $vf0, $vf0x', 'vaddy.x\t$vf20, $vf20, $vf20y',
               'vaddz.x\t$vf20, $vf20, $vf20z', 'vaddw.x\t$vf20, $vf20, $vf20w', 'qmfc2\t$2, $vf20',
               'sw\t$2, 0($9)']
    else:
        for k in range(U):
            body += [f'lwc1\t$f{k % 16}, {4 * k}($8)', f'madda.s\t$f{k % 16}, $f{k % 16}']
        per = 4 * U
        start = ['mtc1\t$0, $f16', 'mula.s\t$f16, $f16']
        end = ['madd.s\t$f17, $f16, $f16', 'swc1\t$f17, 0($9)']
    nl = max(1, per // 64)
    if D:
        body += [f'pref\t0, {64 * (D + l)}($8)' for l in range(nl)]
    body += [f'addiu\t$8, $8, {per}']
    lp = sched_loop(body, 'bne\t$8, $11, 2b', '2')
    out += ['\t.p2align 3', '1:\t' + start[0]] + [f'\t{l}' for l in start[1:]] + ['\taddiu\t$11, $8, 1024']
    out += [lp.rstrip('\n')] + [f'\t{l}' for l in end] + ['\tbne\t$8, $10, 1b', '\taddiu\t$9, $9, 4']
    out += [f'\t{l}' for l in la('$4', 'energy_out') + ['j\tapp_hash', 'ori\t$5, $0, 64']]
    out.append(f'\t.end\t{name}')
    return '\n'.join(out) + '\n'
