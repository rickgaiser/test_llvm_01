#!/usr/bin/env python3
"""A list scheduler and cycle model for R5900 code, for the generators of the
hand-written kernels.

The model (EE Core User's Manual 2.12, and the bench's uarch tests on the
PS2): two instructions issue together when they are an aligned 8-byte pair
and do not conflict. A pair holds at most one load/store (also lwc1, swc1,
lqc2, sqc2, mtc1, mfc1, qmtc2, qmfc2, pref), one COP1 instruction (FPU
operations, lwc1, swc1, mtc1, mfc1) and one COP2 instruction (VU0 macro
operations, lqc2, sqc2, qmtc2, qmfc2). An MMI operation uses both integer
pipes: it only pairs with a GPR load or store. MAC1 (mult1, madd1) is in
pipe 1 with the loads and stores. The multipliers take a new operation every
2 cycles, the dividers every 37 (div) and 7 (div.s, vdiv).

Latencies, from the issue of an instruction to that of its consumer: GPR
loads 2, lwc1 and lqc2 2, mtc1 and qmtc2 2, mfc1 2, ALU and MMI 1, FPU and
VU0 operations 4 (5 to a store or a move to a GPR), chains through ACC 1
(FPU, VU0) and 2 (madd, pmaddw: their throughput), mult and madd 4 to a
GPR, div.s 7, vdiv 7 (to vwaitq/Q).

  schedule(lines) -> lines in a good order
  cycles(lines)   -> cycles of the lines as written (in-order issue)
"""
import re

LS_GPR = {'lb', 'lbu', 'lh', 'lhu', 'lw', 'lwu', 'ld', 'lq', 'sb', 'sh', 'sw',
          'sd', 'sq', 'pref'}
STORES = {'sb', 'sh', 'sw', 'sd', 'sq', 'swc1', 'sqc2'}
BRANCHES = {'b', 'beq', 'bne', 'bnez', 'beqz', 'bgtz', 'blez', 'bltz', 'bgez',
            'j', 'jr', 'jal', 'bc1t', 'bc1f'}


def parse(line):
    """(op, dests, srcs, kind) of an instruction line."""
    t = line.strip().split(None, 1)
    op = t[0]
    args = [a.strip() for a in t[1].split(',')] if len(t) > 1 else []
    base = op.split('.')[0]

    def regs(a):
        m = re.match(r'(-?\d+)?\((\$\w+)\)', a)
        if m:
            return [m.group(2)]
        r = re.match(r'(\$(?:vf)?\w+?)([xyzw]{1,4})?$', a)
        if a.startswith('$vf'):
            return [re.match(r'\$vf\d+', a).group(0)]
        return [a] if a.startswith('$') else []

    dests, srcs = [], []
    if op in STORES:
        srcs = regs(args[0]) + regs(args[1])
        kind = 'store'
    elif op == 'pref':
        srcs = regs(args[1])
    elif op in BRANCHES:
        srcs = [r for a in args for r in regs(a)]
    elif op in ('mula.s', 'madda.s', 'msuba.s', 'adda.s', 'suba.s'):
        srcs = regs(args[0]) + regs(args[1])
        dests = ['ACC']
        if op != 'mula.s' and op != 'adda.s' and op != 'suba.s':
            srcs.append('ACC')
    elif op in ('madd.s', 'msub.s'):
        dests = regs(args[0])
        srcs = regs(args[1]) + regs(args[2]) + ['ACC']
    elif op.startswith('v') and op not in ('vwaitq',):
        # VU0 macro: the first operand is the destination ($ACC or a VF),
        # vmadd*/vmsub* (not the ...a forms) read ACC, vdiv writes Q
        if base == 'vdiv' or base == 'vsqrt' or base == 'vrsqrt':
            dests = ['Q']
            srcs = [r for a in args[1:] for r in regs(a)]
        else:
            d = args[0]
            dests = ['VACC'] if d == '$ACC' else regs(d)
            srcs = [r for a in args[1:] for r in regs(a)]
            if re.match(r'v(madd|msub|opmsub)[a-z]*', base) and not d == '$ACC' or \
               re.match(r'v(madda|msuba)', base):
                srcs.append('VACC')
            if base.endswith('q') and base not in ('vwaitq',):
                srcs += ['Q', 'QW']
    elif op == 'vwaitq':
        # the readers of Q wait for it (QW): without it they read the old Q
        srcs = ['Q']
        dests = ['QW']
    elif op in ('mtsab', 'mtsah'):
        dests = ['SA']; srcs = regs(args[0])
    elif op == 'qfsrv':
        dests = regs(args[0]); srcs = regs(args[1]) + regs(args[2]) + ['SA']
    elif op in ('mtc1', 'qmtc2'):
        dests = regs(args[1]); srcs = regs(args[0])
    elif op in ('mfc1', 'qmfc2'):
        dests = regs(args[0]); srcs = regs(args[1])
    elif base in ('mult', 'multu', 'madd', 'maddu', 'mult1', 'multu1', 'madd1',
                  'maddu1'):
        hl = 'HL1' if op.endswith('1') else 'HL'
        if len(args) == 3:
            dests = regs(args[0]); srcs = regs(args[1]) + regs(args[2])
        else:
            srcs = regs(args[0]) + regs(args[1])
        dests.append(hl)
        if base.startswith('madd'):
            srcs.append(hl)
    elif op in ('mflo', 'mfhi'):
        dests = regs(args[0]); srcs = ['HL']
    elif op in ('mflo1', 'mfhi1'):
        dests = regs(args[0]); srcs = ['HL1']
    elif op in ('pmultw', 'pmaddw', 'pmulth', 'pmaddh'):
        dests = regs(args[0]) + ['HL', 'HL1']
        srcs = regs(args[1]) + regs(args[2])
        if op.startswith('pmadd'):
            srcs += ['HL', 'HL1']
    elif op in ('pmflo', 'pmfhi') or base == 'pmfhl':
        dests = regs(args[0]); srcs = ['HL', 'HL1']
    elif op in ('pmtlo', 'pmthi', 'pmthl.lw'):
        dests = ['HL', 'HL1']; srcs = regs(args[0])
    else:
        dests = regs(args[0]) if args else []
        srcs = [r for a in args[1:] for r in regs(a)]
    dests = [d for d in dests if d not in ('$0', '$zero')]
    srcs = [s for s in srcs if s not in ('$0', '$zero', '$vf0')]
    return op, dests, srcs


