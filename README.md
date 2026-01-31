# PS2 Compiler Performance Comparison: GCC vs LLVM

## Test Environment
- **Platform**: PlayStation 2 (real hardware)
- **CPU**: Emotion Engine R5900 @ 294.912 MHz
- **GCC**: mips64r5900el-ps2-elf-gcc 15.2.0
  - branch: ps2dev/gcc/ee-v15.2 (with instr scheduling)
  - flags:  `-O3 -ffast-math`
- **LLVM/Clang**: clang 23.0.0git
  - branch: rickgaiser/llvm/r5900-phase2-fpu-v2 (with FPU support)
  - flags:  `-O3 -ffast-math --target=mips64el -mcpu=r5900 -fno-pic -mno-abicalls`

## Performance Results

### Integer Tests

| Test | GCC | LLVM | Winner | Delta |
|------|-----|------|--------|-------|
| Vec List Fixed-Point Q16.16 | **89.71 MOPS** | 51.42 MOPS | GCC | +74% |
| Int Scalar Multiply | 36.79 MOPS | **43.99 MOPS** | LLVM | +20% |
| Dual Pipeline Int | **148.47 MOPS** | 108.18 MOPS | GCC | +37% |
| MADD Pipeline | **144.06 MOPS** | 103.37 MOPS | GCC | +39% |

### FPU Tests

| Test | GCC | LLVM | Winner | Delta |
|------|-----|------|--------|-------|
| Matrix x Vector | **162.69 MFLOPS** | 81.36 MFLOPS | GCC | +100% |
| Matrix x Matrix | **206.43 MFLOPS** | 119.99 MFLOPS | GCC | +72% |
| Vector List Transform | **243.40 MFLOPS** | 194.78 MFLOPS | GCC | +25% |

### VPU Tests (v4sf vector type)

| Test | GCC | LLVM | Winner | Delta |
|------|-----|------|--------|-------|
| Vec Multiply | 12.05 MFLOPS | **21.99 MFLOPS** | LLVM | +82% |
| Vec Add | 12.05 MFLOPS | **21.99 MFLOPS** | LLVM | +82% |
| Vec MADD | 22.17 MFLOPS | **36.56 MFLOPS** | LLVM | +65% |
| Mat x Vec (SPRAM) | **159.70 MFLOPS** | 145.54 MFLOPS | GCC | +10% |
| Dot Product | **66.92 MFLOPS** | 52.07 MFLOPS | GCC | +29% |
