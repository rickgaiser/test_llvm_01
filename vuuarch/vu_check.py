#!/usr/bin/env python3
"""Compare a vuuarch run log (PS2) with the VU target's model.

    vu_check.py <run log> [tolerance, default 0.10] [--mca <llvm bin dir>]

Each timing line is `vuN <name> <measured> model <model> <note>`; the model
column is the cycles per body that gen.py expects. With --mca the model is
instead what the VU target's scheduling model (VUSchedule.td) predicts:
llvm-mca runs the probe body of gen.py 100 and 200 times, and the difference
gives the cycles per body without the start. Lines without a model are value
checks and are skipped. Exits 1 if
any probe is off by more than the tolerance, except those in EXPECTED: there
the hardware does not stall, and the model's latency is a distance the
compiler keeps for correctness (the value checks show the stale reads).
"""

import os
import re
import subprocess
import sys
import tempfile

EXPECTED = {
    "clip_fcand": "clip flags are not interlocked (clip_read*)",
    "add_fmand": "MAC flags are not interlocked (mac_read*)",
    "ibne_vi": "a branch right after its VI write reads the old VI (ibne_read0)",
}

# With --mca: llvm-mca sees whole registers, without the fields of a VF
# register. The compiler tracks them (VUInstrInfo::getOperandLatency: an
# implicit use of the other fields has latency 1).
EXPECTED_MCA = {
    "fields": "llvm-mca does not track the fields of a VF register",
}

LINE = re.compile(r"^(?:\[\s*[\d.]+\]\s*)?vu(\d)\s+(\S+)\s+(-?[\d.]+)\s+model\s+(\S+)\s+(.*)$")


def mca_cycles(llvm, cpu, words, iterations):
    with tempfile.NamedTemporaryFile("w", suffix=".s") as f:
        f.write(words + "\n")
        f.flush()
        r = subprocess.run([f"{llvm}/llvm-mca", "-mtriple=vu", f"-mcpu={cpu}",
                            f"-iterations={iterations}", f.name],
                           capture_output=True, text=True)
    m = re.search(r"Total Cycles:\s+(\d+)", r.stdout)
    if not m:
        raise RuntimeError(r.stderr.strip().splitlines()[0] if r.stderr else "no output")
    return int(m.group(1))


def mca_models(llvm):
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import gen
    models = {}
    for p in gen.PROBES:
        if p["model"] is None:
            continue
        cpu = f"vu{p['vu']}"
        try:
            models[(str(p["vu"]), p["name"])] = (
                mca_cycles(llvm, cpu, gen.asm([w.replace("{n}", "0") for w in p["body"]]), 200) -
                mca_cycles(llvm, cpu, gen.asm([w.replace("{n}", "0") for w in p["body"]]), 100)) / 100
        except RuntimeError as e:
            print(f"vu{p['vu']} {p['name']:14} not modeled by llvm-mca: {e}")
    return models


def main():
    args = sys.argv[1:]
    models = None
    if "--mca" in args:
        i = args.index("--mca")
        models = mca_models(args[i + 1])
        del args[i:i + 2]
    path = args[0]
    tol = float(args[1]) if len(args) > 1 else 0.10
    total = bad = 0
    for line in open(path, errors="replace"):
        m = LINE.match(line.strip())
        if not m or m.group(4) == "-":
            continue
        vu, name, meas, model = m.group(1), m.group(2), float(m.group(3)), float(m.group(4))
        if models is not None:
            if (vu, name) not in models:
                continue
            model = models[(vu, name)]
        total += 1
        off = abs(meas - model) / model
        if off <= tol:
            continue
        if models is not None and name in EXPECTED_MCA:
            print(f"vu{vu} {name:14} {meas:7.2f} model {model:6.2f}  expected: {EXPECTED_MCA[name]}")
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