def unit(op):
    """The resources of an instruction: a set of 'LS', 'C1', 'C2', 'MMI',
    'MAC0', 'MAC1', 'ALU', 'BR', 'DIV1' (div.s), 'VDIV', 'DIV', 'DIV1I'."""
    base = op.split('.')[0]
    if op in LS_GPR:
        return {'LS'}
    if op in ('lwc1', 'swc1', 'mtc1', 'mfc1'):
        return {'LS', 'C1'}
    if op in ('lqc2', 'sqc2', 'qmtc2', 'qmfc2'):
        return {'LS', 'C2'}
    if op in BRANCHES:
        return {'BR'}
    if op == 'div.s':
        return {'C1', 'FDIV'}
    if op.endswith('.s') or op.startswith('cvt.') or op.startswith('c.'):
        return {'C1'}
    if base in ('vdiv', 'vsqrt', 'vrsqrt'):
        return {'C2', 'VDIV'}
    if op.startswith('v'):
        return {'C2'}
    if op in ('pmultw', 'pmaddw', 'pmulth', 'pmaddh'):
        return {'MMI', 'MAC0', 'MAC1'}
    if op.startswith('p') and op != 'pref':
        return {'MMI'}
    if base in ('mult', 'multu', 'madd', 'maddu'):
        return {'MAC0'}
    if base in ('mult1', 'multu1', 'madd1', 'maddu1'):
        return {'MAC1', 'LS'}      # pipe 1, with the loads and stores
    if base in ('div', 'divu'):
        return {'IDIV0'}
    if base in ('div1', 'divu1'):
        return {'IDIV1', 'LS'}
    return {'ALU'}


def latency(prod, cons):
    p = prod.split('.')[0]
    if prod in ('lw', 'lh', 'lhu', 'lb', 'lbu', 'ld', 'lwu', 'lq', 'lwc1', 'lqc2',
                'mtc1', 'qmtc2', 'mfc1'):
        return 2
    if prod in ('div.s',):
        return 7
    if prod == 'vwaitq':
        return 1
    if p in ('vdiv', 'vsqrt', 'vrsqrt'):
        return 7
    if prod in ('mula.s', 'madda.s', 'adda.s', 'msuba.s', 'suba.s') and \
       cons in ('madda.s', 'madd.s', 'msuba.s', 'msub.s'):
        return 1
    if prod.startswith('v') and cons.startswith('v') and \
       re.match(r'v(mula|madda|adda|msuba|suba|opmula)', p):
        return 1                       # ACC chain
    store_like = cons in STORES or cons in ('mfc1', 'qmfc2')
    if prod.endswith('.s') or prod.startswith('cvt.') or prod.startswith('v'):
        return 5 if store_like else 4
    MUL = ('mult', 'multu', 'madd', 'maddu', 'mult1', 'multu1', 'madd1',
           'maddu1', 'pmultw', 'pmaddw', 'pmulth', 'pmaddh')
    if p in MUL and cons.split('.')[0] in ('madd', 'maddu', 'madd1', 'maddu1',
                                           'pmaddw', 'pmaddh'):
        return 2                       # accumulator chain: at the throughput
    if p in MUL:
        return 4
    if p in ('div', 'divu', 'div1', 'divu1'):
        return 37
    return 1


