# PS2 Compiler Performance Comparison: GCC vs LLVM

## Test Environment
- **Platform**: PlayStation 2 (real hardware)
- **CPU**: Emotion Engine R5900 @ 294.912 MHz
- **GCC**: mips64r5900el-ps2-elf-gcc 15.2.0
  - branch: ps2dev/gcc/ee-v15.2 (with instr scheduling)
  - flags:  `-O3 -ffast-math -G0`
- **LLVM/Clang**: clang 24.0.0git
  - upstream: llvm/llvm-project main @ 922869f0e554 (2026-10-20)
  - feature:  rickgaiser/llvm/EmotionEngine-rebase @ 421935ba9f99 (full R5900 support, on top of upstream)
  - flags:    `-O3 -ffast-math --target=mips64el -mcpu=r5900 -mabi=n32 -fno-pic -mno-abicalls -G 0 -ffp-contract=fast -fomit-frame-pointer`

## Performance Results (2026-10-20)

Higher is better. Delta is relative to GCC.

| Test                     |    gcc | upstream | feature | upstream vs gcc | feature vs gcc |
|--------------------------|-------:|---------:|--------:|----------------:|---------------:|
| INT/Vec List FP (MOPS)   |  88.86 |    51.46 |  173.37 |            -42% |           +95% |
| INT/Int Scalar (MOPS)    |  55.48 |    35.23 |   87.26 |            -36% |           +57% |
| INT/Dual Pipeline (MOPS) | 148.28 |   108.18 |  182.16 |            -27% |           +23% |
| INT/MADD Pipeline (MOPS) | 144.24 |   103.47 |  238.52 |            -28% |           +65% |
| FPU/Mat x Vec (MFLOPS)   | 162.69 |    81.35 |  199.98 |            -50% |           +23% |
| FPU/Mat x Mat (MFLOPS)   | 206.45 |   119.99 |  266.63 |            -42% |           +29% |
| FPU/Vec List (MFLOPS)    | 242.83 |   195.71 |  318.91 |            -19% |           +31% |
| VPU/Vec Mul (MFLOPS)     |   9.52 |    23.65 |   23.58 |           +148% |          +148% |
| VPU/Vec Add (MFLOPS)     |   9.52 |    23.64 |   23.57 |           +148% |          +148% |
| VPU/Vec MADD (MFLOPS)    |  17.45 |    39.79 |   40.83 |           +128% |          +134% |
| VPU/Mat x Vec (MFLOPS)   | 159.73 |   144.30 |  320.17 |            -10% |          +100% |
| VPU/Dot Prod (MFLOPS)    |  69.03 |    53.01 |   77.57 |            -23% |           +12% |

Results of both clang variants match gcc.

## Usage

```sh
./bench.sh build [variant...]   # gcc upstream pr feature feature-vu0 -> toolchain/build/bench/<variant>/
./bench.sh codegen              # code size per function per variant
./bench.sh run <variant>        # run on the PS2 (192.168.1.10), saves run.log
./bench.sh report               # results table from the saved run logs
```

feature-vu0 is the feature clang with `-mvu0`: v4f32 is then a VU0 (COP2) vector type,
and `__mips_vu0` enables the VU0 inline asm tests.

Generated assembly is in `toolchain/build/bench/<variant>/*.s`.
