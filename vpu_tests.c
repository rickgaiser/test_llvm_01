#include "vpu_tests.h"

// UCAB (Uncached Accelerated) memory access - bypasses cache, uses write buffer
// PS2 memory map: 0x00000000 = cached, 0x20000000 = uncached, 0x30000000 = UCAB
#define UCAB_ADDR(ptr) ((typeof(ptr))((unsigned int)(ptr) | 0x30000000))

void vpu_vector_add(v4sf * restrict a ALIGNED16,
                   v4sf * restrict b ALIGNED16,
                   v4sf * restrict result ALIGNED16,
                   int count) {
    //#pragma GCC unroll 4
    for (int i = 0; i < count; i++) {
        result[i] = a[i] + b[i];
    }
}

void vpu_vector_mul(v4sf * restrict a ALIGNED16,
                   v4sf * restrict b ALIGNED16,
                   v4sf * restrict result ALIGNED16,
                   int count) {
    //#pragma GCC unroll 4
    for (int i = 0; i < count; i++) {
        result[i] = a[i] * b[i];
    }
}

void vpu_vector_madd(v4sf * restrict a ALIGNED16,
                    v4sf * restrict b ALIGNED16,
                    v4sf * restrict c ALIGNED16,
                    v4sf * restrict result ALIGNED16,
                    int count) {
    //#pragma GCC unroll 4
    for (int i = 0; i < count; i++) {
        result[i] = a[i] * b[i] + c[i];
    }
}

