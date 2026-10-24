#!/usr/bin/env python3
"""VU micro-mode microbenchmarks: generate, assemble and table the probes.

Each probe is a body of micro instruction words ("upper ; lower"). The body
is unrolled K1 and K2 times inside a loop of R iterations, as two programs
with the same prologue and loop overhead. The EE times both (vuuarch.c), and

    cycles per body = (T(K2) - T(K1)) / (R * (K2 - K1))

which cancels the start, the wait for the end and the loop overhead. The EE
and the VUs run at the same clock, so EE cycles are VU cycles.

A probe with `show` also stores registers after the loop (its `post` words
write quadwords from data address 1), and the EE prints them: value checks,
for example whether a Q read waits for the divide.

    gen.py <out dir> <llvm bin dir>

writes <out>/probes/*.vsm (+ .vu.elf, .bin), <out>/probes.S (the code) and
<out>/probes.h (the table).
"""

import os
import re
import subprocess
import sys

R = 64
K1, K2 = 4, 12

# xyz (w is one more: ADDi adds I to vf0 = 0,0,0,1): vf1 chain value 0.5,
# vf2 = 1.0, vf3 = 0.5, vf4 = 2.0, vf5..vf12 = 2.0 (inputs of independent
# ops); ACC = 0.5 (w 1.5), vi2 = 0, vi3 = 1, vi4 = 0, data[0] = 0.
# vi7 is the loop counter (FCAND writes vi1). 1.0 is in ft of every multiply: exact.
PROLOGUE = [
    "nop ; sq.xyzw vf0, 0(vi0)",
    "nop ; loi 1.0",
    "addi.xyzw vf2, vf0, i ; loi 0.5",
    "addi.xyzw vf3, vf0, i ; nop",
    "addi.xyzw vf1, vf0, i ; loi 2.0",
    "addi.xyzw vf4, vf0, i ; iaddiu vi2, vi0, 0",
] + [f"addi.xyzw vf{r}, vf0, i ; nop" for r in range(5, 13)] + [
    "mula.xyzw acc, vf3, vf2 ; iaddiu vi3, vi0, 1",
    "nop ; iaddiu vi4, vi0, 0",
    "nop ; nop",
    "nop ; nop",
    "nop ; nop",
    "nop ; nop",
]

# name, vu, body words, model (cycles per body predicted by the VU target's
# current latencies, VUInstrInfo::getResultLatency), note. Optional keys:
# setup (words after the prologue), post (words after the loop), show
# (quadwords from data address 1 to print). {p} in setup is the program's
# label prefix.
PROBES = []


def probe(name, body, model, note, vu=0, **kw):
    PROBES.append(dict(name=name, vu=vu, body=body, model=model, note=note, **kw))


# --- FMAC latencies: a chain through vf1 -----------------------------------
probe("add", ["add.xyzw vf1, vf1, vf2 ; nop"], 4, "ADD -> ADD")
probe("mul", ["mul.xyzw vf1, vf1, vf2 ; nop"], 4, "MUL -> MUL")
probe("madd", ["madd.xyzw vf1, vf1, vf2 ; nop"], 4, "MADD -> MADD (fs)")
probe("addbc_fs", ["addx.xyzw vf1, vf1, vf2x ; nop"], 4, "ADDx -> ADDx (fs)")
probe("addbc_ft", ["addx.xyzw vf1, vf2, vf1x ; nop"], 4, "ADDx -> ADDx (bc field)")
probe("max", ["max.xyzw vf1, vf1, vf2 ; nop"], 4, "MAX -> MAX")
probe("mini", ["mini.xyzw vf1, vf1, vf2 ; nop"], 4, "MINI -> MINI")
probe("abs", ["abs.xyzw vf1, vf1 ; nop"], 4, "ABS -> ABS")
probe("addi", ["addi.xyzw vf1, vf1, i ; nop"], 4, "ADDi -> ADDi")
probe("addq", ["addq.xyzw vf1, vf1, q ; nop"], 4, "ADDq -> ADDq")
probe("ftoi_itof", ["ftoi0.xyzw vf1, vf1 ; nop", "itof0.xyzw vf1, vf1 ; nop"], 8,
      "FTOI0 -> ITOF0 -> FTOI0")
