# Hand-written kernels: to do

The kernels in `asm/*.S` start naive (a direct translation, see the comment of
each kernel). This lists what is left to do: optimizations to try, and gaps in
the LLVM assembler found while writing them.

## Optimizations to try

- **Sums in the accumulator** (`test_dual_pipeline_int`, `test_dual_pipeline_madd`,
  and other sums of products): keep a sum in HI:LO with the two-operand
  `madd rs, rt` (and one in HI1:LO1 with `madd1`) instead of `mult` and `addu`
  per element. It saves the register of each product and one instruction per
  element, and the two accumulators run in two pipes. Read the sum once at
  the end with `mflo`/`mflo1`.
- **Pair loads, stores and PREF with MMI operations** (MMI kernels): an MMI
  operation takes both integer pipes and only pairs with a load, store or
  PREF in its aligned 8-byte pair. Measured on the N64 To CT16 loop: -10 %.
- **No write prefetch where it competes with the read miss** (N64 To CT16:
  +15..30 % with it), but it helps Color Key: decide per loop, by measuring.
- **Space the PREF from the first store into a new destination line** (one
  miss is served at a time; N64 To CT16, unroll 4: after MMI op 30 of 48).
- **Prefetch lead that does not map onto the cache sets of the other streams**
  (Color Key: a lead of 1 KB onto buffers 1 KB apart modulo 4 KB is 7..11 %
  slower).
- **Loads one block ahead** (software pipelining), so a load does not share
  an aligned pair with the instruction that uses its value.
- **Unroll and pair for dual issue**: done in every kernel (none is naive now); more
  variants are cheap to try with `asm/harness/gen_app.py`.

- **Resample** (27.6 Msmp/s, feature clang 27.3) and the FPU **Transform** (4.90, feature
  4.97, which drops the zero entries of the matrix it sees initialized): only even with the
  compiler; MMI for the positions, or a shorter dispatch of the work per output.
- **Code size**: the kernels were chosen for speed; smaller variants of equal speed (e.g.
  N64 To CT16 with 2 blocks per iteration, 0.4 % slower) are not tried systematically.

## LLVM assembler gaps

- An immediate out of range is truncated without an error: `addiu $10, $9, 36864`
  assembles as `addiu $10, $9, -28672` (GNU as expands or rejects it). It made a
  hand-written loop run away and crash the PS2.

- Two-operand `madd rs, rt`, `maddu rs, rt`, `madd1 rs, rt`, `maddu1 rs, rt`
  for the R5900 (binutils accepts them; rd is $0). The kernels write
  `madd $0, rs, rt` meanwhile.
- `mtps`, `mtpc`, `mfpc`, `mfps` (performance counters), and `mtc0`/`mfc0`
  with a select other than 0 on the R5900 (PCR0 is $25 select 1, PCR1 select
  3).
