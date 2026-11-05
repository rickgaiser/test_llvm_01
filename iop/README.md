# IOP bench

What the PS2's IOP spends its time on, as C and as hand-written assembly, compared across
gcc, clang and hand code on a real R3000A. See `libs/maxsdk/docs/iop-core.md` for the CPU itself.

Each kernel exists in six variants, all in one module and run on the same data:

| Variant | Built with |
|---|---|
| gcc-Os | IOP gcc 15.2 with ps2sdk's flags: what ps2sdk ships |
| gcc-O2 | The same at `-O2` |
| clang-Os | Feature clang, `-mcpu=r3000a`, flags of mxlink's `IOP_CLANG` |
| clang-O2 | The same at `-O2` |
| clang-O2r | clang-O2 with `restrict` on the pointers: the only difference in the source |
| asm | `kernels_asm.S` |

Times are in cycles of the 36.864 MHz clock: the best of 5 calls with interrupts off. Buffers are
larger than the D-cache, so streaming kernels pay their misses. Every output is hashed and compared
with the first variant's, so a fast wrong result fails the run.

## Running

Only on a real R3000A: SCPH-70xxx or older, see `iop-core.md`.

```
source envsetup.sh
make -C apps/llvm_bench/iop           # iop_bench.irx
make -C apps/llvm_bench/iop sim       # first: all variants in a MIPS I interpreter
make -C libs/maxsdk                    # probe/p25_iop.elf, which loads the module
ps2client execee host:p25_iop.elf host:iop_bench.irx > bench.log
python3 apps/llvm_bench/iop/report.py bench.log
```

`make sim` runs every variant with the real load delay, reports HI/LO hazards and compares results,
so a bad loop fails on the host and does not hang the IOP. It also gives instruction counts. A run
on the PS2 takes about 10 seconds.

## Kernels

| Kernel | What | From ps2sdk |
|---|---|---|
| cpy_byte, cpy_word, set_word, str_len | Memory, strings | sysclib |
| cpy_unal | Copy with the source aligned to 2 | A payload behind an Ethernet header |
| net_sum | Internet checksum | lwIP `lwip_standard_chksum` |
| net_swap | Byte swap of halfwords | lwIP `PP_HTONS` |
| fifo_rd | Words from a register | smap `CopyFromFIFO`; the bench reads uncached RAM, not SMAP |
| mc_edc, mc_ecc | Memory card checks | mcman `mcman_calcEDC`, `McDataChecksum` |
| bit_rev | Bit reversal of each byte | mx4sio_bd `reverse_buffer` |
| crc32 | Table CRC | bdm GPT |
| fat_scan | First free FAT entry | vfat |
| up_24000, up_gen | Upsampling | audsrv upsamplers |
| mix_vol, gain_q16 | Volume with saturation, fixed-point gain | Software mixers |
| ring_put | Into a ring buffer | audsrv |
| list_walk | Down a linked list | threadman, sysmem |
| itoa10 | Decimal digits | printf's `%u` |
| mul_const, mul64 | `* 1000`, 64-bit product | Timer conversions |

## Results

On an SCPH-70004, 2026-11-05, in cycles; 4 KiB of data unless noted in `main.c`.

