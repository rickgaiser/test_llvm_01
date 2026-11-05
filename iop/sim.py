#!/usr/bin/env python3
"""Every variant of every kernel in a small MIPS I interpreter, with the load delay
of the real CPU: a bad hand-written loop should fail here and not hang the IOP.
Compares outputs with the first variant and counts instructions."""
import subprocess, sys

elf, prefix = sys.argv[1], sys.argv[2]
only = sys.argv[3:]
MASK = 0xffffffff

def sh(*c):
    return subprocess.run(c, capture_output=True, text=True, check=True).stdout

syms = {}
for l in sh(prefix + 'nm', elf).splitlines():
    p = l.split()
    if len(p) == 3:
        syms[p[2]] = int(p[0], 16)

import tempfile, os
tmp = tempfile.mkdtemp()
binf = os.path.join(tmp, 'b')
subprocess.run([prefix + 'objcopy', '-O', 'binary', elf, binf], check=True)
image = open(binf, 'rb').read()
BASE = 0x10000
SRC, DST, FIFO, STOP = 0x100000, 0x120000, 0x140000, 0xdead0000

class Bad(Exception):
    pass

def s32(v):
    return v - (1 << 32) if v & 0x80000000 else v

def s16(v):
    return v - 0x10000 if v & 0x8000 else v

def run(mem, pc, args, limit=3000000):
    r = [0] * 32
    r[4:8] = args
    r[29] = 0x1f0000
    r[31] = STOP
    hi = lo = 0
    pending = None          # (reg, value): a load, visible after the next instruction
    nxt = pc + 4
    count = 0
    mdist = 99              # instructions since mfhi/mflo
    def rd(a, n):
        a &= 0x1fffffff
        return int.from_bytes(mem[a:a + n], 'little')
    def wr(a, n, v):
        a &= 0x1fffffff
        mem[a:a + n] = (v & ((1 << (8 * n)) - 1)).to_bytes(n, 'little')
    while pc != STOP:
        count += 1
        if count > limit:
            raise Bad('too long at %x' % pc)
        i = rd(pc, 4)
        op, rs, rt, rd_, sa, fn = i >> 26, (i >> 21) & 31, (i >> 16) & 31, (i >> 11) & 31, (i >> 6) & 31, i & 63
        imm = i & 0xffff
        simm = s16(imm)
        A, B = r[rs], r[rt]
        target = None
        load = None
        dest = None; val = 0
        mdist += 1
        if op == 0:
            if fn == 0: dest, val = rd_, B << sa
            elif fn == 2: dest, val = rd_, B >> sa
            elif fn == 3: dest, val = rd_, s32(B) >> sa
            elif fn == 4: dest, val = rd_, B << (A & 31)
            elif fn == 6: dest, val = rd_, B >> (A & 31)
            elif fn == 7: dest, val = rd_, s32(B) >> (A & 31)
            elif fn == 8: target = A
            elif fn == 9: target = A; dest, val = rd_, pc + 8
            elif fn == 13: raise Bad('break at %x' % pc)
            elif fn == 16: dest, val = rd_, hi; mdist = 0
            elif fn == 17: hi = A
            elif fn == 18: dest, val = rd_, lo; mdist = 0
            elif fn == 19: lo = A
            elif fn in (24, 25, 26, 27):
                if mdist <= 2: raise Bad('HI/LO hazard at %x' % pc)
                if fn == 24: p = s32(A) * s32(B); lo, hi = p & MASK, (p >> 32) & MASK
                elif fn == 25: p = A * B; lo, hi = p & MASK, (p >> 32) & MASK
                elif B == 0: raise Bad('div 0 at %x' % pc)
                elif fn == 26:
                    a, b = s32(A), s32(B); q = abs(a) // abs(b); q = -q if (a < 0) != (b < 0) else q
                    lo, hi = q & MASK, (a - q * b) & MASK
                else: lo, hi = A // B, A % B
            elif fn in (32, 33): dest, val = rd_, A + B
            elif fn in (34, 35): dest, val = rd_, A - B
            elif fn == 36: dest, val = rd_, A & B
            elif fn == 37: dest, val = rd_, A | B
            elif fn == 38: dest, val = rd_, A ^ B
            elif fn == 39: dest, val = rd_, ~(A | B)
            elif fn == 42: dest, val = rd_, int(s32(A) < s32(B))
            elif fn == 43: dest, val = rd_, int(A < B)
            else: raise Bad('function %d at %x' % (fn, pc))
        elif op == 1:
            take = s32(A) < 0 if rt & 1 == 0 else s32(A) >= 0
            if rt & 16: dest, val = 31, pc + 8
            if take: target = pc + 4 + simm * 4
        elif op in (2, 3):
            target = ((pc + 4) & 0xf0000000) | ((i & 0x3ffffff) << 2)
            if op == 3: dest, val = 31, pc + 8
        elif op == 4:
            if A == B: target = pc + 4 + simm * 4
        elif op == 5:
            if A != B: target = pc + 4 + simm * 4
        elif op == 6:
            if s32(A) <= 0: target = pc + 4 + simm * 4
        elif op == 7:
            if s32(A) > 0: target = pc + 4 + simm * 4
        elif op in (8, 9): dest, val = rt, A + simm
        elif op == 10: dest, val = rt, int(s32(A) < simm)
        elif op == 11: dest, val = rt, int(A < (simm & MASK))
        elif op == 12: dest, val = rt, A & imm
        elif op == 13: dest, val = rt, A | imm
        elif op == 14: dest, val = rt, A ^ imm
        elif op == 15: dest, val = rt, imm << 16
        elif op in (32, 33, 35, 36, 37, 34, 38):
            ad = (A + simm) & MASK
            if op == 32: v = rd(ad, 1); v = v - 256 if v & 128 else v
            elif op == 33:
                if ad & 1: raise Bad('unaligned at %x' % pc)
                v = s16(rd(ad, 2))
            elif op == 35:
                if ad & 3: raise Bad('unaligned at %x' % pc)
                v = rd(ad, 4)
            elif op == 36: v = rd(ad, 1)
            elif op == 37:
                if ad & 1: raise Bad('unaligned at %x' % pc)
                v = rd(ad, 2)
            else:
                # lwl, lwr merge with the value in flight
                old = pending[1] if pending and pending[0] == rt else B
                w = rd(ad & ~3, 4); o = ad & 3
                if op == 34:
                    m = (1 << (8 * (o + 1))) - 1
                    v = ((w & m) << (8 * (3 - o))) | (old & ~(m << (8 * (3 - o))) & MASK)
                else:
                    v = (w >> (8 * o)) | (old & ~(MASK >> (8 * o)) & MASK)
            load = (rt, v & MASK)
        elif op in (40, 41, 43, 42, 46):
            ad = (A + simm) & MASK
            if op == 40: wr(ad, 1, B)
            elif op == 41:
                if ad & 1: raise Bad('unaligned at %x' % pc)
                wr(ad, 2, B)
            elif op == 43:
                if ad & 3: raise Bad('unaligned at %x' % pc)
                wr(ad, 4, B)
            else:
                w = rd(ad & ~3, 4); o = ad & 3
                if op == 42:
                    m = MASK >> (8 * (3 - o))
                    w = (w & ~m & MASK) | (B >> (8 * (3 - o)))
                else:
                    m = (MASK << (8 * o)) & MASK
                    w = (w & ~m & MASK) | ((B << (8 * o)) & MASK)
                wr(ad & ~3, 4, w)
        else:
            raise Bad('opcode %d at %x' % (op, pc))
        # the load before this one lands now, unless this instruction wrote the
        # register (measured) or loads it too (measured: the first is dropped)
        if pending and pending[0] != 0 and pending[0] != dest and not (load and load[0] == pending[0]):
            r[pending[0]] = pending[1]
        pending = load
        if dest:
            r[dest] = val & MASK
        pc, nxt = nxt, (target & MASK if target is not None else nxt + 4)
    if pending and pending[0]:
        r[pending[0]] = pending[1]
    return r[2], count