probe("move", ["nop ; move.xyzw vf1, vf1"], 4, "MOVE -> MOVE (lower)")
probe("mr32", ["nop ; mr32.xyzw vf1, vf1"], 4, "MR32 -> MR32 (lower)")
probe("add_move", ["add.xyzw vf1, vf1, vf2 ; nop", "nop ; move.xyzw vf1, vf1"], 8,
      "ADD -> MOVE -> ADD")
probe("add_x", ["add.x vf1, vf1, vf2 ; nop"], 4, "ADD.x -> ADD.x")
probe("fields", ["add.x vf1, vf1, vf2 ; nop", "add.y vf1, vf1, vf2 ; nop",
                 "add.z vf1, vf1, vf2 ; nop", "add.w vf1, vf1, vf2 ; nop"], 4,
      "x,y,z,w of one register: 16 if not tracked per field")
probe("indep4", [f"add.xyzw vf{r}, vf{r}, vf2 ; nop" for r in range(5, 9)], 4,
      "4 independent chains: 1 per cycle")
probe("indep2", [f"add.xyzw vf{r}, vf{r}, vf2 ; nop" for r in range(5, 7)], 4,
      "2 independent chains, latency bound")

# --- ACC ---------------------------------------------------------------------
probe("madda", ["madda.xyzw acc, vf2, vf3 ; nop"], 1, "MADDA -> MADDA (ACC)")
probe("mula_madd", ["mula.xyzw acc, vf1, vf3 ; nop", "madd.xyzw vf1, vf1, vf3 ; nop"], 5,
      "MULA -> MADD (ACC) -> MULA (vf1)")
probe("madd_acc", ["madda.xyzw acc, vf2, vf3 ; nop", "madd.xyzw vf5, vf2, vf3 ; nop"], 2,
      "MADDA -> MADD (ACC only)")
probe("opmula", ["opmula.xyz acc, vf1, vf2 ; nop", "opmsub.xyz vf1, vf1, vf2 ; nop"], 5,
      "OPMULA -> OPMSUB -> OPMULA (cross product)")

# --- flags -------------------------------------------------------------------
probe("clip", ["clipw.xyz vf4, vf3w ; nop"], 1, "CLIP throughput")
probe("clip_fcand", ["clipw.xyz vf4, vf3w ; nop", "nop ; fcand vi1, 1"], 5,
      "CLIP, FCAND: no stall, stale flags for 3 words (clip_read*)")
probe("add_fmand", ["add.xyzw vf5, vf2, vf3 ; nop", "nop ; fmand vi4, vi3"], 5,
      "ADD, FMAND: no stall, stale flags for 3 words (mac_read*)")

# --- FDIV and Q ----------------------------------------------------------------
probe("div", ["nop ; div q, vf2x, vf3x"], 7, "DIV throughput")
probe("sqrt", ["nop ; sqrt q, vf2x"], 7, "SQRT throughput")
probe("rsqrt", ["nop ; rsqrt q, vf2x, vf3x"], 13, "RSQRT throughput")
probe("div_waitq", ["nop ; div q, vf2x, vf3x", "nop ; waitq"], 8,
      "DIV, WAITQ (next DIV after the wait)")
probe("div_chain", ["nop ; div q, vf2x, vf1x", "nop ; waitq", "addq.x vf1, vf0, q ; nop"], 12,
      "DIV -> WAITQ -> ADDq -> DIV")
probe("div_mulq", ["nop ; div q, vf2x, vf1x", "mulq.x vf1, vf2, q ; waitq"], 11,
      "DIV -> MULq with WAITQ in its word -> DIV")
probe("div_fmac", ["nop ; div q, vf2x, vf3x"] +
      [f"add.xyzw vf{r}, vf{r}, vf2 ; nop" for r in range(5, 11)], 7,
      "DIV + 6 independent ADDs: FMAC runs during the divide")
probe("div_rsqrt", ["nop ; div q, vf2x, vf3x", "nop ; rsqrt q, vf2x, vf3x"], 20,
      "DIV then RSQRT: the divider is one unit")

