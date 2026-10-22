#!/usr/bin/env python3
"""Compare the R5900 scheduling model with the measured uarch kernels.

Rebuilds the loop of each KERNEL() in uarch_tests.c as assembly (the .word
encodings are replaced by the instruction in their comment), runs llvm-mca on
it and prints the predicted cycles per repetition next to the cycles measured
on the PS2 (a run.log of `bench.sh run`).

  mca_check.py [--mca llvm-mca] [--log run.log] [--table "VU0 macro mode"]
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
TOP = os.path.normpath(os.path.join(HERE, "../../.."))

# The function-like encoding macros.
R3_OPS = {
    "PADDW": "paddw", "PCGTW": "pcgtw", "PMAXW": "pmaxw", "PAND": "pand",
    "PEXTLW": "pextlw", "PPACW": "ppacw", "PPACH": "ppach", "PPACB": "ppacb",
    "PCPYLD": "pcpyld", "PCPYUD": "pcpyud", "PMULTW": "pmultw",
    "PMULTH": "pmulth", "MULT3": "mult",
}


def parse_source(path):
    text = open(path).read()
    # #define NAME W(0x...)  // asm
    words = {}
    for m in re.finditer(r"^#define (\w+) W\(0x[0-9a-fA-F]+\)\s*//\s*(.*)$",
                         text, re.M):
        words[m.group(1)] = m.group(2).strip()
    # #define NAME "asm\n"...  (string-only macros)
    strs = {}
    for m in re.finditer(r'^#define (\w+) ((?:"[^"]*"\s*)+)$', text, re.M):
        strs[m.group(1)] = "".join(re.findall(r'"([^"]*)"', m.group(2))).replace(
            "\\n", "\n")
    kernels = {}
    for m in re.finditer(r"^KERNEL\((\w+),\s*(\d+),\s*(\w+),(.*?)\)\s*$",
                         text, re.M | re.S):
        name, reps, body = m.group(1), int(m.group(2)), m.group(4)
        if "\nKERNEL" in body:
            continue
        kernels[name] = (reps, body)
    # {"label", k_name, reps, "unit", "note"}
    labels = {}
    for m in re.finditer(r'\{"([^"]+)",\s*(k_\w+),\s*(\d+),\s*"(\w+)"',
                         text):
        labels[m.group(2)] = m.group(1)
    return words, strs, kernels, labels


def expand(body, words, strs):
    """The assembly of a kernel body, or None if it uses an unknown macro."""
    out = []
    pos = 0
    tok = re.compile(r'\s*(?:"((?:[^"\\]|\\.)*)"|(\w+)\(([^)]*)\)|(\w+))')
    body = body.strip()
    while pos < len(body):
        m = tok.match(body, pos)
        if not m:
            return None
        pos = m.end()
        if m.group(1) is not None:
            out.append(m.group(1).replace("\\n", "\n"))
        elif m.group(2):
            fn, args = m.group(2), [a.strip() for a in m.group(3).split(",")]
            if fn in R3_OPS:
                d, s, t = args
                out.append(f"{R3_OPS[fn]} ${d}, ${s}, ${t}\n")
            elif fn == "MADD_S":
                out.append("madd.s $f{}, $f{}, $f{}\n".format(*args))
            elif fn == "MADDA_S":
                out.append("madda.s $f{}, $f{}\n".format(*args))
            elif fn == "MULA_S":
                out.append("mula.s $f{}, $f{}\n".format(*args))
            else:
                return None
        else:
            name = m.group(4)
            if name in words:
                out.append(words[name] + "\n")
            elif name in strs:
                out.append(strs[name])
            else:
                return None
    asm = "".join(out)
    # The comments use the VU manual spelling.
    asm = asm.replace("$ACC", "$acc").replace("$Q", "$q")
    return asm


def measured(log, table):
    res = {}
    on = table is None
    for line in open(log):
        if line.startswith("---"):
            on = table is None or table in line
            continue
        m = re.match(r"\s\s(.+?)\s+([\d.]+) cyc/", line)
        if on and m:
            res[m.group(1).strip()] = float(m.group(2))
    return res


def mca_cycles(mca, asm, reps, iters=100):
    loop = asm * reps + "addiu $2, $2, -1\nbnez $2, loop\nnop\n"
    src = ".set noreorder\n.set noat\nloop:\n" + loop
    with tempfile.NamedTemporaryFile("w", suffix=".s", delete=False) as f:
        f.write(src)
        path = f.name
    try:
        r = subprocess.run([mca, "-mtriple=mips64el", "-mcpu=r5900",
                            "-mattr=+vu0", f"-iterations={iters}", path],
                           capture_output=True, text=True)
    finally:
        os.unlink(path)
    m = re.search(r"Total Cycles:\s+(\d+)", r.stdout)
    if not m:
        return None, r.stderr.strip().splitlines()[:1]
    return int(m.group(1)) / (iters * reps), None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--mca", default=os.path.join(TOP, "build/llvm/bin/llvm-mca"))
    ap.add_argument("--log", default=os.path.join(TOP, "build/bench/gcc/run.log"))
    ap.add_argument("--table", default="VU0 macro mode")
    ap.add_argument("--src", default=os.path.join(HERE, "uarch_tests.c"))
    ap.add_argument("--tolerance", type=float, default=0.15)
    ap.add_argument("-v", action="store_true", help="print the assembly")
    a = ap.parse_args()

    words, strs, kernels, labels = parse_source(a.src)
    meas = measured(a.log, a.table or None)
    print(f"{'kernel':34} {'PS2':>7} {'model':>7}  ")
    bad = 0
    for kname, label in labels.items():
        if label not in meas or kname not in kernels:
            continue
        reps, body = kernels[kname]
        asm = expand(body, words, strs)
        if asm is None:
            print(f"{label:34} {meas[label]:7.2f} {'?':>7}  (unknown macro)")
            continue
        cyc, err = mca_cycles(a.mca, asm, reps)
        if cyc is None:
            print(f"{label:34} {meas[label]:7.2f} {'err':>7}  {err}")
            continue
        off = abs(cyc - meas[label]) > max(a.tolerance * meas[label], 0.3)
        bad += off
        print(f"{label:34} {meas[label]:7.2f} {cyc:7.2f}  {'<--' if off else ''}")
        if a.v:
            print("    " + asm.strip().replace("\n", "\n    "))
    print(f"\n{bad} kernel(s) off by more than {a.tolerance:.0%}")


if __name__ == "__main__":
    sys.exit(main())