def lcg():
    v = 12345
    for _ in range(16384 // 4 + 16):
        v = (v * 1664525 + 1013904223) & MASK
        yield v

def prep(name):
    src = bytearray()
    for w in lcg():
        src += w.to_bytes(4, 'little')
    s, n, x = SRC, 4096, 0
    if name == 'cpy_unal': s = SRC + 2
    elif name == 'set_word': x = 0x5a5aa5a5
    elif name == 'str_len':
        for i in range(4096): src[i] |= 1
        src[4093] = 0
    elif name in ('bit_rev', 'mix_vol'): x = 0x2000
    elif name == 'gain_q16': n, x = 2048, 0x00412000
    elif name == 'fifo_rd': x = FIFO | 0xa0000000
    elif name == 'ring_put': n, x = 1024, 1536
    elif name == 'list_walk':
        for i in range(256):
            p = 0 if i == 255 else SRC + (i + 1) * 16
            src[i * 16:i * 16 + 4] = p.to_bytes(4, 'little')
    elif name == 'fat_scan':
        for i in range(1024): src[i * 4] |= 1
        src[1021 * 4:1021 * 4 + 4] = bytes(4)
    elif name == 'itoa10': n = 512
    elif name == 'mul64': n = 1024
    return src, s, n, x

kernels = sorted({k[4:] for k in syms if k.startswith('asm_')}, key=lambda k: syms['asm_' + k])
bad = 0
print('%-10s %9s %9s %9s %9s %9s %9s' % ('', 'gcc-Os', 'gcc-O2', 'clang-Os', 'clang-O2', 'clang-O2r', 'asm'))
for k in kernels:
    if only and k not in only:
        continue
    want = None
    row = []
    for v in ('gos_', 'go2_', 'cos_', 'co2_', 'cor_', 'asm_'):
        src, s, n, x = prep(k)
        mem = bytearray(0x200000)
        mem[BASE:BASE + len(image)] = image
        mem[SRC:SRC + len(src)] = src
        if k in ('bit_rev', 'mix_vol'):
            mem[DST:DST + len(src)] = src
        mem[FIFO:FIFO + 4] = (0x12345678).to_bytes(4, 'little')
        try:
            ret, count = run(mem, syms[v + k], [DST, s, n, x])
        except Bad as e:
            print(k, v, 'FAILED:', e); bad += 1; row.append('FAIL'); continue
        out = (ret, bytes(mem[DST:DST + 12288]))
        if want is None:
            want = out
        elif out != want:
            d = next((i for i in range(12288) if out[1][i] != want[1][i]), -1)
            print(k, v, 'DIFFERS: ret %x want %x, first byte %d' % (ret, want[0], d)); bad += 1
        row.append(str(count))
    print('%-10s %9s %9s %9s %9s %9s %9s' % (k, *row))
print('wrong:', bad)
sys.exit(1 if bad else 0)