BUSY = {'MAC0': 2, 'MAC1': 2, 'FDIV': 7, 'VDIV': 7, 'IDIV0': 37, 'IDIV1': 37}


def can_pair(u1, u2):
    if 'MMI' in u1 or 'MMI' in u2:
        other = u2 if 'MMI' in u1 else u1
        return other == {'LS'}
    if 'BR' in u1 and 'BR' in u2:
        return False
    for r in ('LS', 'C1', 'C2'):
        if r in u1 and r in u2:
            return False
    if 'MAC0' in u1 and 'MAC0' in u2 or 'MAC1' in u1 and 'MAC1' in u2:
        return False
    return True


class Sim:
    """In-order issue in aligned pairs."""

    def __init__(self):
        self.ready = {}        # register -> cycle its value can be used
        self.writer = {}       # register -> op that wrote it
        self.busy = {}         # unit -> first free cycle
        self.cycle = -1        # issue cycle of the last instruction
        self.pos = 0           # number of instructions issued
        self.last_units = None
        self.last_dests = []

    def issue_time(self, line):
        op, dests, srcs = parse(line)
        u = unit(op)
        t = self.cycle + 1
        if self.pos % 2 == 1 and self.last_units is not None and \
           can_pair(self.last_units, u):
            t = self.cycle
        for s in srcs:
            if s in self.ready:
                t = max(t, self.ready[s] + latency(self.writer[s], op))
        for r in u:
            if r in self.busy:
                t = max(t, self.busy[r])
        if t == self.cycle and self.pos % 2 == 1:
            # same cycle as the first of the pair: no dependency on it
            if any(s in self.last_dests for s in srcs):
                t = self.cycle + 1
        return t

    def step(self, line):
        t = self.issue_time(line)
        op, dests, srcs = parse(line)
        u = unit(op)
        for d in dests:
            self.ready[d] = t
            self.writer[d] = op
        for r in u:
            if r in BUSY:
                self.busy[r] = t + BUSY[r]
        self.cycle = t
        self.last_units = u if self.pos % 2 == 0 else None
        self.last_dests = dests
        self.pos += 1
        return t


def cycles(lines, align=0):
    s = Sim()
    s.pos = align
    t = 0
    for l in lines:
        t = s.step(l)
    return t + 1


LOADS = {'lb', 'lbu', 'lh', 'lhu', 'lw', 'lwu', 'ld', 'lq', 'lwc1', 'lqc2'}


def schedule(lines, keep_last=0, memdeps=False, noalias=()):
    """A good order of lines: each next instruction is the one that issues
    earliest (in the model), ties to the longest path to the end. Keeps the
    dependencies (also the order of the stores and of the writers and readers
    of a register), and the last keep_last lines last (the loop branch and its
    delay slot)."""
    body = lines[:len(lines) - keep_last] if keep_last else list(lines)
    tail = lines[len(lines) - keep_last:] if keep_last else []
    n = len(body)
    info = [parse(l) for l in body]
    preds = [set() for _ in range(n)]
    last_w, readers, last_store = {}, {}, None
    def base(line):
        m = re.search(r'\((\$\w+)\)', line)
        return m.group(1) if m else None
    for i, (op, dests, srcs) in enumerate(info):
        l = body[i]
        for s in srcs:
            if s in last_w:
                preds[i].add(last_w[s])
        for d in dests:
            if d in last_w:
                preds[i].add(last_w[d])
            for r in readers.get(d, []):
                if r != i:
                    preds[i].add(r)
        for s in srcs:
            readers.setdefault(s, []).append(i)
        for d in dests:
            last_w[d] = i
            readers[d] = []
        if op in STORES:
            if last_store is not None:
                preds[i].add(last_store)
            last_store = i
            if memdeps:
                # a store after the loads before it
                preds[i] |= set(k for k in range(i) if info[k][0] in LOADS and
                                info[k][2][-1:] != [] and base(body[k]) not in noalias)
        elif memdeps and op in LOADS and last_store is not None and base(l) not in noalias:
            # memdeps: no load moves above a store (they may alias)
            preds[i].add(last_store)
    succs = [[] for _ in range(n)]
    for i in range(n):
        for p in preds[i]:
            succs[p].append(i)
    prio = [0] * n
    for i in reversed(range(n)):
        op = info[i][0]
        prio[i] = max([prio[j] + latency(op, info[j][0]) for j in succs[i]] or [1])
    done, out, sim = set(), [], Sim()
    while len(done) < n:
        cand = [i for i in range(n) if i not in done and preds[i] <= done]
        best = min(cand, key=lambda i: (sim.issue_time(body[i]), -prio[i], i))
        sim.step(body[best])
        out.append(body[best])
        done.add(best)
    return out + tail
