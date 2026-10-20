# PS2 R5900 FPU vs IEEE 754 — Special-Value Arithmetic

Measured on PS2 hardware (EE core, R5900 FPU) compiled with
`clang --target=mips64el -mcpu=r5900 -ffinite-math-only -O0 -G 0`.

**Summary: 11/30 MATCH, 19 DIFF**

The R5900 FPU has no special-case hardware for NaN/INF.  All bit patterns
are treated as ordinary numbers by the ALU.  The only "clamp" behaviour is
the exponent-overflow saturation described at the bottom.

---

## fadd (add.s)  — 4/6 match

| Operation    | PS2 bits       | PS2 value      | IEEE 754 | Result |
|:-------------|:---------------|:---------------|:---------|:-------|
| +INF + 1e6   | `0x7F800000`   | +INF           | +INF     | MATCH  |
| -INF + 1e6   | `0xFF800000`   | -INF           | -INF     | MATCH  |
| +INF + +INF  | `0x7FFFFFFF`   | *(exp-ovfl)*   | +INF     | **DIFF** |
| +INF + -INF  | `0x00000000`   | 0.0            | NaN      | **DIFF** |
| NaN + 1e6    | `0x7FC00000`   | *(NaN bits)*   | NaN      | MATCH  |
| NaN + NaN    | `0x7FFFFFFF`   | *(exp-ovfl)*   | NaN      | MATCH† |

`+INF + +INF`: mantissas both zero, exponents 255+255-127 = 383 → overflow → `0x7FFFFFFF`.
`+INF + -INF`: add.s sees sign-cancel, mantissa result = 0, sign = 0 → plain 0.0 (should be NaN).
`NaN + NaN`: NaN has exp=0xFF; same exponent path → overflow → `0x7FFFFFFF` (happens to be a NaN bit pattern).

---

## fsub (sub.s)  — 3/5 match

| Operation    | PS2 bits       | PS2 value      | IEEE 754 | Result |
|:-------------|:---------------|:---------------|:---------|:-------|
| +INF - 1e6   | `0x7F800000`   | +INF           | +INF     | MATCH  |
| -INF - 1e6   | `0xFF800000`   | -INF           | -INF     | MATCH  |
| +INF - +INF  | `0x00000000`   | 0.0            | NaN      | **DIFF** |
| +INF - -INF  | `0x7FFFFFFF`   | *(exp-ovfl)*   | +INF     | **DIFF** |
| NaN - 1e6    | `0x7FC00000`   | *(NaN bits)*   | NaN      | MATCH  |

`+INF - +INF`: same as adding equal-magnitude opposite-sign values → 0 (should be NaN).
`+INF - -INF`: treated as +INF + +INF → exponent overflow → `0x7FFFFFFF` (not +INF).

---

## fmul (mul.s)  — 1/7 match

| Operation    | PS2 bits       | PS2 value      | IEEE 754 | Result |
|:-------------|:---------------|:---------------|:---------|:-------|
| +INF * 1e6   | `0x7FFFFFFF`   | *(exp-ovfl)*   | +INF     | **DIFF** |
| -INF * 1e6   | `0xFFFFFFFF`   | *(exp-ovfl)*   | -INF     | **DIFF** |
| +INF * -1e6  | `0xFFFFFFFF`   | *(exp-ovfl)*   | -INF     | **DIFF** |
| +INF * 0     | `0x00000000`   | 0.0            | NaN      | **DIFF** |
| -INF * 0     | `0x80000000`   | -0.0           | NaN      | **DIFF** |
| +INF * +INF  | `0x7FFFFFFF`   | *(exp-ovfl)*   | +INF     | **DIFF** |
| NaN * 1e6    | `0x7FFFFFFF`   | *(exp-ovfl)*   | NaN      | MATCH† |

`+INF * 1e6`: exp = 255 + 146 - 127 = **274** — overflows 8-bit field → `0x7FFFFFFF`.
`+INF * +INF`: exp = 255 + 255 - 127 = **383** → `0x7FFFFFFF`.
`+INF * 0`:  mul.s computes 0×mantissa = 0, exponent collapses → plain 0 (should be NaN).
`-INF * 0`:  same but sign bit preserved → -0.0.

---

## fdiv (div.s)  — 1/7 match

| Operation    | PS2 bits       | PS2 value      | IEEE 754 | Result |
|:-------------|:---------------|:---------------|:---------|:-------|
| +INF / 1e6   | `0x758637BD`   | 3.40282e+32    | +INF     | **DIFF** |
| -INF / 1e6   | `0xF58637BD`   | -3.40282e+32   | -INF     | **DIFF** |
| +INF / +INF  | `0x3F800000`   | 1.0            | NaN      | **DIFF** |
| 1e6 / 0      | `0x7FFFFFFF`   | *(exp-ovfl)*   | +INF     | **DIFF** |
| 0 / 0        | `0x7FFFFFFF`   | *(exp-ovfl)*   | NaN      | MATCH† |
| +INF / 0     | `0x7FFFFFFF`   | *(exp-ovfl)*   | +INF     | **DIFF** |
| NaN / 1e6    | `0x75C9539B`   | 5.10424e+32    | NaN      | **DIFF** |

