#!/usr/bin/env python3
"""vuk: a tiny kernel language for the PS2 Vector Units, compiled to LLVM IR.

    vuk.py kernel.vuk -o kernel.ll [-H kernel.h] [-D NAME=VALUE ...]

The IR is for `llc -mtriple=vu`. See vuk.md for the language.
"""

import argparse
import re
import struct
import sys

# ---------------------------------------------------------------- lexer

TOKEN_RE = re.compile(r"""
    (?P<ws>\s+|//[^\n]*)
  | (?P<num>\d+\.\d+|\d+)
  | (?P<name>[A-Za-z_][A-Za-z0-9_]*)
  | (?P<op>\.\.|[-+*/()\[\]{}:;,=])
""", re.VERBOSE)

KEYWORDS = {"const", "kernel", "var", "for", "in"}


class VukError(Exception):
    pass


def lex(src):
    toks, pos, line = [], 0, 1
    while pos < len(src):
        m = TOKEN_RE.match(src, pos)
        if not m:
            raise VukError(f"line {line}: unexpected character {src[pos]!r}")
        kind, text = m.lastgroup, m.group()
        if kind != "ws":
            if kind == "name" and text in KEYWORDS:
                kind = text
            toks.append((kind, text, line))
        line += text.count("\n")
        pos = m.end()
    toks.append(("eof", "", line))
    return toks


# ---------------------------------------------------------------- parser
#
# AST nodes are tuples: ("num", v) ("name", n) ("index", name, expr)
# ("bin", op, a, b) ("neg", a); statements ("var", n, e) ("set", target, e)
# ("for", var, lo, hi, body).

class Parser:
    def __init__(self, toks):
        self.toks, self.i = toks, 0

    def peek(self, kind=None, text=None):
        k, t, _ = self.toks[self.i]
        return (kind is None or k == kind) and (text is None or t == text)

    def take(self, kind=None, text=None):
        k, t, line = self.toks[self.i]
        if not self.peek(kind, text):
            raise VukError(f"line {line}: expected {text or kind}, got {t or k!r}")
        self.i += 1
        return t

    def line(self):
        return self.toks[self.i][2]

    def program(self):
        consts, kernels = [], []
        while not self.peek("eof"):
            if self.peek("const"):
                self.take("const")
                name = self.take("name")
                self.take("op", "=")
                consts.append((name, self.expr()))
                self.take("op", ";")
            else:
                kernels.append(self.kernel())
        return consts, kernels

    def kernel(self):
        self.take("kernel")
        name = self.take("name")
        self.take("op", "(")
        params = []
        while not self.peek("op", ")"):
            pname = self.take("name")
            self.take("op", ":")
            params.append((pname, self.type()))
            if not self.peek("op", ")"):
                self.take("op", ",")
        self.take("op", ")")
        return name, params, self.block()

    def type(self):
        base = self.take("name")
        if base not in ("i16", "f32", "f32x4"):
            raise VukError(f"line {self.line()}: unknown type {base}")
        if not self.peek("op", "["):
            return (base, None)
        self.take("op", "[")
        size = None if self.peek("op", "]") else self.expr()
        self.take("op", "]")
        return (base, size if size is not None else "unsized")

    def block(self):
        self.take("op", "{")
        body = []
        while not self.peek("op", "}"):
            body.append(self.stmt())
        self.take("op", "}")
        return body

    def stmt(self):
        line = self.line()
        if self.peek("var"):
            self.take("var")
            name = self.take("name")
            self.take("op", "=")
            e = self.expr()
            self.take("op", ";")
            return ("var", name, e, line)
        if self.peek("for"):
            self.take("for")
            v = self.take("name")
            self.take("in")
            lo = self.expr()
            self.take("op", "..")
            hi = self.expr()
            return ("for", v, lo, hi, self.block(), line)
        target = self.postfix()
        self.take("op", "=")
        e = self.expr()
        self.take("op", ";")
        return ("set", target, e, line)

    def expr(self):
        a = self.term()
        while self.peek("op", "+") or self.peek("op", "-"):
            op = self.take()
            a = ("bin", op, a, self.term())
        return a

    def term(self):
        a = self.unary()
        while self.peek("op", "*") or self.peek("op", "/"):
            op = self.take()
            a = ("bin", op, a, self.unary())
        return a

    def unary(self):
        if self.peek("op", "-"):
            self.take()
            return ("neg", self.unary())
        return self.postfix()

    def postfix(self):
        if self.peek("num"):
            return ("num", self.take())
        if self.peek("op", "("):
            self.take()
            e = self.expr()
            self.take("op", ")")
            return e
        name = self.take("name")
        if self.peek("op", "["):
            self.take()
            idx = self.expr()
            self.take("op", "]")
            return ("index", name, idx)
        return ("name", name)


