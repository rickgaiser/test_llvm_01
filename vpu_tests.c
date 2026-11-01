#include <math.h>
#include "vpu_tests.h"

// UCAB (Uncached Accelerated) memory access - bypasses cache, uses write buffer
// PS2 memory map: 0x00000000 = cached, 0x20000000 = uncached, 0x30000000 = UCAB
#define UCAB_ADDR(ptr) ((typeof(ptr))((unsigned int)(ptr) | 0x30000000))

// With BENCH_ASM these kernels are those of asm/vpu_tests.S.
#ifndef BENCH_ASM
// Divisions of all four fields: through the FPU and the VU0 divider
void vpu_vector_divide(v4sf * restrict a ALIGNED16,
                       v4sf * restrict b ALIGNED16,
                       v4sf * restrict result ALIGNED16,
                       int count) {
    for (int i = 0; i < count; i++) {
        result[i] = a[i] / b[i];
    }
}

void vpu_vector_add(v4sf * restrict a ALIGNED16,
                   v4sf * restrict b ALIGNED16,
                   v4sf * restrict result ALIGNED16,
                   int count) {
    for (int i = 0; i < count; i++) {
        result[i] = a[i] + b[i];
    }
}

void vpu_vector_mul(v4sf * restrict a ALIGNED16,
                   v4sf * restrict b ALIGNED16,
                   v4sf * restrict result ALIGNED16,
                   int count) {
    for (int i = 0; i < count; i++) {
        result[i] = a[i] * b[i];
    }
}

void vpu_vector_madd(v4sf * restrict a ALIGNED16,
                    v4sf * restrict b ALIGNED16,
                    v4sf * restrict c ALIGNED16,
                    v4sf * restrict result ALIGNED16,
                    int count) {
    for (int i = 0; i < count; i++) {
        result[i] = a[i] * b[i] + c[i];
    }
}

// vecs_out[i] = mat * vecs_in[i], the matrix in columns
void vpu_matrix_vector_multiply(v4sf mat[4] ALIGNED16,
                               v4sf * restrict vecs_in ALIGNED16,
                               v4sf * restrict vecs_out ALIGNED16,
                               int count) {
    for (int i = 0; i < count; i++) {
        v4sf v = vecs_in[i];
        vecs_out[i] = mat[0] * v[0] + mat[1] * v[1] + mat[2] * v[2] + mat[3] * v[3];
    }
}

void vpu_dot_product(v4sf * restrict a ALIGNED16,
                    v4sf * restrict b ALIGNED16,
                    float * restrict result ALIGNED16,
                    int count) {
    for (int i = 0; i < count; i++) {
        v4sf prod = a[i] * b[i];
        // Horizontal sum of 4 components
        result[i] = prod[0] + prod[1] + prod[2] + prod[3];
    }
}

void vpu_hmax_hmin(v4sf * restrict a ALIGNED16,
                   v4sf * restrict b ALIGNED16,
                   float * restrict max_out ALIGNED16,
                   float * restrict min_out ALIGNED16,
                   int count) {
    for (int i = 0; i < count; i++) {
        v4sf v = a[i] + b[i];
        // Horizontal max and min of 4 components
        max_out[i] = fmaxf(fmaxf(v[0], v[1]), fmaxf(v[2], v[3]));
        min_out[i] = fminf(fminf(v[0], v[1]), fminf(v[2], v[3]));
    }
}

#endif // BENCH_ASM

#ifdef __mips_vu0
// Compute-only benchmark: no loads/stores in loop, measures pure FMAC throughput
// Theoretical max for MUL+3*MADD: (4 + 3*8) FLOPs / 4 cycles * 300 MHz = 2100 MFLOPS
// (vs 2400 MFLOPS for pure MADD: 4*8 FLOPs / 4 cycles * 300 MHz)
void vpu_matrix_vector_multiply_compute_only(v4sf mat[4] ALIGNED16,
                                             v4sf * restrict vec_in ALIGNED16,
                                             v4sf * restrict vec_out ALIGNED16,
                                             int iterations) {
    __asm__ __volatile__(
        ".set noreorder                   \n"

        // Load matrix into vf1-vf4
        "lqc2    $vf1, 0(%[mat])          \n"
        "lqc2    $vf2, 16(%[mat])         \n"
        "lqc2    $vf3, 32(%[mat])         \n"
        "lqc2    $vf4, 48(%[mat])         \n"

        // Load 2 input vectors into vf5-vf6
        "lqc2    $vf5, 0(%[in])           \n"
        "lqc2    $vf6, 16(%[in])          \n"

        // Main loop: pure compute, no memory access
        // Process 2 vectors per iteration
        "1:                               \n"
        "vmulax.xyzw  $ACC, $vf1, $vf5     \n"
        "nop                              \n"
        "vmadday.xyzw $ACC, $vf2, $vf5    \n"
        "nop                              \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf5    \n"
        "nop                              \n"
        "vmaddw.xyzw  $vf7, $vf4, $vf5    \n"   // vf7 = result0
        "nop                              \n"

        "vmulax.xyzw  $ACC, $vf1, $vf6    \n"
        "nop                              \n"
        "vmadday.xyzw $ACC, $vf2, $vf6    \n"
        "addiu   %[iter], %[iter], -2     \n"   // IU - dual-issue with VU
        "vmaddaz.xyzw $ACC, $vf3, $vf6    \n"
        "bgtz    %[iter], 1b              \n"
        "vmaddw.xyzw  $vf8, $vf4, $vf6    \n"   // delay slot: vf8 = result1

        // Store final results
        "sqc2    $vf7, 0(%[out])          \n"
        "sqc2    $vf8, 16(%[out])         \n"
        ".set reorder                     \n"

        : [iter] "+r" (iterations)
        : [mat] "r" (mat), [in] "r" (vec_in), [out] "r" (vec_out)
        : "memory"
    );
}

