#!/usr/bin/env python3
"""Static efficiency of the VU kernels, from llc's assembly (no hardware).

    efficiency.py D vu0_poly.s vu1_poly.s

A VU executes one instruction word (upper + lower) per cycle. The packetizer
and the hazard pass already put the waits in as NOPs, so the words of the
loop are its cycles (no stalls are left, except for WAITQ/WAITP). The loop
is the innermost one: from its header to its branch and delay slot.

Useful work is D multiply-adds per lane, and the upper pipe peaks at one
4-lane FMAC instruction per cycle, so the ideal is D cycles per quadword.
"""
import re
import sys

BRANCH = re.compile(r"^(ib\w+|b|bal|jr|jalr)\b")


def loop_words(path):
    """The (upper, lower) words of the innermost loop."""
    lines = open(path).read().splitlines()
    start = next(i for i, l in enumerate(lines) if "Inner Loop Header" in l)
    words, branch_seen = [], False
    for l in lines[start + 1:]:
        t = l.split(";")[0].strip()
        if not t or t.startswith(".") or t.endswith(":"):
            continue
        # "upper<TAB>operands<TAB>lower<TAB>operands"; a nop has no operands
        f = t.split("\t")
        upper = f[0]
        lower = f[1] if upper.startswith("nop") else (f[2] if len(f) > 2 else "nop")
        words.append((upper, lower))
        if branch_seen:  # the delay slot
            break
        branch_seen = bool(BRANCH.match(words[-1][1]))
    return words


def main():
    d = int(sys.argv[1])
    print(f"| unit | cycles/quadword (loop words) | FMAC instructions | ideal (D={d}) | efficiency |")
    print("|---|---|---|---|---|")
    for path in sys.argv[2:]:
        words = loop_words(path)
        fmac = sum(1 for u, _ in words if not u.startswith("nop"))
        name = path.rsplit("/", 1)[-1].removesuffix(".s")
        print(f"| {name} | {len(words)} | {fmac} | {d} | {100 * d / len(words):.0f}% |")


if __name__ == "__main__":
    main()