# Q read N words after a DIV, without WAITQ: the old or the new Q? DIV 2/0.5
# = 4 and DIV 1/0.5 = 2 alternate, so an early read sees the other value.
for n in range(0, 8):
    probe(f"q_read{n}",
          ["nop ; div q, vf4x, vf3x"] + ["nop ; nop"] * n + ["addq.x vf5, vf0, q ; nop",
                                                              "nop ; waitq",
                                                              "nop ; div q, vf2x, vf3x"] +
          ["nop ; nop"] * n + ["addq.y vf5, vf0, q ; nop", "nop ; waitq"],
          None, f"Q read {n} words after DIV: x=40800000,y=40000000 if it waits",
          post=["nop ; nop"] * 4 + ["nop ; sq.xyzw vf5, 1(vi0)"], show=1)

# --- I register --------------------------------------------------------------
probe("loi_addi", ["nop ; loi 1.0", "addi.xyzw vf5, vf2, i ; nop"], 2, "LOI -> ADDi (+1)")
probe("loi_same", ["nop ; loi 1.0", "addi.x vf5, vf0, i ; loi 3.0", "nop ; nop"], 3,
      "ADDi with LOI in its word: x=3f800000 (1.0) if the old I is read",
      post=["nop ; nop"] * 4 + ["nop ; sq.xyzw vf5, 1(vi0)"], show=1)

# --- integer and memory -------------------------------------------------------
probe("iadd", ["nop ; iadd vi4, vi4, vi3"], 1, "IADD -> IADD")
probe("iaddiu", ["nop ; iaddiu vi4, vi4, 1"], 1, "IADDIU -> IADDIU")
probe("ilw", ["nop ; ilw.x vi4, 0(vi4)"], 4, "ILW -> ILW (address)")
probe("lq_add", ["nop ; lq.xyzw vf5, 0(vi0)", "add.xyzw vf6, vf5, vf2 ; nop"], 5,
      "LQ -> ADD (+1)")
probe("sq_lq", ["add.xyzw vf1, vf1, vf2 ; nop", "nop ; sq.xyzw vf1, 1(vi0)",
                "nop ; lq.xyzw vf1, 1(vi0)"], 9, "ADD -> SQ -> LQ -> ADD")
probe("mtir_mfir", ["nop ; mtir vi4, vf1x", "nop ; mfir.x vf1, vi4"], 5,
      "MTIR -> MFIR -> MTIR")
probe("iadd_mfir", ["nop ; iadd vi4, vi4, vi3", "nop ; mfir.x vf5, vi4"], 2,
      "IADD -> MFIR (+1)")

# --- branches ----------------------------------------------------------------
probe("b_taken", ["nop ; b b{n}", "nop ; nop", "b{n}: nop ; nop"], 3,
      "taken B, delay slot, target word")
probe("ibne_vi", ["nop ; iaddiu vi4, vi4, 1", "nop ; ibne vi4, vi0, b{n}", "nop ; nop",
                  "b{n}: nop ; nop"], 5,
      "IADDIU, IBNE: no stall, the branch reads the old VI (ibne_read0)")
probe("ibne_gap", ["nop ; iaddiu vi4, vi4, 1", "nop ; nop", "nop ; ibne vi4, vi0, b{n}",
                   "nop ; nop", "b{n}: nop ; nop"], 5, "IADDIU, NOP, IBNE")

