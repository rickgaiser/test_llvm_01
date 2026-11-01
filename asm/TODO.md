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
- **Unroll and pair for dual issue** in every naive kernel.

## LLVM assembler gaps

- Two-operand `madd rs, rt`, `maddu rs, rt`, `madd1 rs, rt`, `maddu1 rs, rt`
  for the R5900 (binutils accepts them; rd is $0). The kernels write
  `madd $0, rs, rt` meanwhile.
- `mtps`, `mtpc`, `mfpc`, `mfps` (performance counters), and `mtc0`/`mfc0`
  with a select other than 0 on the R5900 (PCR0 is $25 select 1, PCR1 select
  3).