`+INF / 1e6`: INF (0x7F800000) is treated as a regular float = max_float (3.40282e+38).
max_float / 1e6 = 3.40282e+32 — a valid finite result, not +INF.
`+INF / +INF`: max_float / max_float = 1.0 (should be NaN).
`NaN / 1e6`: NaN mantissa is treated as a normal mantissa → finite result.
`1e6 / 0` and `+INF / 0`: div.s has no divide-by-zero trap; exponent overflows → `0x7FFFFFFF`.
`0 / 0`: same path → `0x7FFFFFFF` (happens to be a NaN bit pattern → counted as MATCH†).

---

## fsqrt (sqrt.s via sqrtf library)  — 3/5 match

| Operation    | PS2 bits       | PS2 value      | IEEE 754 | Result |
|:-------------|:---------------|:---------------|:---------|:-------|
| sqrt(+INF)   | `0x7FFFFFFF`   | *(exp-ovfl)*   | +INF     | **DIFF** |
| sqrt(NaN)    | `0x7FFFFFFF`   | *(exp-ovfl)*   | NaN      | MATCH† |
| sqrt(-1e6)   | `0x3F800000`   | 1.0            | NaN      | **DIFF** |
| sqrt(-INF)   | `0x3F800000`   | 1.0            | NaN      | **DIFF** |
| sqrt(0)      | `0x00000000`   | 0.0            | 0.0      | MATCH  |

`sqrt(+INF)`: exp = 255; sqrt exponent = (255+127)/2 overflows → `0x7FFFFFFF`.
`sqrt(-1e6)` and `sqrt(-INF)`: the sqrtf **library** (not the raw sqrt.s instruction) returns 1.0
for negative inputs.  The raw sqrt.s ignores the sign bit, but the library wrapper
apparently converges to 1.0 via its Newton-Raphson refinement when given a large
negative value (observed empirically; sign bit is not stripped before the computation).

---

## Overflow clamp modes (normal floats, no INF/NaN input) — 14/14 PASS

Two distinct saturation behaviours exist for normal-float overflow on R5900:

### Mode A — Exponent overflow (result exponent > 127 after biasing)

The 8-bit biased-exponent field wraps or overflows the adder.
Result is **saturated** to `0x7FFFFFFF` (positive) or `0xFFFFFFFF` (negative).

| Test            | Operation        | PS2 bits     |
|:----------------|:-----------------|:-------------|
| fadd +overflow  | max_f + max_f    | `0x7FFFFFFF` |
| fadd -overflow  | -max_f + -max_f  | `0xFFFFFFFF` |
| fsub +overflow  | max_f - -max_f   | `0x7FFFFFFF` |
| fsub -overflow  | -max_f - max_f   | `0xFFFFFFFF` |
| fmul *2         | max_f * 2        | `0x7FFFFFFF` |
| fmul *-2        | -max_f * 2       | `0xFFFFFFFF` |
| fmul big        | max_f * max_f    | `0x7FFFFFFF` |
| fmul mixed      | 1e6 * max_f      | `0x7FFFFFFF` |
| fdiv /0.5       | max_f / 0.5      | `0x7FFFFFFF` |
| fdiv /-0.5      | -max_f / 0.5     | `0xFFFFFFFF` |
| fdiv /0.25      | max_f / 0.25     | `0x7FFFFFFF` |

### Mode B — Mantissa saturation (exponent stays at 127)

Adding a value with a much lower exponent does not raise the result exponent.
The mantissa rounds up and saturates to all-ones → **true clamp** at ±max_float.

| Test            | Operation        | PS2 bits     | Float value   |
|:----------------|:-----------------|:-------------|:--------------|
| fadd +sat       | max_f + 1e6      | `0x7F7FFFFF` | +3.40282e+38  |
| fadd -sat       | -max_f + -1e6    | `0xFF7FFFFF` | -3.40282e+38  |
| fsub +sat       | max_f - -1e6     | `0x7F7FFFFF` | +3.40282e+38  |

IEEE 754 mandates +INF / -INF for all of the above overflow cases.

---

## Key architectural differences

| Behaviour              | IEEE 754          | PS2 R5900                                   |
|:-----------------------|:------------------|:--------------------------------------------|
| INF + finite           | INF               | INF (MATCH — INF swamps finite)             |
| INF + (-INF)           | NaN               | 0.0 (sign-cancel, no NaN detection)         |
| INF * nonzero          | INF               | exp-overflow → `0x7FFFFFFF`/`0xFFFFFFFF`    |
| INF * 0                | NaN               | ±0.0 (no NaN detection)                     |
| INF / finite           | INF               | finite (INF mantissa treated as max_float)  |
| INF / INF              | NaN               | 1.0 (max_float / max_float)                 |
| x / 0                  | INF or NaN        | `0x7FFFFFFF` (no divide-by-zero trap)       |
| sqrt(-x)               | NaN               | library returns 1.0                         |
| sqrt(+INF)             | +INF              | `0x7FFFFFFF` (exponent overflow in sqrt)    |
| NaN propagation        | NaN               | partial — only when exponent path overflows |
| Normal overflow        | ±INF              | `0x7FFFFFFF`/`0xFFFFFFFF` (Mode A)          |
| Mantissa round-up sat  | ±INF              | `0x7F7FFFFF`/`0xFF7FFFFF` (Mode B)          |

† MATCH cases where the IEEE result is NaN and PS2 also produces a NaN bit pattern
(`0x7FFFFFFF` = exp 0xFF, mantissa 0x7FFFFF), but **via a different mechanism** — the PS2
exponent-overflow saturator rather than NaN propagation hardware.
