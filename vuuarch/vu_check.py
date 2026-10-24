#!/usr/bin/env python3
"""Compare a vuuarch run log (PS2) with the VU target's model.

    vu_check.py <run log> [tolerance, default 0.10]

Each timing line is `vuN <name> <measured> model <model> <note>`; the model
column is the cycles per body that the VU target's latencies predict
(gen.py). Lines without a model are value checks and are skipped. Exits 1 if
any probe is off by more than the tolerance, except those in EXPECTED: there
the hardware does not stall, and the model's latency is a distance the
compiler keeps for correctness (the value checks show the stale reads).
"""

import re
import sys

EXPECTED = {
    "clip_fcand": "clip flags are not interlocked (clip_read*)",
    "add_fmand": "MAC flags are not interlocked (mac_read*)",
    "ibne_vi": "a branch right after its VI write reads the old VI (ibne_read0)",
}

LINE = re.compile(r"^(?:\[\s*[\d.]+\]\s*)?vu(\d)\s+(\S+)\s+(-?[\d.]+)\s+model\s+(\S+)\s+(.*)$")


def main():
    path = sys.argv[1]
    tol = float(sys.argv[2]) if len(sys.argv) > 2 else 0.10
    total = bad = 0
    for line in open(path, errors="replace"):
        m = LINE.match(line.strip())
        if not m or m.group(4) == "-":
            continue
        vu, name, meas, model = m.group(1), m.group(2), float(m.group(3)), float(m.group(4))
        total += 1
        off = abs(meas - model) / model
        if off <= tol:
            continue
        if name in EXPECTED:
            print(f"vu{vu} {name:14} {meas:7.2f} model {model:6.2f}  expected: {EXPECTED[name]}")
            continue
        bad += 1
        print(f"vu{vu} {name:14} {meas:7.2f} model {model:6.2f}  OFF {off * 100:.0f}%")
    print(f"{total} probes, {bad} off by more than {tol * 100:.0f}%")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