# --- flag and branch reads: new or stale value? -------------------------------
# Each counts in vi10 (and vi11) how often the read saw the stale value, over
# all R * K bodies, and stores the counts at data address 1 (x, y).
COUNTS = ["nop ; nop"] * 4 + ["nop ; isw.x vi10, 1(vi0)", "nop ; isw.y vi11, 1(vi0)"]
ZERO = ["nop ; iaddiu vi10, vi0, 0", "nop ; iaddiu vi11, vi0, 0", "nop ; iaddiu vi8, vi0, 0xf"]
for d in range(0, 5):
    # MAC flag Z bits (xyzw = 0xf): SUB gives zeros (Z set), ADD 1+1 does not
    probe(f"mac_read{d}",
          ["sub.xyzw vf5, vf0, vf0 ; nop"] + ["nop ; nop"] * 4 +
          ["add.xyzw vf6, vf2, vf2 ; nop"] + ["nop ; nop"] * d +
          ["nop ; fmand vi4, vi8", "nop ; iadd vi10, vi10, vi4"] + ["nop ; nop"] * 4,
          None, f"FMAND {d} words after ADD: x=0 if it sees the ADD (x = stale count*15)",
          setup=ZERO, post=COUNTS, show=1)
    # clip flags: vf4 (2) against vf3.w (1.5) sets +x+y+z, vf2 (1) against
    # vf4.w (3) nothing
    probe(f"clip_read{d}",
          ["clipw.xyz vf4, vf3w ; nop"] + ["nop ; nop"] * d +
          ["nop ; fcand vi1, 0x3f", "nop ; iadd vi10, vi10, vi1"] + ["nop ; nop"] * 4 +
          ["clipw.xyz vf2, vf4w ; nop"] + ["nop ; nop"] * d +
          ["nop ; fcand vi1, 0x3f", "nop ; iadd vi11, vi11, vi1"] + ["nop ; nop"] * 4,
          None, f"FCAND {d} words after CLIP: x=R*K y=0 if it sees the CLIP",
          setup=ZERO, post=COUNTS, show=1)
# the raw clip flag: bits 0-5 the last CLIP, 6-11 the one before. vf4 (2)
# against vf3.w (1.5) sets +x +y +z (0x15) if fs is judged against ft.w.
probe("clip_get", ["clipw.xyz vf4, vf3w ; nop"] + ["nop ; nop"] * 4 +
      ["clipw.xyz vf2, vf4w ; nop"] + ["nop ; nop"] * 4, None,
      "FCGET: x=0x540 if CLIP judges fs.xyz against ft.w",
      post=["nop ; fcget vi12", "nop ; nop", "nop ; nop", "nop ; isw.x vi12, 1(vi0)"],
      show=1)
probe("clip_fcand_late", ["clipw.xyz vf4, vf3w ; nop"] + ["nop ; nop"] * 4 +
      ["nop ; fcand vi1, 0x3f"] + ["nop ; nop"] * 3 + ["nop ; iadd vi10, vi10, vi1"],
      None, "FCAND, 3 words, IADD: x=R*K if FCAND sees the CLIP",
      setup=ZERO, post=COUNTS, show=1)
# not taken = the branch read the old vi4 (0). The 0 is written 5 words
# earlier, out of the window in which a branch reads the value from before
# the writes (PCSX2 analyzeBranchVI; with the 0 written 2 words earlier the
# branch read the previous body's 1).
for d in range(0, 3):
    probe(f"ibne_read{d}",
          ["nop ; iaddiu vi4, vi0, 0"] + ["nop ; nop"] * 4 + ["nop ; iaddiu vi4, vi0, 1"] +
          ["nop ; nop"] * d +
          ["nop ; ibne vi4, vi0, t{n}", "nop ; nop", "nop ; iaddiu vi10, vi10, 1",
           "t{n}: nop ; nop"],
          None, f"IBNE {d} words after IADDIU: x=0 if it sees the new VI, R*K if the old",
          setup=ZERO, post=COUNTS, show=1)

# --- VU1: the same FMAC, and the EFU ------------------------------------------
probe("vu1_add", ["add.xyzw vf1, vf1, vf2 ; nop"], 4, "VU1 ADD -> ADD", vu=1)
probe("vu1_div", ["nop ; div q, vf2x, vf3x"], 7, "VU1 DIV throughput", vu=1)
for op, arg, model in [("esadd", "vf2", 10), ("esqrt", "vf2x", 11), ("esum", "vf2", 11),
                       ("ercpr", "vf2x", 11), ("ersadd", "vf2", 17), ("eleng", "vf2", 17),
                       ("ersqrt", "vf2x", 17), ("erleng", "vf2", 23), ("esin", "vf3x", 28),
                       ("eexp", "vf3x", 43), ("eatan", "vf3x", 53)]:
    probe(f"{op}_wait", [f"nop ; {op} p, {arg}", "nop ; waitp"], model + 1,
          f"{op.upper()}, WAITP (+1)", vu=1)
probe("efu_tp", ["nop ; ercpr p, vf2x"], 11, "ERCPR throughput", vu=1)
probe("efu_div", ["nop ; ercpr p, vf2x", "nop ; div q, vf2x, vf3x", "nop ; waitq",
                  "nop ; waitp"], 12, "ERCPR + DIV in parallel", vu=1)