| Kernel | gcc-Os | gcc-O2 | clang-Os | clang-O2 | clang-O2r | asm | clang-O2 before |
|---|---:|---:|---:|---:|---:|---:|---:|
| cpy_byte | 30778 | 22594 | 22594 | 16428 | 14380 | 5701 | 26681 |
| cpy_word | 9265 | 7204 | 7208 | 6728 | 6210 | 5700 | 8242 |
| cpy_unal | 12343 | 9021 | 9024 | 7502 | 7227 | 6986 | 9593 |
| set_word | 4133 | 3108 | 3110 | 2118 | 2118 | 2105 | 4134 |
| str_len | 26137 | 17948 | 17947 | 17947 | 17947 | 8766 | 22071 |
| net_sum | 17964 | 9776 | 9783 | 7741 | 7741 | 5945 | 11829 |
| net_swap | 24121 | 20854 | 18743 | 15169 | 14156 | 11060 | 20269 |
| mc_edc | 26156 | 17957 | 17957 | 13377 | 13374 | 4144 | 17957 |
| mc_ecc | 62848 | 60690 | 68887 | 57762 | 57756 | 43432 | 75085 |
| bit_rev | 23368 | 21837 | 21835 | 21321 | 21321 | 20870 | 23107 |
| up_24000 | 18383 | 18377 | 18383 | 18387 | 18391 | 15168 | 18384 |
| up_gen | 6108 | 6110 | 6109 | 6108 | 5026 | 5026 | 7539 |
| mix_vol | 55594 | 49955 | 55597 | 49507 | 47967 | 32895 | 57520 |
| gain_q16 | 19156 | 17367 | 17367 | 16608 | 15086 | 13296 | - |
| fifo_rd | 14372 | 13430 | 13426 | 13426 | 13426 | 8551 | 13426 |
| ring_put | 7591 | 11670 | 7597 | 6266 | 6266 | 1455 | 13863 |
| list_walk | 3621 | 3374 | 3370 | 3376 | 3369 | 3132 | 3369 |
| fat_scan | 9753 | 5663 | 5664 | 5666 | 5663 | 4389 | 5665 |
| crc32 | 53300 | 45101 | 45105 | 39611 | 39596 | 33730 | 49277 |
| itoa10 | 57249 | 55742 | 27801 | 27797 | 27797 | 26417 | 29047 |
| mul_const | 23339 | 12601 | 12604 | 10798 | 10314 | 10306 | 13112 |
| mul64 | 4457 | 4453 | 4328 | 4291 | 3792 | 3443 | 6537 |

"Before" is clang before the changes below. Geometric means over the kernels with a before value:

| Ratio | Before | After |
|---|---:|---:|
| clang-O2 / gcc-O2 | 1.08 | **0.85** |
| clang-Os / gcc-Os | 0.89 | **0.79** |

asm / clang-O2 is 0.70 over all kernels, asm / clang-O2r 0.73, and clang-O2r / clang-O2 0.96. Code size of all kernels: gcc-Os 1520 bytes, gcc-O2 1568,
clang-Os 1576, clang-O2 4536: `-O2` buys its speed with 3 times the code. mxlink's `ps2link` at `-Os`
kept the size of every function that had not changed.

## What went into LLVM

All for `-mcpu=r3000a` only. R5900 code of maxsdk is unchanged (44 of 44 objects).

| Change | Why it is faster | Effect |
|---|---|---|
| `MipsSinkLoopInc`: the increment of a loop pointer goes behind the last load through it, stores behind it use the new value with the offset changed | Selection put the increment first, so old and new value overlapped and each loop ended with a `move`. Now the increment fills the load's delay slot and the store can fill the branch's | `cpy_word` 6 to 5 instructions per word; most loops |
| `getPreferredAddressingMode`: post-indexed | Strength reduction leaves one pointer and no index | `str_len` 5 to 4 per byte |
| Selects branch on the comparison: `beq`, `bne`, `bltz`, `bgez` | There is no conditional move, so a select is a branch anyway; `xor`, `sltiu` and `slt` before it were wasted | `ring_put` |
| Selects with one constant branch over it | The other value stays in its register, no `j` and no `move` | `ring_put` at `-Os` 12838 to 7595, with the above |
| `MipsAccOrderDAGMutation`: a multiply waits until HI/LO has been read | There is one pair. The scheduler started the multiplies early for their latency, and results went through the stack | `mul64` 6537 to 4287 |
| Multiply operands: the one known to be smaller in rs | Measured: only rs sets the time, 8, 11 or 15 cycles. `mult` compares sign bits, `multu` leading zeros | Not measured alone; `itoa10` shows what the wrong order costs: 27797 against 32120 |
| Partial and runtime unrolling at `-O2`, at most 4 times, single-block loops, never at `-Os` | Increments and the branch are a large part of a short loop on a single-issue CPU | `cpy_byte` 22595 to 16428, `set_word` 3110 to 2116 |

