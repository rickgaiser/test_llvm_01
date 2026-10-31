# PS2 Compiler Performance Comparison: GCC vs LLVM

## Test Environment
- **Platform**: PlayStation 2 (real hardware)
- **CPU**: Emotion Engine R5900 @ 294.912 MHz
- **GCC**: mips64r5900el-ps2-elf-gcc 15.2.0
  - branch: ps2dev/gcc/ee-v15.2 (with instr scheduling)
  - flags:  `-O3 -ffast-math -G0`
- **LLVM/Clang**: clang 24.0.0git
  - feature:  rickgaiser/llvm-project/r5900-feature @ 5c758efb9a74 (2026-10-31, full R5900 support,
    on top of llvm/llvm-project main @ 922869f0e554)
  - flags:    `-O3 -ffast-math --target=mips64el -mcpu=r5900 -mabi=n32 -fno-pic -mno-abicalls -G 0 -ffp-contract=fast -fomit-frame-pointer`

## Performance Results (2026-10-31)

Higher is better. Delta is relative to GCC.

| Test                                |    gcc | feature | feature-vu0 | feature vs gcc | feature-vu0 vs gcc |
|-------------------------------------|-------:|--------:|------------:|---------------:|-------------------:|
| INT/Vec List FP (MOPS)              |  89.32 |  174.22 |      173.54 |           +95% |               +94% |
| INT/Int Scalar (MOPS)               |  55.48 |   89.81 |       89.81 |           +62% |               +62% |
| INT/Dual Pipeline (MOPS)            | 148.28 |  267.59 |      266.97 |           +80% |               +80% |
| INT/MADD Pipeline (MOPS)            | 144.06 |  252.49 |      253.04 |           +75% |               +76% |
| INT/Int Divide (MOPS)               |   6.81 |   12.73 |       12.73 |           +87% |               +87% |
| FPU/Mat x Vec (MFLOPS)              | 159.98 |  199.98 |      199.91 |           +25% |               +25% |
| FPU/Mat x Mat (MFLOPS)              | 207.55 |  284.39 |      284.42 |           +37% |               +37% |
| FPU/Vec List (MFLOPS)               | 243.09 |  374.40 |      577.07 |           +54% |              +137% |
| VPU/Vec Mul (MFLOPS)                |  12.04 |   26.99 |       37.54 |          +124% |              +212% |
| VPU/Vec Add (MFLOPS)                |  12.04 |   26.98 |       37.54 |          +124% |              +212% |
| VPU/Vec MADD (MFLOPS)               |  22.02 |   45.95 |       71.61 |          +109% |              +225% |
| VPU/Mat x Vec (MFLOPS)              | 168.97 |  325.26 |     1246.58 |           +92% |              +638% |
| VPU/Dot Prod (MFLOPS)               |  67.34 |   97.55 |      126.12 |           +45% |               +87% |
| VPU/HMax/HMin (Mvec/s)              |   7.06 |    7.73 |       10.88 |            +9% |               +54% |
| STREAM/Sum (MB/s)                   | 316.06 |  351.25 |      351.26 |           +11% |               +11% |
| STREAM/Scale (MB/s)                 | 234.52 |  255.01 |      255.02 |            +9% |                +9% |
| STREAM/SAXPY (MB/s)                 | 318.29 |  305.80 |      305.80 |            -4% |                -4% |
| STREAM/Sum pref (MB/s)              | 387.94 |  435.19 |      435.20 |           +12% |               +12% |
| STREAM/Scale pref (MB/s)            | 321.64 |  340.56 |      341.77 |            +6% |                +6% |
| STREAM/SAXPY pref (MB/s)            | 363.08 |  369.76 |      369.68 |            +2% |                +2% |
| GFX/Alpha Blend (Mpix/s)            |   6.78 |    9.27 |        9.27 |           +37% |               +37% |
| GFX/CLUT8 (Mpix/s)                  |  16.47 |   20.86 |       20.86 |           +27% |               +27% |
| GFX/Color Key (Mpix/s)              |  12.70 |   17.84 |       17.84 |           +40% |               +40% |
| GFX/Alpha Test (Mpix/s)             |  12.59 |   16.46 |       16.46 |           +31% |               +31% |
| GFX/To CT16 (Mpix/s)                |  18.92 |   29.75 |       29.78 |           +57% |               +57% |
| GFX/Scale NN (Mpix/s)               |  17.32 |   20.54 |       20.54 |           +19% |               +19% |
| GEOM/Transform (Mvert/s)            |   2.98 |    4.36 |        4.57 |           +46% |               +53% |
| GEOM/Lighting (Mvert/s)             |   2.10 |    2.32 |        6.51 |           +10% |              +210% |
| GEOM/Lighting v4sf (Mvert/s)        |   1.95 |    2.24 |        2.47 |           +15% |               +27% |
| GEOM/Field Scale v4sf (Mvert/s)     |   4.17 |    4.09 |        4.79 |            -2% |               +15% |
| GEOM/Field Offset bc v4sf (Mvert/s) |   4.17 |    4.09 |        4.86 |            -2% |               +17% |
| GEOM/Project v4sf (Mvert/s)         |   5.07 |    5.21 |        5.80 |            +3% |               +14% |
| GEOM/Weighted Sum v4sf (Mvert/s)    |   5.65 |    8.31 |       12.42 |           +47% |              +120% |
| GEOM/MxV Row 4x4 (Mvert/s)          |   3.84 |    4.68 |        6.48 |           +22% |               +69% |
| GEOM/MxV Col 4x4 (Mvert/s)          |   3.84 |    4.68 |        6.49 |           +22% |               +69% |
| GEOM/MxV Row 3x4 (Mvert/s)          |   3.72 |    4.74 |        5.99 |           +27% |               +61% |
| GEOM/MxV Col 3x4 (Mvert/s)          |   3.72 |    4.74 |        5.99 |           +27% |               +61% |
| GEOM/Backface Cull (Mtri/s)         |   7.02 |    8.81 |        8.69 |           +25% |               +24% |
| GEOM/Face Normals (Mtri/s)          |   4.54 |    4.96 |        4.98 |            +9% |               +10% |
| GEOM/Face Normals v4sf (Mtri/s)     |   4.34 |    4.69 |        5.91 |            +8% |               +36% |
| GEOM/Frustum Cull (Mobj/s)          |   3.82 |    3.94 |        7.75 |            +3% |              +103% |
| SOUND/Mix 8 Voices (Msmp/s)         |  33.55 |   85.00 |       85.03 |          +153% |              +153% |
| SOUND/ADPCM Decode (Msmp/s)         |  12.33 |   21.74 |       21.74 |           +76% |               +76% |
| SOUND/FIR 32 (Msmp/s)               |   1.68 |   10.57 |       10.58 |          +529% |              +530% |
| SOUND/Resample (Msmp/s)             |  17.83 |   27.55 |       27.55 |           +55% |               +55% |
| SOUND/Correlate Q15 (MMAC/s)        |  58.68 |  264.26 |      264.35 |          +350% |              +350% |
| SOUND/Energy f32 (Msmp/s)           |  29.69 |   68.25 |       68.24 |          +130% |              +130% |
| EMU/6502 Interp (MIPS)              |  11.10 |   14.54 |       14.54 |           +31% |               +31% |
| EMU/R3000 Interp (MIPS)             |   8.54 |   10.58 |       10.40 |           +24% |               +22% |
| EMU/SNES Tiles (Mpix/s)             |  32.35 |   45.70 |       45.67 |           +41% |               +41% |
| EMU/N64 To CT16 (Mpix/s)            |  18.02 |   50.37 |       50.37 |          +180% |              +180% |
| EMU/Guest Loads (Mload/s)           |  13.48 |   14.72 |       14.72 |            +9% |                +9% |
| VPU/ASM Mat x Vec (MFLOPS)          |      - |       - |     1368.42 |              - |                  - |
| VPU/ASM Main Mem (MFLOPS)           |      - |       - |      350.38 |              - |                  - |
| VPU/Compute Only (MFLOPS)           |      - |       - |     2097.99 |              - |                  - |

feature-vu0 is the feature clang with `-mvu0`. The last three rows are VU0 inline asm tests, which
only run in that build.

The results match gcc, except for the Vec Divide hash (Test 9b: gcc 0x83964590, feature and
feature-vu0 0x6f81d5ed).
The loop is plain `div.s` with gcc and clang, and every clang build gives 0x6f81d5ed. The cause
is not known yet.

## Usage

```sh
./bench.sh build [variant...]   # gcc upstream pr feature feature-vu0 -> build/apps/llvm_bench/<variant>/
./bench.sh codegen              # code size per function per variant
./bench.sh run <variant>        # run on the PS2 (192.168.1.10), saves run.log
./bench.sh report               # results table from the saved run logs
```

feature-vu0 is the feature clang with `-mvu0`: v4f32 is then a VU0 (COP2) vector type,
and `__mips_vu0` enables the VU0 inline asm tests.

Generated assembly is in `build/apps/llvm_bench/<variant>/*.s` (in ps2max).