# ---------------------------------------------------------------- code generation
#
# Values: ("const", python number) for compile-time integers, or
# (type, "%ssa") with type i16, f32x4. Scalar f32 values are kept as splat
# f32x4 vectors, which is what the VU computes with anyway.

class Gen:
    def __init__(self, consts):
        self.consts = consts
        self.lines, self.n, self.label = [], 0, "entry"
        self.uses = set()

    def tmp(self):
        self.n += 1
        return f"%t{self.n}"

    def emit(self, s):
        self.lines.append("  " + s)

    def block(self, label):
        self.lines.append(f"{label}:")
        self.label = label

    def const_eval(self, e, env=None):
        """Evaluate a compile-time integer expression, or return None."""
        kind = e[0]
        if kind == "num":
            return float(e[1]) if "." in e[1] else int(e[1])
        if kind == "name":
            if env is not None and e[1] in env and env[e[1]][0] == "const":
                return env[e[1]][1]
            return self.consts.get(e[1])
        if kind == "neg":
            v = self.const_eval(e[1], env)
            return None if v is None else -v
        if kind == "bin":
            a, b = self.const_eval(e[2], env), self.const_eval(e[3], env)
            if a is None or b is None:
                return None
            return {"+": a + b, "-": a - b, "*": a * b,
                    "/": (a // b if isinstance(a, int) and isinstance(b, int) else a / b)}[e[1]]
        return None

    # -------- values

    def splat(self, x):
        h = struct_hex(float(x))
        return f"<float {h}, float {h}, float {h}, float {h}>"

    def as_vec(self, v):
        if v[0] == "const":
            return self.splat(v[1])
        if v[0] != "f32x4":
            raise VukError("an i16 value cannot be used as a vector")
        return v[1]

    def as_i16(self, v):
        if v[0] == "const":
            if not isinstance(v[1], int):
                raise VukError(f"{v[1]} is not an integer")
            return str(v[1])
        if v[0] != "i16":
            raise VukError("a vector cannot be used as an index or count")
        return v[1]

    def value(self, e, env):
        c = self.const_eval(e, env)
        if c is not None:
            return ("const", c)
        kind = e[0]
        if kind == "name":
            if e[1] not in env:
                raise VukError(f"unknown name {e[1]}")
            v = env[e[1]]
            if v[0] == "array":
                raise VukError(f"{e[1]} is an array; index it")
            return v
        if kind == "neg":
            v = self.value(e[1], env)
            if v[0] == "i16":
                t = self.tmp()
                self.emit(f"{t} = sub i16 0, {v[1]}")
                return ("i16", t)
            t = self.tmp()
            self.emit(f"{t} = fneg <4 x float> {self.as_vec(v)}")
            return ("f32x4", t)
        if kind == "index":
            return self.load(e[1], e[2], env)
        if kind == "bin":
            a, b = self.value(e[2], env), self.value(e[3], env)
            if "i16" in (a[0], b[0]):
                op = {"+": "add", "-": "sub", "*": "mul"}.get(e[1])
                if op is None:
                    raise VukError("integer division is not supported")
                t = self.tmp()
                self.emit(f"{t} = {op} i16 {self.as_i16(a)}, {self.as_i16(b)}")
                return ("i16", t)
            op = {"+": "fadd", "-": "fsub", "*": "fmul", "/": "fdiv"}[e[1]]
            t = self.tmp()
            self.emit(f"{t} = {op} <4 x float> {self.as_vec(a)}, {self.as_vec(b)}")
            return ("f32x4", t)
        raise VukError(f"cannot compute {e}")

    def address(self, name, idx, env):
        arr = env.get(name)
        if arr is None or arr[0] != "array":
            raise VukError(f"{name} is not an array")
        _, elem, base = arr
        if elem != "f32x4":
            raise VukError(f"{name} is a const array; only f32x4 arrays are in memory")
        i = self.value(idx, env)
        if i[0] == "const":
            return str(base + i[1])
        t = self.tmp()
        self.emit(f"{t} = add i16 {self.as_i16(i)}, {base}")
        return t

    def load(self, name, idx, env):
        arr = env.get(name)
        if arr is not None and arr[0] == "table":
            # const f32[] parameter: loaded at kernel entry, index must be constant
            k = self.const_eval(idx, env)
            if not isinstance(k, int) or not 0 <= k < len(arr[1]):
                raise VukError(f"{name}[...] needs a constant index in range")
            return ("f32x4", arr[1][k])
        a = self.address(name, idx, env)
        t = self.tmp()
        self.emit(f"{t} = call <4 x float> @llvm.vu.lq(i16 {a})")
        self.uses.add("lq")
        return ("f32x4", t)

    # -------- statements

    def stmts(self, body, env):
        for s in body:
            self.stmt(s, env)

    def stmt(self, s, env):
        try:
            kind = s[0]
            if kind == "var":
                env[s[1]] = self.value(s[2], env)
            elif kind == "set":
                target, e = s[1], s[2]
                v = self.value(e, env)
                if target[0] == "name":
                    if target[1] not in env or env[target[1]][0] in ("array", "table"):
                        raise VukError(f"cannot assign to {target[1]}")
                    env[target[1]] = v
                elif target[0] == "index":
                    a = self.address(target[1], target[2], env)
                    self.emit(f"call void @llvm.vu.sq(<4 x float> {self.as_vec(v)}, i16 {a})")
                    self.uses.add("sq")
                else:
                    raise VukError("cannot assign to an expression")
            elif kind == "for":
                self.loop(s, env)
        except VukError as err:
            if not str(err).startswith("line"):
                raise VukError(f"line {s[-1]}: {err}") from None
            raise

    def loop(self, s, env):
        _, var, lo_e, hi_e, body, _ = s
        lo, hi = self.const_eval(lo_e, env), self.const_eval(hi_e, env)
        if lo is not None and hi is not None:
            # compile-time bounds: unroll
            for k in range(lo, hi):
                env[var] = ("const", k)
                self.stmts(body, env)
            env.pop(var, None)
            return

        # run-time loop: i in lo..hi, with phis for the variables it changes
        lo_v = self.as_i16(self.value(lo_e, env))
        hi_v = self.as_i16(self.value(hi_e, env))
        n = self.n = self.n + 1
        head, done = f"loop{n}", f"done{n}"
        carried = [k for k, v in env.items() if v[0] in ("i16", "f32x4")]

        empty = self.tmp()
        self.emit(f"{empty} = icmp sge i16 {lo_v}, {hi_v}")
        pre = self.label
        self.emit(f"br i1 {empty}, label %{done}, label %{head}")
        self.block(head)
        i = f"%i{n}"
        phi_at = len(self.lines)
        before = {k: env[k] for k in carried}
        phis = {k: f"%p{n}_{k}" for k in carried}
        for k in carried:
            env[k] = (before[k][0], phis[k])
        env[var] = ("i16", i)
        self.stmts(body, env)
        nxt = self.tmp()
        self.emit(f"{nxt} = add i16 {i}, 1")
        cond = self.tmp()
        self.emit(f"{cond} = icmp eq i16 {nxt}, {hi_v}")
        latch = self.label
        self.emit(f"br i1 {cond}, label %{done}, label %{head}")

        header = [f"  {i} = phi i16 [ {lo_v}, %{pre} ], [ {nxt}, %{latch} ]"]
        after = {}
        for k in carried:
            if env[k][1] == phis[k]:
                # not changed in the loop: use the value from before it
                pat = re.compile(re.escape(phis[k]) + r"\b")
                self.lines[phi_at:] = [pat.sub(before[k][1], l) for l in self.lines[phi_at:]]
                env[k] = before[k]
                continue
            ty = "i16" if before[k][0] == "i16" else "<4 x float>"
            header.append(f"  {phis[k]} = phi {ty} [ {before[k][1]}, %{pre} ], "
                          f"[ {env[k][1]}, %{latch} ]")
            after[k] = (before[k][0], env[k][1], phis[k], ty)
        self.lines[phi_at:phi_at] = header

        self.block(done)
        del env[var]
        for k, (ty0, last, phi, ty) in after.items():
            t = f"%x{n}_{k}"
            self.emit(f"{t} = phi {ty} [ {before[k][1]}, %{pre} ], [ {last}, %{latch} ]")
            env[k] = (ty0, t)


def struct_hex(x):
    """An f32 constant in LLVM's hex form (the double with the f32's value)."""
    f = struct.unpack("<f", struct.pack("<f", x))[0]
    return "0x%016X" % struct.unpack("<Q", struct.pack("<d", f))[0]


def compile_kernel(gen, kernel, header):
    name, params, body = kernel
    env, addr = {}, 0
    gen.lines, gen.label = [], "entry"
    gen.lines.append(f"define void @{name}() {{")
    gen.block("entry")
    header.append(f"/* kernel {name}: VU data memory layout, in quadwords */")
    for i, (pname, (ty, size)) in enumerate(params):
        tag = f"VUK_{name.upper()}_{pname.upper()}"
        if size is None and ty == "i16":
            t = gen.tmp()
            gen.emit(f"{t} = call i16 @llvm.vu.ilw(i16 {addr}, i32 0)")
            gen.uses.add("ilw")
            env[pname] = ("i16", t)
            header.append(f"#define {tag} {addr}  /* i16, field x */")
            addr += 1
        elif size is None and ty == "f32":
            t = gen.tmp()
            gen.emit(f"{t} = call <4 x float> @llvm.vu.lq(i16 {addr})")
            s = gen.tmp()
            gen.emit(f"{s} = shufflevector <4 x float> {t}, <4 x float> poison, <4 x i32> zeroinitializer")
            gen.uses.add("lq")
            env[pname] = ("f32x4", s)
            header.append(f"#define {tag} {addr}  /* f32, field x */")
            addr += 1
        elif ty == "f32" and size != "unsized":
            count = gen.const_eval(size)
            if not isinstance(count, int) or count <= 0:
                raise VukError(f"{pname}: size must be a positive constant")
            vals = []
            for q in range((count + 3) // 4):
                t = gen.tmp()
                gen.emit(f"{t} = call <4 x float> @llvm.vu.lq(i16 {addr + q})")
                gen.uses.add("lq")
                for f in range(4):
                    if len(vals) == count:
                        break
                    s = gen.tmp()
                    gen.emit(f"{s} = shufflevector <4 x float> {t}, <4 x float> poison, "
                             f"<4 x i32> <i32 {f}, i32 {f}, i32 {f}, i32 {f}>")
                    vals.append(s)
            env[pname] = ("table", vals)
            header.append(f"#define {tag} {addr}  /* f32[{count}], 4 per quadword */")
            addr += (count + 3) // 4
        elif ty == "f32x4" and size is not None:
            if size == "unsized":
                if i != len(params) - 1:
                    raise VukError(f"{pname}: an unsized array must be the last parameter")
                count = None
            else:
                count = gen.const_eval(size)
            env[pname] = ("array", "f32x4", addr)
            header.append(f"#define {tag} {addr}  /* f32x4[{count if count else ''}] */")
            addr += count or 0
        else:
            raise VukError(f"{pname}: unsupported parameter type")
    header.append(f"#define VUK_{name.upper()}_SIZE {addr}  /* quadwords before any unsized array */")
    # how a host runtime launches it (maxsdk MX_VUK_KERNEL): the code starts
    # at micro address 0, each item is one f32x4 quadword sent as UNPACK
    # V4-32 and transformed in place, at fixed addresses (no double buffer)
    header.append(f"#define VUK_{name.upper()}_ENTRY 0  /* micro address, 64-bit units */")
    header.append(f"#define VUK_{name.upper()}_ITEM_QW 1  /* quadwords per item */")
    header.append(f"#define VUK_{name.upper()}_FORMAT 0x0C  /* UNPACK V4-32 */")
    header.append(f"#define VUK_{name.upper()}_DBUF 0  /* no double buffering */")
    header.append("")
    gen.stmts(body, env)
    gen.emit("ret void")
    gen.lines.append("}")
    return gen.lines


DECLS = {
    "lq": "declare <4 x float> @llvm.vu.lq(i16)",
    "sq": "declare void @llvm.vu.sq(<4 x float>, i16)",
    "ilw": "declare i16 @llvm.vu.ilw(i16, i32)",
}


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("input")
    ap.add_argument("-o", "--output", default="-")
    ap.add_argument("-H", "--header", help="write the data memory layout as a C header")
    ap.add_argument("-D", action="append", default=[], metavar="NAME=VALUE")
    args = ap.parse_args()

    try:
        src = open(args.input).read()
        consts_ast, kernels = Parser(lex(src)).program()
        defines = {}
        for d in args.D:
            k, _, v = d.partition("=")
            defines[k] = int(v, 0)
        gen = Gen(dict(defines))
        for cname, e in consts_ast:
            if cname in defines:
                continue  # -D wins
            v = gen.const_eval(e)
            if v is None:
                raise VukError(f"const {cname} is not a compile-time constant")
            gen.consts[cname] = v
        header, funcs = [], []
        for k in kernels:
            funcs.append("\n".join(compile_kernel(gen, k, header)))
    except VukError as err:
        sys.exit(f"{args.input}: error: {err}")

    out = [f"; generated by vuk from {args.input}"]
    out += [DECLS[u] for u in sorted(gen.uses)] + [""] + funcs + [""]
    text = "\n".join(out)
    if args.output == "-":
        sys.stdout.write(text)
    else:
        open(args.output, "w").write(text)
    if args.header:
        guard = re.sub(r"\W", "_", args.header.split("/")[-1]).upper()
        consts = [f"#define VUK_{k} {v}" for k, v in gen.consts.items()]
        open(args.header, "w").write(
            f"/* generated by vuk from {args.input} */\n#ifndef {guard}\n#define {guard}\n\n"
            + "\n".join(consts) + "\n\n" + "\n".join(header) + f"\n#endif\n")


if __name__ == "__main__":
    main()