// vecs_out[i] = mat[0] * vecs_in[i].x + mat[1] * vecs_in[i].y
//             + mat[2] * vecs_in[i].z + mat[3] * vecs_in[i].w
// for count vectors, in VU0 macro mode. count must be even and at least 2;
// all pointers 16-byte aligned. Uses vf1-vf8 and ACC.
//
// Speed: 6 cycles per vector in the loop, on SPRAM and in the data cache,
// which is the limit: all instructions of the work (4 VU0 operations, one
// lqc2 and one sqc2 per vector) use the COP2 slot, and they never issue
// together (measured). On SPRAM, 128 vectors per call: 1367 MFLOPS.
// Size: 148 bytes. It replaces loops with 1, 2, 3, 4, 8 and 12 vectors per
// iteration: 1v and 2v were slower (921, 1268 MFLOPS), 3v-12v as fast with
// 236-668 bytes. From main memory, which the loop waits for, it takes 24
// cycles per vector (350 MFLOPS); the 12v loop took 21.5 (392 MFLOPS), as
// its 12 loads in a row overlap more cache misses.
//
// How (all costs measured on the PS2, bench uarch_tests.c):
//   - Software pipelined, 2 vectors per iteration. lqc2 -> use takes 2
//     cycles and vmaddw -> sqc2 5, and the input is read until the last
//     operation (vmaddw), so a loop that keeps the COP2 slot busy needs 2
//     input registers (vf5, vf6) and 2 result registers (vf7, vf8).
//   - Iteration p finishes the second vector of pair p and stores both
//     results of pair p; in between, it loads pair p + 1, computes its first
//     vector and starts its second one. The exit test comes before the
//     loads: nothing is loaded past the end of the input, and nothing is
//     stored before the start of the output.
//   - The 12 COP2 instructions take 12 cycles: each addiu and branch issues
//     together with a COP2 instruction of its aligned 8-byte block (the
//     comments give the cycle of each instruction). Two other placements
//     cost cycles:
//       - sqc2 in a delay slot, as the store just before the loads: on
//         SPRAM, a load right after a store costs 2 cycles in this pattern
//         (14 cycles per iteration, 12 in the data cache), so the delay
//         slots hold VU0 operations,
//       - sqc2 16(out) together with addiu out, which writes its base
//         register: they do not issue together.
//   - Per call, the last result is stored 4 cycles after the loop, since
//     vmaddw -> sqc2 takes 5 (0.3% for 128 vectors).
void vpu_matrix_vector_multiply_asm(v4sf mat[4] ALIGNED16,
                                    v4sf * restrict vecs_in ALIGNED16,
                                    v4sf * restrict vecs_out ALIGNED16,
                                    int count) {
    v4sf *end = vecs_in + count;

    __asm__ __volatile__(
        ".set noreorder                   \n"

        // Prologue: the matrix in vf1-vf4 and the first pair in vf5 and
        // vf6; the first result in vf7, the second one up to vmadday.
        "lqc2    $vf1, 0(%[mat])          \n"
        "lqc2    $vf2, 16(%[mat])         \n"
        "lqc2    $vf3, 32(%[mat])         \n"
        "lqc2    $vf4, 48(%[mat])         \n"
        "lqc2    $vf5, 0(%[in])           \n"
        "lqc2    $vf6, 16(%[in])          \n"
        "addiu   %[in], %[in], 32         \n"
        "vmulax.xyzw  $ACC, $vf1, $vf5    \n"
        "vmadday.xyzw $ACC, $vf2, $vf5    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf5    \n"
        "vmaddw.xyzw  $vf7, $vf4, $vf5    \n"
        "vmulax.xyzw  $ACC, $vf1, $vf6    \n"
        "vmadday.xyzw $ACC, $vf2, $vf6    \n"

        // Loop, one pair per iteration; the cycle of each instruction.
        // in points to the next pair, out to the current one.
        ".align 3                         \n"
        "1:                               \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf6    \n"   //  0
        "sqc2    $vf7, 0(%[out])          \n"   //  1  first result
        "beq     %[in], %[end], 2f        \n"   //  2
        "vmaddw.xyzw  $vf8, $vf4, $vf6    \n"   //  2  second result
        "lqc2    $vf5, 0(%[in])           \n"   //  3  next pair
        "lqc2    $vf6, 16(%[in])          \n"   //  4
        "addiu   %[in], %[in], 32         \n"   //  5
        "vmulax.xyzw  $ACC, $vf1, $vf5    \n"   //  5
        "vmadday.xyzw $ACC, $vf2, $vf5    \n"   //  6
        "vmaddaz.xyzw $ACC, $vf3, $vf5    \n"   //  7
        "vmaddw.xyzw  $vf7, $vf4, $vf5    \n"   //  8  first result
        "sqc2    $vf8, 16(%[out])         \n"   //  9  second result
        "vmulax.xyzw  $ACC, $vf1, $vf6    \n"   // 10
        "addiu   %[out], %[out], 32       \n"   // 10
        "b       1b                       \n"   // 11
        "vmadday.xyzw $ACC, $vf2, $vf6    \n"   // 11

        // Exit: the first result is stored, the second one computed.
        "2:                               \n"
        "sqc2    $vf8, 16(%[out])         \n"
        ".set reorder                     \n"

        : [in] "+r" (vecs_in), [out] "+r" (vecs_out)
        : [mat] "r" (mat), [end] "r" (end)
        : "memory"
    );
}

#endif // __mips_vu0