The scheduling model has the measured values too (`mult` 14, `div` 37, stores 2 cycles on a resource
of their own, the delay of `mfc0`). That is for accuracy: no kernel changed, within 0.02%.

Tests: `r3000a-loop-inc.ll`, `r3000a-select-branch.ll`, `r3000a-mult-rs.ll`, `r3000a-acc-order.ll`,
`r3000a-unroll.ll`. A first version of the rs change compared sign bits for `multu` too, which took the
constant of `itoa10` for small and made it slower: the bench found that.

## What only the source can do

The hand-written kernels are about 30% faster than clang-O2 overall. Where the gap is large, the reason
is something a compiler may not or cannot do. These are the changes to make in ps2sdk's C:

| Kernel | Hand against clang-O2 | Method | Why a compiler cannot |
|---|---:|---|---|
| ring_put | 0.23 | Two straight word copies, no position test per byte | It would have to prove the split and the alignment |
| mc_edc | 0.31 | XOR words, fold at the end | Byte reduction through words, alignment unknown |
| cpy_byte | 0.35 | Test alignment once, copy words | `-fno-builtin` and unknown alignment |
| str_len | 0.49 | `(w - 0x01010101) & ~w & 0x80808080`, a word at a time | Reading ahead of the string |
| fifo_rd | 0.64 | Eight register reads, then eight stores | An uncached load waits for the write buffer, so `lw`, `sw` pairs pay that each time. The store may alias the register |
| mix_vol | 0.66 | The next multiply runs while this sample is finished | Software pipelining across iterations; also pointers may alias |
| net_swap | 0.73 | Two halfwords per word | Alignment unknown |
| mc_ecc | 0.75 | Bit 7 of the running XOR is the parity of the count, so `a3` follows from `t0` | An algebraic fact of this code |
| net_sum | 0.77 | 32 bits at a time, carries counted with `sltu` | Alignment; a different algorithm |
| gain_q16 | 0.80 | Multiply overlapped with the store and the next load | As mix_vol |
| mul64 | 0.80 | The next loads under the last multiply | As mix_vol |
| up_24000 | 0.82 | One load per pair, one word store per channel | Stores may alias the source, so all loads stay |
| crc32 | 0.85 | One load per 4 bytes, XOR all at once | Alignment |

Other findings:

- **Stores set the pace of copies:** one per 2 cycles at most. `set_word` reaches 2.05 cycles per word.
  In `cpy_word`, loads first or interleaved with the stores make no difference (5698, 5709).
- **What holds the scheduler back is aliasing, not the scheduler:** a store may write what the next load
  reads, so `lw; sw; lw; nop; sw` has to stay. `restrict` takes the `nop`s behind loads in clang-O2 from
  52 to 13. Most of the 13 are true dependencies (`mc_ecc`'s table, `list_walk`). Breaking
  anti-dependencies after register allocation changed one `nop`, so the reuse of `$1` is not the cause.
- **`restrict` pays where instructions set the pace:** `up_gen` 18% faster, `mul64` 12%, `gain_q16` 9%,
  `cpy_word` 8%. Where stores or misses do, nothing changes although the `nop`s go: `up_24000` loses
  43% of its instructions and no cycles. Worth adding to ps2sdk's copying and audio code.
- **Clang found a better `up_gen` than the first hand version:** packing two samples into a word store
  cost more in shifts than it saved, as the source stays in the D-cache. Halfword stores with two
  chains in turn took the hand version from 5972 to 5026, equal to clang-O2r.

## Limits of these numbers

- One console, warm I-cache, interrupts off, no DMA at the same time. Real code competes for the bus.
- `fifo_rd` reads uncached RAM: a real SMAP register may be slower.
- The hand kernels need sizes that are a multiple of 16 or 32 and aligned data, and have no tail
  code. Library versions need both, which costs little on 4 KiB.
- ps2sdk builds with `-Os`, so unrolling does not reach it by default.
