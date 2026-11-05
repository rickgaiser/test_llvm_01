#!/usr/bin/env python3
"""Cycles per kernel and variant from a bench log, as a Markdown table."""
import collections, re, sys
t = collections.OrderedDict()
wrong = 0
for l in open(sys.argv[1], errors='replace'):
    m = re.match(r'b (\S+)\s+(\S+)\s+(\d+) (\S+)', l)
    if m:
        t.setdefault(m[1], {})[m[2]] = int(m[3])
        wrong += m[4] != 'ok'
v = ['gcc-Os', 'gcc-O2', 'clang-Os', 'clang-O2', 'clang-O2r', 'asm']
print('| Kernel | ' + ' | '.join(v) + ' | clang-O2 / gcc-O2 | asm / best C |')
print('|---|' + '---:|' * 8)
for k, r in t.items():
    if len(r) < 6:
        print('| %s | incomplete |' % k)
        continue
    b = min(r[x] for x in v[:5])
    print('| %s | ' % k + ' | '.join(str(r[x]) for x in v) +
          ' | %.2f | %.2f |' % (r['clang-O2'] / r['gcc-O2'], r['asm'] / b))
print('\nWrong results:', wrong)