probe("efu_mfp", ["nop ; ercpr p, vf2x", "nop ; waitp", "nop ; mfp.x vf5, p",
                  "add.x vf6, vf5, vf2 ; nop"], 17, "ERCPR, WAITP, MFP -> ADD", vu=1)


def program(p, k, pre):
    setup = [w.replace("{p}", pre) for w in p.get("setup", [])]
    words = PROLOGUE + setup + [f"nop ; iaddiu vi7, vi0, {R}", f"{pre}loop:"]
    for n in range(k):
        words += [w.replace("{n}", f"{pre}{n}").replace("{i}", str(n)) for w in p["body"]]
    words += [
        "nop ; isubiu vi7, vi7, 1",
        "nop ; nop",
        f"nop ; ibne vi7, vi0, {pre}loop",
        "nop ; nop",
    ]
    words += p.get("post", [])
    words += ["nop[e] ; nop", "nop ; nop"]
    # an even number of words: the K2 program starts on a quadword, so VU1
    # can upload each program on its own (vuuarch.c)
    if sum(1 for w in words if not re.match(r"\w+:\s*$", w)) % 2:
        words.append("nop ; nop")
    return words


def asm(words):
    out = []
    for w in words:
        m = re.match(r"(\w+):\s*(.*)$", w)
        if m:
            out.append(m.group(1) + ":")
            w = m.group(2)
            if not w:
                continue
        u, l = (s.strip() for s in w.split(";"))
        out.append(f"\t{u}\t\t{l}")
    return "\n".join(out)


def main():
    out, llvm = sys.argv[1], sys.argv[2]
    pdir = os.path.join(out, "probes")
    os.makedirs(pdir, exist_ok=True)
    blobs = ['.section .vutext, "a"']
    table = []
    for p in PROBES:
        name = p["name"]
        src = ['\t.section .vutext, "ax", @progbits', "k1:", asm(program(p, K1, "a_")),
               "k2:", asm(program(p, K2, "b_"))]
        text = "\n".join(src) + "\n"
        vsm = os.path.join(pdir, name + ".vsm")
        elf = os.path.join(pdir, name + ".vu.elf")
        binf = os.path.join(pdir, name + ".bin")
        with open(vsm, "w") as f:
            f.write(text)
        cpu = "vu1" if p["vu"] else "vu0"
        subprocess.run([f"{llvm}/llvm-mc", "-triple=vu", f"-mcpu={cpu}", "-filetype=obj",
                        vsm, "-o", elf], check=True)
        subprocess.run([f"{llvm}/llvm-objcopy", "-O", "binary", "--only-section=.vutext",
                        elf, binf], check=True)
        syms = subprocess.run([f"{llvm}/llvm-nm", elf], check=True, capture_output=True,
                              text=True).stdout
        k2 = next(int(l.split()[0], 16) for l in syms.splitlines() if l.endswith(" k2"))
        size = os.path.getsize(binf)
        limit = 16384 if p["vu"] else 4096
        if size > limit:
            sys.exit(f"{name}: {size} bytes do not fit VU{p['vu']} micro memory")
        blobs += [".balign 16", f".globl p_{name}", f"p_{name}:", f'.incbin "{binf}"',
                  f".globl p_{name}_end", f"p_{name}_end:"]
        model = -1 if p["model"] is None else int(p["model"] * 100)
        table.append(f'  {{"{name}", {p["vu"]}, p_{name}, p_{name}_end, {k2 // 8}, '
                     f'{model}, {p.get("show", 0)}, "{p["note"]}"}},')
    with open(os.path.join(out, "probes.S"), "w") as f:
        f.write("\n".join(blobs) + "\n")
    with open(os.path.join(out, "probes.h"), "w") as f:
        f.write("/* generated by gen.py */\n")
        f.write(f"#define R {R}\n#define K1 {K1}\n#define K2 {K2}\n")
        for p in PROBES:
            f.write(f"extern const unsigned char p_{p['name']}[], p_{p['name']}_end[];\n")
        f.write("static const struct probe probes[] = {\n" + "\n".join(table) + "\n};\n")


if __name__ == "__main__":
    main()
