#ifndef VPU_TESTS_H
#define VPU_TESTS_H

// v4sf is a vector of 4 single-precision floats (128-bit)
// Maps to PS2 VU0 registers in macro mode
typedef float v4sf __attribute__((vector_size(16)));

#define VPU_NUM_VECTORS 256

// Alignment hint macro - Clang supports on params, GCC doesn't
#ifdef __clang__
#define ALIGNED16 __attribute__((aligned(16)))
#else
#define ALIGNED16
#endif

// Test functions - v4sf is already 128-bit aligned, add explicit alignment hints
void vpu_vector_add(v4sf * restrict a ALIGNED16,
                   v4sf * restrict b ALIGNED16,
                   v4sf * restrict result ALIGNED16,
                   int count);

void vpu_vector_mul(v4sf * restrict a ALIGNED16,
                   v4sf * restrict b ALIGNED16,
                   v4sf * restrict result ALIGNED16,
                   int count);

void vpu_vector_madd(v4sf * restrict a ALIGNED16,
                    v4sf * restrict b ALIGNED16,
                    v4sf * restrict c ALIGNED16,
                    v4sf * restrict result ALIGNED16,
                    int count);

void vpu_matrix_vector_multiply(v4sf mat[4] ALIGNED16,
                               v4sf * restrict vecs_in ALIGNED16,
                               v4sf * restrict vecs_out ALIGNED16,
                               int count);

void vpu_dot_product(v4sf * restrict a ALIGNED16,
                    v4sf * restrict b ALIGNED16,
                    float * restrict result ALIGNED16,
                    int count);

#ifdef __mips_vu0
void vpu_matrix_vector_multiply_asm(v4sf mat[4] ALIGNED16,
                                   v4sf * restrict vecs_in ALIGNED16,
                                   v4sf * restrict vecs_out ALIGNED16,
                                   int count);

void vpu_matrix_vector_multiply_asm_1v(v4sf mat[4] ALIGNED16,
                                       v4sf * restrict vecs_in ALIGNED16,
                                       v4sf * restrict vecs_out ALIGNED16,
                                       int count);

void vpu_matrix_vector_multiply_asm_2v(v4sf mat[4] ALIGNED16,
                                       v4sf * restrict vecs_in ALIGNED16,
                                       v4sf * restrict vecs_out ALIGNED16,
                                       int count);

void vpu_matrix_vector_multiply_asm_3v(v4sf mat[4] ALIGNED16,
                                       v4sf * restrict vecs_in ALIGNED16,
                                       v4sf * restrict vecs_out ALIGNED16,
                                       int count);

void vpu_matrix_vector_multiply_asm_4v(v4sf mat[4] ALIGNED16,
                                       v4sf * restrict vecs_in ALIGNED16,
                                       v4sf * restrict vecs_out ALIGNED16,
                                       int count);

void vpu_matrix_vector_multiply_asm_8v(v4sf mat[4] ALIGNED16,
                                       v4sf * restrict vecs_in ALIGNED16,
                                       v4sf * restrict vecs_out ALIGNED16,
                                       int count);

void vpu_matrix_vector_multiply_asm_12v(v4sf mat[4] ALIGNED16,
                                        v4sf * restrict vecs_in ALIGNED16,
                                        v4sf * restrict vecs_out ALIGNED16,
                                        int count);

void vpu_matrix_vector_multiply_compute_only(v4sf mat[4] ALIGNED16,
                                             v4sf * restrict vec_in ALIGNED16,
                                             v4sf * restrict vec_out ALIGNED16,
                                             int iterations);
#endif // __mips_vu0

#endif // VPU_TESTS_H