// 3-stage software pipeline with 2 vectors in flight:
//   Stage 1: Load 2 vectors from memory
//   Stage 2: Compute 2 matrix * vector
//   Stage 3: Store 2 results to memory
//
// Pipeline structure:
//   Prologue: Load v0,v1 (S1), then Compute v0,v1 (S2) + Load v2,v3 (S1)
//   Loop:     Store (S3), Compute (S2), Load (S1) - all for different vector pairs
//   Epilogue: Store + Compute + Store final pairs
void vpu_matrix_vector_multiply(v4sf mat[4] ALIGNED16,
                               v4sf * restrict vecs_in ALIGNED16,
                               v4sf * restrict vecs_out ALIGNED16,
                               int count) {
    if (count <= 0) return;

    // Keep matrix in registers
    v4sf m0 = mat[0], m1 = mat[1], m2 = mat[2], m3 = mat[3];

    // Handle odd count - process first vector separately
    if (count & 1) {
        v4sf v = *vecs_in++;
        *vecs_out++ = m0 * v[0] + m1 * v[1] + m2 * v[2] + m3 * v[3];
        if (--count == 0) return;
    }

    // ===== PROLOGUE: prime the pipeline =====
    // Stage 1: Load first 2 vectors
    v4sf v0 = *vecs_in++;
    v4sf v1 = *vecs_in++;
    count -= 2;

    if (count == 0) {
        // Only 2 vectors - compute and store directly
        *vecs_out++ = m0 * v0[0] + m1 * v0[1] + m2 * v0[2] + m3 * v0[3];
        *vecs_out = m0 * v1[0] + m1 * v1[1] + m2 * v1[2] + m3 * v1[3];
        return;
    }

    // Stage 2 + Stage 1: Compute first pair, Load second pair
    v4sf result0 = m0 * v0[0] + m1 * v0[1] + m2 * v0[2] + m3 * v0[3];
    v4sf result1 = m0 * v1[0] + m1 * v1[1] + m2 * v1[2] + m3 * v1[3];
    v0 = *vecs_in++;
    v1 = *vecs_in++;
    count -= 2;

    // ===== MAIN LOOP: steady-state 3-stage pipeline =====
    while (count > 0) {
        // Stage 3: Store results from 2 iterations ago
        *vecs_out++ = result0;
        *vecs_out++ = result1;

        // Stage 2: Compute vectors from 1 iteration ago
        result0 = m0 * v0[0] + m1 * v0[1] + m2 * v0[2] + m3 * v0[3];
        result1 = m0 * v1[0] + m1 * v1[1] + m2 * v1[2] + m3 * v1[3];

        // Stage 1: Load next 2 vectors
        v0 = *vecs_in++;
        v1 = *vecs_in++;

        count -= 2;
    }

    // ===== EPILOGUE: drain the pipeline =====
    // Store results from previous iteration
    *vecs_out++ = result0;
    *vecs_out++ = result1;

    // Compute and store final 2 vectors
    *vecs_out++ = m0 * v0[0] + m1 * v0[1] + m2 * v0[2] + m3 * v0[3];
    *vecs_out = m0 * v1[0] + m1 * v1[1] + m2 * v1[2] + m3 * v1[3];
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

// 1 vertex in flight - baseline
void vpu_matrix_vector_multiply_asm_1v(v4sf mat[4] ALIGNED16,
                                       v4sf * restrict vecs_in ALIGNED16,
                                       v4sf * restrict vecs_out ALIGNED16,
                                       int count) {
    v4sf *end = vecs_in + count;

    __asm__ __volatile__(
        ".set noreorder                   \n"

        // Load matrix into vf1-vf4
        "lqc2    $vf1, 0(%[mat])          \n"
        "lqc2    $vf2, 16(%[mat])         \n"
        "lqc2    $vf3, 32(%[mat])         \n"
        "lqc2    $vf4, 48(%[mat])         \n"

        // Prologue: load and compute first vector
        "lqc2    $vf5, 0(%[in])           \n"
        "addiu   %[in], %[in], 16         \n"

        "vmulax.xyzw  $ACC, $vf1, $vf5     \n"
        "vmadday.xyzw $ACC, $vf2, $vf5    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf5    \n"
        "vmaddw.xyzw  $vf7, $vf4, $vf5    \n"

        "beq     %[in], %[end], 2f        \n"
        "nop                              \n"

        // Main loop - 1 vector per iteration
        "1:                               \n"
        // Load
        "lqc2    $vf5, 0(%[in])           \n"
        "addiu   %[in], %[in], 16         \n"
        // Store prev
        "sqc2    $vf7, 0(%[out])          \n"
        "addiu   %[out], %[out], 16       \n"
        // Compute
        "vmulax.xyzw  $ACC, $vf1, $vf5    \n"
        "vmadday.xyzw $ACC, $vf2, $vf5    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf5    \n"
        "bne     %[in], %[end], 1b        \n"
        "vmaddw.xyzw  $vf7, $vf4, $vf5    \n"

        // Epilogue
        "2:                               \n"
        "sqc2    $vf7, 0(%[out])          \n"
        ".set reorder                     \n"

        : [in] "+r" (vecs_in), [out] "+r" (vecs_out)
        : [mat] "r" (mat), [end] "r" (end)
        : "memory"
    );
}

// 2 vertices in flight
void vpu_matrix_vector_multiply_asm_2v(v4sf mat[4] ALIGNED16,
                                       v4sf * restrict vecs_in ALIGNED16,
                                       v4sf * restrict vecs_out ALIGNED16,
                                       int count) {
    v4sf *end = vecs_in + count;

    __asm__ __volatile__(
        ".set noreorder                   \n"

        // Load matrix into vf1-vf4
        "lqc2    $vf1, 0(%[mat])          \n"
        "lqc2    $vf2, 16(%[mat])         \n"
        "lqc2    $vf3, 32(%[mat])         \n"
        "lqc2    $vf4, 48(%[mat])         \n"

        // Prologue: load and compute first 2 vectors
        "lqc2    $vf5, 0(%[in])           \n"
        "lqc2    $vf6, 16(%[in])          \n"
        "addiu   %[in], %[in], 32         \n"

        "vmulax.xyzw  $ACC, $vf1, $vf5     \n"
        "vmadday.xyzw $ACC, $vf2, $vf5    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf5    \n"
        "vmaddw.xyzw  $vf7, $vf4, $vf5    \n"
        "vmulax.xyzw  $ACC, $vf1, $vf6    \n"
        "vmadday.xyzw $ACC, $vf2, $vf6    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf6    \n"
        "vmaddw.xyzw  $vf8, $vf4, $vf6    \n"

        "beq     %[in], %[end], 2f        \n"
        "nop                              \n"

        // Main loop - 2 vectors per iteration (not rescheduled)
        "1:                               \n"
        // Load 2
        "lqc2    $vf5, 0(%[in])           \n"
        "lqc2    $vf6, 16(%[in])          \n"
        "addiu   %[in], %[in], 32         \n"
        // Store 2 prev
        "sqc2    $vf7, 0(%[out])          \n"
        "sqc2    $vf8, 16(%[out])         \n"
        "addiu   %[out], %[out], 32       \n"
        // Compute 2
        "vmulax.xyzw  $ACC, $vf1, $vf5    \n"
        "vmadday.xyzw $ACC, $vf2, $vf5    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf5    \n"
        "vmaddw.xyzw  $vf7, $vf4, $vf5    \n"
        "vmulax.xyzw  $ACC, $vf1, $vf6    \n"
        "vmadday.xyzw $ACC, $vf2, $vf6    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf6    \n"
        "bne     %[in], %[end], 1b        \n"
        "vmaddw.xyzw  $vf8, $vf4, $vf6    \n"

        // Epilogue
        "2:                               \n"
        "sqc2    $vf7, 0(%[out])          \n"
        "sqc2    $vf8, 16(%[out])         \n"
        ".set reorder                     \n"

        : [in] "+r" (vecs_in), [out] "+r" (vecs_out)
        : [mat] "r" (mat), [end] "r" (end)
        : "memory"
    );
}

// 3 vertices in flight
void vpu_matrix_vector_multiply_asm_3v(v4sf mat[4] ALIGNED16,
                                       v4sf * restrict vecs_in ALIGNED16,
                                       v4sf * restrict vecs_out ALIGNED16,
                                       int count) {
    v4sf *end = vecs_in + count;

    __asm__ __volatile__(
        ".set noreorder                   \n"

        // Load matrix into vf1-vf4
        "lqc2    $vf1, 0(%[mat])          \n"
        "lqc2    $vf2, 16(%[mat])         \n"
        "lqc2    $vf3, 32(%[mat])         \n"
        "lqc2    $vf4, 48(%[mat])         \n"

        // Prologue: load and compute first 3 vectors
        "lqc2    $vf5, 0(%[in])           \n"
        "lqc2    $vf6, 16(%[in])          \n"
        "lqc2    $vf9, 32(%[in])          \n"
        "addiu   %[in], %[in], 48         \n"

        "vmulax.xyzw  $ACC, $vf1, $vf5     \n"
        "vmadday.xyzw $ACC, $vf2, $vf5    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf5    \n"
        "vmaddw.xyzw  $vf7, $vf4, $vf5    \n"
        "vmulax.xyzw  $ACC, $vf1, $vf6    \n"
        "vmadday.xyzw $ACC, $vf2, $vf6    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf6    \n"
        "vmaddw.xyzw  $vf8, $vf4, $vf6    \n"
        "vmulax.xyzw  $ACC, $vf1, $vf9    \n"
        "vmadday.xyzw $ACC, $vf2, $vf9    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf9    \n"
        "vmaddw.xyzw  $vf11, $vf4, $vf9   \n"

        "beq     %[in], %[end], 2f        \n"
        "nop                              \n"

        // Main loop - 3 vectors per iteration (not rescheduled)
        "1:                               \n"
        // Load 3
        "lqc2    $vf5, 0(%[in])           \n"
        "nop                              \n"
        "lqc2    $vf6, 16(%[in])          \n"
        "nop                              \n"
        "lqc2    $vf9, 32(%[in])          \n"
        "nop                              \n"

        // Store 3 prev
        "sqc2    $vf7, 0(%[out])          \n"
        "nop                              \n"
        "sqc2    $vf8, 16(%[out])         \n"
        "nop                              \n"
        "sqc2    $vf11, 32(%[out])        \n"
        "nop                              \n"

        // Compute 3
        "vmulax.xyzw  $ACC, $vf1, $vf5    \n"
        "vmadday.xyzw $ACC, $vf2, $vf5    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf5    \n"
        "vmaddw.xyzw  $vf7, $vf4, $vf5    \n"
        "vmulax.xyzw  $ACC, $vf1, $vf6    \n"
        "vmadday.xyzw $ACC, $vf2, $vf6    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf6    \n"
        "vmaddw.xyzw  $vf8, $vf4, $vf6    \n"
        "vmulax.xyzw  $ACC, $vf1, $vf9    \n"
        "vmadday.xyzw $ACC, $vf2, $vf9    \n"
        "addiu   %[in], %[in], 48         \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf9    \n"
        "addiu   %[out], %[out], 48       \n"
        "bne     %[in], %[end], 1b        \n"
        "vmaddw.xyzw  $vf11, $vf4, $vf9   \n"

        // Epilogue
        "2:                               \n"
        "sqc2    $vf7, 0(%[out])          \n"
        "sqc2    $vf8, 16(%[out])         \n"
        "sqc2    $vf11, 32(%[out])        \n"
        ".set reorder                     \n"

        : [in] "+r" (vecs_in), [out] "+r" (vecs_out)
        : [mat] "r" (mat), [end] "r" (end)
        : "memory"
    );
}

// 4 vertices in flight
void vpu_matrix_vector_multiply_asm_4v(v4sf mat[4] ALIGNED16,
                                       v4sf * restrict vecs_in ALIGNED16,
                                       v4sf * restrict vecs_out ALIGNED16,
                                       int count) {
    v4sf *end = vecs_in + count;

    __asm__ __volatile__(
        ".set noreorder                   \n"

        // Load matrix into vf1-vf4
        "lqc2    $vf1, 0(%[mat])          \n"
        "lqc2    $vf2, 16(%[mat])         \n"
        "lqc2    $vf3, 32(%[mat])         \n"
        "lqc2    $vf4, 48(%[mat])         \n"

        // Prologue: load and compute first 4 vectors
        "lqc2    $vf5, 0(%[in])           \n"
        "lqc2    $vf6, 16(%[in])          \n"
        "lqc2    $vf9, 32(%[in])          \n"
        "lqc2    $vf10, 48(%[in])         \n"
        "addiu   %[in], %[in], 64         \n"

        "vmulax.xyzw  $ACC, $vf1, $vf5     \n"
        "vmadday.xyzw $ACC, $vf2, $vf5    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf5    \n"
        "vmaddw.xyzw  $vf7, $vf4, $vf5    \n"
        "vmulax.xyzw  $ACC, $vf1, $vf6    \n"
        "vmadday.xyzw $ACC, $vf2, $vf6    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf6    \n"
        "vmaddw.xyzw  $vf8, $vf4, $vf6    \n"
        "vmulax.xyzw  $ACC, $vf1, $vf9    \n"
        "vmadday.xyzw $ACC, $vf2, $vf9    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf9    \n"
        "vmaddw.xyzw  $vf11, $vf4, $vf9   \n"
        "vmulax.xyzw  $ACC, $vf1, $vf10   \n"
        "vmadday.xyzw $ACC, $vf2, $vf10   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf10   \n"
        "vmaddw.xyzw  $vf12, $vf4, $vf10  \n"

        "beq     %[in], %[end], 2f        \n"
        "nop                              \n"

        // Main loop - 4 vectors per iteration
        "1:                               \n"
        // Load 4
        "lqc2    $vf5, 0(%[in])           \n"
        "lqc2    $vf6, 16(%[in])          \n"
        "lqc2    $vf9, 32(%[in])          \n"
        "lqc2    $vf10, 48(%[in])         \n"
        "addiu   %[in], %[in], 64         \n"
        // Store 4 prev
        "sqc2    $vf7, 0(%[out])          \n"
        "sqc2    $vf8, 16(%[out])         \n"
        "sqc2    $vf11, 32(%[out])        \n"
        "sqc2    $vf12, 48(%[out])        \n"
        "addiu   %[out], %[out], 64       \n"
        // Compute 4
        "vmulax.xyzw  $ACC, $vf1, $vf5    \n"
        "vmadday.xyzw $ACC, $vf2, $vf5    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf5    \n"
        "vmaddw.xyzw  $vf7, $vf4, $vf5    \n"
        "vmulax.xyzw  $ACC, $vf1, $vf6    \n"
        "vmadday.xyzw $ACC, $vf2, $vf6    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf6    \n"
        "vmaddw.xyzw  $vf8, $vf4, $vf6    \n"
        "vmulax.xyzw  $ACC, $vf1, $vf9    \n"
        "vmadday.xyzw $ACC, $vf2, $vf9    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf9    \n"
        "vmaddw.xyzw  $vf11, $vf4, $vf9   \n"
        "vmulax.xyzw  $ACC, $vf1, $vf10   \n"
        "vmadday.xyzw $ACC, $vf2, $vf10   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf10   \n"
        "bne     %[in], %[end], 1b        \n"
        "vmaddw.xyzw  $vf12, $vf4, $vf10  \n"

        // Epilogue
        "2:                               \n"
        "sqc2    $vf7, 0(%[out])          \n"
        "sqc2    $vf8, 16(%[out])         \n"
        "sqc2    $vf11, 32(%[out])        \n"
        "sqc2    $vf12, 48(%[out])        \n"
        ".set reorder                     \n"

        : [in] "+r" (vecs_in), [out] "+r" (vecs_out)
        : [mat] "r" (mat), [end] "r" (end)
        : "memory"
    );
}

// 8 vertices in flight
void vpu_matrix_vector_multiply_asm_8v(v4sf mat[4] ALIGNED16,
                                       v4sf * restrict vecs_in ALIGNED16,
                                       v4sf * restrict vecs_out ALIGNED16,
                                       int count) {
    v4sf *end = vecs_in + count;

    __asm__ __volatile__(
        ".set noreorder                   \n"

        // Load matrix into vf1-vf4
        "lqc2    $vf1, 0(%[mat])          \n"
        "lqc2    $vf2, 16(%[mat])         \n"
        "lqc2    $vf3, 32(%[mat])         \n"
        "lqc2    $vf4, 48(%[mat])         \n"

        // Prologue: load and compute first 8 vectors
        // Input: vf5,vf6,vf9,vf10,vf13,vf14,vf15,vf16
        // Output: vf7,vf8,vf11,vf12,vf17,vf18,vf19,vf20
        "lqc2    $vf5, 0(%[in])           \n"
        "lqc2    $vf6, 16(%[in])          \n"
        "lqc2    $vf9, 32(%[in])          \n"
        "lqc2    $vf10, 48(%[in])         \n"
        "lqc2    $vf13, 64(%[in])         \n"
        "lqc2    $vf14, 80(%[in])         \n"
        "lqc2    $vf15, 96(%[in])         \n"
        "lqc2    $vf16, 112(%[in])        \n"
        "addiu   %[in], %[in], 128        \n"

        // Compute first 8 vectors
        "vmulax.xyzw  $ACC, $vf1, $vf5    \n"
        "vmadday.xyzw $ACC, $vf2, $vf5    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf5    \n"
        "vmaddw.xyzw  $vf7, $vf4, $vf5    \n"
        "vmulax.xyzw  $ACC, $vf1, $vf6    \n"
        "vmadday.xyzw $ACC, $vf2, $vf6    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf6    \n"
        "vmaddw.xyzw  $vf8, $vf4, $vf6    \n"
        "vmulax.xyzw  $ACC, $vf1, $vf9    \n"
        "vmadday.xyzw $ACC, $vf2, $vf9    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf9    \n"
        "vmaddw.xyzw  $vf11, $vf4, $vf9   \n"
        "vmulax.xyzw  $ACC, $vf1, $vf10   \n"
        "vmadday.xyzw $ACC, $vf2, $vf10   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf10   \n"
        "vmaddw.xyzw  $vf12, $vf4, $vf10  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf13   \n"
        "vmadday.xyzw $ACC, $vf2, $vf13   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf13   \n"
        "vmaddw.xyzw  $vf17, $vf4, $vf13  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf14   \n"
        "vmadday.xyzw $ACC, $vf2, $vf14   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf14   \n"
        "vmaddw.xyzw  $vf18, $vf4, $vf14  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf15   \n"
        "vmadday.xyzw $ACC, $vf2, $vf15   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf15   \n"
        "vmaddw.xyzw  $vf19, $vf4, $vf15  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf16   \n"
        "vmadday.xyzw $ACC, $vf2, $vf16   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf16   \n"
        "vmaddw.xyzw  $vf20, $vf4, $vf16  \n"

        "beq     %[in], %[end], 2f        \n"
        "nop                              \n"

        // Main loop - 8 vectors per iteration (simple: load all, store all, compute all)
        "1:                               \n"
        // Load 8
        "lqc2    $vf5, 0(%[in])           \n"
        "lqc2    $vf6, 16(%[in])          \n"
        "lqc2    $vf9, 32(%[in])          \n"
        "lqc2    $vf10, 48(%[in])         \n"
        "lqc2    $vf13, 64(%[in])         \n"
        "lqc2    $vf14, 80(%[in])         \n"
        "lqc2    $vf15, 96(%[in])         \n"
        "lqc2    $vf16, 112(%[in])        \n"
        "addiu   %[in], %[in], 128        \n"
        // Store 8 prev
        "sqc2    $vf7, 0(%[out])          \n"
        "sqc2    $vf8, 16(%[out])         \n"
        "sqc2    $vf11, 32(%[out])        \n"
        "sqc2    $vf12, 48(%[out])        \n"
        "sqc2    $vf17, 64(%[out])        \n"
        "sqc2    $vf18, 80(%[out])        \n"
        "sqc2    $vf19, 96(%[out])        \n"
        "sqc2    $vf20, 112(%[out])       \n"
        "addiu   %[out], %[out], 128      \n"
        // Compute 8
        "vmulax.xyzw  $ACC, $vf1, $vf5    \n"
        "vmadday.xyzw $ACC, $vf2, $vf5    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf5    \n"
        "vmaddw.xyzw  $vf7, $vf4, $vf5    \n"
        "vmulax.xyzw  $ACC, $vf1, $vf6    \n"
        "vmadday.xyzw $ACC, $vf2, $vf6    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf6    \n"
        "vmaddw.xyzw  $vf8, $vf4, $vf6    \n"
        "vmulax.xyzw  $ACC, $vf1, $vf9    \n"
        "vmadday.xyzw $ACC, $vf2, $vf9    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf9    \n"
        "vmaddw.xyzw  $vf11, $vf4, $vf9   \n"
        "vmulax.xyzw  $ACC, $vf1, $vf10   \n"
        "vmadday.xyzw $ACC, $vf2, $vf10   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf10   \n"
        "vmaddw.xyzw  $vf12, $vf4, $vf10  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf13   \n"
        "vmadday.xyzw $ACC, $vf2, $vf13   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf13   \n"
        "vmaddw.xyzw  $vf17, $vf4, $vf13  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf14   \n"
        "vmadday.xyzw $ACC, $vf2, $vf14   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf14   \n"
        "vmaddw.xyzw  $vf18, $vf4, $vf14  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf15   \n"
        "vmadday.xyzw $ACC, $vf2, $vf15   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf15   \n"
        "vmaddw.xyzw  $vf19, $vf4, $vf15  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf16   \n"
        "vmadday.xyzw $ACC, $vf2, $vf16   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf16   \n"
        "bne     %[in], %[end], 1b        \n"
        "vmaddw.xyzw  $vf20, $vf4, $vf16  \n"

        // Epilogue
        "2:                               \n"
        "sqc2    $vf7, 0(%[out])          \n"
        "sqc2    $vf8, 16(%[out])         \n"
        "sqc2    $vf11, 32(%[out])        \n"
        "sqc2    $vf12, 48(%[out])        \n"
        "sqc2    $vf17, 64(%[out])        \n"
        "sqc2    $vf18, 80(%[out])        \n"
        "sqc2    $vf19, 96(%[out])        \n"
        "sqc2    $vf20, 112(%[out])       \n"
        ".set reorder                     \n"

        : [in] "+r" (vecs_in), [out] "+r" (vecs_out)
        : [mat] "r" (mat), [end] "r" (end)
        : "memory"
    );
}

// 12 vertices in flight - maximum using all available registers
// vf0 = reserved, vf1-4 = matrix, vf5-16 = input (12), vf17-28 = output (12)
void vpu_matrix_vector_multiply_asm_12v(v4sf mat[4] ALIGNED16,
                                        v4sf * restrict vecs_in ALIGNED16,
                                        v4sf * restrict vecs_out ALIGNED16,
                                        int count) {
    v4sf *end = vecs_in + count;

    __asm__ __volatile__(
        ".set noreorder                   \n"

        // Load matrix into vf1-vf4
        "lqc2    $vf1, 0(%[mat])          \n"
        "lqc2    $vf2, 16(%[mat])         \n"
        "lqc2    $vf3, 32(%[mat])         \n"
        "lqc2    $vf4, 48(%[mat])         \n"

        // Prologue: load and compute first 12 vectors
        // Input: vf5-vf16, Output: vf17-vf28
        "lqc2    $vf5, 0(%[in])           \n"
        "lqc2    $vf6, 16(%[in])          \n"
        "lqc2    $vf7, 32(%[in])          \n"
        "lqc2    $vf8, 48(%[in])          \n"
        "lqc2    $vf9, 64(%[in])          \n"
        "lqc2    $vf10, 80(%[in])         \n"
        "lqc2    $vf11, 96(%[in])         \n"
        "lqc2    $vf12, 112(%[in])        \n"
        "lqc2    $vf13, 128(%[in])        \n"
        "lqc2    $vf14, 144(%[in])        \n"
        "lqc2    $vf15, 160(%[in])        \n"
        "lqc2    $vf16, 176(%[in])        \n"
        "addiu   %[in], %[in], 192        \n"

        // Compute first 12 vectors
        "vmulax.xyzw  $ACC, $vf1, $vf5    \n"
        "vmadday.xyzw $ACC, $vf2, $vf5    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf5    \n"
        "vmaddw.xyzw  $vf17, $vf4, $vf5   \n"
        "vmulax.xyzw  $ACC, $vf1, $vf6    \n"
        "vmadday.xyzw $ACC, $vf2, $vf6    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf6    \n"
        "vmaddw.xyzw  $vf18, $vf4, $vf6   \n"
        "vmulax.xyzw  $ACC, $vf1, $vf7    \n"
        "vmadday.xyzw $ACC, $vf2, $vf7    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf7    \n"
        "vmaddw.xyzw  $vf19, $vf4, $vf7   \n"
        "vmulax.xyzw  $ACC, $vf1, $vf8    \n"
        "vmadday.xyzw $ACC, $vf2, $vf8    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf8    \n"
        "vmaddw.xyzw  $vf20, $vf4, $vf8   \n"
        "vmulax.xyzw  $ACC, $vf1, $vf9    \n"
        "vmadday.xyzw $ACC, $vf2, $vf9    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf9    \n"
        "vmaddw.xyzw  $vf21, $vf4, $vf9   \n"
        "vmulax.xyzw  $ACC, $vf1, $vf10   \n"
        "vmadday.xyzw $ACC, $vf2, $vf10   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf10   \n"
        "vmaddw.xyzw  $vf22, $vf4, $vf10  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf11   \n"
        "vmadday.xyzw $ACC, $vf2, $vf11   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf11   \n"
        "vmaddw.xyzw  $vf23, $vf4, $vf11  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf12   \n"
        "vmadday.xyzw $ACC, $vf2, $vf12   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf12   \n"
        "vmaddw.xyzw  $vf24, $vf4, $vf12  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf13   \n"
        "vmadday.xyzw $ACC, $vf2, $vf13   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf13   \n"
        "vmaddw.xyzw  $vf25, $vf4, $vf13  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf14   \n"
        "vmadday.xyzw $ACC, $vf2, $vf14   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf14   \n"
        "vmaddw.xyzw  $vf26, $vf4, $vf14  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf15   \n"
        "vmadday.xyzw $ACC, $vf2, $vf15   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf15   \n"
        "vmaddw.xyzw  $vf27, $vf4, $vf15  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf16   \n"
        "vmadday.xyzw $ACC, $vf2, $vf16   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf16   \n"
        "vmaddw.xyzw  $vf28, $vf4, $vf16  \n"

        "beq     %[in], %[end], 2f        \n"
        "nop                              \n"

        // Main loop - 12 vectors per iteration
        "1:                               \n"
        // Load 12
        "lqc2    $vf5, 0(%[in])           \n"
        "lqc2    $vf6, 16(%[in])          \n"
        "lqc2    $vf7, 32(%[in])          \n"
        "lqc2    $vf8, 48(%[in])          \n"
        "lqc2    $vf9, 64(%[in])          \n"
        "lqc2    $vf10, 80(%[in])         \n"
        "lqc2    $vf11, 96(%[in])         \n"
        "lqc2    $vf12, 112(%[in])        \n"
        "lqc2    $vf13, 128(%[in])        \n"
        "lqc2    $vf14, 144(%[in])        \n"
        "lqc2    $vf15, 160(%[in])        \n"
        "lqc2    $vf16, 176(%[in])        \n"
        "addiu   %[in], %[in], 192        \n"
        // Store 12 prev
        "sqc2    $vf17, 0(%[out])         \n"
        "sqc2    $vf18, 16(%[out])        \n"
        "sqc2    $vf19, 32(%[out])        \n"
        "sqc2    $vf20, 48(%[out])        \n"
        "sqc2    $vf21, 64(%[out])        \n"
        "sqc2    $vf22, 80(%[out])        \n"
        "sqc2    $vf23, 96(%[out])        \n"
        "sqc2    $vf24, 112(%[out])       \n"
        "sqc2    $vf25, 128(%[out])       \n"
        "sqc2    $vf26, 144(%[out])       \n"
        "sqc2    $vf27, 160(%[out])       \n"
        "sqc2    $vf28, 176(%[out])       \n"
        "addiu   %[out], %[out], 192      \n"
        // Compute 12
        "vmulax.xyzw  $ACC, $vf1, $vf5    \n"
        "pref    0, 0(%[in])              \n"
        "vmadday.xyzw $ACC, $vf2, $vf5    \n"
        "pref    0, 64(%[in])             \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf5    \n"
        "pref    0, 128(%[in])            \n"
        "vmaddw.xyzw  $vf17, $vf4, $vf5   \n"
        "pref    0, 192(%[in])            \n"
        "vmulax.xyzw  $ACC, $vf1, $vf6    \n"
        "pref    0, 256(%[in])            \n"
        "vmadday.xyzw $ACC, $vf2, $vf6    \n"
        "pref    0, 320(%[in])            \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf6    \n"
        "vmaddw.xyzw  $vf18, $vf4, $vf6   \n"
        "vmulax.xyzw  $ACC, $vf1, $vf7    \n"
        "vmadday.xyzw $ACC, $vf2, $vf7    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf7    \n"
        "vmaddw.xyzw  $vf19, $vf4, $vf7   \n"
        "vmulax.xyzw  $ACC, $vf1, $vf8    \n"
        "vmadday.xyzw $ACC, $vf2, $vf8    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf8    \n"
        "vmaddw.xyzw  $vf20, $vf4, $vf8   \n"
        "vmulax.xyzw  $ACC, $vf1, $vf9    \n"
        "vmadday.xyzw $ACC, $vf2, $vf9    \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf9    \n"
        "vmaddw.xyzw  $vf21, $vf4, $vf9   \n"
        "vmulax.xyzw  $ACC, $vf1, $vf10   \n"
        "vmadday.xyzw $ACC, $vf2, $vf10   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf10   \n"
        "vmaddw.xyzw  $vf22, $vf4, $vf10  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf11   \n"
        "vmadday.xyzw $ACC, $vf2, $vf11   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf11   \n"
        "vmaddw.xyzw  $vf23, $vf4, $vf11  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf12   \n"
        "vmadday.xyzw $ACC, $vf2, $vf12   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf12   \n"
        "vmaddw.xyzw  $vf24, $vf4, $vf12  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf13   \n"
        "vmadday.xyzw $ACC, $vf2, $vf13   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf13   \n"
        "vmaddw.xyzw  $vf25, $vf4, $vf13  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf14   \n"
        "vmadday.xyzw $ACC, $vf2, $vf14   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf14   \n"
        "vmaddw.xyzw  $vf26, $vf4, $vf14  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf15   \n"
        "vmadday.xyzw $ACC, $vf2, $vf15   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf15   \n"
        "vmaddw.xyzw  $vf27, $vf4, $vf15  \n"
        "vmulax.xyzw  $ACC, $vf1, $vf16   \n"
        "vmadday.xyzw $ACC, $vf2, $vf16   \n"
        "vmaddaz.xyzw $ACC, $vf3, $vf16   \n"
        "bne     %[in], %[end], 1b        \n"
        "vmaddw.xyzw  $vf28, $vf4, $vf16  \n"

        // Epilogue
        "2:                               \n"
        "sqc2    $vf17, 0(%[out])         \n"
        "sqc2    $vf18, 16(%[out])        \n"
        "sqc2    $vf19, 32(%[out])        \n"
        "sqc2    $vf20, 48(%[out])        \n"
        "sqc2    $vf21, 64(%[out])        \n"
        "sqc2    $vf22, 80(%[out])        \n"
        "sqc2    $vf23, 96(%[out])        \n"
        "sqc2    $vf24, 112(%[out])       \n"
        "sqc2    $vf25, 128(%[out])       \n"
        "sqc2    $vf26, 144(%[out])       \n"
        "sqc2    $vf27, 160(%[out])       \n"
        "sqc2    $vf28, 176(%[out])       \n"
        ".set reorder                     \n"

        : [in] "+r" (vecs_in), [out] "+r" (vecs_out)
        : [mat] "r" (mat), [end] "r" (end)
        : "memory"
    );
}

// Default version - use 2v for compatibility
void vpu_matrix_vector_multiply_asm(v4sf mat[4] ALIGNED16,
                                   v4sf * restrict vecs_in ALIGNED16,
                                   v4sf * restrict vecs_out ALIGNED16,
                                   int count) {
    vpu_matrix_vector_multiply_asm_2v(mat, vecs_in, vecs_out, count);
}
#endif // __mips_vu0
